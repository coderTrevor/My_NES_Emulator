#include "peripheral.h"
#include "Bus.h"


Peripheral::Peripheral(Bus *pBus, uint16_t addressStart, uint16_t addressEnd)
{
    pBus->attachPeripheral(addressStart, addressEnd, this);
}


Peripheral::~Peripheral()
{
}
