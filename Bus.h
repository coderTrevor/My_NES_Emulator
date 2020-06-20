#pragma once
#include <cstdint>

class Peripheral;

typedef struct MEM_MAP_ENTRY
{
    uint16_t startAddr;
    uint16_t endAddr;
    Peripheral *pPeripheral;
}MEM_MAP_ENTRY;

#define MAX_PERIPHERALS 8

class CPU_6502;
class Bus
{
public:
    Bus();
    ~Bus();

    uint8_t read(uint16_t addr);
    void write(uint16_t addr, uint8_t data);
    void attachPeripheral(uint16_t startAddr, uint16_t endAddr, Peripheral *pPer);

    CPU_6502 *pCPU;
    bool isCPU_Bus;

protected:
    int numPeripherals;
    MEM_MAP_ENTRY peripherals[MAX_PERIPHERALS];
};

