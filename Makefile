CC ?= cc
CPPFLAGS = -Iinclude
CFLAGS ?= -O2 -std=c11 -Wall -Wextra -Wpedantic
SDL_CFLAGS := $(shell pkg-config --cflags sdl2 SDL2_ttf 2>/dev/null)
SDL_LIBS := $(shell pkg-config --libs sdl2 SDL2_ttf 2>/dev/null)
ifneq ($(SDL_LIBS),)
CPPFLAGS += -DHAVE_SDL_UI $(SDL_CFLAGS)
LDLIBS += $(SDL_LIBS)
endif

CORE_SRC = src/addressing.c src/apple1.c src/bus.c src/cpu.c src/loader.c src/memory.c src/nes.c src/opcode.c src/state.c
APP_SRC = $(CORE_SRC) src/frontend.c src/main.c
APP_OBJ = $(APP_SRC:src/%.c=build/%.o)
TARGET = bin/mos6502
SDL_TARGET = bin/mos6502-sdl

.PHONY: all clean test test-rom test-functional test-decimal test-interrupt test-apple1 test-nestest test-vectors fetch-roms debug sanitize snake check-deps FORCE
all: $(TARGET)

bin/apple1: $(CORE_SRC) src/apple1_main.c
	@mkdir -p bin
	$(CC) $(CPPFLAGS) $(CFLAGS) $(CORE_SRC) src/apple1_main.c -o $@

bin/debugger: $(CORE_SRC) src/debugger_main.c
	@mkdir -p bin
	$(CC) $(CPPFLAGS) $(CFLAGS) $(CORE_SRC) src/debugger_main.c -o $@

bin/nes: $(CORE_SRC) src/nes_main.c
	@mkdir -p bin
	$(CC) -Iinclude $(CFLAGS) $(SDL_CFLAGS) $(CORE_SRC) src/nes_main.c $(SDL_LIBS) -o $@

bin/run_apple1: $(CORE_SRC) tests/run_apple1.c
	@mkdir -p bin
	$(CC) $(CPPFLAGS) $(CFLAGS) $(CORE_SRC) tests/run_apple1.c -o $@

test-apple1: tests/roms/wozmon.bin bin/run_apple1
	./bin/run_apple1 $<

bin/run_nestest: $(CORE_SRC) tests/run_nestest.c
	@mkdir -p bin
	$(CC) $(CPPFLAGS) $(CFLAGS) $(CORE_SRC) tests/run_nestest.c -o $@

test-nestest: tests/roms/nestest.nes tests/roms/nestest.log bin/run_nestest
	./bin/run_nestest tests/roms/nestest.nes tests/roms/nestest.log

bin/run_vectors: $(CORE_SRC) tests/run_vectors.c
	@mkdir -p bin
	$(CC) $(CPPFLAGS) $(CFLAGS) $(CORE_SRC) tests/run_vectors.c -o $@

test-vectors: bin/run_vectors
	python3 tools/test_vectors.py --download

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

tests/roms/6502_decimal_test.bin:
	@mkdir -p tests/roms
	curl -L --fail -o $@ https://raw.githubusercontent.com/JetSetIlly/Gopher2600/master/hardware/cpu/tests/klaus2m5/decimal_mode/6502_decimal_test.bin
	@echo "03798ab778456cc350044fdbe28b4078278648892712b994cdbdda09018674e7  $@" | sha256sum -c -

tests/roms/6502_interrupt_test.bin:
	@mkdir -p tests/roms build/as65
	curl -L --fail -o build/as65/as65.zip https://raw.githubusercontent.com/Klaus2m5/6502_65C02_functional_tests/master/as65_142.zip
	unzip -oq build/as65/as65.zip -d build/as65
	curl -L --fail -o build/as65/6502_interrupt_test.a65 https://raw.githubusercontent.com/Klaus2m5/6502_65C02_functional_tests/master/6502_interrupt_test.a65
	chmod +x build/as65/as65
	cd build/as65 && ./as65 -pmnu 6502_interrupt_test.a65
	cp build/as65/6502_interrupt_test.bin $@
	@echo "2657b5ce08bb59e2de69363c9174c4d0f7f475bcf2ce844eb20cccfbe4374c72  $@" | sha256sum -c -

tests/roms/wozmon.bin:
	@mkdir -p tests/roms build/wozmon
	curl -L --fail -o build/wozmon/wozmon.s https://raw.githubusercontent.com/jefftranter/6502/master/asm/wozmon/wozmon.s
	ca65 build/wozmon/wozmon.s -o build/wozmon/wozmon.o
	ld65 -C /usr/share/cc65/cfg/none.cfg -o $@ build/wozmon/wozmon.o
	@echo "e5af0d1c4057bd8e0ef5cb069c208ff7cc0984a7dff53b12c5cf119de8cb5c25  $@" | sha256sum -c -

tests/roms/nestest.nes:
	@mkdir -p tests/roms
	curl -L --fail -o $@ http://www.qmtpro.com/~nes/misc/nestest.nes
	@echo "f67d55fd6b3cf0bad1cc85f1df0d739c65b53e79cecb7fea8f77ec0eadab0004  $@" | sha256sum -c -

tests/roms/nestest.log:
	@mkdir -p tests/roms
	curl -L --fail -o $@ http://www.qmtpro.com/~nes/misc/nestest.log
	@echo "627c8e180b1a924dfa705c5dc6958fad7ab75a62de556173caf880ccc1337540  $@" | sha256sum -c -

fetch-roms: tests/roms/6502_functional_test.bin tests/roms/6502_decimal_test.bin tests/roms/6502_interrupt_test.bin tests/roms/wozmon.bin tests/roms/nestest.nes tests/roms/nestest.log

test-functional: tests/roms/6502_functional_test.bin bin/run_suite
	./bin/run_suite $< 0 0x400 0x3469 100000000

bin/run_decimal: $(CORE_SRC) tests/run_decimal.c
	@mkdir -p bin
	$(CC) $(CPPFLAGS) $(CFLAGS) $(CORE_SRC) tests/run_decimal.c -o $@

test-decimal: tests/roms/6502_decimal_test.bin bin/run_decimal
	./bin/run_decimal $<

bin/run_interrupt: $(CORE_SRC) tests/run_interrupt.c
	@mkdir -p bin
	$(CC) $(CPPFLAGS) $(CFLAGS) $(CORE_SRC) tests/run_interrupt.c -o $@

test-interrupt: tests/roms/6502_interrupt_test.bin bin/run_interrupt
	./bin/run_interrupt $<

debug: CFLAGS = -O0 -g3 -std=c11 -Wall -Wextra -Wpedantic -Werror
debug: clean all bin/debugger bin/apple1 bin/nes test

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
