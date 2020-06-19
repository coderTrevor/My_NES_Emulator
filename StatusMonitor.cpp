#include "StatusMonitor.h"
#include <stdio.h>
#include <cstdint>
#include <SDL.h>
#include "SDL_picofont.h"
#include "System.h"

#define COLOR_FROM_SDL_COLOR(format, sdlColor) SDL_MapRGB(format, sdlColor.r, sdlColor.g, sdlColor.b)

SDL_Rect simpleDisplayRect = { SIMPLE_DISPLAY_X,
                                SIMPLE_DISPLAY_Y,
                                SIMPLE_DISPLAY_WIDTH * SIMPLE_DISPLAY_PIXEL_SCALE,
                                SIMPLE_DISPLAY_HEIGHT * SIMPLE_DISPLAY_PIXEL_SCALE };
SDL_Rect nesDisplayRect = { NES_DISPLAY_X,
                            NES_DISPLAY_Y,
                            NES_DISPLAY_WIDTH * NES_DISPLAY_PIXEL_SCALE,
                            NES_DISPLAY_HEIGHT * NES_DISPLAY_PIXEL_SCALE };

SDL_Rect pattern2Rect = { STATUS_MONITOR_WIDTH - NES_MARGIN - NES_PATTERN_WIDTH,
                            STATUS_MONITOR_HEIGHT - NES_MARGIN - NES_PATTERN_HEIGHT,
                            NES_PATTERN_WIDTH,
                            NES_PATTERN_HEIGHT };


SDL_Rect pattern1Rect = { pattern2Rect.x - NES_PATTERN_WIDTH - NES_MARGIN,
                            pattern2Rect.y,
                            NES_PATTERN_WIDTH,
                            NES_PATTERN_HEIGHT };

#ifdef SYSTEM_SIMPLE
StatusMonitor::StatusMonitor(RAM *pRAM, CPU_6502 *pCPU)
{
    this->pRAM = pRAM;
    this->pCPU = pCPU;
    cpuRunning = false;

    // Create an SDL window to display the status

    screenSurface = NULL;     // the surface to render to

    SDL_Init(SDL_INIT_VIDEO);              // Initialize SDL2

                                           // Create an application window with the following settings:
    window = SDL_CreateWindow(
        "Simple 6502 Emulator",            // window title
        SDL_WINDOWPOS_UNDEFINED,           // initial x position
        SDL_WINDOWPOS_UNDEFINED,           // initial y position
        STATUS_MONITOR_WIDTH,              // width, in pixels
        STATUS_MONITOR_HEIGHT,             // height, in pixels
        0
    );

    // Check that the window was successfully created
    if (window == NULL) {
        // In the case that the window could not be made...
        printf("Could not create window: %s\n", SDL_GetError());
        return;
    }

    // Get window surface
    screenSurface = SDL_GetWindowSurface(window);
    
    SetupColors();

    // Draw a black background
    SDL_FillRect(screenSurface, NULL, colorBlack);
}

// Draws the simple (32 x 32) display of the virtual system
void StatusMonitor::DrawDisplay()
{
    // Draw a box around the display
    SDL_Rect border = { simpleDisplayRect.x - 1,
        simpleDisplayRect.y - 1,
        simpleDisplayRect.w + 2,
        simpleDisplayRect.h + 2 };
    SDL_FillRect(screenSurface, &border, colorWhite);

    // Draw each pixel
    SDL_Rect pixel = { simpleDisplayRect.x,
        simpleDisplayRect.y,
        SIMPLE_DISPLAY_PIXEL_SCALE,
        SIMPLE_DISPLAY_PIXEL_SCALE };

    // in the simple system, displayed pixels occupy 0x200 - 0x599
    uint16_t pixelOffset = 0x200;
    uint32_t color;
    for (int y = 0; y < 32; ++y)
    {
        for (int x = 0; x < 32; ++x)
        {
            // determine color based on the ram value
            uint8_t col = pRAM->mem[pixelOffset];
            switch (col & 0x0F)
            {
                case 0:
                    color = colorBlack;
                    break;
                case 1:
                    color = colorWhite;
                    break;
                case 2:
                    color = colorRed;
                    break;
                case 3:
                    color = colorCyan;
                    break;
                case 4:
                    color = colorPurple;
                    break;
                case 5:
                    color = colorGreen;
                    break;
                case 6:
                    color = colorBlue;
                    break;
                case 7:
                    color = colorYellow;
                    break;
                case 8:
                    color = colorOrange;
                    break;
                case 9:
                    color = colorBrown;
                    break;
                case 0xa:
                    color = colorLightRed;
                    break;
                case 0xb:
                    color = colorDarkGray;
                    break;
                case 0xc:
                    color = colorGray;
                    break;
                case 0xd:
                    color = colorLightGreen;
                    break;
                case 0xe:
                    color = colorLightBlue;
                    break;
                case 0xf:
                    color = colorLightGray;
                    break;
                default:
                    printf("Unhandled color: 0x%X\n", col & 0x0F);
                    color = colorLightGray;
                    break;
            }

            // Draw the pixel
            SDL_FillRect(screenSurface, &pixel, color);

            pixel.x += SIMPLE_DISPLAY_PIXEL_SCALE;

            pixelOffset++;
        }

        pixel.x = simpleDisplayRect.x;
        pixel.y += SIMPLE_DISPLAY_PIXEL_SCALE;
    }
}
#endif

#ifdef SYSTEM_NES
StatusMonitor::StatusMonitor(RAM *pRAM, CPU_6502 *pCPU, PPU *pPPU)
{
    this->pRAM = pRAM;
    this->pCPU = pCPU;
    this->pPPU = pPPU;
    cpuRunning = false;

    // Create an SDL window to display the status

    screenSurface = NULL;     // the surface to render to

    SDL_Init(SDL_INIT_VIDEO);              // Initialize SDL2

                                           // Create an application window with the following settings:
    window = SDL_CreateWindow(
        "My NES Emulator",                 // window title
        SDL_WINDOWPOS_UNDEFINED,           // initial x position
        SDL_WINDOWPOS_UNDEFINED,           // initial y position
        STATUS_MONITOR_WIDTH,              // width, in pixels
        STATUS_MONITOR_HEIGHT,             // height, in pixels
        0
    );

    // Check that the window was successfully created
    if (window == NULL) {
        // In the case that the window could not be made...
        printf("Could not create window: %s\n", SDL_GetError());
        return;
    }

    // Get window surface
    screenSurface = SDL_GetWindowSurface(window);

    SetupColors();

    // Draw a black background
    SDL_FillRect(screenSurface, NULL, colorBlack);
}

void plotPixel(uint8_t pixel, SDL_PixelFormat *format, uint32_t *address)
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

void StatusMonitor::CopyTileToPixels(SDL_PixelFormat *format, uint8_t *pTileLSB, uint8_t *pTileMSB, uint32_t *pPixels, uint32_t tileX, uint32_t tileY)
{
    uint32_t pixelOffset = (tileY * 128 * 8) + (tileX * 8);

    for (int y = 0; y < 8; ++y)
    {
        uint8_t lowBytes = pTileLSB[y];
        uint8_t highBytes = pTileMSB[y];

        // Consider each pixel
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
        /*for (int x = 0; x < 8; ++x)
        {
            if (y % 2 == 0)
            {
                pPixels[pixelOffset++] = white;
                pPixels[pixelOffset++] = black;
            }
            else
            {
                pPixels[pixelOffset++] = black;
                pPixels[pixelOffset++] = white;
            }
        }*/
        pixelOffset += 120;
    }
}


void StatusMonitor::DrawPattern(SDL_Surface *pSurface, uint8_t *pPatternMemory)
{
    // Lock surface so we can access the pixels directly
    SDL_LockSurface(pSurface);

    uint32_t offset = 0;

    // Tiles are stored LSB of an entire tile followed by MSB of an entire tile
    uint8_t tileLSB[8];
    uint8_t tileMSB[8];

    // Draw 16 tiles top to bottom
    for (int y = 0; y < 16; ++y)
    {
        // Draw 16 tiles across
        for (int x = 0; x < 16; ++x)
        {
            // Extract data for the next tile
            SDL_memcpy(tileLSB, pPatternMemory + offset, 8);
            SDL_memcpy(tileMSB, pPatternMemory + offset + 8, 8);

            // Copy tile to surface
            //if((x+y) % 2 == 0)
                CopyTileToPixels(pSurface->format, tileLSB, tileMSB, (uint32_t *)pSurface->pixels, x, y);

            offset += 16;
        }
    }

    SDL_UnlockSurface(pSurface);
}

// Draws the display of the NES monitor
void StatusMonitor::DrawDisplay()
{
    // Draw a box around the display
    SDL_Rect border = { nesDisplayRect.x - 1,
        nesDisplayRect.y - 1,
        nesDisplayRect.w + 2,
        nesDisplayRect.h + 2 };
    
    SDL_FillRect(screenSurface, &border, colorWhite);

    // Draw the tv display
    SDL_BlitScaled(pPPU->pTV_Display, NULL, screenSurface, &nesDisplayRect);
    

    // Draw the pattern tables
    // Draw pattern table 1
    DrawPattern(pPPU->pPattern1, pPPU->pPatternTable->mem);

    // Draw a border
    border = { pattern1Rect.x - 1,
        pattern1Rect.y - 1,
        pattern1Rect.w + 2,
        pattern1Rect.h + 2 };

    SDL_FillRect(screenSurface, &border, colorWhite);
    // Draw pattern 1 pixels
    SDL_BlitSurface(pPPU->pPattern1, NULL, screenSurface, &pattern1Rect);


    // Draw pattern table 2
    DrawPattern(pPPU->pPattern2, &pPPU->pPatternTable->mem[0x1000]);

    // Draw a border
    border = { pattern2Rect.x - 1,
        pattern2Rect.y - 1,
        pattern2Rect.w + 2,
        pattern2Rect.h + 2 };

    SDL_FillRect(screenSurface, &border, colorWhite);
    // Draw pattern 2 pixels
    SDL_BlitSurface(pPPU->pPattern2, NULL, screenSurface, &pattern2Rect);
}
#endif

StatusMonitor::~StatusMonitor()
{
    // TODO: Cleanup
}

bool StatusMonitor::EventLoop()
{
    SDL_Event event;
    if (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
            case SDL_KEYDOWN:
                switch (event.key.keysym.sym)
                {
                    case SDLK_s:
                        pCPU->Step();
                        break;
                    case SDLK_r:
                        pCPU->Reset();
                        break;
                    case SDLK_g:
                        cpuRunning = true;
                        pCPU->running = true;
                        break;
                }

                // write key to 0xff
                pCPU->bus.write(0xff, event.key.keysym.sym);
                break;
            case SDL_QUIT:
                return false;
                break;
        }
    }

    Draw();

    if (cpuRunning && pCPU->running)
    {
        for (int i = 0; i < 100 && cpuRunning && pCPU->running; ++i)
            cpuRunning = pCPU->Step();
    }

    return true;
}

void StatusMonitor::Draw()
{
    // Draw a black background
    SDL_FillRect(screenSurface, NULL, colorBlack);

    DrawDisplay();

    DrawCPU_Status();

    // Update the surface
    SDL_UpdateWindowSurface(window);
}

void StatusMonitor::DrawStatusReg(char *regName, bool set, int x, int y)
{
    SDL_Color color;

    if (set)
        color = sdlColorWhite;
    else
        color = sdlColorPurple;

    SDL_Surface *pFont = FNT_Render(regName, color);

    SDL_Rect destRect = { x, y, pFont->w, pFont->h };

    SDL_BlitSurface(pFont, NULL, screenSurface, &destRect);

    SDL_FreeSurface(pFont);
}

void StatusMonitor::DrawReg(char * regName, uint16_t value, int x, int y, bool showDecimal)
{
    char str[32];

    if(showDecimal)
        sprintf(str, "%s: 0x%X (%d)", regName, value, value);
    else
        sprintf(str, "%s: 0x%X", regName, value);

    SDL_Surface *pFont = FNT_Render(str, sdlColorWhite);

    SDL_Rect destRect = { x, y, pFont->w, pFont->h };

    SDL_BlitSurface(pFont, NULL, screenSurface, &destRect);

    SDL_FreeSurface(pFont);
}

void StatusMonitor::SetupColors()
{
    // SDL_Color colors
    sdlColorBlack      = { 0x00, 0x00, 0x00, 0xFF };
    sdlColorWhite      = { 0xFF, 0xFF, 0xFF, 0xFF };
    sdlColorRed        = { 0x88, 0x00, 0x00, 0xFF };
    sdlColorCyan       = { 0xAA, 0xFF, 0xEE, 0xFF };
    sdlColorPurple     = { 0xCC, 0x44, 0xCC, 0xFF };
    sdlColorGreen      = { 0x00, 0xCC, 0x55, 0xFF };
    sdlColorBlue       = { 0x00, 0x00, 0xAA, 0xFF };
    sdlColorYellow     = { 0xEE, 0xEE, 0x77, 0xFF };
    sdlColorOrange     = { 0xDD, 0x88, 0x55, 0xFF };
    sdlColorBrown      = { 0x66, 0x44, 0x00, 0xFF };
    sdlColorLightRed   = { 0xFF, 0x77, 0x77, 0xFF };
    sdlColorDarkGray   = { 0x33, 0x33, 0x33, 0xFF };
    sdlColorGray       = { 0x77, 0x77, 0x77, 0xFF };
    sdlColorLightGreen = { 0xAA, 0xFF, 0x66, 0xFF };
    sdlColorLightBlue  = { 0x00, 0x88, 0xFF, 0xFF };
    sdlColorLightGray  = { 0xBB, 0xBB, 0xBB, 0xFF };
    
    // uint32_t colors
    colorBlack = COLOR_FROM_SDL_COLOR(screenSurface->format, sdlColorBlack);
    colorWhite = COLOR_FROM_SDL_COLOR(screenSurface->format, sdlColorWhite);
    colorRed   = COLOR_FROM_SDL_COLOR(screenSurface->format, sdlColorRed);
    colorCyan  = COLOR_FROM_SDL_COLOR(screenSurface->format, sdlColorCyan);
    colorPurple    = COLOR_FROM_SDL_COLOR(screenSurface->format, sdlColorPurple);
    colorGreen     = COLOR_FROM_SDL_COLOR(screenSurface->format, sdlColorGreen);
    colorBlue      = COLOR_FROM_SDL_COLOR(screenSurface->format, sdlColorBlue);
    colorYellow    = COLOR_FROM_SDL_COLOR(screenSurface->format, sdlColorYellow);
    colorOrange    = COLOR_FROM_SDL_COLOR(screenSurface->format, sdlColorOrange);
    colorBrown     = COLOR_FROM_SDL_COLOR(screenSurface->format, sdlColorBrown);
    colorLightRed  = COLOR_FROM_SDL_COLOR(screenSurface->format, sdlColorLightRed);
    colorDarkGray  = COLOR_FROM_SDL_COLOR(screenSurface->format, sdlColorDarkGray);
    colorGray      = COLOR_FROM_SDL_COLOR(screenSurface->format, sdlColorGray);
    colorLightGreen = COLOR_FROM_SDL_COLOR(screenSurface->format, sdlColorLightGreen);
    colorLightBlue = COLOR_FROM_SDL_COLOR(screenSurface->format, sdlColorLightBlue);
    colorLightGray = COLOR_FROM_SDL_COLOR(screenSurface->format, sdlColorLightGray);
}

void StatusMonitor::DrawCPU_Status()
{
    // Start with the status flags
    DrawStatusReg("N", pCPU->flags.negative, STATUS_MONITOR_WIDTH - (16 * 9), 16);
    DrawStatusReg("V", pCPU->flags.overflow, STATUS_MONITOR_WIDTH - (16 * 8), 16);
    DrawStatusReg("-", pCPU->flags.ignored, STATUS_MONITOR_WIDTH - (16 * 7), 16);
    DrawStatusReg("B", pCPU->flags.breakCommand, STATUS_MONITOR_WIDTH - (16 * 6), 16);
    DrawStatusReg("D", pCPU->flags.decimal, STATUS_MONITOR_WIDTH - (16 * 5), 16);
    DrawStatusReg("I", pCPU->flags.irqDisable, STATUS_MONITOR_WIDTH - (16 * 4), 16);
    DrawStatusReg("Z", pCPU->flags.zero, STATUS_MONITOR_WIDTH - (16 * 3), 16);
    DrawStatusReg("C", pCPU->flags.carry, STATUS_MONITOR_WIDTH - (16 * 2), 16);

    // Next draw the numeric registers
    DrawReg("A", pCPU->a, STATUS_MONITOR_WIDTH - (16 * 8), 16 * 2);
    DrawReg("X", pCPU->x, STATUS_MONITOR_WIDTH - (16 * 8), 16 * 3);
    DrawReg("Y", pCPU->y, STATUS_MONITOR_WIDTH - (16 * 8), 16 * 4);
    DrawReg("PC", pCPU->PC, STATUS_MONITOR_WIDTH - (16 * 8), 16 * 5, false);
    DrawReg("SP", 0x100 + pCPU->SP, STATUS_MONITOR_WIDTH - (16 * 8), 16 * 6, false);
    
    // Now draw some memory and/or a disassembly
    // TODO
}

