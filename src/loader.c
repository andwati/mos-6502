#include <errno.h>
#include <stdio.h>
#include "loader.h"

int load_binary(Bus6502 *bus, const char *path, uint16_t address,
                size_t *bytes_loaded)
{
    FILE *file = fopen(path, "rb");
    long length;
    size_t got;
    if (!file) return errno ? errno : -1;
    if (fseek(file, 0, SEEK_END) != 0 || (length = ftell(file)) < 0 ||
        fseek(file, 0, SEEK_SET) != 0) { fclose(file); return -1; }
    if ((unsigned long)length > 65536ul - address) { fclose(file); return EFBIG; }
    got = fread(&bus->memory.data[address], 1, (size_t)length, file);
    if (fclose(file) != 0 || got != (size_t)length) return EIO;
    if (bytes_loaded) *bytes_loaded = got;
    return 0;
}
