#include "../include/Cartridge.hpp"
// #include "../include/Mapper.hpp"
#include <fstream>

#include <iostream>

NES::Cartridge::Cartridge(std::string filename)
{
    std::cout << "Parsing cartridge ROM" << std::endl;
    std::fstream NESfile;
    NESfile.open(filename, std::ios::in | std::ios::binary);
    if (NESfile.is_open())
    {
        NESfile.read((char*)&header, sizeof(header));
        if ((header.name[0] == 'N') && (header.name[1] == 'E') && (header.name[2] == 'S') && (header.name[3] == 0x1A))
        {
            if ((header.flags7 & 0x0C) == 0x08)
                inesFormat = 2;
            else
                inesFormat = 1;
        }
        if (inesFormat == 0)
            return;
        mapperID = ((header.flags7 & 0xF0) | (header.flags6 >> 4));
        if (header.flags6 & 0x04)
        {
            // NESfile.seekg(512, std::ios_base::cur);
            std::cout << "512 byte trainer present" << std::endl;
            trainerPresent = true;
            trainer = new uint8_t[512];
            NESfile.read((char*)(trainer), 512);
        }
        VRAM4screen = (header.flags6 & 0x08)? true : false;
        vertMirror = (header.flags6 & 0x01)? true : false;
        if (inesFormat == 1)
        {
            std::cout << "INES format 1 detected" << std::endl;
            nPrgROM = header.nPrgROM;
            prgROM = new uint8_t[16384 * nPrgROM];
            NESfile.read((char*)(prgROM), 16384 * nPrgROM);
            std::cout << "PRG ROM size: " << std::dec << (int)(16384 * nPrgROM) << " bytes" << std::endl;

            nChrROM = header.nChrROM;       // NOTE: if 0, chrROM is utilized as CHR RAM
            chrROM = new uint8_t[8192 * ((nChrROM > 0)? nChrROM : 1)];
            NESfile.read((char*)(chrROM), 8192 * ((nChrROM > 0)? nChrROM : 1));
            std::cout << "CHR ROM size: " << std::dec << (int)(8192 * ((nChrROM > 0)? nChrROM : 1)) << " bytes" << std::endl;
        }
        else    // iNES 2.0 format (https://wiki.nesdev.com/w/index.php/NES_2.0)
        {
            std::cout << "INES format 2 detected" << std::endl;
            nPrgROM = ((header.flags9 & 0x0F) == 0x0F)? ((header.nPrgROM >> 2) * ((2 * (header.nPrgROM & 0x03)) + 1)) : (((uint16_t)(header.flags9 & 0x0F) << 8) | header.nPrgROM);
            prgROM = new uint8_t[16384 * nPrgROM];
            NESfile.read((char*)(prgROM), 16384 * nPrgROM);
            std::cout << "PRG ROM size: " << std::dec << (int)(16384 * nPrgROM) << " bytes" << std::endl;

            nChrROM = ((header.flags9 & 0xF0) == 0xF0)? ((header.nChrROM >> 2) * ((2 * (header.nChrROM & 0x03)) + 1)) : (((uint16_t)(header.flags9 & 0xF0) << 4) | header.nChrROM);
            chrROM = new uint8_t[8192 * nChrROM];
            NESfile.read((char*)(chrROM), 8192 * nChrROM);
            std::cout << "CHR ROM size: " << std::dec << (int)(8192 * nChrROM) << " bytes" << std::endl;
        }
        NESfile.close();
        // NOTE: not reading remaining data, as it is for PlayChoice arcade console
    }
}

NES::Cartridge::~Cartridge()
{
    delete[] trainer;
    delete[] prgROM;
    delete[] chrROM;
}

/*
// NES file format:
    - Header (16 bytes)
    - Trainer, if present (0 or 512 bytes)
    - PRG ROM data (16384 * x bytes)
    - CHR ROM data, if present (8192 * y bytes)
    - PlayChoice INST-ROM, if present (0 or 8192 bytes)
    - PlayChoice PROM, if present (16 bytes Data, 16 bytes CounterOut) (this is often missing, see PC10 ROM-Images for details)
*/