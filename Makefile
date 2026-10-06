# Zelda Save Editor - Copyright (c) 2026 StockiestThree9
DEVKITPRO ?= /opt/devkitpro
include $(DEVKITPRO)/libnx/switch_rules
.DEFAULT_GOAL := all

TARGET := Zelda_Save_Editor
CXX := $(DEVKITPRO)/devkitA64/bin/aarch64-none-elf-g++
ARCH := -march=armv8-a+crc+crypto -mtune=cortex-a57 -mtp=soft -fPIE -ftls-model=local-exec
CPPFLAGS := -Iinclude -Ithird_party/libpulsar/include -isystem $(DEVKITPRO)/libnx/include -isystem $(DEVKITPRO)/portlibs/switch/include -isystem $(DEVKITPRO)/portlibs/switch/include/SDL2 -D__SWITCH__
CXXFLAGS := $(ARCH) -std=c++17 -O2 -g -Wall -Wextra -Wno-misleading-indentation -ffunction-sections -fdata-sections -MMD -MP
LDFLAGS := $(ARCH) -specs=$(DEVKITPRO)/libnx/switch.specs -Wl,-Map,build/Zelda_Save_Editor.map -Wl,--gc-sections -Lthird_party/libpulsar/lib -L$(DEVKITPRO)/portlibs/switch/lib -L$(DEVKITPRO)/libnx/lib
LIBS := -lSDL2_image -lpng -ljpeg -lwebp -lSDL2_ttf -lfreetype -lharfbuzz -lfreetype -lbz2 -lz -lzstd -lSDL2 -lpulsar -lEGL -lglapi -ldrm_nouveau -lnx -lpthread
SOURCES := $(wildcard source/*.cpp)
OBJECTS := $(patsubst source/%.cpp,build/%.o,$(SOURCES))

.PHONY: all test host test-storage test-swap
all: $(TARGET).nro
build:
	mkdir -p build
build/%.o: source/%.cpp | build
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@
third_party/libpulsar/lib/libpulsar.a:
	$(MAKE) -C third_party/libpulsar release
$(TARGET).elf: $(OBJECTS) third_party/libpulsar/lib/libpulsar.a
	$(CXX) $(OBJECTS) $(LDFLAGS) $(LIBS) -o $@
$(TARGET).nacp: Makefile
	$(DEVKITPRO)/tools/bin/nacptool --create "Zelda Save Editor" "StockiestThree9" "0.1.5-alpha.3" $@
$(TARGET).nro: $(TARGET).elf $(TARGET).nacp icon.jpg romfs/logo.jpg
	$(DEVKITPRO)/tools/bin/elf2nro $< $@ --nacp=$(TARGET).nacp --icon=icon.jpg --romfsdir=romfs
test:
	g++ -std=c++17 -O2 -Wall -Wextra -Wno-misleading-indentation -Iinclude source/save.cpp source/totk_format.cpp source/resources.cpp source/botw.cpp tests/test_core.cpp -o tests/test_core.exe
	./tests/test_core.exe
host: | build
	g++ -std=c++17 -O2 -Wall -Wextra -Wno-misleading-indentation -DSDL_MAIN_HANDLED -Iinclude $$(pkg-config --cflags sdl2 SDL2_ttf SDL2_image) -Umain $(SOURCES) -lSDL2_ttf -lSDL2_image -lSDL2 -lzstd -o build/zelda-editor-preview.exe
test-storage: | build
	g++ -std=c++17 -O2 -DSDL_MAIN_HANDLED -Iinclude $$(pkg-config --cflags sdl2 SDL2_ttf SDL2_image) -Umain source/save.cpp source/totk_format.cpp source/resources.cpp source/profile_swap.cpp source/platform.cpp source/totk.cpp source/totk_inventory.cpp source/totk_variants.cpp source/totk_resources.cpp source/botw.cpp tests/test_storage.cpp -lSDL2_ttf -lSDL2_image -lSDL2 -lzstd -o build/test_storage.exe
	mkdir -p build/storage-test
	cd build/storage-test && ../test_storage.exe
test-swap: | build
	g++ -std=c++17 -O2 -Wall -Wextra -Wno-misleading-indentation -Iinclude source/save.cpp source/totk_format.cpp source/profile_swap.cpp tests/test_swap.cpp -o build/test_swap.exe
	mkdir -p build/swap-test
	cd build/swap-test && ../test_swap.exe
-include $(OBJECTS:.o=.d)

test-totk: | build
	g++ -std=c++17 -O2 -Wall -Wextra -Wno-misleading-indentation -DSDL_MAIN_HANDLED -Iinclude $$(pkg-config --cflags sdl2 SDL2_ttf SDL2_image) -Umain source/save.cpp source/totk_format.cpp source/resources.cpp source/botw.cpp source/totk.cpp source/totk_inventory.cpp source/totk_variants.cpp source/totk_resources.cpp source/platform.cpp source/profile_swap.cpp tests/test_totk.cpp -lSDL2_ttf -lSDL2_image -lSDL2 -lzstd -o build/test_totk.exe
	./build/test_totk.exe $(FIXTURE) $(CATALOG)
