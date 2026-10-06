#include "app.h"
#include <raylib.h>
#include <cstdlib>
#include <filesystem>
#include <string>
#include <vector>

namespace {
struct EncoderConfig {
    const char *name;
    const char *options;
};

bool testFFmpeg(const std::string& encoder)
{
#if defined(_WIN32)
    const char* ffmpegBin = "ffmpeg.exe";
#else
    const char* ffmpegBin = "ffmpeg";
#endif
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "%s -hide_banner -encoders 2>&1", ffmpegBin);

    auto pipe = SpawnProcessPipe(cmd, true);
    if (!pipe || !pipe->stream)
        return false;

    std::string line;
    bool found = false;
    while (std::getline(*pipe->stream, line))
    {
        if (line.find(encoder) != std::string::npos)
        {
            found = true;
            break;
        }
    }

    pipe->waitAndClose();
    return found;
}

EncoderConfig selectBest()
{
    std::vector<EncoderConfig> candidates = {
        {"h264_nvenc", "-cq 20 -preset p6"},
        {"h264_amf", "-quality quality"},
        {"h264_qsv", "-global_quality 20"},
        {"h264_videotoolbox", "-q:v 65"}
    };
    for (const auto& config : candidates)
    {
        if (testFFmpeg(config.name))
        {
            TraceLog(LOG_INFO, "Hardware encoder detected: %s", config.name);
            return config;
        }
    }
    TraceLog(LOG_INFO, "No hardware encoder found, falling back to libx264");
    return {"libx264", "-crf 16 -preset slow"};
}
}

void XboxStartup::writeVideoFrame()
{
    if (!ffmpegProcess || !ffmpegProcess->stream)
        return;

    Image frame = LoadImageFromScreen();
    if (!IsImageValid(frame))
    {
        TraceLog(LOG_ERROR, "Failed to capture framebuffer");
        return;
    }
    if (frame.format != PIXELFORMAT_UNCOMPRESSED_R8G8B8A8)
    {
        ImageFormat(&frame, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
    }

    const size_t frameSize = static_cast<size_t>(frame.width) * frame.height * 4;

    if (!ffmpegProcess->write(reinterpret_cast<const char*>(frame.data), frameSize))
    {
        TraceLog(LOG_ERROR, "FFmpeg pipe write failed or pipe is broken");
        running = false;
    }

    UnloadImage(frame);
    ++frameNumber;
}

void XboxStartup::startVideoCapture()
{
    const int w = GetScreenWidth();
    const int h = GetScreenHeight();
    EncoderConfig encoder = selectBest();
    char command[2048];

#if defined(_WIN32)
    snprintf(
        command,
        sizeof(command),
        "cmd.exe /c ffmpeg.exe "
        "-hide_banner "
        "-nostdin "
        "-y "
        "-loglevel verbose "
        "-f rawvideo "
        "-pixel_format rgba "
        "-video_size %dx%d "
        "-framerate %d "
        "-i - "
        "-an "
        "-c:v %s "
        "%s "
        "-pix_fmt yuv420p "
        "\".capture/capture_video.mp4\" "
        "2> ffmpeg_video.log",
        w,
        h,
        framerate,
        encoder.name,
        encoder.options
    );
#else
    snprintf(
        command,
        sizeof(command),
        "sh -c \"ffmpeg "
        "-hide_banner "
        "-nostdin "
        "-y "
        "-loglevel verbose "
        "-f rawvideo "
        "-pixel_format rgba "
        "-video_size %dx%d "
        "-framerate %d "
        "-i - "
        "-an "
        "-c:v %s "
        "%s "
        "-pix_fmt yuv420p "
        "'.capture/capture_video.mp4' "
        "2> ffmpeg_video.log\"",
        w,
        h,
        framerate,
        encoder.name,
        encoder.options
    );
#endif

    TraceLog(LOG_INFO, "Starting FFmpeg:");
    TraceLog(LOG_INFO, "%s", command);

    ffmpegProcess = SpawnProcessPipe(command, false);
    if (!ffmpegProcess || !ffmpegProcess->stream)
    {
        TraceLog(LOG_ERROR, "Failed to create FFmpeg pipe");
        ffmpegProcess.reset();
        return;
    }

    frameNumber = 0;
    TraceLog(LOG_INFO, "FFmpeg pipe opened: %dx%d @ %d fps using %s",
        w, h,
        framerate, encoder.name
    );
}

bool XboxStartup::stopVideoCapture()
{
    if (!ffmpegProcess)
        return false;

    TraceLog(LOG_INFO, "Stopping FFmpeg after %d frames", frameNumber);
    const int result = ffmpegProcess->waitAndClose();
    ffmpegProcess.reset();

    TraceLog(LOG_INFO, "FFmpeg video encoder exited with code %d", result);
    if (result != 0)
    {
        TraceLog(LOG_ERROR, "FFmpeg video encoding failed. Check ffmpeg_video.log");
        return false;
    }

    if (std::remove("ffmpeg_video.log") != 0)
        TraceLog(LOG_WARNING, "Failed to delete ffmpeg_video.log");

    return true;
}

void XboxStartup::mergeAudioIntoVideo()
{
    constexpr const char* inputVideo = ".capture/capture_video.mp4";
    constexpr const char* inputAudio = ".capture/audio.wav";
    constexpr const char* muxedVideo = ".capture/capture.mp4";
    constexpr const char* logFile = "ffmpeg_mux.log";

#if defined(_WIN32)
    constexpr const char* ffmpeg = "ffmpeg.exe";
#else
    constexpr const char* ffmpeg = "ffmpeg";
#endif

    char command[2048];

    snprintf(
        command,
        sizeof(command),
        "%s "
        "-hide_banner "
        "-nostdin "
        "-y "
        "-loglevel verbose "
        "-i \"%s\" "
        "-i \"%s\" "
        "-map 0:v:0 "
        "-map 1:a:0 "
        "-c:v copy "
        "-c:a aac "
        "-b:a 192k "
        "-movflags +faststart "
        "\"%s\" "
        "2> %s",
        ffmpeg,
        inputVideo,
        inputAudio,
        muxedVideo,
        logFile
    );

    TraceLog(LOG_INFO, "Merging video and audio:");
    TraceLog(LOG_INFO, "%s", command);

    if (std::system(command) != 0)
    {
        TraceLog(LOG_ERROR, "FFmpeg audio mux failed. Check %s",
            logFile
        );
        return;
    }

    auto removeFile = [](const std::filesystem::path& path, const char* description)
    {
        std::error_code ec;

        if (!std::filesystem::remove(path, ec))
        {
            TraceLog(LOG_WARNING, "Failed to delete %s%s",
                description, ec ? TextFormat(": %s", ec.message().c_str()) : ""
            );
        }
    };

    removeFile(logFile, "ffmpeg_mux.log");
    removeFile(inputVideo, "capture_video.mp4");
    removeFile(inputAudio, "audio.wav");

    std::error_code ec;
    std::filesystem::copy_file(muxedVideo, outfile,
        std::filesystem::copy_options::overwrite_existing, ec
    );

    if (ec)
    {
        TraceLog(LOG_WARNING, "Failed to copy %s to %s: %s",
            muxedVideo, outfile.string().c_str(), ec.message().c_str()
        );
        return;
    }

    removeFile(muxedVideo, "capture.mp4");

    ec.clear();
    std::filesystem::remove(".capture", ec);

    if (ec)
        TraceLog(LOG_WARNING, "Failed to delete .capture: %s", ec.message().c_str());

    TraceLog(LOG_INFO, "Final video written to %s", outfile.string().c_str());
}