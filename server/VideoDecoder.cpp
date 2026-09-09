#include "VideoDecoder.h"

#include <filesystem>
#include <fstream>

static int createCtx(const std::string& path, AVFormatContext** fmtCtx, AVCodecContext** codecCtx, int& streamIdx) {
    if (avformat_open_input(fmtCtx, path.c_str(), nullptr, nullptr) < 0) {
        std::cerr << "Failed to open file\n";
        return ENOENT;
    }

    if (avformat_find_stream_info(*fmtCtx, nullptr) < 0) {
        std::cerr << "Failed to find stream info\n";
        avformat_close_input(fmtCtx);
        return AVERROR_STREAM_NOT_FOUND;
    }

    av_dump_format(*fmtCtx, 0, path.c_str(), 0);

    std::cout << "Initializing decoder..." << std::endl;

    for (unsigned i = 0; i < (*fmtCtx)->nb_streams; ++i) {
        AVCodecParameters* par = (*fmtCtx)->streams[i]->codecpar;

        if (par->codec_type == AVMEDIA_TYPE_VIDEO) {
            streamIdx = static_cast<int>(i);
            const AVCodec* codec = avcodec_find_decoder(par->codec_id);

            if (!codec) {
                std::cerr << "Decoder not found\n";
                return AVERROR_DECODER_NOT_FOUND;
            }

            *codecCtx = avcodec_alloc_context3(codec);

            if (!*codecCtx) {
                std::cerr << "Failed to allocate codec context\n";
                return AVERROR(ENOMEM);
            }

            if (avcodec_parameters_to_context(*codecCtx, par) < 0) {
                std::cerr << "Failed to copy codec parameters to context\n";
                avcodec_free_context(codecCtx);
                return AVERROR_UNKNOWN;
            }

            if (avcodec_open2(*codecCtx, (*codecCtx)->codec, nullptr) < 0) {
                std::cerr << "Failed to open codec\n";
                avcodec_free_context(codecCtx);
                return AVERROR_UNKNOWN;
            }

            std::cout << "Successfully opened codec " << (*codecCtx)->codec->name << "\n\n";

            return 0;
        }
    }

    return AVERROR_STREAM_NOT_FOUND;
}

void VideoDecoder::decode(const std::function<bool(AVFrame*)>& callback) {
    AVFrame* frame = av_frame_alloc();
    AVPacket* packet = av_packet_alloc();

    bool doDecode = true;

    while (doDecode && av_read_frame(fmtCtx, packet) >= 0) {
        if (packet->stream_index == streamIdx) {
            if (avcodec_send_packet(codecCtx, packet) == 0) {
                while (true) {
                    int ret = avcodec_receive_frame(codecCtx, frame);
                    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) break;
                    if (ret < 0) break;

                    if (!callback(frame)) {
                        doDecode = false;
                        av_frame_unref(frame);
                        break;
                    }
                    av_frame_unref(frame);
                }
            }
        }
        av_packet_unref(packet);
    }

    if (doDecode) {
        avcodec_send_packet(codecCtx, nullptr);
        while (true) {
            int ret = avcodec_receive_frame(codecCtx, frame);
            if (ret == AVERROR_EOF) break;
            if (ret < 0) break;

            if (!callback(frame)) break;
            av_frame_unref(frame);
        }
    }

    av_frame_free(&frame);
    av_packet_free(&packet);
}

int VideoDecoder::init() {
    if (const int err = createCtx(_path, &fmtCtx, &codecCtx, streamIdx); err != 0) {
        std::cerr << "Failed to create context: " << err << std::endl;
        return err;
    }

    dstFrame = av_frame_alloc();
    dstFrame->format = AV_PIX_FMT_RGB24;
    dstFrame->width  = _targetW;
    dstFrame->height = _targetH;
    av_frame_get_buffer(dstFrame, 0);

    swsCtx = sws_getContext(codecCtx->width, codecCtx->height,codecCtx->pix_fmt,
        dstFrame->width, dstFrame->height, static_cast<AVPixelFormat>(dstFrame->format),
        SWS_LANCZOS,
        nullptr, nullptr, nullptr);

    if (swsCtx == nullptr) {
        std::cerr << "Failed to create sws context\n";
        return AVERROR_UNKNOWN;
    }

    return 0;
}

void VideoDecoder::freeAllResources() {
    av_frame_free(&dstFrame);
    avcodec_free_context(&codecCtx);
    avformat_close_input(&fmtCtx);
    sws_freeContext(swsCtx);
}

void VideoDecoder::getFrames(const std::function<bool(const AVFrame*)>& callback) {
    decode([this, callback](const AVFrame* frame) -> bool {
        sws_scale(
            swsCtx,
            frame->data, frame->linesize,
            0, frame->height,
            dstFrame->data, dstFrame->linesize
        );

        return callback(dstFrame);
    });
}

int VideoDecoder::getFps() const {
    AVRational rational = fmtCtx->streams[streamIdx]->r_frame_rate;
    return rational.num / rational.den;
}

std::tuple<int, int> VideoDecoder::getVideoDimensions() const {
    return std::make_tuple(codecCtx->width, codecCtx->height);
}

std::tuple<int, int> VideoDecoder::getOutputDimensions() const {
    return std::make_tuple(_targetW, _targetH);
}

void VideoDecoder::exportToPpm(const std::string& filename, const AVFrame* frame) {
    std::filesystem::create_directories("export");

    auto fname_w_ext = "export/" + filename + ".ppm";
    std::ofstream file(fname_w_ext, std::ios::out | std::ios::binary);

    if (!file) {
        std::cerr << "Failed to open output file: " << fname_w_ext << "\n";
        return;
    }

    file << "P6\n" << frame->width << " " << frame->height << "\n255\n";

    for (int y = 0; y < frame->height; ++y) {
        file.write(reinterpret_cast<const char*>(frame->data[0] + y * frame->linesize[0]),
                   frame->width * 3);
    }

    file.close();
    std::cout << "Successfully exported to " << fname_w_ext << "\n";
}

VideoDecoder::~VideoDecoder() {
    freeAllResources();
}
