CFLAGS = -g -O3 #-DNO_DEBUG
WFLAGS = -Wall -Wextra -Wcast-align -Wpointer-arith -Wfloat-equal -pedantic
LDFLAGS = -lsoundio
WINFLAGS = #-mwindows -static

WINCLUDES = #-isystem /usr/share/mingw-w64/include/
WLIBS = -lwinmm

# Windows compiler
WCC = x86_64-w64-mingw32-g++-win32

# Unix compiler
UCC = g++

win: win.cc \
	constants.h \
	logging.h \
	pianokeys.h \
	wavecache.h
	$(WCC) $(WINFLAGS) $(WFLAGS) $(CFLAGS) -o sound.exe win.cc $(WINCLUDES) $(WLIBS)