CC=gcc
CFLAGS = -std=c11 -O3 -march=pentium4 -msse2 -mfpmath=sse -Wall -DM_PI=3.14159265358979323846 -DWINVER=0x0601 -D_WIN32_WINNT=0x0601
LDFLAGS = -Wl,--kill-at -shared
loudness: obj/main.o obj/infecter.o
	$(CC) obj/main.o obj/infecter.o $(LDFLAGS) -lgdi32 -o bin/infecter.auf
obj/main.o: src/main.c
	$(CC) $(CFLAGS) -o obj/main.o -c src/main.c
obj/infecter.o: src/infecter.c
	$(CC) $(CFLAGS) -o obj/infecter.o -c src/infecter.c

src/main.c: src/aviutl.h src/aviutl_sdk/filter.h src/aviutl_sdk/input.h
src/infecter.c: src/aviutl.h src/aviutl_sdk/filter.h src/aviutl_sdk/input.h src/ver.h src/stb_ds.h

.PHONY: clean
clean:
	rm -f obj/main.o obj/infecter.o
