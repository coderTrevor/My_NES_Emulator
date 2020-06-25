#pragma once
#include <string.h>
#include "RAM.h"
#include "PPU.h"
#include "CPU_6502.h"

class Snapshot
{
public:
    Snapshot(const char *ROMname, RAM *pSystemRAM, CPU_6502 *pCPU, PPU *pPPU);
    ~Snapshot();

    void Save();
    void Load();

    RAM *pSystemRAM;
    CPU_6502 *pCPU;
    PPU *pPPU;
    char fileName[256];
};

