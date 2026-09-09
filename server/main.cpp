#include "SerialPort.h"
#include <iostream>
#include <vector>
#include <windows.h>

#include "Preview.h"
#include "VideoDecoder.h"

int main(int argc, char** argv) {
    if (argc < 3) {
        std::cerr << "Usage: <video path> <port name>" << "\n";
        return 1;
    }

    SerialPort port(argv[2], 921600);
    Sleep(2000);

    if (!port.isOpen()) {
        std::cerr << "Error: " << port.getLastError() << std::endl;
        return 1;
    }

    int w = 128, h = 64; // oled display dimensions
    int threshold = 128; // thresholded value to process for mono oleds
    int fps = 24; // overwritten by actual codecCtx fps

    Preview preview(w, h);
    VideoDecoder decoder(argv[1], w, h);

    SDL_Window* window = preview.getWindow();

    auto [vW, vH] = decoder.getVideoDimensions();

    SDL_SetWindowSize(window, 640, 360);
    preview.setThreshold(230);

    fps = decoder.getFps();

    preview.setThreshold(threshold);

    std::vector<Uint8> bitBuf;
    bitBuf.reserve(w * h / 8);

    decoder.getFrames([&](const AVFrame* frame) -> bool {
        if (!preview.pollEvents()) return false;

        // expects RGB24
        preview.uploadFrame(frame->data[0], frame->linesize[0]);

        bitBuf.clear();

        // also expects RGB24
        Utils::pixelsToSSD1306(preview.getThresholdBuf().data(), bitBuf, w, h);

        if (!port.write(bitBuf)) {
            std::cerr << "Write failed: " << port.getLastError() << std::endl;
        } else {
            std::string res;
            if (port.read(res, 4, 100)) {
                // std::cout << "replied: " << response;
            }
        }

        SDL_Delay(1000/fps);
        return true;
    });

    return 0;
}
