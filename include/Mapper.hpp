#ifndef _Mapper
#define _Mapper

#include <cstdint>
#include <string>

namespace ricoh2A03
{
    class CPU;
}

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

        #ifdef DEBUG
            virtual uint8_t cpuReadDebug(uint16_t addr) = 0;
            virtual uint8_t ppuReadDebug(uint16_t addr) = 0;
        #endif

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



    Mapper* createMapper(std::string filename, ricoh2A03::CPU *cpu);   // use this to initialize Mapper and internal Cartridge



    class Mapper0 : public Mapper
    {
    public:
        Mapper0(Cartridge *c);
        ~Mapper0();
        uint8_t cpuRead(uint16_t addr);
        bool cpuWrite(uint16_t addr, uint8_t data);
        uint8_t ppuRead(uint16_t addr);
        bool ppuWrite(uint16_t addr, uint8_t data);
        #ifdef DEBUG
            uint8_t cpuReadDebug(uint16_t addr);
            uint8_t ppuReadDebug(uint16_t addr);
        #endif
    };



    class Mapper1 : public Mapper
    {
    public:
        Mapper1(Cartridge *c);
        ~Mapper1();
        uint8_t cpuRead(uint16_t addr);
        bool cpuWrite(uint16_t addr, uint8_t data);
        uint8_t ppuRead(uint16_t addr);
        bool ppuWrite(uint16_t addr, uint8_t data);
        #ifdef DEBUG
            uint8_t cpuReadDebug(uint16_t addr);
            uint8_t ppuReadDebug(uint16_t addr);
        #endif
    private:
        uint8_t regLoad = 0x00;     // shift register to load data into below registers
        uint8_t regCtrl = 0x1C;     // needed on startup for reading last PRG ROM bank
        uint8_t regChrBank0 = 0x00;
        uint8_t regChrBank1 = 0x00;
        uint8_t regPrgBank = 0x00;

        uint8_t loadCount = 0;      // not a register; helper counter for number of consecutive loads
    };



    class Mapper2 : public Mapper
    {
    public:
        Mapper2(Cartridge *c);
        ~Mapper2();
        uint8_t cpuRead(uint16_t addr);
        bool cpuWrite(uint16_t addr, uint8_t data);
        uint8_t ppuRead(uint16_t addr);
        bool ppuWrite(uint16_t addr, uint8_t data);
        #ifdef DEBUG
            uint8_t cpuReadDebug(uint16_t addr);
            uint8_t ppuReadDebug(uint16_t addr);
        #endif
    private:
        uint8_t regBankSelect = 0x00;
    };



    class Mapper3 : public Mapper
    {
    public:
        Mapper3(Cartridge *c);
        ~Mapper3();
        uint8_t cpuRead(uint16_t addr);
        bool cpuWrite(uint16_t addr, uint8_t data);
        uint8_t ppuRead(uint16_t addr);
        bool ppuWrite(uint16_t addr, uint8_t data);
        #ifdef DEBUG
            uint8_t cpuReadDebug(uint16_t addr);
            uint8_t ppuReadDebug(uint16_t addr);
        #endif
    private:
        uint8_t regBankSelect = 0x00;
    };



    class Mapper4 : public Mapper
    {
    public:
        Mapper4(Cartridge *c, ricoh2A03::CPU *cpu);
        ~Mapper4();
        uint8_t cpuRead(uint16_t addr);
        bool cpuWrite(uint16_t addr, uint8_t data);
        uint8_t ppuRead(uint16_t addr);
        bool ppuWrite(uint16_t addr, uint8_t data);

        // memory-neutral reading for logging and debugging (no modifying cpu/ppu registers on read)
        #ifdef DEBUG
            uint8_t cpuReadDebug(uint16_t addr);
            uint8_t ppuReadDebug(uint16_t addr);
        #endif

    private:
        uint8_t regBankSelect = 0x00;
        // uint8_t regBankData = 0x00;      // update bankRegisters[]
        uint8_t regMirror = 0x00;
        uint8_t regPrgRamProtect = 0x00;
        uint8_t regIrqLatch = 0x00;
        // uint8_t regIrqReload = 0x00;     // update irqCounter
        // uint8_t regIrqDisable = 0x00;    // update irqEnable
        // uint8_t regIrqEnable = 0x00;     // update irqEnable

        uint8_t bankRegisters[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};    // R0 through R5 for CHR banks, R6 and R7 for PRG banks
        uint8_t irqCounter = 0;
        bool irqEnable = false;
        bool A12down = false;               // PPU A12 (0x1000) needs to stay 0 for 3 CPU cycles before triggering irqCounter on rising edge
        uint64_t A12FirstDown = 0;          // cycle of 1st A12 down without being up (very small chance of overflow BS though)

        ricoh2A03::CPU *cpu = nullptr;      // for clock cycle counting

        void updateIrqCounter(uint16_t addr);

        // (https://wiki.nesdev.org/w/index.php/MMC3)
        // (https://wiki.nesdev.org/w/index.php?title=MMC3_pinout)
        // (https://github.com/furrtek/VGChips/tree/master/Nintendo/MMC3C)
        // "The MMC3 scanline counter is based entirely on PPU A12, triggered on a rising edge after the line has remained low for three falling edges of M2."
        // note: irqCounter depends solely on A12 (0x1000) for PPU address bus for triggering
        // note: m2 is cpu clock signal (https://wiki.nesdev.org/w/index.php/CPU_pin_out_and_signal_description)
        // (https://archive.nes.science/nesdev-forums/f3/t8917.xhtml) (though this is for mapper 1)
        // TLDR should always trigger as 
    };
};

// https://wiki.nesdev.com/w/index.php/List_of_mappers
// https://wiki.nesdev.com/w/index.php/Mapper#iNES_1.0_mapper_grid

#endif