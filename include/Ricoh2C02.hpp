#ifndef _RICOH2C02
#define _RICOH2C02

#include <cstdint>

namespace NES
{
    class Memory;
};

namespace ricoh2C02
{
    enum PPUCTRLmask
    {
        nametableIndex =            ((uint8_t)(0x03)),
        vramIncrement =             ((uint8_t)(1) << 2),
        spritePatternTable8x8Addr = ((uint8_t)(1) << 3),
        backgroundPatternTable =    ((uint8_t)(1) << 4),
        spriteSize =                ((uint8_t)(1) << 5),
        ppuMasterSlave =            ((uint8_t)(1) << 6),    // unused
        vblankInterval =            ((uint8_t)(1) << 7)
    };

    enum PPUMASKmask
    {
        grayscale =                 (uint8_t)(1),
        showLeftmostBackground =    ((uint8_t)(1) << 1),
        showLeftmostSprite =        ((uint8_t)(1) << 2),
        showBackground =            ((uint8_t)(1) << 3),
        showSprites =               ((uint8_t)(1) << 4),
        emphasizeRed =              ((uint8_t)(1) << 5),
        emphasizeGreen =            ((uint8_t)(1) << 6),
        emphasizeBlue =             ((uint8_t)(1) << 7)
    };

    enum PPUSTATUSmask
    {
        spriteOverflowFlag =    ((uint8_t)(1) << 5),
        sprite0HitFlag =        ((uint8_t)(1) << 6),
        vblankFlag =            ((uint8_t)(1) << 7)
    };

    enum VRAMmask
    {
        coarseX = 0x001F,
        coarseY = 0x03E0,
        nametableID0 = 0x0400,
        nametableID1 = 0x0800,
        nametableID = 0x0C00,
        fineY = 0x7000
    };

    enum OAMmask
    {
        // byte 0 is y position
        byte1Bank =         0x01,
        byte1TileID =       0xFE,
        byte2PaletteID =    0x03,
        byte2Priority =     0x20,
        byte2FlipHoriz =    0x40,
        byte2FlipVert =     0x80
        // byte 3 is x position
    };

    class PPU
    {
        struct RGB
        {
            uint8_t R, G, B;
        };

    public:
        PPU(NES::Memory *m);
        ~PPU();

        void rst();

        uint8_t cpuRead(uint16_t addr);             // reading & writing to PPU registers alter PPU state
        bool cpuWrite(uint16_t addr, uint8_t data);
        #ifdef DEBUG
            uint8_t cpuReadDebug(uint16_t addr);        // reading & writing to PPU registers WITHOUT altering PPU state (debugging)
        #endif

        bool DMAtransfer(); // for DMA transfer

        void tick();
        bool frameComplete();
        uint8_t* const getScreen();

        bool triggerNMI();

        #ifdef DEBUG
            uint8_t* const getChrROM();
            uint8_t* const getOAM();
            uint8_t* const getNT();
        #endif

    private:
        NES::Memory *mem;

        uint8_t *screenBuffer;  // screen following SDL_PIXELFORMAT_RGB24; 256 x 240
        bool frameDone = false;

        // for triggering NMI on CPU for vblank
        bool NMI = false;

        // PPU registers and helper variables
        uint8_t registers[9] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};  // see Ricoh2C02.cpp for details
        uint16_t PPUCTRLpost30000 = 0x0000;                                             // counter for PPUCTRL writes as writes are ignored first 30000 cycles
        uint8_t PPUDATAbuffer = 0x00;                                                   // potential 2-cycle buffered read for PPUDATA

        // PPU pixel rendering loop position variables
        uint16_t screenX = 0, screenY = 261;    // 341 x 262 clock cycles
        bool oddFrame = false;                  // for potential pixel skip

        // PPU internal registers: https://wiki.nesdev.com/w/index.php/PPU_rendering
        // PPU scrolling internal register behavior: https://wiki.nesdev.com/w/index.php/PPU_scrolling

        // internal background registers
        ///*
        uint16_t vramAddrCurr = 0x0000;     // (v) current VRAM address
        uint16_t vramAddrTemp = 0x0000;     // (t) temporary VRAM address; can also be thought of as the address of the top left onscreen tile
        //*/
        uint8_t fineX = 0x00;               // (x) horizontal sprite-level scrolling (3 bits with values 0-7)
        bool writeToggle = false;           // (w) 2-byte write for PPUSCROLL and APPUADDR
        uint8_t bgPalette1shifter = 0x00, bgPalette0shifter = 0x00;
        uint16_t bgMSBshifter = 0x0000, bgLSBshifter = 0x0000;
        bool bgPalette1Latch = false,       
             bgPalette0Latch = false;
        uint8_t bgNextTileID = 0x00,
                bgNextTileAttr = 0x00,
                bgNextMSB = 0x00,
                bgNextLSB = 0x00;
        /*
        // registers v and t breakdown (see VRAMmask enums)
            // for v and t, note bits 12-14 are not used in reading from nametable and are ignored when doing so
            // however, bits 12-14 will be used for sprite offsets
            0yyy NN YYYYY XXXXX
             ||| || ||||| +++++-- coarse X scroll
             ||| || +++++-------- coarse Y scroll
             ||| ++-------------- nametable select (hold the base address of the nametable minus $2000)
             +++----------------- fine Y scroll (Y offset of a scanline within a tile) (aka sprite offsets)
        */

        // internal sprite registers and OAM
        uint8_t *OAMprimary;    // 64 entries of 4 bytes each
        uint8_t *OAMsecondary;  // 8 entries of 4 bytes each (literally a buffer for next scanline sprites to load into shifters after screenX = 256)
        uint8_t sprLSBshifter[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
        uint8_t sprMSBshifter[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
        uint8_t sprAttrLatch[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
        uint8_t sprPosX[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

        // sprite rendering helper variables
        uint8_t nxtSprToRender = 0;         // buffered value for below as we check this the scanline before
        uint8_t sprToRender = 0;            // number of sprites buffered to write in a scanline
        bool nxtRenderSprite0 = false;      // buffered value for below as we check this the scanline before
        bool renderSprite0 = false;         // to check for sprite 0 in secondary OAM

        // helper variables to calculate sprite address in CHR ROM when preloading shifters (literally just for cycles 257-320)
        uint8_t currSpriteinOAM2 = 0x00;
        uint16_t patTableAddr = 0x0000;
        uint16_t tileID = 0x00;
        uint8_t tileRow = 0x00;

        /*
        // Sprite representation (http://wiki.nesdev.com/w/index.php/PPU_OAM)
            // byte 0: Y position of top of sprite;
                       Sprite data is delayed by one scanline
                       You must subtract 1 from the sprite's Y coordinate before writing it here
                       Hide a sprite by writing any values in $EF-$FF here
                       Sprites are never displayed on the first line of the picture, and it is impossible to place a sprite partially off the top of the screen
            // byte 1: Tile index number
                       For 8x8 sprites, this is the tile number of this sprite within the pattern table selected in bit 3 of PPUCTRL ($2000)
                       For 8x16 sprites, the PPU ignores the pattern table selection and selects a pattern table from bit 0 of this number.
                       76543210
                       ||||||||
                       |||||||+- Bank ($0000 or $1000) of tiles
                       +++++++-- Tile number of top of sprite (0 to 254; bottom half gets the next tile)
            // byte 2: Attributes
                       76543210
                       ||||||||
                       ||||||++- Palette (4 to 7) of sprite
                       |||+++--- Unimplemented
                       ||+------ Priority (0: in front of background; 1: behind background)
                       |+------- Flip sprite horizontally
                       +-------- Flip sprite vertically
            // byte 3: X position of left side of sprite
                       X-scroll values of $F9-FF results in parts of the sprite to be past the right edge of the screen, thus invisible
                       It is not possible to have a sprite partially visible on the left edge
                       Instead, left-clipping through PPUMASK ($2001) can be used to simulate this effect
        */
        
        // DMA helper variables
        uint16_t DMAaddr = 0x0000;

        // represents "values" in PPU address range 0x3F00 - 0x3F1F
        // literally ripped from http://wiki.nesdev.com/w/index.php/PPU_palettes#2C02
        inline static const RGB paletteRGB[] = {
            {84, 84, 84},
            {0, 30, 116},
            {8, 16, 144},
            {48, 0, 136},
            {68, 0, 100},
            {92, 0, 48},
            {84, 4, 0},
            {60, 24, 0},
            {32, 42, 0},
            {8, 58, 0},
            {0, 64, 0},
            {0, 60, 0},
            {0, 50, 60},
            {0, 0, 0},
            {0, 0, 0},
            {0, 0, 0},

            {152, 150, 152},
            {8, 76, 196},
            {48, 50, 236},
            {92, 30, 228},
            {136, 20, 176},
            {160, 20, 100},
            {152, 34, 32},
            {120, 60, 0},
            {84, 90, 0},
            {40, 114, 0},
            {8, 124, 0},
            {0, 118, 40},
            {0, 102, 120},
            {0, 0, 0},
            {0, 0, 0},
            {0, 0, 0},

            {236, 238, 236},
            {76, 154, 236},
            {120, 124, 236},
            {176, 98, 236},
            {228, 84, 236},
            {236, 88, 180},
            {236, 106, 100},
            {212, 136, 32},
            {160, 170, 0},
            {116, 196, 0},
            {76, 208, 32},
            {56, 204, 108},
            {56, 180, 204},
            {60, 60, 60},
            {0, 0, 0},
            {0, 0, 0},

            {236, 238, 236},
            {168, 204, 236},
            {188, 188, 236},
            {212, 178, 236},
            {236, 174, 236},
            {236, 174, 212},
            {236, 180, 176},
            {228, 196, 144},
            {204, 210, 120},
            {180, 222, 120},
            {168, 226, 144},
            {152, 226, 180},
            {160, 214, 228},
            {160, 162, 160},
            {0, 0, 0},
            {0, 0, 0}
        };

        #ifdef DEBUG
            uint8_t *chr;
            uint8_t *oam;
            uint8_t *nt;
        #endif
    };
}

#endif

/*
// http://wiki.nesdev.com/w/index.php/PPU_registers
// http://nesdev.com/NESDoc.pdf (appendix B)
// PPU CONTROL REGISTERS (CPU MEMORY ADDRESSES 0x2000 - 0x2007 and 0x4014)
    // (0x2000) CONTROL REGISTER 1 (PPUCTRL)
    // (0x2001) CONTROL REGISTER 2 (PPU MASK)
    // (0x2002) STATUS REGISTER (PPUSTATUS)
    // (0x2003) SPR-RAM ADDRESS REGISTER (OAMADDR)
    // (0x2004) SPR-RAM I/O REGISTER (OAMADDR)
    // (0x2005) VRAM ADDRESS REGISTER 1 (PPUSCROLL)
    // (0x2006) VRAM ADDRESS REGISTER 2 (PPUADDR)
    // (0x2007) VRAM I/O REGISTER (PPUDATA)
    // (0x4014) DMA REGISTER (OAMDMA)
*/

/*
// MEMORY RECAP:
    // PATTERN MEMORY "CHR ROM" (PPU 0x0000 - 0x1FFF) (SPRITES)
        // (in total 2 16x16 grid of tiles [tile = 2 8x8 bitmaps])
        // (1 pattern table = 16 x 16 x 2 x 8 = 4096 bytes [0x1000])
        // (each 8x8 sprite is 2 bits, so 4 possible vales indexing color in palette memory)
    // NAMETABLE MEMORY "VRAM" (PPU 0x2000 - 0x3EFF) (LAYOUTS)
        // 32 x 32 byte array (0x0400)
        // each byte is an ID into pattern memory (16 x 16 = 256)
        // maximum theoretical resolution os (32 x 8)^2 = 256^2
        // visible on-screen resolution is 256 x 240 (last 2 rows not on-screen) (32 x 30)
        // example: name table 0 and attribute table 0
            // name table 0 at 0x2000 - 0x23BF (960 = 32 x 30)
            // attribute table 0 at 0x23C0 - 0x23FF (64 = 32 x 2)
        // need at least 2 name tables for scrolling for some games (other 2 are mirrors)
            // makes use of offsets
        // attribute table at bottom 2 rows determine which palette to use
    // PALETTE MEMORY (PPU 0x3F00 - 0x3FFF) (COLORS)
        // color = 1 byte
        // 0x3F00 is background color
        // 0x3F01 [palette index 0] and onwards has offset of 4 bytes
            // (ie 0x3F05 [palette index 1] all the way to 0x3F1D [palette index 7])
            // (0x3F01 - 0x3F1F [last bytes 0x3F20 is "unused"])
        // for each offset of 4 bytes, last bytes isn't used
        // all "unused" locations mirror background color
        // palette indices 0-3 make up image palette, 4-7 make up sprite palette
        // basically:
            // $3F00 	    Universal background color
            // $3F01-$3F03  Background palette 0
            // $3F05-$3F07  Background palette 1
            // $3F09-$3F0B  Background palette 2
            // $3F0D-$3F0F  Background palette 3
            // $3F11-$3F13  Sprite palette 0
            // $3F15-$3F17  Sprite palette 1
            // $3F19-$3F1B  Sprite palette 2
            // $3F1D-$3F1F  Sprite palette 3 
        // dereferencing color: (given palette ID [0-7] and pixel [0-4]) 0x3F00 + (ID x 4) + pixel
            // NOTE: pixel value of 0 or 4 means transparency (will always end up in unused location)
            43210
            |||||
            |||++- Pixel value from tile data
            |++--- Palette number from attribute table or OAM
            +----- Background/Sprite select
        // for our code, need to convert byte value to RGB
        // MIRRORS 0x3F20 - 0x3FFF
*/

/*
// BACKGROUND RENDERING EXAMPLE: (data follows this pipeline, but buffered in multiple steps)
    // READ BYTE FROM NAME TABLE FOR SPRITE ID
        // SPRITE_ID = (0x2000 + (0x0400 * NT_ID) + ((coarseY * 32) + coarseX))
    // READ BYTE FROM ATTRIBUTE TABLE FOR PALETTE ID, AND EXTRACT 2 BITS
        // PALETTE_ID = (0x23C0 + (0x0400 * NT_ID) + ((coarseY / 4) * 8) - (coarseX / 4))
        // IF (coarseY & 0x02), (PALETTE_ID >> 4)
        // IF (coarseX & 0x02), (PALETTE_ID >> 2)
    // GET COLOR FROM PALETTE AND SPRITE (2 BITS FOR 4 COLORS FROM SPRITE + 2 BITS FOR PALETTE)
        // (READ SPRITE COLOR LSB AND MSB) 
            // LSB = (0x1000 * (PPUCTRL & PPUCTRLmask::backgroundPatternTable)) + (SPRITE_ID << 4) + fineY
            // MSB = (0x1000 * (PPUCTRL & PPUCTRLmask::backgroundPatternTable)) + (SPRITE_ID << 4) + fineY + 8
        // (GET COLOR)
            // LSB_bit = LSB_byte & (0x01 << (7 - fineX))
            // MSB_bit = MSB_byte & (0x01 << (7 - fineX))
            // color = 0x3F00 + (PALETTE_ID << 2) + (MSB_bit << 1) + (LSB_bit)
        // (RENDER COLOR FOR SPECIFIC PIXEL)
*/
