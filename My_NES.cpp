// My_NES.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "Bus.h"
#include "CPU_6502.h"
#include "RAM.h"
#include "StatusMonitor.h"
#include "iNES_File.h"
#include "System.h"
#include "NES_Controller.h"
#include "Snapshot.h"

bool debugOutput = false;

#ifdef SYSTEM_SIMPLE
void SimpleMain()
{
    CPU_6502 cpu;

    RAM ram(&(cpu.bus), 0, 0xFFff);

    uint16_t startAddress = 0x600;

    // Put start address (0x600) at 0xFFFC and 0xFFFD
    ram.mem[0xFFFC] = startAddress & 0xFF;  // 0x00;
    ram.mem[0xFFFD] = startAddress >> 8;    // 0x06;
     
    // load in a hex dump
    //char dump[] = "0600: a9 01 8d 00 02 a9 05 8d 01 02 a9 08 8d 02 02 ";
    //char dump[] = "0600: a9 80 85 01 65 01";
    //char dump[] = "0600: a9 c0 aa e8 69 c4 00 ";
    //char dump[] = "0600: a2 08 ca 8e 00 02 e0 03 d0 f8 8e 01 02 00";
    //char dump[] = "0600: a9 01 85 f0 a9 cc 85 f1 6c f0 00";
    //char dump[] = "0600: a2 01 a9 05 85 01 a9 07 85 02 a0 0a 8c 05 07 a1 0610: 00";
    //char dump[] = "0600: a0 01 a9 03 85 01 a9 07 85 02 a2 0a 8e 04 07 b1 0610: 01";
    //char dump[] = "0600: a2 00 a0 00 8a 99 00 02 48 e8 c8 c0 10 d0 f5 68 0610: 99 00 02 c8 c0 20 d0 f7";
    //char dump[] = "0600: 20 09 06 20 0c 06 20 12 06 a2 00 60 e8 e0 05 d0 0610: fb 60 00";
    //char dump[] = "0600: a9 05 38 e9 05";
    //char dump[] = "0600: a9 05 38 e9 03";

    char dump[] = "0600: a9 80 85 00 a9 02 85 01 a9 f3 85 02 a9 00 85 03 "
        "0610: a9 03 85 04 84 90 38 91 00 18 91 03 ";

    char drNicksSnakeGame[] = "0600: 20 06 06 20 38 06 20 0d 06 20 2a 06 60 a9 02 85"
        " 0610: 02 a9 04 85 03 a9 11 85 10 a9 10 85 12 a9 0f 85"
        " 0620: 14 a9 04 85 11 85 13 85 15 60 a5 fe 85 00 a5 fe"
        " 0630: 29 03 18 69 02 85 01 60 20 4d 06 20 8d 06 20 c3"
        " 0640: 06 20 19 07 20 20 07 20 2d 07 4c 38 06 a5 ff c9"
        " 0650: 77 f0 0d c9 64 f0 14 c9 73 f0 1b c9 61 f0 22 60"
        " 0660: a9 04 24 02 d0 26 a9 01 85 02 60 a9 08 24 02 d0"
        " 0670: 1b a9 02 85 02 60 a9 01 24 02 d0 10 a9 04 85 02"
        " 0680: 60 a9 02 24 02 d0 05 a9 08 85 02 60 60 20 94 06"
        " 0690: 20 a8 06 60 a5 00 c5 10 d0 0d a5 01 c5 11 d0 07"
        " 06a0: e6 03 e6 03 20 2a 06 60 a2 02 b5 10 c5 10 d0 06"
        " 06b0: b5 11 c5 11 f0 09 e8 e8 e4 03 f0 06 4c aa 06 4c"
        " 06c0: 35 07 60 a6 03 ca 8a b5 10 95 12 ca 10 f9 a5 02"
        " 06d0: 4a b0 09 4a b0 19 4a b0 1f 4a b0 2f a5 10 38 e9"
        " 06e0: 20 85 10 90 01 60 c6 11 a9 01 c5 11 f0 28 60 e6"
        " 06f0: 10 a9 1f 24 10 f0 1f 60 a5 10 18 69 20 85 10 b0"
        " 0700: 01 60 e6 11 a9 06 c5 11 f0 0c 60 c6 10 a5 10 29"
        " 0710: 1f c9 1f f0 01 60 4c 35 07 a0 00 a5 fe 91 00 60"
        " 0720: a6 03 a9 00 81 10 a2 00 a9 01 81 10 60 a2 00 ea"
        " 0730: ea ca d0 fb 60 ";

    char mySnakeGame[] = "0600: 20 8f 06 a2 ff 9a 20 f6 07 a9 00 a2 0f a0 0f 20 "
        "0610: d2 07 a9 01 85 03 86 20 84 60 20 72 07 20 72 07 "
        "0620: 20 72 07 20 5c 07 4c 38 06 a5 60 c5 19 d0 0f 20 "
        "0630: 5c 07 20 72 07 4c 3e 06 a5 20 c5 18 f0 eb a6 18 "
        "0640: a4 19 a9 05 20 d2 07 a5 ff c9 61 f0 18 a5 ff c9 "
        "0650: 64 f0 1b a5 ff c9 77 f0 27 a5 ff c9 73 f0 18 20 "
        "0660: 8f 06 4c 38 06 20 17 07 20 8f 06 4c 38 06 20 00 "
        "0670: 07 20 8f 06 4c 38 06 20 2e 07 20 8f 06 4c 38 06 "
        "0680: 20 45 07 20 8f 06 4c 38 06 20 aa 06 4c 00 06 a2 "
        "0690: 02 a9 dc e5 03 e5 03 e5 03 e5 03 e5 03 e5 03 e5 "
        "06a0: 03 18 e9 01 d0 fc ca d0 e8 60 a2 14 a0 ff 88 d0 "
        "06b0: fd ca d0 f8 60 86 06 84 07 a5 03 85 04 a9 20 85 "
        "06c0: 08 a9 60 85 10 a9 00 85 09 85 11 20 d3 06 c6 04 "
        "06d0: d0 f9 60 a5 03 c5 04 d0 11 a5 04 a8 88 b1 08 48 "
        "06e0: b1 10 a8 68 aa a9 01 20 d2 07 a4 04 88 d0 01 60 "
        "06f0: a4 04 88 88 b1 08 48 b1 10 c8 91 10 68 91 08 60 "
        "0700: a6 20 a4 60 a9 00 e8 e0 20 d0 03 4c 89 06 20 b5 "
        "0710: 06 e6 20 20 98 07 60 a6 20 a4 60 a9 00 e0 00 d0 "
        "0720: 03 4c 89 06 ca 20 b5 06 c6 20 20 98 07 60 a6 20 "
        "0730: a4 60 a9 00 c8 c0 20 d0 03 4c 89 06 20 b5 06 e6 "
        "0740: 60 20 98 07 60 a6 20 a4 60 a9 00 c0 00 d0 03 4c "
        "0750: 89 06 88 20 b5 06 c6 60 20 98 07 60 a5 fe 29 1f "
        "0760: 85 18 a5 fe 29 1f 85 19 a6 18 a4 19 a9 05 20 d2 "
        "0770: 07 60 a9 1f 18 c5 03 b0 01 60 a9 00 85 09 85 11 "
        "0780: a9 20 85 08 a9 3c 85 10 a4 03 b1 08 48 b1 10 c8 "
        "0790: 91 10 68 91 08 e6 03 60 a6 20 a4 60 20 b0 07 c9 "
        "07a0: 00 d0 03 4c 89 06 a6 20 a4 60 a9 00 20 d2 07 60 "
        "07b0: a9 00 85 00 a9 02 85 01 c0 00 4c c9 07 a5 00 18 "
        "07c0: 69 20 90 02 e6 01 85 00 88 d0 f2 86 02 a4 02 b1 "
        "07d0: 00 60 48 a9 00 85 00 a9 02 85 01 c0 00 4c ec 07 "
        "07e0: a5 00 18 69 20 90 02 e6 01 85 00 88 d0 f2 68 86 "
        "07f0: 02 a4 02 91 00 60 a9 01 85 02 a0 08 a5 02 a2 20 "
        "0800: ca 9d 00 02 9d 20 02 9d 40 02 9d 60 02 9d 80 02 "
        "0810: 9d a0 02 9d c0 02 9d e0 02 9d 00 03 9d 20 03 9d "
        "0820: 40 03 9d 60 03 9d 80 03 9d a0 03 9d c0 03 9d e0 "
        "0830: 03 9d 00 04 9d 20 04 9d 40 04 9d 60 04 9d 80 04 "
        "0840: 9d a0 04 9d c0 04 9d e0 04 9d 00 05 9d 20 05 9d "
        "0850: 40 05 9d 60 05 9d 80 05 9d a0 05 9d c0 05 9d e0 "
        "0860: 05 d0 9d 60 ";

    ram.loadHexDump(mySnakeGame);

    // Create the status monitor
    StatusMonitor statusMonitor(&ram, &cpu);

    cpu.Reset();

    while (statusMonitor.EventLoop())
    {

    }
}
#endif

#ifdef SYSTEM_NES

Snapshot *pSnapshot;

void NES_Main()
{
    CPU_6502 cpu;
    
    PPU ppu(&cpu);
    ppu.PPU_Bus.isCPU_Bus = false;

    NES_Controller nesController1(&(cpu.bus));
    RAM ram(&(cpu.bus), 0, 0xFFff);

    const char *ROM_Name = "Super Mario Bros. (World).nes";
    //const char *ROM_Name = "DK.nes";

    // Patch SMB to always return 9 lives
    if (strcmp(ROM_Name, "Super Mario Bros. (World).nes") == 0)
    {
        cpu.bus.patchRead = true;
        cpu.bus.patchedAddress = 0x75A;
        cpu.bus.patchedData = 8;
    }

    // Patch donkey kong to always return 2 lives
    if (strcmp(ROM_Name, "DK.nes") == 0)
    {
        cpu.bus.patchRead = true;
        cpu.bus.patchedAddress = 0x55;
        cpu.bus.patchedData = 2;
    }
    
    //iNES_File ROM("01-basics.nes");
    //iNES_File ROM("05-zp_xy.nes");
    //iNES_File ROM("03-dummy_reads.nes");
    //iNES_File ROM("nestest.nes");
    //iNES_File ROM("scanline.nes");
    iNES_File ROM(ROM_Name);    
    //iNES_File ROM("Popeye.nes");
    //iNES_File ROM("Ice Climber (USA, Europe).nes");

    // Create snapshot for this ROM
    pSnapshot = new Snapshot(ROM_Name, &ram, &cpu, &ppu);

    // TEMP:
    // Insert the prg data into the RAM
    switch (ROM.prgSize)
    {
        case (32 * 1024):
            memcpy(&ram.mem[0x8000], ROM.pPRGdata, ROM.prgSize);
            break;
        case (16 * 1024):
            memcpy(&ram.mem[0x8000], ROM.pPRGdata, ROM.prgSize);
            memcpy(&ram.mem[0xC000], ROM.pPRGdata, ROM.prgSize);
            break;
        default:
            printf("Don't know how to map this ROM!\n");
            return;
    }

    // TEMP:
    // Insert the CHR data into the PPU pattern table
    memcpy(ppu.pPatternTable->mem, ROM.pCHRdata, ROM.chrRomSize);

    // Create the status monitor
    StatusMonitor statusMonitor(&ram, &cpu, &ppu, &nesController1);

    cpu.Reset();

    while (statusMonitor.EventLoop())
    {

    }
}
#endif

int main(int argc, char* argv[])
{
#ifdef SYSTEM_SIMPLE
    SimpleMain();
#else
    NES_Main();
#endif

    return 0;
}

