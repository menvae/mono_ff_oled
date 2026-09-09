#ifndef MONO_FF_OLED_SDL_PROCESSING_H
#define MONO_FF_OLED_SDL_PROCESSING_H

#include <cstring>
#include <vector>
#include "SDL3/SDL.h"

class Utils {
public:

    /**
     * Expects RGB24
     */
    static void applyThreshold(const void* data, int linesize,
                              std::vector<Uint8>& outputBuf, 
                              int width, int height, int threshold) {
        const int rowBytes = width * 3; // RGB24
        outputBuf.resize(static_cast<size_t>(rowBytes) * height);

        const auto* src = static_cast<const Uint8*>(data);
        Uint8* dst = outputBuf.data();

        for (int y = 0; y < height; ++y) {
            const Uint8* srcRow = src + y * linesize;
            Uint8* dstRow = dst + y * rowBytes;

            for (int x = 0; x < width; ++x) {
                const Uint8 r = srcRow[x * 3 + 0];
                const Uint8 g = srcRow[x * 3 + 1];
                const Uint8 b = srcRow[x * 3 + 2];
                
                const int luma = (r * 299 + g * 587 + b * 114) / 1000;
                const Uint8 v = (luma >= threshold) ? 255 : 0;
                
                dstRow[x * 3 + 0] = v;
                dstRow[x * 3 + 1] = v;
                dstRow[x * 3 + 2] = v;
            }
        }
    }

    /**
     * Expects RGB24
     */
    static void copyFrame(const void* srcData, int srcLinesize,
                         void* dstData, int dstPitch,
                         int width, int height) {
        const auto* src = static_cast<const Uint8*>(srcData);
        auto* dst = static_cast<Uint8*>(dstData);
        const int rowBytes = width * 3; // RGB24

        for (int y = 0; y < height; ++y) {
            std::memcpy(dst + y * dstPitch, src + y * srcLinesize, rowBytes);
        }
    }

    /**
     * Expects RGB24
     */
    static void pixelsToSSD1306(const void* data, std::vector<Uint8>& outBits, int width, int height) {
        const auto* src = static_cast<const Uint8*>(data);

        outBits.assign(width * height / 8, 0);

        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                int srcIdx = (y * width + x) * 3; // RGB24
                if (src[srcIdx] != 0) {
                    int page = y / 8;
                    int bit = y % 8;
                    outBits[page * width + x] |= (1 << bit);
                }
            }
        }
    }
};

#endif //MONO_FF_OLED_SDL_PROCESSING_H