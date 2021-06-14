#include "PPU.h"
#include "StatusMonitor.h"


// PPU is mapped from 0x2000 - 0x3FFF on the CPU bus
PPU::PPU(CPU_6502 *pCPU)
    : Peripheral(&pCPU->bus, 0x2000, 0x3FFF)
{
    // Map OAMDMA register in the CPU bus
    pCPU->bus.attachPeripheral(OAMDMA, OAMDMA, this);

    // 8 KB pattern table
    pPatternTable = new RAM(&PPU_Bus, 0, 0x1FFF);
 
    // 2 KB name table
    pNameTable = new RAM(&PPU_Bus, 0x2000, 0x27FF, 2048);

    // 256 bytes of palette data
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
    pPaletteSurface = SDL_CreateRGBSurface(0,
                                            16,
                                            4,
                                            32,
                                            0xFF000000,
                                            0x00FF0000,
                                            0x0000FF00,
                                            0x000000FF);

    SetupPaletteValues();

    // Initialize the surface for the nametable display
    pNametableSurface = SDL_CreateRGBSurface(0,
                                             NAMETABLE_RES_X,
                                             NAMETABLE_RES_Y,
                                             32,
                                             0xFF000000,
                                             0x00FF0000,
                                             0x0000FF00,
                                             0x000000FF);

    scanline = 0;
    paused = false;
    lowByteActive = false;
    this->pCPU = pCPU;
    statusReg.entireRegister = 0;
    controlReg.entireRegister = 0;
    maskReg.entireRegister = 0;
    OAM_Address = 0;
    uninitialized = 1;
    oddFrame = false;

    for (int i = 0; i < SCANLINES; ++i)
    {
        scrollX_ForScanline[i] = 0;
        controlReg_ForScanline[i] = 0;
    }

    lastValidScanlineForScroll = 0;
    lastValidScanlineForControl = 0;

    scrollY = 0;
    writingToScrollY = false;

    // Setup vertical mirroring, horizontal scrolling
    verticalMirrorOffset = 0;
    horizontalMirrorOffset = 0x400;
}

PPU::~PPU()
{
    SDL_FreeSurface(pNametableSurface);
    SDL_FreeSurface(pPaletteSurface);
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
            writingToScrollY = false;
            break;

        case OAMADDR:
            // 	aaaa aaaa	OAM read / write address
            break;

        case OAMDATA:
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
            //printf("PPUDATA read from 0x%X\n", VRAM_Address);

            if (VRAM_Address >= 0x3000 && VRAM_Address <= 0x3EFF)
            {
                printf("Mirroring 0x%X - ", VRAM_Address);
                VRAM_Address -= 0x1000;
            }

            if (VRAM_Address >= 0x2000 && VRAM_Address <= 0x23FF)
            {
                printf("W Nametable 0 - 0x%X\n", VRAM_Address);
            }
            else if (VRAM_Address >= 0x2400 && VRAM_Address <= 0x27FF)
            {
                printf("W Nametable 1 - 0x%X\n", VRAM_Address);
            }
            if (VRAM_Address >= 0x2800 && VRAM_Address <= 0x2BFF)
            {
                printf("W Nametable 2 - 0x%X\n", VRAM_Address);
            }
            else if (VRAM_Address >= 0x2C00 && VRAM_Address <= 0x2FFF)
            {
                printf("W Nametable 3 - 0x%X\n", VRAM_Address);
            }

            // reads from VRAM are delayed by one read, so we'll be returning the buffered data from the previous read
            data = readBuffer;
            readBuffer = PPU_Bus.read(VRAM_Address);

            // Reads from palette memory aren't delayed
            if (VRAM_Address >= 0x3F00 && VRAM_Address <= 0x3FFF)
                data = readBuffer;

            if (controlReg.VRAM_AddressIncBy32)
                VRAM_Address += 32;
            else
                ++VRAM_Address;

            // TODO: Mirroring
            VRAM_Address &= 0x3FFF;

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
        //printf("OAMDMA written: 0x%X\n", value);
        //paused = true;
        uint8_t *dest = (uint8_t*)OAM_Memory;
        dest += OAM_Address;
        uint8_t *end = (uint8_t*)&OAM_Memory[64];

        uint16_t srcAddr = (uint16_t)value << 8;
        for (int i = 0; i < 256 && dest < end; ++i, ++dest, ++srcAddr)
            *dest = pCPU->bus.read(srcAddr);

        /*printf("OAM now:\n");
        for (int i = 0; i < 64; ++i)
        {
            if (OAM_Memory[i].yPos >= 248)
                continue;

            printf("Sprite %d\t - (%d, %d)\t - \n", OAM_Memory[i].tileIndex, OAM_Memory[i].xPos, OAM_Memory[i].yPos);
            if (OAM_Memory[i].tileIndex != 0 && OAM_Memory[i].xPos == 0)
                paused = true;
        }
        printf("\n");*/


        return;
    }

    address &= 0x7;

    //printf("PPU reg %d - 0x%X\n", address, value);
    uint8_t prevControlReg;
    int i;
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

            prevControlReg = controlReg_ForScanline[lastValidScanlineForControl];
            for (i = lastValidScanlineForControl; i < scanline; ++i)
                controlReg_ForScanline[i] = prevControlReg;
            for (i = scanline; i < SCANLINES; ++i)
                controlReg_ForScanline[i] = value;
            lastValidScanlineForControl = scanline;
            
            if(debugOutput)
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
            //printf("OAM Address written: 0x%X\n", value);
            OAM_Address = value;
            break;

        case OAMDATA:
            // 	dddd dddd	OAM data read / write
            printf("OAM Data written\n");
            break;

        case PPUSCROLL:
            // xxxx xxxx	fine scroll position(two writes : X scroll, Y scroll)
            //printf("Scroll written on line %d 0x%X\n", scanline, value);
            if (writingToScrollY)
                scrollY = value;
            else
            {
                uint8_t prevScrollX = scrollX_ForScanline[lastValidScanlineForScroll];
                for (i = lastValidScanlineForScroll; i < scanline; ++i)
                    scrollX_ForScanline[i] = prevScrollX;
                for (i = scanline; i < SCANLINES; ++i)
                    scrollX_ForScanline[i] = value;
                lastValidScanlineForScroll = scanline;
                //printf("Scanline: %d - Scroll: (%d, %d)\n", scanline, scrollX_ForScanline[scanline], scrollY);
            }

            writingToScrollY = !writingToScrollY;

            break;

        case PPUADDR:
            // aaaa aaaa	PPU read / write address(two writes : most significant byte, least significant byte)
            if(debugOutput)
                printf("0x%X - ", value);
            
            if (lowByteActive)
            {
                if(debugOutput)
                    printf("low byte active\n");

                VRAM_Address += value;
                
                
                //printf("PPU Address: 0x%X ", VRAM_Address);

                // Handle nametable mirroring
                if (VRAM_Address >= 0x3000 && VRAM_Address <= 0x3EFF)
                {
                    VRAM_Address -= 0x1000;
                    //printf("M 0x%X\n", VRAM_Address);
                }
                //else
                  //  printf("\n");
            }
            else
            {
                if (debugOutput)
                    printf("high byte active\n");
                VRAM_Address = (uint16_t)value << 8;
            }

            lowByteActive = !lowByteActive;

            if (debugOutput)
                printf("PPUADDR 0x%X\n", VRAM_Address);

            // Handle mirroring of VRAM_Address
            //VRAM_Address &= 0x3FFF;
            /*
            if (VRAM_Address >= 0x3F00 && VRAM_Address <= 0x3EFF)
                VRAM_Address -= 0x1000;*/

            //paused = true;
            break;

        case PPUDATA:
            // dddd dddd	PPU data read / write
            /*paused = true;
            printf("PPUDATA written to\n");

            printf("control reg: 0x%X\n", controlReg.entireRegister);*/

            if (VRAM_Address >= 0x3000 && VRAM_Address <= 0x3EFF)
            {
                printf("Mirroring 0x%X - ", VRAM_Address);
                VRAM_Address -= 0x1000;
            }

            /*if (VRAM_Address >= 0x2000 && VRAM_Address <= 0x23FF)
            {
                printf("W Nametable 0 - 0x%X\n", VRAM_Address);
            }
            else if (VRAM_Address >= 0x2400 && VRAM_Address <= 0x27FF)
            {
                printf("W Nametable 1 - 0x%X\n", VRAM_Address);
            }
            if (VRAM_Address >= 0x2800 && VRAM_Address <= 0x2BFF)
            {
                printf("W Nametable 2 - 0x%X\n", VRAM_Address);
            }
            else if (VRAM_Address >= 0x2C00 && VRAM_Address <= 0x2FFF)
            {
                printf("W Nametable 3 - 0x%X\n", VRAM_Address);
            }
            */
            PPU_Bus.write(VRAM_Address, value);
            
            if (controlReg.VRAM_AddressIncBy32)
                VRAM_Address += 32;
            else
                ++VRAM_Address;

            if (VRAM_Address > 0x3FFF)
                printf("\nVRAM Address incremented beyond 0x3FFF\n\n");

            break;

        default:
            printf("This should never happen.\n");
            break;
    }
}

void PPU::CopyTileToImage(uint8_t tileNumber, int tileX, int fineX, int tileY, uint32_t *pPixels, int pixelsPerRow, int paletteNumber)
{
    // Tiles are stored LSB of an entire tile followed by MSB of an entire tile
    uint8_t tileLSB[8];
    uint8_t tileMSB[8];

    uint32_t pixelOffset = tileY * pixelsPerRow * 8 + tileX * 8 - fineX;
    int tileWidth = 8;
    int startPixel = 0;

    // If tile 0 scrolls off the screen, chop off the left part of it
    if (tileX == 0 && fineX > 0)
    {
        pixelOffset += fineX;
        startPixel = fineX;
    }

    // tile 32 is on the far right of the screen, so chop off the right part of it
    if (tileX == 32)
        tileWidth = fineX;

    // fineX of 8 is a hack to indicate we're drawing to the nametable display, so don't actually offset anything by fineX
    if (fineX == 8)
    {
        tileWidth = 8;
        startPixel = 0;
        
        if(tileX != 0)
            pixelOffset += 8;
    }

    uint32_t patternOffset = tileNumber * 16;

    uint8_t *pPatternMemory = pPatternTable->mem;

    if (controlReg.backgroundPatternTableSelect)
        pPatternMemory += 0x1000;

    // Get palette data for the tile
    uint32_t colors[4];
    // TODO: mapping palette to RGBA values could happen when palette is written to, offering an optimization
    colors[0] = paletteColorValues[pPalette->paletteMem.universalBackground];
    colors[1] = paletteColorValues[pPalette->paletteMem.paletteTable[paletteNumber].colors[0]];
    colors[2] = paletteColorValues[pPalette->paletteMem.paletteTable[paletteNumber].colors[1]];
    colors[3] = paletteColorValues[pPalette->paletteMem.paletteTable[paletteNumber].colors[2]];

    // Extract data for the tile, bitplane of low bits is stored before bitplane of high bits
    SDL_memcpy(tileLSB, pPatternMemory + patternOffset, 8);
    SDL_memcpy(tileMSB, pPatternMemory + patternOffset + 8, 8);

    // Copy pixels
    uint8_t pixels[8];

    // for each row
    for (int y = 0; y < 8; ++y)
    {
        uint8_t lowBytes = tileLSB[y];
        uint8_t highBytes = tileMSB[y];

        // Determine palette value of each of the 8 pixels in the tile row
        pixels[7] = lowBytes & 1 | ((highBytes & 1) << 1);
        pixels[6] = (lowBytes & 2) >> 1 | (highBytes & 2);
        pixels[5] = (lowBytes & 4) >> 2 | ((highBytes & 4) >> 1);
        pixels[4] = (lowBytes & 8) >> 3 | ((highBytes & 8) >> 2);
        pixels[3] = (lowBytes & 0x10) >> 4 | ((highBytes & 0x10) >> 3);
        pixels[2] = (lowBytes & 0x20) >> 5 | ((highBytes & 0x20) >> 4);
        pixels[1] = (lowBytes & 0x40) >> 6 | ((highBytes & 0x40) >> 5);
        pixels[0] = (lowBytes & 0x80) >> 7 | ((highBytes & 0x80) >> 6);
        
        // Plot pixels to the image buffer
        for (int pixX = startPixel; pixX < tileWidth; ++pixX)
            pPixels[pixelOffset++] = colors[pixels[pixX]];

        // Advance pixelOffset to the beginning of the tile on the next line
        pixelOffset += pixelsPerRow - tileWidth;
        
        if (tileX == 0)
            pixelOffset += startPixel;
    }
}

void PPU::DrawSprite(uint8_t tileNumber, int x, int y, uint32_t * pPixels, OAM_ATTRIBUTES_BYTE attributes)
{
    int paletteNumber = attributes.paletteNumber + 4;
    bool flipHorizontal = attributes.flipHorizontally;
    bool flipVertical = attributes.flipVertically;
    bool drawBehindBackground = attributes.drawBehindBackground;

    // Tiles are stored LSB of an entire tile followed by MSB of an entire tile
    uint8_t tileLSB[8];
    uint8_t tileMSB[8];

    uint32_t pixelOffset = y * 256 + x;

    // Don't let sprites go off the bottom of the screen
    int tileHeight = 8;
    if (y + tileHeight >= 240)
        tileHeight = (240 - y);

    // Don't let sprites go off the right edge of the screen
    int tileWidth = 8;
    if (x + tileWidth >= 256)
        tileWidth = (256 - x);

    uint32_t patternOffset = tileNumber * 16;

    uint8_t *pPatternMemory = pPatternTable->mem;

    if (controlReg.spritePatternTableSelect)
        pPatternMemory += 0x1000;

    // Get color data for the tile
    uint32_t colors[4];
    // TODO: mapping palette to RGBA values could happen when palette is written to, offering an optimization
    colors[0] = 0;
    colors[1] = paletteColorValues[pPalette->paletteMem.paletteTable[paletteNumber].colors[0]];
    colors[2] = paletteColorValues[pPalette->paletteMem.paletteTable[paletteNumber].colors[1]];
    colors[3] = paletteColorValues[pPalette->paletteMem.paletteTable[paletteNumber].colors[2]];

    // Determine the color of empty background (only used by sprites drawn behind background objects)
    uint32_t emptyBackgroundColor = paletteColorValues[pPalette->paletteMem.universalBackground];

    // Extract data for the tile, bitplane of low bits is stored before bitplane of high bits
    SDL_memcpy(tileLSB, pPatternMemory + patternOffset, 8);
    SDL_memcpy(tileMSB, pPatternMemory + patternOffset + 8, 8);

    // Determine how rows of pixels should be evaluated; top-down or bottom-up
    int yChange = 1;
    int yStart = 0;
    int yDone = tileHeight;
    if (flipVertical)
    {
        yChange = -1;
        yStart = tileHeight - 1;
        yDone = -1;
    }
    
    // Copy each each row of pixels
    for (int y = yStart; y != yDone; y += yChange)
    {
        uint8_t lowBytes = tileLSB[y];
        uint8_t highBytes = tileMSB[y];
        uint8_t pixels[8];

        // Determine the color of each pixel
        pixels[0] = lowBytes & 1 | ((highBytes & 1) << 1);
        pixels[1] = (lowBytes & 2) >> 1 | (highBytes & 2);
        pixels[2] = (lowBytes & 4) >> 2 | ((highBytes & 4) >> 1);
        pixels[3] = (lowBytes & 8) >> 3 | ((highBytes & 8) >> 2);
        pixels[4] = (lowBytes & 0x10) >> 4 | ((highBytes & 0x10) >> 3);
        pixels[5] = (lowBytes & 0x20) >> 5 | ((highBytes & 0x20) >> 4);
        pixels[6] = (lowBytes & 0x40) >> 6 | ((highBytes & 0x40) >> 5);
        pixels[7] = (lowBytes & 0x80) >> 7 | ((highBytes & 0x80) >> 6);

        if (drawBehindBackground)
        {
            // Check each sprite pixel against what's already on the screen
            for (int i = 0; i < tileWidth; ++i)
            {
                // Make the sprite's pixel transparent if there's something on the background
                if (pPixels[pixelOffset + i] != emptyBackgroundColor)
                    flipHorizontal ? pixels[i] = 0 : pixels[7 - i] = 0;
            }
        }

        // Copy the pixels that aren't transparent
        if (flipHorizontal)
        {
            for(int i = 0; i < tileWidth; ++i)
                pixels[i] ? pPixels[pixelOffset++] = colors[pixels[i]] : ++pixelOffset;
        }
        else
        {
            for (int i = 0; i < tileWidth; ++i)
                pixels[7 - i] ? pPixels[pixelOffset++] = colors[pixels[7 - i]] : ++pixelOffset;
        }

        pixelOffset += 256 - tileWidth;
    }
}

void PPU::DrawNametables()
{
    SDL_LockSurface(pNametableSurface);

    uint32_t pixelOffset = 0;

    uint32_t *pPixels = (uint32_t *)pNametableSurface->pixels;

    // Base nametable address controlReg.baseNametableAddress
    // (0 = $2000; 1 = $2400; 2 = $2800; 3 = $2C00)
    uint16_t nametableBase = 0x2000;// +(controlReg.baseNametableAddress * 0x400);
    
    int yOffset = 0;
    int xOffset = 0;
    uint16_t nametableOffset = 0;

    for (int y = 0; y < 60; ++y)
    {
        if (y > 29)
        {
            yOffset = -30;
            nametableOffset = verticalMirrorOffset;
        }
        for (int x = 0; x < 64; ++x)
        {

            if (x > 31)
            {
                xOffset = -32;
                nametableOffset = horizontalMirrorOffset;
            }
            else
            {
                xOffset = 0;
                nametableOffset = 0;
            }

            uint8_t tileID = pNameTable->mem[nametableBase + nametableOffset + (y + yOffset) * 32 + x + xOffset];

            // Get palette number for the current tile (1-4)
            int paletteNumber = GetPaletteNumberForTile(x + xOffset, (y + yOffset), nametableBase + nametableOffset);

            // Copy tile to image x, y
            CopyTileToImage(tileID, x, 8, y, pPixels, NAMETABLE_RES_X, paletteNumber);
        }
    }

    SDL_UnlockSurface(pNametableSurface);
}

int PPU::GetPaletteNumberForTile(int x, int y, uint16_t nametableBase)
{
    // The palette entry defines the palette for four 2x2 sets of tiles.
    // determine which of the four quadrants the tile is occupying
    int quadX = (x & 3) / 2;    // 0 for left quadrant, 1 for right
    int quadY = (y & 3) / 2;    // 0 for top quadrant, 1 for bottom

    // Get first entry into palette table
    ATTRIBUTE_TABLE_ENTRY *pEntry = (ATTRIBUTE_TABLE_ENTRY *)(&pNameTable->mem[nametableBase + 0x3C0]);

    // Advance to palette number for this tile
    x /= 4;
    y /= 4;
    pEntry += (y * 8) + (x);

    // return the palette entry for the proper quadrant
    if ((quadX == 0) && (quadY == 0))
        return pEntry->topLeftPaletteIndex;

    if ((quadX == 1) && (quadY == 0))
        return pEntry->topRightPaletteIndex;

    if ((quadX == 0) && (quadY == 1))
        return pEntry->bottomLeftPaletteIndex;

    if ((quadX == 1) && (quadY == 1))
        return pEntry->bottomRightPaletteIndex;

    printf("\nShouldn't reach here!\n");
    return 0;
}

void PPU::UpdateImage()
{
    SDL_LockSurface(pTV_Display);

    uint32_t pixelOffset = 0;

    uint32_t *pPixels = (uint32_t *)pTV_Display->pixels;
    
    // Base nametable address controlReg.baseNametableAddress
    // (0 = $2000; 1 = $2400; 2 = $2800; 3 = $2C00)
    uint16_t nametableBase = 0x2000;

    // Copy background tiles from nametable
    int yOffset = 0;
    int xOffset = 0;
    uint16_t nametableOffset = 0;

    for (int y = 0; y < 30; ++y)
    {
        // Get the scroll x and control register values the CPU set when it was handling this set of scanlines
        int tileX_Offset = scrollX_ForScanline[y * 8] / 8;
        int fineX = scrollX_ForScanline[y * 8] & 0x7;
        CONTROL_REG ctrl;
        ctrl.entireRegister = controlReg_ForScanline[y * 8];
        //printf("Control register: 0x%X for line %d\n", ctrl.baseNametableAddress, y * 8);

        for (int x = tileX_Offset; x < 33 + tileX_Offset; ++x)
        {
            // Determine if we're drawing from nametable 0 or nametable 1
            if (x >= 32)
            {
                xOffset = -32;
                if (ctrl.baseNametableAddress == 1)
                    nametableOffset = 0;
                else
                    nametableOffset = horizontalMirrorOffset;
            }
            else
            {
               xOffset = 0;
               if (ctrl.baseNametableAddress == 1)
                    nametableOffset = horizontalMirrorOffset;
               else
                    nametableOffset = 0;
            }

            // Get the tileID from the name table
            uint8_t tileID = pNameTable->mem[nametableBase + nametableOffset + y * 32 + x + xOffset];

            // Get palette number for the current tile (1-4)
            int paletteNumber = GetPaletteNumberForTile(x + xOffset, y, nametableBase + nametableOffset);

            // Copy tile to image x, y
            CopyTileToImage(tileID, x - tileX_Offset, fineX, y, pPixels, 256, paletteNumber);
        }
    }

    // Draw sprites
    // Draw higher-index sprites first so lower-index sprites will overlap them
    for (int i = 63; i >= 0; --i)
    {
        // sprites off the screen arent drawn
        if (OAM_Memory[i].yPos >= 0xEF)
            continue;

        DrawSprite(OAM_Memory[i].tileIndex,
                   OAM_Memory[i].xPos,
                   OAM_Memory[i].yPos + 1,
                   pPixels,
                   OAM_Memory[i].attributes);
    }

    SDL_UnlockSurface(pTV_Display);

}

void PPU::SetupPaletteValues()
{
    int i = 0;
    SDL_PixelFormat *pFormat = pPaletteSurface->format;

    paletteColorValues[i++] = SDL_MapRGB(pFormat, 84,   84,  84 );
    paletteColorValues[i++] = SDL_MapRGB(pFormat, 0,    30, 116 );
    paletteColorValues[i++] = SDL_MapRGB(pFormat, 8,    16, 144 );
    paletteColorValues[i++] = SDL_MapRGB(pFormat, 48,   0,  136 );
    paletteColorValues[i++] = SDL_MapRGB(pFormat, 68,   0,  100 );
    paletteColorValues[i++] = SDL_MapRGB(pFormat, 92,   0,   48 );
    paletteColorValues[i++] = SDL_MapRGB(pFormat, 84,   4,    0 );
    paletteColorValues[i++] = SDL_MapRGB(pFormat, 60,   24,   0 );
    paletteColorValues[i++] = SDL_MapRGB(pFormat, 32,   42,   0 );
    paletteColorValues[i++] = SDL_MapRGB(pFormat, 8,    58,   0 );
    paletteColorValues[i++] = SDL_MapRGB(pFormat, 0,    64,   0 );
    paletteColorValues[i++] = SDL_MapRGB(pFormat, 0,    60,   0 );
    paletteColorValues[i++] = SDL_MapRGB(pFormat, 0,    50,  60 );
    paletteColorValues[i++] = SDL_MapRGB(pFormat, 0, 0, 0);
    paletteColorValues[i++] = SDL_MapRGB(pFormat, 0, 0, 0);
    paletteColorValues[i++] = SDL_MapRGB(pFormat, 0, 0, 0);
    
    paletteColorValues[i++] = SDL_MapRGB(pFormat, 152, 150, 152 );
    paletteColorValues[i++] = SDL_MapRGB(pFormat, 8,    76, 196 );
    paletteColorValues[i++] = SDL_MapRGB(pFormat, 48,   50, 236 );
    paletteColorValues[i++] = SDL_MapRGB(pFormat, 92,   30, 228 );
    paletteColorValues[i++] = SDL_MapRGB(pFormat, 136,  20, 176 );
    paletteColorValues[i++] = SDL_MapRGB(pFormat, 160,  20, 100 );
    paletteColorValues[i++] = SDL_MapRGB(pFormat, 152,  34,  32 );
    paletteColorValues[i++] = SDL_MapRGB(pFormat, 120,  60,   0 );
    paletteColorValues[i++] = SDL_MapRGB(pFormat, 84,   90,   0 );
    paletteColorValues[i++] = SDL_MapRGB(pFormat, 40,   114,  0 );
    paletteColorValues[i++] = SDL_MapRGB(pFormat, 8,    124,  0 );
    paletteColorValues[i++] = SDL_MapRGB(pFormat, 0,    118, 40 );
    paletteColorValues[i++] = SDL_MapRGB(pFormat, 0,    102,120 );
    paletteColorValues[i++] = SDL_MapRGB(pFormat, 0,    0,    0 );
    paletteColorValues[i++] = SDL_MapRGB(pFormat, 0, 0, 0);
    paletteColorValues[i++] = SDL_MapRGB(pFormat, 0, 0, 0);
        
    paletteColorValues[i++] = SDL_MapRGB(pFormat, 236, 238, 236 );
    paletteColorValues[i++] = SDL_MapRGB(pFormat,  76, 154, 236 );
    paletteColorValues[i++] = SDL_MapRGB(pFormat, 120, 124, 236 );
    paletteColorValues[i++] = SDL_MapRGB(pFormat, 176,  98, 236 );
    paletteColorValues[i++] = SDL_MapRGB(pFormat, 228,  84, 236 );
    paletteColorValues[i++] = SDL_MapRGB(pFormat, 236,  88, 180 );
    paletteColorValues[i++] = SDL_MapRGB(pFormat, 236, 106, 100 );
    paletteColorValues[i++] = SDL_MapRGB(pFormat, 212, 136,  32 );
    paletteColorValues[i++] = SDL_MapRGB(pFormat, 160, 170,   0 );
    paletteColorValues[i++] = SDL_MapRGB(pFormat, 116, 196,   0 );
    paletteColorValues[i++] = SDL_MapRGB(pFormat,  76, 208,  32 );
    paletteColorValues[i++] = SDL_MapRGB(pFormat,  56, 204, 108 );
    paletteColorValues[i++] = SDL_MapRGB(pFormat,  56, 180, 204 );
    paletteColorValues[i++] = SDL_MapRGB(pFormat,  60,  60,  60 );
    paletteColorValues[i++] = SDL_MapRGB(pFormat, 0, 0, 0);
    paletteColorValues[i++] = SDL_MapRGB(pFormat, 0, 0, 0);
    
    paletteColorValues[i++] = SDL_MapRGB(pFormat, 236, 238, 236 );
    paletteColorValues[i++] = SDL_MapRGB(pFormat, 168, 204, 236 );
    paletteColorValues[i++] = SDL_MapRGB(pFormat, 188, 188, 236 );
    paletteColorValues[i++] = SDL_MapRGB(pFormat, 212, 178, 236 );
    paletteColorValues[i++] = SDL_MapRGB(pFormat, 236, 174, 236 );
    paletteColorValues[i++] = SDL_MapRGB(pFormat, 236, 174, 212 );
    paletteColorValues[i++] = SDL_MapRGB(pFormat, 236, 180, 176 );
    paletteColorValues[i++] = SDL_MapRGB(pFormat, 228, 196, 144 );
    paletteColorValues[i++] = SDL_MapRGB(pFormat, 204, 210, 120 );
    paletteColorValues[i++] = SDL_MapRGB(pFormat, 180, 222, 120 );
    paletteColorValues[i++] = SDL_MapRGB(pFormat, 168, 226, 144 );
    paletteColorValues[i++] = SDL_MapRGB(pFormat, 152, 226, 180 );
    paletteColorValues[i++] = SDL_MapRGB(pFormat, 160, 214, 228 );
    paletteColorValues[i++] = SDL_MapRGB(pFormat, 160, 162, 160 );
    paletteColorValues[i++] = SDL_MapRGB(pFormat, 0, 0, 0);
    paletteColorValues[i++] = SDL_MapRGB(pFormat, 0, 0, 0);
    
    // Copy colors to palette image
    SDL_LockSurface(pPaletteSurface);

    uint32_t *pPixels = (uint32_t *)pPaletteSurface->pixels;

    for (i = 0; i < 64; ++i)
        pPixels[i] = paletteColorValues[i];

    SDL_UnlockSurface(pPaletteSurface);
}
