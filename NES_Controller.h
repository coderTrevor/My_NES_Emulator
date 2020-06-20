#pragma once

#include "peripheral.h"
/*
0 - A
1 - B
2 - Select
3 - Start
4 - Up
5 - Down
6 - Left
7 - Right
*/
#define BTN_A       0
#define BTN_B       1
#define BTN_SELECT  2
#define BTN_START   3
#define BTN_UP      4
#define BTN_DOWN    5
#define BTN_LEFT    6
#define BTN_RIGHT   7

typedef union
{
    struct
    {
        bool a      : 1;
        bool b      : 1;
        bool select : 1;
        bool start  : 1;
        bool up     : 1;
        bool down   : 1;
        bool left   : 1;
        bool right  : 1;
    };
    uint8_t allBits;
}CONTROLLER_BUTTONS;

class NES_Controller : public Peripheral
{
public:
    NES_Controller(Bus *pBus);
    ~NES_Controller();

    uint8_t read(uint16_t address);
    void write(uint16_t address, uint8_t value);

    /*bool selectPressed;
    bool startPressed;
    bool upPressed;
    bool downPressed;*/

    CONTROLLER_BUTTONS buttons;

    uint8_t latch;
};

