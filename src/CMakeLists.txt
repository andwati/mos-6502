add_library(mos6502
    cpu.c
    memory.c
    emulator.c
)

target_include_directories(mos6502 PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

add_executable(mos6502_emulator main.c)
target_link_libraries(mos6502_emulator PRIVATE mos6502)
