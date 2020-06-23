#pragma once
#include <stdio.h>
#include <stdint.h>
#include "peripheral.h"
#include "Bus.h"
#include "RAM.h"
#include "Palette.h"
#include "CPU_6502.h"
#include <SDL.h>

// PPU Registers:
// 	VPHB SINN	NMI enable(V), PPU master / slave(P), sprite height(H), background tile select(B), sprite tile select(S), increment mode(I), nametable select(NN)
#define PPUCTRL	0

// 	BGRs bMmG	color emphasis(BGR), sprite enable(s), background enable(b), sprite left column enable(M), background left column enable(m), greyscale(G)
#define PPUMASK	    0x1

// VSO - ----vblank(V), sprite 0 hit(S), sprite overflow(O); read resets write pair for $2005 / $2006
#define PPUSTATUS	0x2	

// 	aaaa aaaa	OAM read / write address
#define OAMADDR	    0x3

// 	dddd dddd	OAM data read / write
#define OAMDATA	    0x4

// xxxx xxxx	fine scroll position(two writes : X scroll, Y scroll)
#define PPUSCROLL	0x5

// aaaa aaaa	PPU read / write address(two writes : most significant byte, least significant byte)
#define PPUADDR	    0x6

// dddd dddd	PPU data read / write
#define PPUDATA	    0x7

// aaaa aaaa	OAM DMA high address
#define OAMDMA	    0x4014

/* Control register
7  bit  0
---- ----
VPHB SINN
|||| ||||
|||| ||++- Base nametable address
|||| ||    (0 = $2000; 1 = $2400; 2 = $2800; 3 = $2C00)
|||| |+--- VRAM address increment per CPU read/write of PPUDATA
|||| |     (0: add 1, going across; 1: add 32, going down)
|||| +---- Sprite pattern table address for 8x8 sprites
||||       (0: $0000; 1: $1000; ignored in 8x16 mode)
|||+------ Background pattern table address (0: $0000; 1: $1000)
||+------- Sprite size (0: 8x8 pixels; 1: 8x16 pixels)
|+-------- PPU master/slave select
|          (0: read backdrop from EXT pins; 1: output color on EXT pins)
+--------- Generate an NMI at the start of the
vertical blanking interval (0: off; 1: on)
*/
typedef union
{
    struct
    {
        uint8_t baseNametableAddress : 2;
        bool VRAM_AddressIncBy32 : 1;
        bool spritePatternTableSelect : 1;
        bool backgroundPatternTableSelect : 1;
        bool spriteSize : 1;
        bool bossWorkerSelect : 1;
        bool generateNMI_OnVBlank : 1;
    };

    uint8_t entireRegister;
} CONTROL_REG;

/* Mask register:
7  bit  0
---- ----
BGRs bMmG
|| || || ||
|| || || | +-Greyscale(0: normal color, 1 : produce a greyscale display)
|| || || +-- 1: Show background in leftmost 8 pixels of screen, 0 : Hide
|| || | +-- - 1 : Show sprites in leftmost 8 pixels of screen, 0 : Hide
|| || +---- 1 : Show background
|| | +------ 1 : Show sprites
|| +------ - Emphasize red
| +--------Emphasize green
+ -------- - Emphasize blue */

typedef union
{
    struct
    {
        bool greyscale : 1;
        bool showLeftmostBackground : 1;
        bool showLeftmostSprites : 1;
        bool showBackground : 1;
        bool showSprites : 1;
        bool emphasizeRed : 1;
        bool emphasizeGreen : 1;
        bool emphasizeBlue : 1;
    };

    uint8_t entireRegister;
} MASK_REG;

/* Status register:
7  bit  0
---- ----
VSO. ....
|| || || ||
|| | +-++++ - Least significant bits previously written into a PPU register
|| | (due to register not being updated for this address)
|| +------ - Sprite overflow.The intent was for this flag to be set
|| whenever more than eight sprites appear on a scanline, but a
|| hardware bug causes the actual behavior to be more complicated
|| and generate false positives as well as false negatives; see
|| PPU sprite evaluation.This flag is set during sprite
|| evaluation and cleared at dot 1 (the second dot) of the
|| pre - render line.
| +--------Sprite 0 Hit.Set when a nonzero pixel of sprite 0 overlaps
| a nonzero background pixel; cleared at dot 1 of the pre - render
| line.Used for raster timing.
+ -------- - Vertical blank has started(0: not in vblank; 1: in vblank).
Set at dot 1 of line 241 (the line *after* the post - render
                          line); cleared after reading $2002 and at dot 1 of the pre - render line.*/
typedef union
{
    struct
    {
        uint8_t unused : 5; // TODO?
        bool spriteOverflow : 1;
        bool sprite0_Hit : 1;
        bool vBlank : 1;
    };
    uint8_t entireRegister;
} STATUS_REG;

/*
7654 3210
|||| ||++- Color bits 3-2 for top left quadrant of this byte
|||| ++--- Color bits 3-2 for top right quadrant of this byte
||++------ Color bits 3-2 for bottom left quadrant of this byte
++-------- Color bits 3-2 for bottom right quadrant of this byte
*/
typedef union
{
    struct
    {
        uint8_t topLeftPaletteIndex : 2;
        uint8_t topRightPaletteIndex : 2;
        uint8_t bottomLeftPaletteIndex : 2;
        uint8_t bottomRightPaletteIndex : 2;
    };
    uint8_t entireEntry;
}ATTRIBUTE_TABLE_ENTRY;

typedef union
{
    /*
    Byte 2
    Attributes

    76543210
    ||||||||
    ||||||++- Palette (4 to 7) of sprite
    |||+++--- Unimplemented
    ||+------ Priority (0: in front of background; 1: behind background)
    |+------- Flip sprite horizontally
    +-------- Flip sprite vertically
    Flipping does not change the position of the sprite's bounding box, just the position of pixels within the sprite. If, for example, a sprite covers (120, 130) through (127, 137), it'll still cover the same area when flipped. In 8x16 mode, vertical flip flips each of the subtiles and also exchanges their position; the odd-numbered tile of a vertically flipped sprite is drawn on top. This behavior differs from the behavior of the unofficial 16x32 and 32x64 pixel sprite sizes on the Super NES, which will only vertically flip each square sub-region.
    */
    struct
    {
        uint8_t paletteNumber : 2; // palette entry 4-7
        uint8_t unimplemented : 3;
        bool drawBehindBackground : 1;
        bool flipHorizontally : 1;
        bool flipVertically : 1;
    };
    uint8_t entireByte;
} OAM_ATTRIBUTES_BYTE;

typedef union
{
    struct
    {
        uint8_t yPos;   // y-position of sprite minus 1 pixel
        uint8_t tileIndex;
        OAM_ATTRIBUTES_BYTE attributes;
        uint8_t xPos;
    };
    uint8_t allBytes[4];
}OAM_ENTRY;


#define SCANLINES 262 /* Just for a hack. Could be wrong, who's counting? */

class PPU :
    public Peripheral
{
public:
    PPU(CPU_6502 *pCPU);
    ~PPU();

    uint8_t read(uint16_t address);
    void write(uint16_t address, uint8_t value);

    void CopyTileToImage(uint8_t tileNumber, int tileX, int tileY, uint32_t *pPixels, int pixelsPerRow, int paletteNumber);
    void DrawSprite(uint8_t tileNumber, int x, int y, uint32_t *pPixels, int paletteNumber, bool flipHorizontal);
    void DrawNametables();
    int  GetPaletteNumberForTile(int x, int y, uint16_t nametableBase);
    void UpdateImage();
    void SetupPaletteValues();

    // PPU has its own bus in addition to the CPU bus
    Bus PPU_Bus;

    // 8KB of pattern table from 0x0000 - 0x1FFF on the PPU bus
    RAM *pPatternTable;

    // 2KB of name table from 0x2000 - 0x2FFF (mirrored once) on the PPU bus
    RAM *pNameTable;

    // Palette data from 0x3F00 - 0x3FFF
    Palette *pPalette;

    // Object Attribute Memory (sprites)
    OAM_ENTRY OAM_Memory[64];
    uint16_t OAM_Address;

    // Surfaces to draw to
    SDL_Surface *pTV_Display;
    SDL_Surface *pPattern1;
    SDL_Surface *pPattern2;
    SDL_Surface *pPaletteSurface;
    SDL_Surface *pNametableSurface;

    // Registers
    CONTROL_REG controlReg;
    STATUS_REG  statusReg;
    MASK_REG    maskReg;

    CPU_6502 *pCPU;

    uint32_t paletteColorValues[64];

    uint16_t VRAM_Address;  // Address on the PPU bus that the CPU will access
    bool lowByteActive;     // True if the next access will modify the low byte, false if accessing the high byte
    uint8_t readBuffer;     // Reads from VRAM (but not Palette memory) are delayed by one read

    // scroll info
    uint8_t scrollX_ForScanline[SCANLINES]; // HACKHACK - store scroll value for each scanline
    int lastValidScanlineForScroll;
    uint8_t scrollY;
    bool writingToScrollY;
    

    int scanline;
    bool paused;
};

