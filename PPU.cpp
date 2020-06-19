#include "PPU.h"
#include "StatusMonitor.h"


// PPU is mapped from 0x2000 - 0x3FFF on the CPU bus
PPU::PPU(Bus *pCPU_Bus)
    : Peripheral(pCPU_Bus, 0x2000, 0x3FFF)
{
    // 8 KB pattern table
    pPatternTable = new RAM(&PPU_Bus, 0, 0x1FFF);
 
    // 2 KB name table
    pNameTable = new RAM(&PPU_Bus, 0x2000, 0x2FFF, 2048);

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

    address &= 0x7;

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
            break;

        case PPUDATA:
            // dddd dddd	PPU data read / write
            break;

        default:
            printf("This should never happen.\n");
            break;
    }

    return uint8_t(0);
}

void PPU::write(uint16_t address, uint8_t value)
{
    address &= 0x7;

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
            break;

        case PPUDATA:
            // dddd dddd	PPU data read / write
            break;

        default:
            printf("This should never happen.\n");
            break;
    }
}
