#include <string.h>
#include <stdio.h>
#include "RAM.h"



RAM::RAM(Bus * pBus, uint16_t addressStart, uint16_t addressEnd)
    : Peripheral(pBus, addressStart, addressEnd)
{
    actualSize = addressEnd - addressStart;
    mirrorMask = actualSize - 1;
}

RAM::RAM(Bus * pBus, uint16_t addressStart, uint16_t addressEnd, uint16_t actualSize)
    : Peripheral(pBus, addressStart, addressEnd)
{
    this->actualSize = actualSize;
    mirrorMask = actualSize - 1;
}

RAM::~RAM()
{
}

void RAM::loadHexDump(char *hexDump)
{
    int offset = 0;

    //int lengthLeft = strlen(hexDump);
    char *hexString = strtok(hexDump, " ");
    while (hexString)
    {
        //printf("%s\n", hexString);

        if (strlen(hexString) == 5)
        {
            // assume this is in the format "1234:" to tell us an offset
            sscanf(hexString, "%X", &offset);
            printf("Setting offset to 0x%X\n", offset);
        }
        else if (strlen(hexString) == 2)
        {
            uint8_t data;
            sscanf(hexString, "%hhX", &data);
            mem[offset] = data;
            printf("mem[0x%X] = 0x%X\n", offset, data);
            offset++;
        }
        else
        {
            printf("Unrecognized string: %s\n", hexString);
        }

        hexString = strtok(NULL, " ");
    }
}
