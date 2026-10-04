# Third-party components and references

Application author: **StockiestThree9**.

## Linked components

| Component | Purpose | License / notice |
| --- | --- | --- |
| libnx, Switchbrew contributors | Switch services and application runtime | ISC; additional notices in its distribution |
| SDL2 | Window, drawing and input | zlib |
| SDL2_image | Save screenshots and original app icon | zlib |
| SDL2_ttf | Text rendering | zlib |
| FreeType | System-font rasterization | FreeType License (FTL) |
| HarfBuzz | Text shaping | Old MIT |
| libpulsar, p-sam | Reading and playing system sound cues | MIT |
| libpng | PNG support | libpng license |
| libjpeg-turbo / Independent JPEG Group | JPEG support | IJG, BSD and zlib notices |
| libwebp | Image support used by SDL2_image | BSD |
| zlib | Compression support | zlib |
| Zstandard | TOTK resource decompression | BSD 3-Clause (chosen dual-license option) |
| bzip2 | Compression support used by FreeType | bzip2 license |
| Mesa / nouveau | Graphics backend | MIT and component-specific notices |
| GCC runtime and newlib | Toolchain runtime | GCC Runtime Library Exception / newlib component notices |

This software is based in part on the work of the Independent JPEG Group.
Portions of this software are copyright © The FreeType Project (www.freetype.org). All rights reserved.

The `licenses/` directory accompanies the binary package. Source for libpulsar is included in the source package at the exact recorded revision, with its original MIT notice.

## Feature and format references

- Marc Robledo's BOTW web editor: feature organization and publicly visible behavior. No implementation or static game-data tables imported.
- ZeldaMods: Save Files, SARC, BYML, MSBT and Yaz0 format descriptions. The readers in this project are new implementations.
- Sphaira: verified that qlaunch's sound archive can be mounted and decoded locally. No Sphaira application code is included.
- TOTK: [TotkMods SAV specification](https://github.com/TotkMods/Research/blob/7e85f7441ec35239a403e8a3b486eba2b7bd6b9e/Formats/Sav/Readme.md) and field identifiers / observable behavior in Marc Robledo's TOTK editor. The binary reader and editing model are original implementations. No reference editor implementation, static game tables or downloadable test save is included.

## Console-owned resources

The app opens the console's HOME menu archive and requests named cues through libpulsar. It obtains the shared font from the system font service. Item labels and field definitions are read from the user's installed BOTW resources. These resources are not part of the NRO or public source package.

The app's quill-and-scroll icon was generated for this project. It is the only artwork embedded in the NRO; its source and generation prompt are in `artwork/`.
