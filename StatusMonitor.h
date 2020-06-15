#pragma once
#include "Bus.h"
#include "CPU_6502.h"
#include "RAM.h"
#include <SDL.h>

#define STATUS_MONITOR_WIDTH    640
#define STATUS_MONITOR_HEIGHT   480

class StatusMonitor
{
public:
    StatusMonitor(RAM *pRAM, CPU_6502 *pCPU);
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

    bool cpuRunning;

protected:
    void DrawStatusReg(char *regName, bool set, int x, int y);
    void DrawReg(char *regName, uint16_t value, int x, int y, bool showDecimal = true);
    void SetupColors();
};

