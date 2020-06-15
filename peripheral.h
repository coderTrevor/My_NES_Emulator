#pragma once
#include <cstdint>

class Bus;
class Peripheral
{
public:
    Peripheral(Bus *pBus, uint16_t addressStart, uint16_t addressEnd);
    ~Peripheral();

    virtual uint8_t read(uint16_t addr) = 0;
    virtual void write(uint16_t addr, uint8_t data) = 0;
};

