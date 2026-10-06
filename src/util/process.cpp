#include "process.h"

#include <algorithm>
#include <limits>
#include <mutex>
#include <vector>

#if defined(_WIN32)
    #include <windows.h>
#else
    #include <sys/wait.h>
    #include <cerrno>
    #include <csignal>
#endif

NativePipeStreamBuf::NativePipeStreamBuf(native_handle_t handle, bool isReadEnd)
    : handle_(handle), isReadEnd_(isReadEnd)
{
    if (isReadEnd_) setg(buffer_.data(), buffer_.data(), buffer_.data());
#if !defined(_WIN32)
    static std::once_flag ignoreSigpipeOnce;
    std::call_once(ignoreSigpipeOnce, [] { ::signal(SIGPIPE, SIG_IGN); });
#endif
}

NativePipeStreamBuf::~NativePipeStreamBuf() 
{ 
    close(); 
}

void NativePipeStreamBuf::close()
{
    if (!open_) return;
    open_ = false;
#if defined(_WIN32)
    if (handle_) { CloseHandle(static_cast<HANDLE>(handle_)); handle_ = nullptr; }
#else
    if (handle_ >= 0) { ::close(handle_); handle_ = -1; }
#endif
}

NativePipeStreamBuf::int_type NativePipeStreamBuf::underflow()
{
    if (!isReadEnd_ || !open_) return traits_type::eof();

#if defined(_WIN32)
    DWORD n = 0;
    for (;;)
    {
        if (ReadFile(static_cast<HANDLE>(handle_), buffer_.data(), (DWORD)buffer_.size(), &n, nullptr))
            break;
        return traits_type::eof();
    }
    if (n == 0) return traits_type::eof();
#else
    ssize_t n;
    for (;;)
    {
        n = ::read(handle_, buffer_.data(), buffer_.size());
        if (n < 0 && errno == EINTR) continue;
        break;
    }
    if (n <= 0) return traits_type::eof();
#endif
    setg(buffer_.data(), buffer_.data(), buffer_.data() + n);
    return traits_type::to_int_type(*gptr());
}

std::streamsize NativePipeStreamBuf::xsputn(const char* s, std::streamsize count)
{
    if (isReadEnd_ || !open_ || broken_ || count <= 0) return 0;

    std::streamsize totalWritten = 0;
    while (totalWritten < count)
    {
#if defined(_WIN32)
        DWORD toWrite = (DWORD)std::min<std::streamsize>(
            count - totalWritten,
            (std::streamsize)(std::numeric_limits<DWORD>::max)());

        DWORD written = 0;
        if (!WriteFile(static_cast<HANDLE>(handle_), s + totalWritten, toWrite, &written, nullptr))
        {
            DWORD err = GetLastError();
            if (err == ERROR_BROKEN_PIPE || err == ERROR_NO_DATA)
                broken_ = true;
            break;
        }
        if (written == 0) break;
        totalWritten += written;
#else
        ssize_t written = ::write(handle_, s + totalWritten, (size_t)(count - totalWritten));
        if (written < 0)
        {
            if (errno == EINTR) continue;
            if (errno == EPIPE) broken_ = true;
            break;
        }
        if (written == 0) break;
        totalWritten += written;
#endif
    }
    return totalWritten;
}

NativePipeStreamBuf::int_type NativePipeStreamBuf::overflow(int_type ch)
{
    if (isReadEnd_ || !open_ || ch == traits_type::eof()) return traits_type::not_eof(ch);
    char c = traits_type::to_char_type(ch);
    return xsputn(&c, 1) == 1 ? ch : traits_type::eof();
}

OwningIOStream::OwningIOStream(std::unique_ptr<std::streambuf> buf)
    : std::iostream(buf.get()), buf_(std::move(buf)) {}

int ProcessPipe::waitAndClose()
{
    stream.reset();
    int exitCode = -1;
#if defined(_WIN32)
    if (hProcess)
    {
        WaitForSingleObject(static_cast<HANDLE>(hProcess), INFINITE);
        DWORD code = 0;
        if (GetExitCodeProcess(static_cast<HANDLE>(hProcess), &code))
            exitCode = static_cast<int>(code);
        CloseHandle(static_cast<HANDLE>(hProcess));
        CloseHandle(static_cast<HANDLE>(hThread));
        hProcess = nullptr;
        hThread = nullptr;
    }
#else
    if (pid > 0)
    {
        int status = 0;
        while (waitpid(pid, &status, 0) < 0)
        {
            if (errno != EINTR) break;
        }
        if (WIFEXITED(status)) exitCode = WEXITSTATUS(status);
        pid = -1;
    }
#endif
    return exitCode;
}

std::unique_ptr<ProcessPipe> SpawnProcessPipe(const std::string& command, bool readFromStdout)
{
    auto pipe = std::make_unique<ProcessPipe>();

#if defined(_WIN32)
    SECURITY_ATTRIBUTES sa{};
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;

    HANDLE hRead = nullptr, hWrite = nullptr;
    if (!CreatePipe(&hRead, &hWrite, &sa, 0)) return nullptr;

    STARTUPINFOA si{};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;

    if (readFromStdout)
    {
        SetHandleInformation(hRead, HANDLE_FLAG_INHERIT, 0);
        si.hStdOutput = hWrite;
        si.hStdError = hWrite;
        si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    }
    else
    {
        SetHandleInformation(hWrite, HANDLE_FLAG_INHERIT, 0);
        si.hStdInput = hRead;
        si.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
        si.hStdError = GetStdHandle(STD_ERROR_HANDLE);
    }

    std::vector<char> cmdBuffer(command.begin(), command.end());
    cmdBuffer.push_back('\0');

    PROCESS_INFORMATION pi{};
    BOOL success = CreateProcessA(
        nullptr, cmdBuffer.data(), nullptr, nullptr, TRUE,
        0, nullptr, nullptr, &si, &pi
    );

    if (!success)
    {
        CloseHandle(hRead);
        CloseHandle(hWrite);
        return nullptr;
    }

    pipe->hProcess = pi.hProcess;
    pipe->hThread = pi.hThread;

    if (readFromStdout)
    {
        CloseHandle(hWrite);
        auto buf = std::make_unique<NativePipeStreamBuf>(hRead, true);
        pipe->stream = std::make_unique<OwningIOStream>(std::move(buf));
    }
    else
    {
        CloseHandle(hRead);
        auto buf = std::make_unique<NativePipeStreamBuf>(hWrite, false);
        pipe->stream = std::make_unique<OwningIOStream>(std::move(buf));
    }
#else
    int pipefd[2];
    if (::pipe(pipefd) < 0) return nullptr;

    pid_t pid = fork();
    if (pid < 0)
    {
        ::close(pipefd[0]);
        ::close(pipefd[1]);
        return nullptr;
    }

    if (pid == 0)
    {
        if (readFromStdout)
        {
            ::close(pipefd[0]);
            ::dup2(pipefd[1], STDOUT_FILENO);
            ::dup2(pipefd[1], STDERR_FILENO);
            ::close(pipefd[1]);
        }
        else
        {
            ::close(pipefd[1]);
            ::dup2(pipefd[0], STDIN_FILENO);
            ::close(pipefd[0]);
        }

        execl("/bin/sh", "sh", "-c", command.c_str(), static_cast<char*>(nullptr));
        _exit(127);
    }

    pipe->pid = pid;
    if (readFromStdout)
    {
        ::close(pipefd[1]);
        auto buf = std::make_unique<NativePipeStreamBuf>(pipefd[0], true);
        pipe->stream = std::make_unique<OwningIOStream>(std::move(buf));
    }
    else
    {
        ::close(pipefd[0]);
        auto buf = std::make_unique<NativePipeStreamBuf>(pipefd[1], false);
        pipe->stream = std::make_unique<OwningIOStream>(std::move(buf));
    }
#endif

    return pipe;
}