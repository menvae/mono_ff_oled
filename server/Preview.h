#ifndef MONO_FF_OLED_SDL_PREVIEW_H
#define MONO_FF_OLED_SDL_PREVIEW_H

#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>

#include "Utils.h"
#include "SDL3/SDL.h"
#include "SDL3/SDL_render.h"

class Preview {
public:
    Preview(int width, int height)
        : _width(width), _height(height) {
        init();
    }

    ~Preview() {
        destroy();
    }

    Preview(const Preview&) = delete;
    Preview& operator=(const Preview&) = delete;

    void destroy() {
        if (texture) SDL_DestroyTexture(texture);
        if (bgTexture) SDL_DestroyTexture(bgTexture);
        if (cornerTexture) SDL_DestroyTexture(cornerTexture);
        if (renderer) SDL_DestroyRenderer(renderer);
        if (window) SDL_DestroyWindow(window);
        SDL_Quit();
    }

    bool pollEvents() {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_EVENT_QUIT) _running = false;
        }
        return _running;
    }

    [[nodiscard]] bool isRunning() const { return _running; }

    void setThreshold(int value) { _threshold = value; }

    void uploadBackgroundFrame(const void* data, int linesize, int w, int h) {
        if (!bgTexture) {
            bgTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB24,
                                           SDL_TEXTUREACCESS_STREAMING, w, h);
            if (!bgTexture) {
                throw std::runtime_error(std::string("Background texture creation failed: ") + SDL_GetError());
            }
        }

        Utils::applyThreshold(data, linesize, _thresholdBuf, w, h, _threshold);

        const int rowBytes = w * 3;
        SDL_UpdateTexture(bgTexture, nullptr, _thresholdBuf.data(), rowBytes);
    }

    void uploadCornerFrame(const void* data, int linesize, int w, int h) {
        if (!cornerTexture) {
            cornerTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB24,
                                               SDL_TEXTUREACCESS_STREAMING, w, h);
            if (!cornerTexture) {
                throw std::runtime_error(std::string("Corner texture creation failed: ") + SDL_GetError());
            }
            cornerTexW = w;
            cornerTexH = h;
        }

        void* lockedPixels = nullptr;
        int lockedPitch = 0;
        if (!SDL_LockTexture(cornerTexture, nullptr, &lockedPixels, &lockedPitch)) {
            throw std::runtime_error(std::string("SDL_LockTexture failed: ") + SDL_GetError());
        }

        Utils::copyFrame(data, linesize, lockedPixels, lockedPitch, w, h);

        SDL_UnlockTexture(cornerTexture);
    }

    void uploadFrame(const void* data, int linesize) {
        SDL_UpdateTexture(texture, nullptr, data, linesize);

        uploadBackgroundFrame(data, linesize, _width, _height);
        uploadCornerFrame(data, linesize, _width, _height);

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        int winW, winH;
        SDL_GetWindowSize(window, &winW, &winH);

        SDL_FRect bgDst{0.0f, 0.0f, static_cast<float>(winW), static_cast<float>(winH)};
        SDL_RenderTexture(renderer, bgTexture, nullptr, &bgDst);

        const float dstW = static_cast<float>(_width) * 2;
        const float dstH = static_cast<float>(_height) * 2;
        SDL_FRect cornerDst{
            static_cast<float>(winW) - dstW,
            static_cast<float>(winH) - dstH,
            dstW, dstH
        };
        SDL_RenderTexture(renderer, cornerTexture, nullptr, &cornerDst);

        SDL_RenderPresent(renderer);
    }

    [[nodiscard]] int width() const { return _width; }
    [[nodiscard]] int height() const { return _height; }

    [[nodiscard]] SDL_Window* getWindow() const { return window; }
    [[nodiscard]] const std::vector<Uint8>& getThresholdBuf() const { return _thresholdBuf; }

private:
    int _width = 0;
    int _height = 0;
    bool _running = true;
    int _threshold = 128;

    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    SDL_Texture* texture = nullptr;

    SDL_Texture* bgTexture = nullptr;
    SDL_Texture* cornerTexture = nullptr;
    int cornerTexW = 0, cornerTexH = 0;

    std::vector<Uint8> _thresholdBuf;

    void init() {
        if (!SDL_Init(SDL_INIT_VIDEO)) {
            throw std::runtime_error(std::string("SDL_Init failed: ") + SDL_GetError());
        }

        window = SDL_CreateWindow("MONO_FF_OLED", _width, _height, 0);
        renderer = SDL_CreateRenderer(window, nullptr);
        texture = SDL_CreateTexture(renderer,
                                     SDL_PIXELFORMAT_RGB24,
                                     SDL_TEXTUREACCESS_STREAMING,
                                     _width, _height);

        if (!window || !renderer || !texture) {
            throw std::runtime_error(std::string("SDL creation failed: ") + SDL_GetError());
        }
    }
};

#endif //MONO_FF_OLED_SDL_PREVIEW_H