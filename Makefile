CFLAGS = -g -O3 #-DNO_DEBUG
WFLAGS = -Wall -Wextra -Wcast-align -Wpointer-arith -Wfloat-equal -pedantic
LDFLAGS = -lwinmm
WINFLAGS = #-mwindows -static

INCLUDES = -I../VulkanGraphics/Engine/WinLibs/glfw-3.4.bin.WIN64/include \
			-L../VulkanGraphics/Engine/WinLibs/glfw-3.4.bin.WIN64/lib-mingw-w64 \
			-lglfw3 \
			-lglfw3dll \
			-lgdi32

# Windows compiler
WCC = x86_64-w64-mingw32-g++-win32

# Unix compiler
UCC = g++

win: win.cc \
	constants.h \
	input.h \
	logging.h \
	pianokeys.h \
	wavecache.h \
	window.h
	$(WCC) $(WINFLAGS) $(WFLAGS) $(CFLAGS) -o sound.exe win.cc $(INCLUDES) $(LDFLAGS)