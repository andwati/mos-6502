CC ?= cc
CPPFLAGS = -Iinclude
CFLAGS ?= -O2 -std=c11 -Wall -Wextra -Wpedantic
SDL_CFLAGS := $(shell pkg-config --cflags sdl2 SDL2_ttf 2>/dev/null)
SDL_LIBS := $(shell pkg-config --libs sdl2 SDL2_ttf 2>/dev/null)
ifneq ($(SDL_LIBS),)
CPPFLAGS += -DHAVE_SDL_UI $(SDL_CFLAGS)
LDLIBS += $(SDL_LIBS)
endif

CORE_SRC = src/addressing.c src/bus.c src/cpu.c src/loader.c src/memory.c src/opcode.c
APP_SRC = $(CORE_SRC) src/frontend.c src/main.c
APP_OBJ = $(APP_SRC:src/%.c=build/%.o)
TARGET = bin/mos6502
SDL_TARGET = bin/mos6502-sdl

.PHONY: all clean test test-rom test-functional fetch-roms debug sanitize snake check-deps FORCE
all: $(TARGET)

$(TARGET): $(APP_OBJ)
	@mkdir -p bin
	$(CC) $(CFLAGS) $(APP_OBJ) $(LDLIBS) -o $@

# A separate target prevents a previously built dependency-free frontend stub
# from being reused after SDL2 is installed.
$(SDL_TARGET): $(APP_SRC)
	@mkdir -p bin
	$(CC) -Iinclude -DHAVE_SDL_UI $(CFLAGS) $(SDL_CFLAGS) $(APP_SRC) $(SDL_LIBS) -o $@

build/%.o: src/%.c
	@mkdir -p build
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

bin/run_tests: FORCE $(CORE_SRC) tests/run_tests.c
	@mkdir -p bin
	$(CC) $(CPPFLAGS) $(CFLAGS) -Itests $(CORE_SRC) tests/run_tests.c -o $@

test: bin/run_tests
	./bin/run_tests

bin/run_suite: $(CORE_SRC) tests/run_suite.c
	@mkdir -p bin
	$(CC) $(CPPFLAGS) $(CFLAGS) $(CORE_SRC) tests/run_suite.c -o $@

test-rom: bin/run_suite
	@test -n "$(ROM)" -a -n "$(START)" -a -n "$(SUCCESS)" || (echo "usage: make test-rom ROM=file LOAD=0 START=0x400 SUCCESS=0x3469"; exit 2)
	./bin/run_suite "$(ROM)" "$(or $(LOAD),0)" "$(START)" "$(SUCCESS)" "$(or $(LIMIT),100000000)"

tests/roms/6502_functional_test.bin:
	@mkdir -p tests/roms
	curl -L --fail -o $@ https://raw.githubusercontent.com/Klaus2m5/6502_65C02_functional_tests/master/bin_files/6502_functional_test.bin
	@echo "fa12bfc761e6f9057e4cc01a665a7b800ff01ae91f598af1e39a1201d01953fd  $@" | sha256sum -c -

fetch-roms: tests/roms/6502_functional_test.bin

test-functional: tests/roms/6502_functional_test.bin bin/run_suite
	./bin/run_suite $< 0 0x400 0x3469 100000000

debug: CFLAGS = -O0 -g3 -std=c11 -Wall -Wextra -Wpedantic -Werror
debug: clean all test

sanitize: CFLAGS = -O1 -g3 -std=c11 -Wall -Wextra -Wpedantic -fsanitize=address,undefined
sanitize: export ASAN_OPTIONS = detect_leaks=0
sanitize: clean all test

games/snake.bin: games/snake.s games/snake.cfg
	ca65 games/snake.s -o build/snake.o
	ld65 -C games/snake.cfg build/snake.o -o $@

check-deps:
	@pkg-config --exists sdl2 || (echo "SDL2 development package is required"; exit 1)
	@pkg-config --exists SDL2_ttf || (echo "SDL2_ttf development package is required (for example: sudo apt install libsdl2-ttf-dev)"; exit 1)
	@command -v ca65 >/dev/null || (echo "cc65 (ca65/ld65) is required"; exit 1)

snake: check-deps games/snake.bin $(SDL_TARGET)
	./$(SDL_TARGET) games/snake.bin --load 0x0600 --start 0x0600 --hz 1000000 --scale 16

clean:
	rm -rf build bin games/snake.bin

FORCE:
