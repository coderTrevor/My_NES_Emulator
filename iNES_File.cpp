#include "iNES_File.h"
#include "Snapshot.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

iNES_File::iNES_File(const char *fileName)
{
    OpenFile(fileName);
}


iNES_File::~iNES_File()
{
    free(pCHRdata);
    free(pPRGdata);
}

bool iNES_File::OpenFile(const char * fileName)
{
    pPRGdata = NULL;
    pCHRdata = NULL;

    printf("Header: %d bytes\n", sizeof(I_NES_HEADER));
    FILE *pFile;
    pFile = fopen(fileName, "rb");
    if (!pFile)
    {
        printf("Unable to open %s\n", fileName);

        return false;
    }

    I_NES_HEADER header;
    if (fread(&header, sizeof(I_NES_HEADER), 1, pFile) != 1)
    {
        printf("Error reading header\n");
        return false;
    }

    // Make sure header starts with NES file type ID
    char fileType[5] = { 0 };
    strncpy(fileType, header.fileID, 4);

    if (strcmp(fileType, "NES\x1A") != 0)
    {
        printf("%s is not an iNES file!\n", fileName);
        return false;
    }

    prgSize = 16 * 1024 * header.PRG_ROM_Size;
    chrRomSize = 8 * 1024 * header.CHR_ROM_Size;

    printf("PRG ROM Size: %d bytes\n", prgSize);
    printf("CHR ROM Size: %d bytes\n", chrRomSize);

    printf("0x%X\n0x%X\n", header.flags6.flags, header.flags7.flags);

    pPRGdata = (uint8_t*)malloc(prgSize);
    pCHRdata = (uint8_t*)malloc(chrRomSize);

    if (fread(pPRGdata, 1, prgSize, pFile) != prgSize)
    {
        printf("Error reading PRG ROM!\n");
        return false;
    }

    if (fread(pCHRdata, 1, chrRomSize, pFile) != chrRomSize)
    {
        printf("Error reading CHR ROM!\n");
        return false;
    }

    int mapperNumber = (header.flags7.mapperNumberUpper4bits << 4)
        | header.flags6.mapperNumberLower4bits;

    printf("Mapper %d\n", mapperNumber);

    printf("Successfully opened %s\n", fileName);

    fclose(pFile);

    return true;
}
