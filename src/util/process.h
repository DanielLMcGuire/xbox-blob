#pragma once

#include <streambuf>
#include <iostream>
#include <array>
#include <memory>
#include <string>

#if !defined(_WIN32)
    #include <sys/types.h>
#endif

class NativePipeStreamBuf : public std::streambuf
{
public:
#if defined(_WIN32)
    using native_handle_t = void*;
#else
    using native_handle_t = int;
#endif

    NativePipeStreamBuf(native_handle_t handle, bool isReadEnd);
    ~NativePipeStreamBuf() override;

    void close();
    bool broken() const { return broken_; }

protected:
    int_type underflow() override;
    std::streamsize xsputn(const char* s, std::streamsize count) override;
    int_type overflow(int_type ch) override;

private:
    native_handle_t         handle_;
    bool                    isReadEnd_;
    bool                    open_ = true;
    bool                    broken_ = false;
    std::array<char, 4096>  buffer_{};
};

class OwningIOStream : public std::iostream
{
public:
    explicit OwningIOStream(std::unique_ptr<std::streambuf> buf);
private:
    std::unique_ptr<std::streambuf> buf_;
};

struct ProcessPipe
{
    std::unique_ptr<OwningIOStream> stream;
#if defined(_WIN32)
    void* hProcess = nullptr; 
    void* hThread = nullptr;
#else
    pid_t pid = -1;
#endif

    int waitAndClose();

    inline bool write(const void* data, size_t size)
    {
        if (!stream) return false;
        stream->write(reinterpret_cast<const char*>(data), size);
        return stream->good();
    }
};

std::unique_ptr<ProcessPipe> SpawnProcessPipe(const std::string& command, bool readFromStdout);