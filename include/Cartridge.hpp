#ifndef _CARTRIDGE
#define _CARTRIDGE

#include <cstdint>
#include <string>

namespace NES
{
    struct Cartridge
    {
        Cartridge(std::string filename);
        ~Cartridge();

        struct inesHeader   // https://wiki.nesdev.com/w/index.php/INES
        {
            char name[4] = {0x00, 0x00, 0x00, 0x00};            // Constant $4E $45 $53 $1A ("NES" followed by MS-DOS end-of-file)
            uint8_t nPrgROM = 0;                                // Size of PRG ROM in 16 KB units
            uint8_t nChrROM = 0;                                // Size of CHR ROM in 8 KB units (Value 0 means the board uses CHR RAM)
            uint8_t flags6 = 0;                                 // Flags 6 - Mapper, mirroring, battery, trainer
            uint8_t flags7 = 0;                                 // Flags 7 - Mapper, VS/Playchoice, NES 2.0
            uint8_t flags8 = 0;                                 // Flags 8 - PRG-RAM size (rarely used extension)
            uint8_t flags9 = 0;                                 // Flags 9 - TV system (rarely used extension)
            uint8_t flags10 = 0;                                // Flags 10 - TV system, PRG-RAM presence (unofficial, rarely used extension)
            char unused[5] = {0x00, 0x00, 0x00, 0x00, 0x00};     // Unused padding (should be filled with zero, but some rippers put their name across bytes 7-15)
        } header;

        uint8_t inesFormat = 0;         // iNes format (only supporting iNes and iNes 2.0)
        uint8_t mapperID = 0;
        bool trainerPresent = false;    // true if Trainer Area is present
        uint16_t nPrgROM = 0;           // equal to header value if format 1.0, else see code for format 2.0
        uint16_t nChrROM = 0;           // equal to header value if format 1.0, else see code for format 2.0
        bool VRAM4screen;               // if true, ignore mirroring control or above mirroring bit; instead provide four-screen VRAM
        bool vertMirror;                // false for horizontal, true for vertical

        uint8_t *trainer = nullptr;     // Trainer Area (loaded into memory 0x7000) (DEPRECATED; UNUSED)
        uint8_t *prgROM = nullptr;      // physical program rom
        uint8_t *chrROM = nullptr;      // physical memory where pattern tables are located
    };
}

#endif