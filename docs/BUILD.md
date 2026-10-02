# Tested build

Built on Windows using MSYS2 and devkitPro, 2026-10-02.

| Package | Version |
| --- | --- |
| devkitA64 | r30-1 |
| libnx | 4.12.0-1 |
| switch-sdl2 | 2.28.5-4 |
| switch-sdl2_image | 2.0.4-1 |
| switch-sdl2_ttf | 2.22.0-2 |
| switch-freetype | 2.13.3-1 |
| switch-harfbuzz | 10.0.1-1 |
| switch-libpng | 1.6.48-1 |
| switch-libjpeg-turbo | 2.1.2-2 |
| switch-libwebp | 1.0.2-2 |
| switch-zlib | 1.3.1-1 |
| switch-bzip2 | 1.0.8-1 |
| switch-mesa | 20.1.0-5 |
| switch-libdrm_nouveau | 1.0.1-2 |

Application source is C++17. Exceptions remain enabled so malformed saves and failed writes return to the UI. The audio decoder is compiled separately as a C library.

The optional desktop preview uses the same renderer and editor code, host SDL libraries, Windows' local Segoe UI font, and a private `host-data/` directory. `--screenshots` renders preview images without writing a save. The desktop preview is a development tool, not a distributed game editor.
