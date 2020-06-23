#pragma once
#include "Bus.h"
#include "CPU_6502.h"
#include "RAM.h"
#include "System.h"
#include "PPU.h"
#include "NES_Controller.h"
#include <SDL.h>
#include <list>

#ifdef SIMPLE_SYSTEM
#define STATUS_MONITOR_WIDTH    640
#define STATUS_MONITOR_HEIGHT   480
#else
#define STATUS_MONITOR_WIDTH    800
#define STATUS_MONITOR_HEIGHT   600
#endif

#define SIMPLE_DISPLAY_PIXEL_SCALE 8
#define SIMPLE_DISPLAY_X           8
#define SIMPLE_DISPLAY_Y           8
#define SIMPLE_DISPLAY_WIDTH       32
#define SIMPLE_DISPLAY_HEIGHT      32
extern SDL_Rect simpleDisplayRect;

#define NES_DISPLAY_PIXEL_SCALE 2
#define NES_DISPLAY_X       8
#define NES_DISPLAY_Y       8
#define NES_DISPLAY_WIDTH   256
#define NES_DISPLAY_HEIGHT  240
#define NES_PATTERN_WIDTH   128
#define NES_PATTERN_HEIGHT  128
#define NES_MARGIN          8

#define NAMETABLE_RES_X    32 * 8 * 2    /* 2 screens wide, each 32 tiles of 8 pixels */
#define NAMETABLE_WIDTH    (NAMETABLE_RES_X / 2)
#define NAMETABLE_RES_Y    30 * 8 * 2    /* 2 screens tall, each 30 tiles of 8 pixels */
#define NAMETABLE_HEIGHT   (NAMETABLE_RES_Y / 2)

#define MAX_FPS_FRAME_TIMES 120

extern SDL_Rect nesDisplayRect;
extern SDL_Rect pattern1Rect;
extern SDL_Rect pattern2Rect;

class StatusMonitor
{
public:
    StatusMonitor(RAM *pRAM, CPU_6502 *pCPU);
    StatusMonitor(RAM *pRAM, CPU_6502 *pCPU, PPU *pPPU, NES_Controller *pController1);
    ~StatusMonitor();

    bool EventLoop();

    void Draw();

    void DrawCPU_Status();

    void DrawDisplay();

    SDL_Window *window;                   //The window we'll be rendering to
    SDL_Surface* screenSurface;
    
    // define some colors
    // Color 0 - black
    uint32_t colorBlack;
    SDL_Color sdlColorBlack;
    // Color 1 - white
    uint32_t colorWhite;
    SDL_Color sdlColorWhite;
    // Color 2 - red
    uint32_t colorRed;
    SDL_Color sdlColorRed;
    // Color 3 - cyan
    uint32_t colorCyan;
    SDL_Color sdlColorCyan;
    // Color 4 - purple
    uint32_t colorPurple;
    SDL_Color sdlColorPurple;
    // Color 5 - green
    uint32_t colorGreen;
    SDL_Color sdlColorGreen;
    // Color 6 - blue
    uint32_t colorBlue;
    SDL_Color sdlColorBlue;
    // Color 7 - yellow
    uint32_t colorYellow;
    SDL_Color sdlColorYellow;
    // Color 8 - orange
    uint32_t colorOrange;
    SDL_Color sdlColorOrange;
    // Color 9 - brown
    uint32_t colorBrown;
    SDL_Color sdlColorBrown;
    // Color a - light red
    uint32_t colorLightRed;
    SDL_Color sdlColorLightRed;
    // Color b - Dark gray
    uint32_t colorDarkGray;
    SDL_Color sdlColorDarkGray;
    // Color c - Gray
    uint32_t colorGray;
    SDL_Color sdlColorGray;
    // Color d - light green
    uint32_t colorLightGreen;
    SDL_Color sdlColorLightGreen;
    // Color e - color light blue
    uint32_t colorLightBlue;
    SDL_Color sdlColorLightBlue;
    // Color f - color light gray
    uint32_t colorLightGray;
    SDL_Color sdlColorLightGray;

    RAM *pRAM;
    CPU_6502 *pCPU;
    PPU *pPPU;
    NES_Controller *pController1;

    bool cpuRunning;

protected:
    void StatusMonitor::CopyTileToPixels(SDL_PixelFormat *format, uint8_t *pTileLSB, uint8_t *pTileMSB, uint32_t *pPixels, uint32_t tileX, uint32_t tileY);
    void DrawPattern(SDL_Surface *pSurface, uint8_t *pPatternMemory);
    void DrawStatusReg(char *regName, bool set, int x, int y);
    void DrawReg(char *regName, uint16_t value, int x, int y, bool showDecimal = true);
    void SetupColors();
    void LimitFPS();
    uint32_t lastFrameTime;
    uint32_t frameTimes[MAX_FPS_FRAME_TIMES];
    int frameTimesIndex;
    std::list<uint16_t> memoryLocations;
};

