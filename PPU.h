#pragma once
#include <stdio.h>
#include <stdint.h>
#include "peripheral.h"
#include "Bus.h"
#include "RAM.h"
#include "Palette.h"
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

class PPU :
    public Peripheral
{
public:
    PPU(Bus *pCPU_Bus);
    ~PPU();

    uint8_t read(uint16_t address);
    void write(uint16_t address, uint8_t value);

    // PPU has its own bus in addition to the CPU bus
    Bus PPU_Bus;

    // 8KB of pattern table from 0x0000 - 0x1FFF on the PPU bus
    RAM *pPatternTable;

    // 2KB of name table from 0x2000 - 0x2FFF (mirrored once) on the PPU bus
    RAM *pNameTable;

    // Palette data from 0x3F00 - 0x3FFF
    Palette *pPalette;

    SDL_Surface *pTV_Display;
    SDL_Surface *pPattern1;
    SDL_Surface *pPattern2;
};

