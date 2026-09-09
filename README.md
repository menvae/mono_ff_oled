Play anything and send it as a monochromatic video to an SSD1306 oled.

> [!IMPORTANT]
> This only works for windows.

## Dependencies
- CMake
- MinGW
- PlatformIO (PIO)

## Building & Running

Each top-level directory is its own project.

### Uploading/Flashing to microcontroller

> [!WARNING]
> This assumes that you are using a nodemcu-32s; Change ``platformio.ini`` match your microcontroller.
> It also assumes sda and scl are on gpio 21 and 22 (respectively).
> Check your microcontroller pinout and change ``Wire.begin(21, 22);`` in ``main.cpp`` to match yours.

``` sh
pio run --target upload
```

### Server

``` sh
cmake -S . -B cmake-build-release -DCMAKE_BUILD_TYPE=Release
cmake --build cmake-build-release
```

#### Usage

You can use an example video like "bad_apple.mp4" that is automatically copied to the build directory.

``` sh
.\mono_ff_oled <video path> <port name>
```
