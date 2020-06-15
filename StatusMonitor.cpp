#include "StatusMonitor.h"
#include <stdio.h>
#include <cstdint>
#include <SDL.h>
#include "SDL_picofont.h"

#define SIMPLE_DISPLAY_PIXEL_SCALE 8
#define SIMPLE_DISPLAY_X           8
#define SIMPLE_DISPLAY_Y           8
#define SIMPLE_DISPLAY_WIDTH       32
#define SIMPLE_DISPLAY_HEIGHT      32
SDL_Rect simpleDisplayRect = { SIMPLE_DISPLAY_X,
                                SIMPLE_DISPLAY_Y,
                                SIMPLE_DISPLAY_WIDTH * SIMPLE_DISPLAY_PIXEL_SCALE,
                                SIMPLE_DISPLAY_HEIGHT * SIMPLE_DISPLAY_PIXEL_SCALE };

#define COLOR_FROM_SDL_COLOR(format, sdlColor) SDL_MapRGB(format, sdlColor.r, sdlColor.g, sdlColor.b)

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
        "An SDL2 window",                  // window title
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

    if (cpuRunning)
    {
        for (int i = 0; i < 100 && cpuRunning; ++i)
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
