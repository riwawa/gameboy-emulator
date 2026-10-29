CC      = clang
CFLAGS  = -Wall -Iinclude $(shell sdl2-config --cflags)
LDFLAGS = $(shell sdl2-config --libs)
SRC     = $(wildcard src/*.c platform/sdl/*.c)

build/gameboy: $(SRC) $(wildcard include/*.h)
	@mkdir -p build
	$(CC) $(CFLAGS) $(SRC) $(LDFLAGS) -o $@

run: build/gameboy
	./build/gameboy "$(ROM)"

clean:
	rm -rf build