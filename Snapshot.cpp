#include <stdio.h>
#include "Snapshot.h"

Snapshot::Snapshot(const char * ROMname, RAM *pSystemRAM, CPU_6502 *pCPU, PPU *pPPU)
{
    strcpy(fileName, ROMname);
    strcat(fileName, ".snp");

    this->pSystemRAM = pSystemRAM;
    this->pCPU = pCPU;
    this->pPPU = pPPU;
}

Snapshot::~Snapshot()
{
}

void Snapshot::Save()
{
    FILE *pFile = fopen(fileName, "wb");
    if (!pFile)
    {
        printf("Unable to open %s!\n", fileName);
        return;
    }

    // Save the RAM contents
    if (fwrite(pSystemRAM->mem, 0xFFFF, 1, pFile) != 1)
    {
        printf("Unable to save %s!\n", fileName);
        return;
    }

    // Save the CPU state
    if (fwrite(&pCPU->a, 1, 1, pFile) != 1
        || fwrite(&pCPU->flags.allFlags, 1, 1, pFile) != 1
        || fwrite(&pCPU->PC, 2, 1, pFile) != 1
        || fwrite(&pCPU->SP, 1, 1, pFile) != 1
        || fwrite(&pCPU->x, 1, 1, pFile) != 1
        || fwrite(&pCPU->y, 1, 1, pFile) != 1
        || fwrite(&pCPU->nmi, sizeof(bool), 1, pFile) != 1)
    {
        printf("Unable to save CPU state!\n");
        return;
    }

    // Save the PPU state
    if (fwrite(&pPPU->pNameTable->mem[0x2000], 0x2FFF, 1, pFile) != 1
       || fwrite(pPPU->controlReg_ForScanline, SCANLINES, 1, pFile) != 1
       || fwrite(&pPPU->controlReg.entireRegister, 1, 1, pFile) != 1
       || fwrite(&pPPU->horizontalMirrorOffset, 2, 1, pFile) != 1
       || fwrite(&pPPU->lastValidScanlineForControl, sizeof(int), 1, pFile) != 1
       || fwrite(&pPPU->lastValidScanlineForScroll, sizeof(int), 1, pFile) != 1
       || fwrite(&pPPU->lowByteActive, sizeof(bool), 1, pFile) != 1
       || fwrite(&pPPU->maskReg.entireRegister, 1, 1, pFile) != 1
       || fwrite(&pPPU->OAM_Address, 2, 1, pFile) != 1
       || fwrite(pPPU->OAM_Memory, sizeof(OAM_ENTRY), 64, pFile) != 64
       || fwrite(&pPPU->pPalette->paletteMem, sizeof(PALETTE_MEM), 1, pFile) != 1
       || fwrite(&pPPU->readBuffer, 1, 1, pFile) != 1
       || fwrite(&pPPU->scanline, sizeof(int), 1, pFile) != 1
       || fwrite(pPPU->scrollX_ForScanline, SCANLINES, 1, pFile) != 1
       || fwrite(&pPPU->statusReg.entireRegister, 1, 1, pFile) != 1
       || fwrite(&pPPU->verticalMirrorOffset, 2, 1, pFile) != 1
       || fwrite(&pPPU->VRAM_Address, 2, 1, pFile) != 1
       || fwrite(&pPPU->writingToScrollY, sizeof(bool), 1, pFile) != 1)
    {
        printf("Unable to save PPU state!\n");
        return;
    }

    printf("%s saved\n", fileName);

    fclose(pFile);
}

void Snapshot::Load()
{

    FILE *pFile = fopen(fileName, "rb");
    if (!pFile)
    {
        printf("Unable to open %s!\n", fileName);
        return;
    }

    // Load the memory contents
    if (fread(pSystemRAM->mem, 0xFFFF, 1, pFile) != 1)
    {
        printf("Unable to read %s!\n", fileName);
        return;
    }

    // Load the CPU state
    if (fread(&pCPU->a, 1, 1, pFile) != 1
        || fread(&pCPU->flags.allFlags, 1, 1, pFile) != 1
        || fread(&pCPU->PC, 2, 1, pFile) != 1
        || fread(&pCPU->SP, 1, 1, pFile) != 1
        || fread(&pCPU->x, 1, 1, pFile) != 1
        || fread(&pCPU->y, 1, 1, pFile) != 1
        || fwrite(&pCPU->nmi, sizeof(bool), 1, pFile) != 1)
    {
        printf("Unable to load CPU state!\n");
        return;
    }

    // Load the PPU state
    if (fread(&pPPU->pNameTable->mem[0x2000], 0x2FFF, 1, pFile) != 1
        || fread(pPPU->controlReg_ForScanline, SCANLINES, 1, pFile) != 1
        || fread(&pPPU->controlReg.entireRegister, 1, 1, pFile) != 1
        || fread(&pPPU->horizontalMirrorOffset, 2, 1, pFile) != 1
        || fread(&pPPU->lastValidScanlineForControl, sizeof(int), 1, pFile) != 1
        || fread(&pPPU->lastValidScanlineForScroll, sizeof(int), 1, pFile) != 1
        || fread(&pPPU->lowByteActive, sizeof(bool), 1, pFile) != 1
        || fread(&pPPU->maskReg.entireRegister, 1, 1, pFile) != 1
        || fread(&pPPU->OAM_Address, 2, 1, pFile) != 1
        || fread(pPPU->OAM_Memory, sizeof(OAM_ENTRY), 64, pFile) != 64
        || fread(&pPPU->pPalette->paletteMem, sizeof(PALETTE_MEM), 1, pFile) != 1
        || fread(&pPPU->readBuffer, 1, 1, pFile) != 1
        || fread(&pPPU->scanline, sizeof(int), 1, pFile) != 1
        || fread(pPPU->scrollX_ForScanline, SCANLINES, 1, pFile) != 1
        || fread(&pPPU->statusReg.entireRegister, 1, 1, pFile) != 1
        || fread(&pPPU->verticalMirrorOffset, 2, 1, pFile) != 1
        || fread(&pPPU->VRAM_Address, 2, 1, pFile) != 1
        || fread(&pPPU->writingToScrollY, sizeof(bool), 1, pFile) != 1)
    {
        //pPPU->statusReg.vBlank = false;
        printf("Unable to load PPU state!\n");
        return;
    }

    printf("Loaded %s\n", fileName);

    fclose(pFile);
}
