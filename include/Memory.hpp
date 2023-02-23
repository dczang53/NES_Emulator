#ifndef _MEMORY
#define _MEMORY

#include <cstdint>
#include <string>

namespace ricoh2A03
{
    class CPU;
    class APU;
}

namespace ricoh2C02
{
    class PPU;
}

namespace NES
{
    class Memory    // for googletest
    {
    public:
        Memory(){};
        ~Memory(){};

        virtual void initCartridge(std::string filename) = 0;

        virtual uint8_t cpuRead(uint16_t addr) = 0;
        virtual bool cpuWrite(uint16_t addr, uint8_t data) = 0;

        virtual uint8_t ppuRead(uint16_t addr) = 0;
        virtual bool ppuWrite(uint16_t addr, uint8_t data) = 0;

        #ifdef DEBUG
            virtual uint8_t cpuReadDebug(uint16_t addr) = 0;
            virtual uint8_t ppuReadDebug(uint16_t addr) = 0;
        #endif

        virtual void connect(ricoh2A03::CPU *c, ricoh2C02::PPU *p, ricoh2A03::APU *a) = 0;

        virtual uint8_t controllerRead(uint8_t player) = 0;
        virtual void controllerWrite(uint8_t player, uint8_t data) = 0;

        virtual bool mapperIrqReq() = 0;
        virtual void mapperIrqReset() = 0;
        virtual void toggleCpuCycle() = 0;
        virtual void ppuRequestDMA() = 0;
        virtual void finalizeDMAreq() = 0;
        virtual bool DMAactive() = 0;
        virtual void handleDMA() = 0;
    };



    class Mapper;

    class NESmemory : public Memory
    {
    public:
        NESmemory();
        ~NESmemory();

        void initCartridge(std::string filename);

        uint8_t cpuRead(uint16_t addr);
        bool cpuWrite(uint16_t addr, uint8_t data);

        uint8_t ppuRead(uint16_t addr);
        bool ppuWrite(uint16_t addr, uint8_t data);

        #ifdef DEBUG
            uint8_t cpuReadDebug(uint16_t addr);
            uint8_t ppuReadDebug(uint16_t addr);
        #endif

        void connect(ricoh2A03::CPU *c, ricoh2C02::PPU *p, ricoh2A03::APU *a);

        uint8_t controllerRead(uint8_t player);
        void controllerWrite(uint8_t player, uint8_t data);

        bool mapperIrqReq();
        void mapperIrqReset();
        void toggleCpuCycle();
        void ppuRequestDMA();
        void finalizeDMAreq();
        bool DMAactive();
        void handleDMA();
    
    private:
        uint8_t *cpuMemory = nullptr;   // modifiable cpu memory 0x0000 - 0x401F
        // uint8_t *ppuMemory = nullptr;   // PPU memory (64kB memory 0x0000 - 0xFFFF) (physically 16kB mirrored 0x0000 - 0x3FFF)
        uint8_t *ppuPalette;            // memory for loading raw palette data
        Mapper *mapper = nullptr;       // mapper for interface to CPU memory 0x4020 - 0xFFFF and PPU memory 0x0000-0x1FFF

        ricoh2A03::CPU *cpu = nullptr;
        ricoh2C02::PPU *ppu = nullptr;
        ricoh2A03::APU *apu = nullptr;

        uint8_t controllerBuffer1 = 0x00;   // buffered input for controller
        uint8_t controllerBuffer2 = 0x00;   // buffered input for controller

        // helper variables for DMA
        bool cpuOddCycle = false;       // just for the sake of DMA delay on odd cpu cycles
        bool reqDMA = false;            // ppu requesting DMA
        uint16_t DMAcycles = 0;         // 256 consecutive writes in a page (512 cpu cycles for reads and writes + 1 idle cycle + 1 idle cycle if beginning on odd cpu cycle)
    };
};

#endif

// NOTE: ppu memory for now is just filler

/*
// CPU MEMORY: (64kB addressable range) (2^16)
    // RAM: (0x0000 - 0x1FFF)
        // RAM addresses go  (0x0000 - 0x1FFF) (8kB); physically only 2kB (0x0000 - 0x07FF); mirrored 3 times
    // I/O Registers: (0x2000 - 0x401F) ("memory mapped I/O registers")
        // PPU: (0x2000 - 0x2007); mirrored every 8 bytes 0x2008 - 0x3FFF (remaining registers up to 0x401F follow this mirroring)
            (note: 0x2006 and 0x2007 are used to read from or write to PPU)
        // APU: (0x4000 - 0x4017) (0x401F?)
            // 0x4014 used by PPU
            // 0x4016 and 0x4017 used by controllers
    // CARTRIDGE: (0x4020 - 0xFFFF) (connected to PATTERN MEMORY through MAPPER) (maps parts of larger than address memory to bus memory?)
        // (0x4020 - 0x5FFF) EXPANSION ROM
        // (0x6000 - 0x7FFF) SRAM
        // (ONLY IF GAME HAS 1 OR 2 BANKS OF PRG-ROM AND NO MAPPING)
            // (0x8000 - 0xBFFF) PRG-ROM BANK 0
            // (0xC000 - 0xFFFF) PRG-ROM BANK 1
        // (ELSE)
            // (0x8000 - 0xFFFF) WHATEVER THE MAPPER DECIDES FOR REGISTERS/BANKS
        // (NOTE: MAPPER WILL CONTROL ENTIRE PORTION 0x4020 - 0xFFFF ANYWAYS)
// PPU MEMORY: (16kB addressable range) (2^14) (addressable to 64kB addressable range, but mirrors of [0x0000 - 0x3FFF] after 0x4000)
    // CHR ROM (PATTERN TABLES): (0x0000 - 0x1FFF) (8kB) (data from cartridge as well; also controlled by MAPPER) (SPRITES)
    // (8x8 sprite = 2 8-byte bitmatrices; LSB + MSB; 2 bits here + 2 bits from attribute table = color)
    // (1 tile / sprite = 16 bytes) (8 byte LSB + 8 byte MSB)
    // (1st sprite using 16 bytes 0x0000-0x000F)
    // (entire table is a 16x16 grid of tiles / sprites)
        // (0x0000 - 0x0FFF) TABLE 0
        // (0x1000 - 0x1FFF) TABLE 1
    // VIDEO RAM (NAME TABLES): (0x2000 - 0x2FFF) (2kB) (total: 0x2000 - 0x3EFF) (LAYOUT)
        // (32x30 bytes for name tables) (32x2 bytes for attribute tables)
        // (name table stores byte ID to sprite in CHR ROM) (2^8 = 256)
        // (each byte in attribute table manages which palette for each adjacent 4x4 grid in name table)
        // (4x4 grid of tiles =  4 blocks of 2x2 tile grids)
        // (each grid managed by single byte of attribute memory)
        // (attribute table byte = 33221100) (only 4 palettes; 2 bits for addressing each)
        // (0 top-left; 1 top-right; 2 bottom-left; 3 bottom-right)
        // (all tiles in 2x2 block share same palette)
        // (total of 64 2x2 blocks = attribute table size)
        // (0x2000 - 0x23BF) NAME TABLE 0
        // (0x23C0 - 0x23FF) ATTRIBUTE TABLE 0
        // (0x2400 - 0x27BF) NAME TABLE 1
        // (0x27C0 - 0x27FF) ATTRIBUTE TABLE 1
        // (0x2800 - 0x2BBF) NAME TABLE 2
        // (0x2BC0 - 0x2BFF) ATTRIBUTE TABLE 2
        // (0x2C00 - 0x2FBF) NAME TABLE 3
        // (0x2FC0 - 0x2FFF) ATTRIBUTE TABLE 3
        // (0x3000 - 0x3EFF) mirrors of (0x2000 - 0x2EFF)
    // PALETTES: (0x3F00 - 0x3FFF) (COLORS)
        // (0x3F00 - 0x3F0F) IMAGE PALETTE (colors for background tiles)
        // (0x3F10 - 0x3F1F) SPRITE PALETTE (colors for sprites)
        // (0x3F20 - 0x3FFF) MIRRORS
        // memory breakdown:
            // 0x3F00           Universal background color
            // 0x3F01-0x3F03    Background palette 0
            // 0x3F05-0x3F07    Background palette 1
            // 0x3F09-0x3F0B    Background palette 2
            // 0x3F0D-0x3F0F    Background palette 3
            // 0x3F11-0x3F13    Sprite palette 0
            // 0x3F15-0x3F17    Sprite palette 1
            // 0x3F19-0x3F1B    Sprite palette 2
            // 0x3F1D-0x3F1F    Sprite palette 3 
    // MIRRORS of 0x0000 - 0x3FFF (0x4000 - 0xFFFF)
    // OAM (not on bus; private to PPU)
*/