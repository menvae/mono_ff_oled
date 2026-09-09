
#ifndef MONO_FF_OLED_VIDEODECODER_H
#define MONO_FF_OLED_VIDEODECODER_H
#include <functional>
#include <iostream>
#include <utility>

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
}


class VideoDecoder {
public:

    VideoDecoder(const VideoDecoder&) = delete;
    VideoDecoder& operator=(const VideoDecoder&) = delete;
    VideoDecoder(std::string path, int width, int height)
    : _path(std::move(path)), _targetW(width), _targetH(height) {

        int ret = init();
        if (ret != 0) {
            char errbuf[256];
            av_strerror(ret, errbuf, sizeof(errbuf));
            throw std::runtime_error(std::string("Failed to initialize VideoDecoder: ") + errbuf);
        }
    }

    void getFrames(const std::function<bool(const AVFrame *)> &callback);

    [[nodiscard]] int getFps() const;
    [[nodiscard]] std::tuple<int, int> getVideoDimensions() const;
    [[nodiscard]] std::tuple<int, int> getOutputDimensions() const;

    static void exportToPpm(const std::string &filename, const AVFrame *frame);

    ~VideoDecoder();
private:
    std::string _path;
    int _targetW, _targetH;

    AVFormatContext* fmtCtx = nullptr;
    AVCodecContext* codecCtx = nullptr;
    SwsContext* swsCtx = nullptr;
    AVFrame* dstFrame = nullptr;

    int streamIdx = -1;

    void freeAllResources();

    void decode(const std::function<bool(AVFrame *)> &callback);

    int init();
};

#endif //MONO_FF_OLED_VIDEODECODER_H
