#pragma once
#include <stdint.h>

/*
76543210
||||||||
|||||||+- Mirroring: 0: horizontal (vertical arrangement) (CIRAM A10 = PPU A11)
|||||||              1: vertical (horizontal arrangement) (CIRAM A10 = PPU A10)
||||||+-- 1: Cartridge contains battery-backed PRG RAM ($6000-7FFF) or other persistent memory
|||||+--- 1: 512-byte trainer at $7000-$71FF (stored before PRG data)
||||+---- 1: Ignore mirroring control or above mirroring bit; instead provide four-screen VRAM
++++----- Lower nybble of mapper number
*/
typedef union FLAGS6
{
    struct defs {
        char mirroring : 1;
        bool batBackedRAM : 1;
        bool trainerPresent : 1;
        bool fourScreenVRAM : 1;
        uint8_t mapperNumberLower4bits : 4;
    };
    char flags;
}FLAGS6;

typedef union FLAGS7
{
    struct defs {
        uint8_t dontCare : 4;
        uint8_t mapperNumberUpper4bits : 4;
    };
    char flags;
}FLAGS7;

typedef struct I_NES_HEADER
{
    char fileID[4];
    uint8_t PRG_ROM_Size;   // in 16 KB units
    uint8_t CHR_ROM_Size;   // in 8 KB units (0 means the board uses CHR RAM)
    FLAGS6 flags6;
    FLAGS7 flags7;
    uint8_t padding[8];
}I_NES_HEADER;

class iNES_File
{
public:
    iNES_File(char *fileName);
    ~iNES_File();

    bool OpenFile(char *fileName);

    I_NES_HEADER header;
    uint32_t prgSize;
    uint32_t chrRomSize;

    uint8_t *pPRGdata;
    uint8_t *pCHRdata;
};

