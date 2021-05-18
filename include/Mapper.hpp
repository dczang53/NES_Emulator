#ifndef _Mapper
#define _Mapper

#include <cstdint>
#include <string>

// TLDR basically an MMU

namespace NES
{
    enum mirror     // https://wiki.nesdev.com/w/index.php/Mirroring#Nametable_Mirroring
    {
        horizontal,
        vertical,
        singleScreenLower,
        singleScreenUpper,
        fourScreen,
        undefined
    };

    class Cartridge;

    class Mapper
    {
    public:
        Mapper(Cartridge *c);
        ~Mapper();

        const uint8_t mapperID;

        virtual uint8_t cpuRead(uint16_t addr) = 0;
        virtual bool cpuWrite(uint16_t addr, uint8_t data) = 0;
        virtual uint8_t ppuRead(uint16_t addr) = 0;
        virtual bool ppuWrite(uint16_t addr, uint8_t data) = 0;

        bool IRQcheck() {return IRQ;}
        void IRQreset() {IRQ = false;}

    protected:
        Cartridge *cart;                // prgROM for CPU 0x8000 - 0xFFFF and chrROM for PPU 0x0000 - 0x1FFF
        uint8_t *EXPROM = nullptr;      // addresses for CPU 0x4020 - 0x5FFF (only used by specific mappers as ROM. RAM, or registers) (see "http://wiki.nesdev.com/w/index.php/Category:Mappers_using_$4020-$5FFF")
        uint8_t *SRAM = nullptr;        // addresses for CPU 0x6000 - 0x7FFF (save RAM unsupported for now)

        uint8_t *NAMETABLE = nullptr;   // nametable mirroring is handled here (also includes attribute tables)
        mirror ntMirror = undefined;
        
        bool IRQ = false;
    };



    Mapper* createMapper(std::string filename);   // use this to initialize Mapper and internal Cartridge



    class Mapper0 : public Mapper
    {
    public:
        Mapper0(Cartridge *c);
        ~Mapper0();
        uint8_t cpuRead(uint16_t addr) override;
        bool cpuWrite(uint16_t addr, uint8_t data) override;
        uint8_t ppuRead(uint16_t addr) override;
        bool ppuWrite(uint16_t addr, uint8_t data) override;
    };



    class Mapper1 : public Mapper
    {
    public:
        Mapper1(Cartridge *c);
        ~Mapper1();
        uint8_t cpuRead(uint16_t addr) override;
        bool cpuWrite(uint16_t addr, uint8_t data) override;
        uint8_t ppuRead(uint16_t addr) override;
        bool ppuWrite(uint16_t addr, uint8_t data) override;
    private:
        uint8_t regLoad = 0x00;
        uint8_t regCtrl = 0x00;
        uint8_t regChrBank0 = 0x00;
        uint8_t regChrBank1 = 0x00;
        uint8_t regPrgBank = 0x00;
    };



    class Mapper2 : public Mapper
    {
    public:
        Mapper2(Cartridge *c);
        ~Mapper2();
        uint8_t cpuRead(uint16_t addr) override;
        bool cpuWrite(uint16_t addr, uint8_t data) override;
        uint8_t ppuRead(uint16_t addr) override;
        bool ppuWrite(uint16_t addr, uint8_t data) override;
    private:
        uint8_t regBankSelect = 0x00;
    };



    class Mapper3 : public Mapper
    {
    public:
        Mapper3(Cartridge *c);
        ~Mapper3();
        uint8_t cpuRead(uint16_t addr) override;
        bool cpuWrite(uint16_t addr, uint8_t data) override;
        uint8_t ppuRead(uint16_t addr) override;
        bool ppuWrite(uint16_t addr, uint8_t data) override;
    private:
        uint8_t regBankSelect = 0x00;
    };



    class Mapper4 : public Mapper
    {
    public:
        Mapper4(Cartridge *c);
        ~Mapper4();
        uint8_t cpuRead(uint16_t addr) override;
        bool cpuWrite(uint16_t addr, uint8_t data) override;
        uint8_t ppuRead(uint16_t addr) override;
        bool ppuWrite(uint16_t addr, uint8_t data) override;
    private:
        // regBankSelect
        uint8_t updateRegister = 0x00;
        bool prgROMbankMode = false;
        bool chrROMinversion = false;
        // regBankData
        uint8_t bankRegisters[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};    // R0 through R5 for CHR banks, R6 and R7 for PRG banks
        // regMirroring
        const bool VRAM4screen;             // not sure how this is used
        bool horizontalMirroring = false;   // not sure how this is used
        // regPrgRamProtect
        bool prgRamWriteEnable = true;
        bool prgRamEnable = false;
        // scanline counting                // in progress as IRQcounter is a bitch
        uint8_t regIRQlatch = 0x00;
        bool regIRQreload = false;
        bool regIRQdisable = false;
        bool regIRQenable = false;

    };
};

// https://wiki.nesdev.com/w/index.php/List_of_mappers
// https://wiki.nesdev.com/w/index.php/Mapper#iNES_1.0_mapper_grid

// https://wiki.nesdev.com/w/index.php/Mapper
// implemented mappers -> 0, 2, 3
// have yet to implement mappers 1 and 4

// https://bytes.vokal.io/mmc3_irqs/

// https://forums.nesdev.com/viewtopic.php?t=12936
// http://wiki.nesdev.com/w/index.php/Category:Mappers_using_$4020-$5FFF
// only a few mappers use memory addresses 0x4020 - 0x5FFF for registers; otherwise unused


// http://bootgod.dyndns.org:7777/profile.php?id=1091
// Donkey Kong -> iNes mapper 0

// http://bootgod.dyndns.org:7777/profile.php?id=5
// Super Mario Bros 3 -> iNES mapper 4

#endif