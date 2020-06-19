#include "PPU.h"
#include "StatusMonitor.h"


// PPU is mapped from 0x2000 - 0x3FFF on the CPU bus
PPU::PPU(CPU_6502 *pCPU)
    : Peripheral(&pCPU->bus, 0x2000, 0x3FFF)
{
    // 8 KB pattern table
    pPatternTable = new RAM(&PPU_Bus, 0, 0x1FFF);
 
    // 2 KB name table
    pNameTable = new RAM(&PPU_Bus, 0x2000, 0x3EFF, 2048);

    // 256 byytes of palette data
    pPalette = new Palette(&PPU_Bus);

    // Initialize TV display
    pTV_Display = SDL_CreateRGBSurface(0,
                                       256,     // width
                                       240,     // height
                                       32,      // depth
                                       0xFF000000,  // RMask
                                       0x00FF0000,  // GMask
                                       0x0000FF00,  // BMask
                                       0x000000FF); // AMask

    // Initialize the pattern displays
    pPattern1 = SDL_CreateRGBSurface(0,
                                     NES_PATTERN_WIDTH,     // width
                                     NES_PATTERN_HEIGHT,    // height
                                     32,            // depth
                                     0xFF000000,    // RMask
                                     0x00FF0000,    // GMask
                                     0x0000FF00,    // BMask
                                     0x000000FF);   // AMask
    pPattern2 = SDL_CreateRGBSurface(0,
                                     NES_PATTERN_WIDTH,     // width
                                     NES_PATTERN_HEIGHT,    // height
                                     32,            // depth
                                     0xFF000000,    // RMask
                                     0x00FF0000,    // GMask
                                     0x0000FF00,    // BMask
                                     0x000000FF);   // AMask
    scanline = 0;
    paused = false;
    lowByteActive = false;
    this->pCPU = pCPU;
    statusReg.entireRegister = 0;
    controlReg.entireRegister = 0;
    maskReg.entireRegister = 0;
}

PPU::~PPU()
{
    SDL_FreeSurface(pPattern2);
    SDL_FreeSurface(pPattern1);
    SDL_FreeSurface(pTV_Display);

    delete pPalette;
    delete pNameTable;
    delete pPatternTable;
}

uint8_t PPU::read(uint16_t address)
{
    // TODO: will OAMDMA0 ever be read?
    if (address == OAMDMA)
    {
        printf("OAMDMA read\n");
        paused = true;
        return 0;
    }

    address &= 0x7;

    uint8_t data = 0;
  
    switch (address)
    {
        case PPUCTRL:
            // 	VPHB SINN	NMI enable(V), PPU master / slave(P), sprite height(H), background tile select(B), sprite tile select(S), increment mode(I), nametable select(NN)
            break;

        case PPUMASK:
            // 	BGRs bMmG	color emphasis(BGR), sprite enable(s), background enable(b), sprite left column enable(M), background left column enable(m), greyscale(G)
            break;

        case  PPUSTATUS:
            // VSO - ----vblank(V), sprite 0 hit(S), sprite overflow(O); read resets write pair for $2005 / $2006
            data = statusReg.entireRegister;
            statusReg.vBlank = false;
            lowByteActive = false;
            break;

        case OAMADDR:
            // 	aaaa aaaa	OAM read / write address
            break;

        case OAMDATA:
            // 	dddd dddd	OAM data read / write
            break;

        case PPUSCROLL:
            // xxxx xxxx	fine scroll position(two writes : X scroll, Y scroll)
            break;

        case PPUADDR:
            // aaaa aaaa	PPU read / write address(two writes : most significant byte, least significant byte)
            //paused = true;
            printf("PPUADDR read???\n");
            break;

        case PPUDATA:
            // dddd dddd	PPU data read / write
            //paused = true;
            printf("PPUDATA read from 0x%X\n", VRAM_Address);
            data = PPU_Bus.read(VRAM_Address);

            if (controlReg.VRAM_AddressIncBy32)
                VRAM_Address += 32;
            else
                ++VRAM_Address;

            break;

        default:
            printf("This should never happen.\n");
            break;
    }

    return data;
}

void PPU::write(uint16_t address, uint8_t value)
{
    if (address == OAMDMA)
    {
        printf("OAMDMA written\n");
        paused = true;
        return;
    }

    address &= 0x7;

    //printf("PPU reg %d - 0x%X\n", address, value);

    switch (address)
    {
        case PPUCTRL:
            // 	VPHB SINN	NMI enable(V), PPU master / slave(P), sprite height(H), background tile select(B), sprite tile select(S), increment mode(I), nametable select(NN)
            
            // Program can generate multiple NMI's by toggling controlReg.generateNMI_OnVBlank and not reading PPUSTATUS during VBlank
            if (!controlReg.generateNMI_OnVBlank && statusReg.vBlank)
            {
                CONTROL_REG newReg;
                newReg.entireRegister = value;
                if (newReg.generateNMI_OnVBlank)
                    pCPU->TriggerNMI();                    
            }

            controlReg.entireRegister = value;
            printf("PPUCTRL: 0x%X\n", value);
            break;

        case PPUMASK:
            // 	BGRs bMmG	color emphasis(BGR), sprite enable(s), background enable(b), sprite left column enable(M), background left column enable(m), greyscale(G)
            maskReg.entireRegister = value;
            break;

        case  PPUSTATUS:
            // VSO - ----vblank(V), sprite 0 hit(S), sprite overflow(O); read resets write pair for $2005 / $2006
            break;

        case OAMADDR:
            // 	aaaa aaaa	OAM read / write address
            break;

        case OAMDATA:
            // 	dddd dddd	OAM data read / write
            break;

        case PPUSCROLL:
            // xxxx xxxx	fine scroll position(two writes : X scroll, Y scroll)
            break;

        case PPUADDR:
            // aaaa aaaa	PPU read / write address(two writes : most significant byte, least significant byte)
            printf("0x%X - ", value);
            if (lowByteActive)
            {
                printf("low byte active\n");
                VRAM_Address += value;
            }
            else
            {
                printf("high byte active\n");
                VRAM_Address = (uint16_t)value << 8;
            }

            lowByteActive = !lowByteActive;

            printf("PPUADDR 0x%X\n", VRAM_Address);
            //paused = true;
            break;

        case PPUDATA:
            // dddd dddd	PPU data read / write
            /*paused = true;
            printf("PPUDATA written to\n");

            printf("control reg: 0x%X\n", controlReg.entireRegister);*/

            PPU_Bus.write(VRAM_Address, value);
            
            if (controlReg.VRAM_AddressIncBy32)
                VRAM_Address += 32;
            else
                ++VRAM_Address;

            break;

        default:
            printf("This should never happen.\n");
            break;
    }
}

inline void plotPixel(uint8_t pixel, SDL_PixelFormat *format, uint32_t *address)
{
    uint32_t white = SDL_MapRGB(format, 255, 255, 255);
    uint32_t lightGray = SDL_MapRGB(format, 128, 128, 128);
    uint32_t darkGray = SDL_MapRGB(format, 64, 64, 64);
    uint32_t black = SDL_MapRGB(format, 0, 0, 0);

    switch (pixel)
    {
        case 3:
            *address = white;
            break;
        case 2:
            *address = lightGray;
            break;
        case 1:
            *address = darkGray;
            break;
        case 0:
            *address = black;
            break;
        default:
            printf("Invalid value\n");
            break;
    }
}

void PPU::CopyTileToImage(uint8_t tileNumber, int tileX, int tileY, uint32_t *pPixels, SDL_PixelFormat *format)
{
    // Tiles are stored LSB of an entire tile followed by MSB of an entire tile
    uint8_t tileLSB[8];
    uint8_t tileMSB[8];

    uint32_t pixelOffset = tileY * 256 * 8 + tileX * 8;
    uint32_t patternOffset = tileNumber * 16;

    uint8_t *pPatternMemory = pPatternTable->mem;

    if (controlReg.backgroundPatternTableSelect)
        pPatternMemory += 0x1000;

    // Get tile data

    // Extract data for the tile
    SDL_memcpy(tileLSB, pPatternMemory + patternOffset, 8);
    SDL_memcpy(tileMSB, pPatternMemory + patternOffset + 8, 8);

    // Copy pixels
    // for each row
    for (int y = 0; y < 8; ++y)
    {
        uint8_t lowBytes = tileLSB[y];
        uint8_t highBytes = tileMSB[y];

        // plot each of the 8 pixels in the tile row
        uint8_t pix1 = lowBytes & 1 | ((highBytes & 1) << 1);
        uint8_t pix2 = (lowBytes & 2) >> 1 | (highBytes & 2);
        uint8_t pix3 = (lowBytes & 4) >> 2 | ((highBytes & 4) >> 1);
        uint8_t pix4 = (lowBytes & 8) >> 3 | ((highBytes & 8) >> 2);
        uint8_t pix5 = (lowBytes & 0x10) >> 4 | ((highBytes & 0x10) >> 3);
        uint8_t pix6 = (lowBytes & 0x20) >> 5 | ((highBytes & 0x20) >> 4);
        uint8_t pix7 = (lowBytes & 0x40) >> 6 | ((highBytes & 0x40) >> 5);
        uint8_t pix8 = (lowBytes & 0x80) >> 7 | ((highBytes & 0x80) >> 6);


        plotPixel(pix8, format, &pPixels[pixelOffset++]);
        plotPixel(pix7, format, &pPixels[pixelOffset++]);
        plotPixel(pix6, format, &pPixels[pixelOffset++]);
        plotPixel(pix5, format, &pPixels[pixelOffset++]);
        plotPixel(pix4, format, &pPixels[pixelOffset++]);
        plotPixel(pix3, format, &pPixels[pixelOffset++]);
        plotPixel(pix2, format, &pPixels[pixelOffset++]);
        plotPixel(pix1, format, &pPixels[pixelOffset++]);

        pixelOffset += 256 - 8;
    }
}

void PPU::UpdateImage()
{
    SDL_LockSurface(pTV_Display);

    uint32_t pixelOffset = 0;

    uint32_t *pPixels = (uint32_t *)pTV_Display->pixels;

    // Copy tiles from nametable
    for (int y = 0; y < 30; ++y)
    {
        for (int x = 0; x < 32; ++x)
        {
            uint8_t tileID = pNameTable->mem[0x2000 + y * 32 + x];

            // Copy tile to image x, y
            CopyTileToImage(tileID, x, y, pPixels, pTV_Display->format);
        }
    }

    SDL_UnlockSurface(pTV_Display);

}
