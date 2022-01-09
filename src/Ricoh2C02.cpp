#include "../include/Ricoh2C02.hpp"
#include "../include/Memory.hpp"
#include <cstring>

#include <iostream>

ricoh2C02::PPU::PPU(NES::Memory *m) : mem(m)
{
    screenBuffer = new uint8_t[256 * 240 * 3]{0};   // 341 * 262 cycles though
    #ifdef DEBUG
        chr = new uint8_t[128 * 256 * 3]{0};
        oam = new uint8_t[64 * 128 * 3]{0};
        nt = new uint8_t[256 * 240 * 4 * 3]{0};
    #endif
    OAMprimary = new uint8_t[64 * 4]{0};
    OAMsecondary = new uint8_t[8 * 4]{0};
}

ricoh2C02::PPU::~PPU()
{
    delete[] screenBuffer;
    #ifdef DEBUG
        delete[] chr;
        delete[] oam;
        delete[] nt;
    #endif
    delete[] OAMprimary;
    delete[] OAMsecondary;
}

#ifdef DEBUG
    uint8_t* const ricoh2C02::PPU::getChrROM()
    {
        memset(chr, 0x00, 128 * 256 * 3 * sizeof(uint8_t));
        const uint8_t palette = 0x00;                       // placeholder assuming palette ID is 0
        for (int y = 0; y < 16; y++)
        {
            for (int x = 0; x < 16; x++)
            {
                uint16_t byteOffset = ((y * 16) + x) * 16;
                for (uint16_t yy = 0; yy < 8; yy++)
                {
                    uint8_t lsb = mem->ppuReadDebug(byteOffset + yy);
                    uint8_t msb = mem->ppuReadDebug(byteOffset + yy + 8);
                    for (uint16_t xx = 0; xx < 8; xx++)
                    {
                        uint8_t color = (((msb & (0x80 >> xx)) >> (7 - xx)) << 1) | ((lsb & (0x80 >> xx)) >> (7 - xx));
                        ((RGB*)chr)[(((y * 8) + yy) * 128) + ((x * 8) + xx)] = paletteRGB[(mem->ppuReadDebug(0x3F00 + (palette << 2) + color)) & 0x3F];
                    }
                }
            }
        }
        for (int y = 0; y < 16; y++)
        {
            for (int x = 0; x < 16; x++)
            {
                uint16_t byteOffset = ((y * 16) + x) * 16;
                for (uint16_t yy = 0; yy < 8; yy++)
                {
                    uint8_t lsb = mem->ppuReadDebug(0x1000 + byteOffset + yy);
                    uint8_t msb = mem->ppuReadDebug(0x1000 + byteOffset + yy + 8);
                    for (uint16_t xx = 0; xx < 8; xx++)
                    {
                        uint8_t color = (((msb & (0x80 >> xx)) >> (7 - xx)) << 1) | ((lsb & (0x80 >> xx)) >> (7 - xx));
                        ((RGB*)chr)[(((y * 8) + yy) * 128) + ((x * 8) + xx) + (16 * 16 * 64)] = paletteRGB[(mem->ppuReadDebug(0x3F00 + (palette << 2) + color)) & 0x3F];
                    }
                }
            }
        }
        return chr;
    }

    uint8_t* const ricoh2C02::PPU::getOAM()
    {
        memset(oam, 0x00, 64 * 128 * 3 * sizeof(uint8_t));
        for (int y = 0; y < 8; y++)
        {
            for (int x = 0; x < 8; x++)
            {
                uint8_t tilePosY = OAMprimary[(((y * 8) + x) * 4) + 0];
                uint8_t tileID = OAMprimary[(((y * 8) + x) * 4) + 1];
                uint8_t tileAttr = OAMprimary[(((y * 8) + x) * 4) + 2];
                uint8_t tilePosX = OAMprimary[(((y * 8) + x) * 4) + 3];
                uint8_t tilePatTable = (registers[0] & PPUCTRLmask::spritePatternTable8x8Addr)? 0x1000 : 0x0000;
                if (registers[0] & PPUCTRLmask::spriteSize)
                {
                    tilePatTable = ((byte1TileID & OAMmask::byte1Bank)? 0x1000 : 0x0000);
                    tileID = tileID & OAMmask::byte1TileID;
                }
                uint8_t height = (registers[0] & PPUCTRLmask::spriteSize)? 16 : 8;
                for (int yy = 0; yy < height; yy++)
                {
                    uint8_t lsb, msb;
                    if (tileAttr & OAMmask::byte2FlipVert)
                    {
                        lsb = mem->ppuReadDebug(tilePatTable + ((tileID + (((registers[0] & PPUCTRLmask::spriteSize) && (yy < 8))? 1 : 0)) << 4) + ((uint16_t)(height - yy - 1) & 0x0007));
                        msb = mem->ppuReadDebug(tilePatTable + ((tileID + (((registers[0] & PPUCTRLmask::spriteSize) && (yy < 8))? 1 : 0)) << 4) + ((uint16_t)(height - yy - 1) & 0x0007) + 8);
                    }
                    else
                    {
                        lsb = mem->ppuReadDebug(tilePatTable + ((tileID + (((registers[0] & PPUCTRLmask::spriteSize) && (yy >= 8))? 1 : 0)) << 4) + ((uint16_t)(yy) & 0x0007));
                        msb = mem->ppuReadDebug(tilePatTable + ((tileID + (((registers[0] & PPUCTRLmask::spriteSize) && (yy >= 8))? 1 : 0)) << 4) + ((uint16_t)(yy) & 0x0007) + 8);
                    }
                    if (tileAttr & OAMmask::byte2FlipHoriz)
                    {
                        uint8_t toFlip = lsb;
                        toFlip = (toFlip & 0xF0) >> 4 | (toFlip & 0x0F) << 4;
                        toFlip = (toFlip & 0xCC) >> 2 | (toFlip & 0x33) << 2;
                        toFlip = (toFlip & 0xAA) >> 1 | (toFlip & 0x55) << 1;
                        lsb = toFlip;
                        toFlip = msb;
                        toFlip = (toFlip & 0xF0) >> 4 | (toFlip & 0x0F) << 4;
                        toFlip = (toFlip & 0xCC) >> 2 | (toFlip & 0x33) << 2;
                        toFlip = (toFlip & 0xAA) >> 1 | (toFlip & 0x55) << 1;
                        msb = toFlip;
                    }
                    for (int xx = 0; xx < 8; xx++)
                    {
                        uint8_t color = (((msb & (0x80 >> xx)) >> (7 - xx)) << 1) | ((lsb & (0x80 >> xx)) >> (7 - xx));
                        ((RGB *)oam)[(((y * 16) + yy) * 64) + (x * 8) + xx] = paletteRGB[(mem->ppuReadDebug(0x3F00 + (((tileAttr & OAMmask::byte2PaletteID) + 0x04) << 2) + color)) & 0x3F];
                    }
                }
            }
        }
        return oam;
    }

    uint8_t* const ricoh2C02::PPU::getNT()
    {
        memset(nt, 0x00, 256 * 240 * 4 * 3 * sizeof(uint8_t));
        for (uint16_t ntNum = 0; ntNum < 4; ntNum++)
        {
            for (uint16_t y = 0; y < 240; y += 32)
            {
                for (uint16_t x = 0; x < 256; x += 32)
                {
                    uint8_t gridAttr = mem->ppuReadDebug(0x23C0 | (ntNum << 10)
                                                                | ((y >> 2) & 0x0038)   // >> 3 >> 2 << 3
                                                                | (x >> 5));            // >> 3 >> 2
                    for (uint16_t t = 0; t < 4; t++)
                    {
                        uint8_t currAttr = gridAttr;
                        if (t & 0x02)
                            currAttr >>= 4;
                        if (t & 0x01)
                            currAttr >>= 2;
                        currAttr &= 0x03;
                        for (uint16_t yy = 0; yy < 16; yy += 8)
                        {
                            if ((y + yy + ((t & 0x02)? 16 : 0)) >= 240)
                                break;
                            for (uint16_t xx = 0; xx < 16; xx += 8)
                            {
                                uint8_t tileID = mem->ppuReadDebug(0x2000 | (ntNum << 10)
                                                                          | (((y + yy + ((t & 0x02)? 16 : 0)) >> 3) & 0x0038)
                                                                          | ((x + xx + ((t & 0x01)? 16 : 0)) >> 3));
                                for (uint16_t yyy = 0; yyy < 8; yyy++)
                                {
                                    uint8_t LSB = mem->ppuReadDebug(((registers[0] & PPUCTRLmask::backgroundPatternTable)? 0x1000 : 0x0000)
                                                                    + (tileID << 4)
                                                                    + yyy);
                                    uint8_t MSB = mem->ppuReadDebug(((registers[0] & PPUCTRLmask::backgroundPatternTable)? 0x1000 : 0x0000)
                                                                    + (tileID << 4)
                                                                    + yyy
                                                                    + 8);
                                    for (uint16_t xxx = 0; xxx < 8; xxx++)
                                    {
                                        uint8_t color = ((MSB & (0x8000 >> xxx))? 0x02 : 0x00) + ((LSB & (0x8000 >> xxx))? 0x01 : 0x00);
                                        ((RGB *)nt)[((y + yy + ((t & 0x02)? 16 : 0) + yyy + ((ntNum & 0x02)? 240 : 0)) * 256 * 2) + (x + xx + ((t & 0x01)? 16 : 0) + xxx + ((ntNum & 0x01)? 256 : 0))] = paletteRGB[(mem->ppuReadDebug(0x3F00 + (currAttr << 2) + color)) & 0x3F];
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        return nt;
    }
#endif

void ricoh2C02::PPU::rst()
{
    frameDone = false;
    NMI = false;
    PPUCTRLpost30000 = 0x0000;
    PPUDATAbuffer = 0x00;
    screenX = 0;
    screenY = 261;
    oddFrame = false;
    vramAddrCurr = 0x0000;
    vramAddrTemp = 0x0000;
    fineX = 0x00;
    writeToggle = false;
    bgPalette1shifter = 0x00;
    bgPalette0shifter = 0x00;
    bgMSBshifter = 0x0000;
    bgLSBshifter = 0x0000;
    bgPalette1Latch = false;
    bgPalette0Latch = false;
    bgNextTileID = 0x00;
    bgNextTileAttr = 0x00;
    bgNextMSB = 0x00;
    bgNextLSB = 0x00;
}

uint8_t ricoh2C02::PPU::cpuRead(uint16_t addr)
{
    if ((addr & 0xFFF8) == 0x2000)
    {
        uint8_t data;
        switch (addr & 0x0007)
        {
            case 0:     // PPUCTRL (only write)
                break;
            case 1:     // PPUMASK (only write)
                break;
            case 2:     // PPUSTATUS (only read)
                writeToggle = false;
                data = registers[2];
                registers[2] &= ~(PPUSTATUSmask::vblankFlag);
                return data;
                break;
            case 3:     // OAMADDR (only write)
                break;
            case 4:     // OAMDATA (read and write)
                if (((screenY <= 239) || (screenY == 261)) && ((screenX >= 1) && (screenX <= 64)))
                    return 0xFF;
                return OAMprimary[registers[3]];
                break;
            case 5:     // PPUSCROLL (only write)
                break;
            case 6:     // PPUADDR (only write)
                break;
            case 7:     // PPUDATA (read and write)
                if (vramAddrCurr < 0x3F00)
                {
                    data = PPUDATAbuffer;
                    PPUDATAbuffer= mem->ppuRead(vramAddrCurr);
                }
                else
                {
                    PPUDATAbuffer= mem->ppuRead(vramAddrCurr);
                    data = PPUDATAbuffer;
                }
                vramAddrCurr = (vramAddrCurr + ((registers[0] & PPUCTRLmask::vramIncrement)? 32 : 1)) & 0x7FFF;
                return data;
                break;
            default:
                break;
        }
    }
    return 0x00;
}

bool ricoh2C02::PPU::cpuWrite(uint16_t addr, uint8_t data)
{
    if ((addr & 0xFFF8) == 0x2000)
    {
        switch (addr & 0x0007)
        {
            case 0:     // PPUCTRL (only write)
                if (PPUCTRLpost30000 < 30000)
                    return false;
                registers[0] = data;
                vramAddrTemp = (vramAddrTemp & ~(VRAMmask::nametableID)) | ((uint16_t)(data & PPUCTRLmask::nametableIndex) << 10);  // set nametable from register
                return true;
                break;
            case 1:     // PPUMASK (only write)
                registers[1] = data;
                return true;
                break;
            case 2:     // PPUSTATUS (only read)
                break;
            case 3:     // OAMADDR (only write)
                registers[3] = data;
                break;
            case 4:     // OAMDATA (read and write)
                OAMprimary[registers[3]] = data;
                registers[3]++;
                break;
            case 5:     // PPUSCROLL (only write)
                if (!writeToggle)
                {
                    vramAddrTemp = (vramAddrTemp & ~(VRAMmask::coarseX)) | ((data & 0xF8) >> 3);
                    fineX = (data & 0x07);
                    writeToggle = true;
                }
                else
                {
                    vramAddrTemp = (vramAddrTemp & ~(VRAMmask::coarseY | VRAMmask::fineY)) | ((uint16_t)(data & 0xF8) << 2) | ((uint16_t)(data & 0x07) << 12);
                    writeToggle = false;
                }
                return true;
                break;
            case 6:     // PPUADDR (only write)
                if (!writeToggle)
                {
                    vramAddrTemp = (vramAddrTemp & 0x00FF) | ((uint16_t)(data & 0x3F) << 8);
                    writeToggle = true;
                }
                else
                {
                    vramAddrTemp = (vramAddrTemp & 0x7F00) | data;
                    vramAddrCurr = vramAddrTemp;
                    writeToggle = false;
                }
                return true;
                break;
            case 7:     // PPUDATA (read and write)
                mem->ppuWrite(vramAddrCurr, data);
                vramAddrCurr = (vramAddrCurr + ((registers[0] & PPUCTRLmask::vramIncrement)? 32 : 1)) & 0x7FFF;
                return true;
                break;
            default:
                break;
        }
        return false;
    }
    else if (addr == 0x4014)    // OAMDMA (only write)
    {
        registers[8] = data;
        DMAaddr = 0x0000;
        mem->ppuRequestDMA();
        return true;
    }
    return false;
}

#ifdef DEBUG
    uint8_t ricoh2C02::PPU::cpuReadDebug(uint16_t addr)
    {
        if ((addr & 0xFFF8) == 0x2000)
            return registers[addr & 0x0007];
        else if (addr == 0x4014)
            return registers[8];
        else
            return 0x00;
    }
#endif



bool ricoh2C02::PPU::DMAtransfer()
{
    if (DMAaddr < 256)
    {
        // OAMprimary[DMAaddr] = mem->cpuRead((uint16_t)(registers[8] << 8) | DMAaddr);
        cpuWrite(0x2004, mem->cpuRead((uint16_t)(registers[8] << 8) | DMAaddr));
        DMAaddr++;
        return true;
    }
    return false;
}



// see https://wiki.nesdev.com/w/index.php/PPU_rendering
// and https://wiki.nesdev.com/w/images/d/d1/Ntsc_timing.png
// and https://wiki.nesdev.com/w/index.php/PPU_sprite_evaluation
// note: cycle timing for ppu reads and vram addr increments are 1 cycle faster to mimic address line being set before actual read happens
// (dunno if above is 100% true, but done just to get mapper 4 to work)
void ricoh2C02::PPU::tick()
{
    // -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
    // background here
    // -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
    uint8_t bgColorAddr = 0x00;
    if (registers[1] & PPUMASKmask::showBackground)
    {
        bgColorAddr = ((bgPalette1shifter & (0x80 >> fineX))? 0x08 : 0x00)
                    + ((bgPalette0shifter & (0x80 >> fineX))? 0x04 : 0x00)
                    + ((bgMSBshifter & (0x8000 >> fineX))? 0x02 : 0x00)
                    + ((bgLSBshifter & (0x8000 >> fineX))? 0x01 : 0x00);
    }
    if ((screenY <= 239) || (screenY == 261))   // for visible scanlines
    {
        if (((screenX >= 0) && (screenX <= 256)) || ((screenX >= 320) && (screenX <= 336)))
        {
            if ((screenX) && (screenX != 320))
            {
                bgPalette1shifter = (bgPalette1shifter << 1) | ((bgPalette1Latch)? 0x01 : 0x00);
                bgPalette0shifter = (bgPalette0shifter << 1) | ((bgPalette0Latch)? 0x01 : 0x00);
                bgMSBshifter <<= 1;
                bgLSBshifter <<= 1;
            }
            switch (screenX & 0x0007)       // preloading data for next tile / 8 pixels
            {                               // note each ppuRead case is 1 cycle early as address line seems to be set before actual read occurs?
                case 0:
                    // get next tile sprite ID
                    if ((screenX != 256) || (screenX != 320))
                        bgNextTileID = mem->ppuRead(0x2000 | (vramAddrCurr & 0x0FFF));
                    break;
                case 1:
                    if ((screenY == 0) && oddFrame && (screenX == 1))
                        bgNextTileID = mem->ppuRead(0x2000 | (vramAddrCurr & 0x0FFF));
                    bgPalette1Latch = (bgNextTileAttr & 0x02)? true : false;
                    bgPalette0Latch = (bgNextTileAttr & 0x01)? true : false;
                    bgMSBshifter |= bgNextMSB;
                    bgLSBshifter |= bgNextLSB;
                    if ((screenY == 261) && (screenX == 1)) // clear vblank, sprite 0, and overflow flags
                        registers[2] &= ~(PPUSTATUSmask::sprite0HitFlag | PPUSTATUSmask::spriteOverflowFlag | PPUSTATUSmask::vblankFlag);
                    break;
                case 2:
                    // AT byte
                    bgNextTileAttr = mem->ppuRead(0x23C0 | (vramAddrCurr & VRAMmask::nametableID)
                                                         | (((vramAddrCurr & VRAMmask::coarseY) >> 4) & 0x0038)     // >> 5 >> 2 << 3
                                                         | ((vramAddrCurr & VRAMmask::coarseX) >> 2));
                    if (vramAddrCurr & VRAMmask::coarseY & 0x0040)
                        bgNextTileAttr >>= 4;
                    if (vramAddrCurr & VRAMmask::coarseX & 0x0002)
                        bgNextTileAttr >>= 2;
                    break;
                case 4:
                    // get next tile LSB
                    bgNextLSB = mem->ppuRead(((registers[0] & PPUCTRLmask::backgroundPatternTable)? 0x1000 : 0x0000)
                                             + (bgNextTileID << 4)
                                             + ((vramAddrCurr & VRAMmask::fineY) >> 12));
                    break;
                case 6:
                    // get next tile MSB
                    bgNextMSB = mem->ppuRead(((registers[0] & PPUCTRLmask::backgroundPatternTable)? 0x1000 : 0x0000)
                                             + (bgNextTileID << 4)
                                             + ((vramAddrCurr & VRAMmask::fineY) >> 12)
                                             + 8);
                    break;
                default:
                    break;
            }
        }
        else if (screenX == 257)
        {
            bgPalette1Latch = (bgNextTileAttr & 0x02)? true : false;
            bgPalette0Latch = (bgNextTileAttr & 0x01)? true : false;
            bgMSBshifter |= bgNextMSB;
            bgLSBshifter |= bgNextLSB;
        }
        else if ((screenX == 338))  // || (screenX == 340))
        {
            bgNextTileID = mem->ppuRead(0x2000 | (vramAddrCurr & 0x0FFF));
        }
    }

    // -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
    // sprites here
    // -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
    uint8_t sprColorAddr = 0x00;
    int currSprInShifter = -1;
    if (registers[1] & PPUMASKmask::showSprites)
    {
        for (uint8_t i = 0; i < sprToRender; i++)
        {
            if (!(sprPosX[i]))
            {
                uint8_t color = ((sprMSBshifter[i] & 0x80)? 0x02 : 0x00)
                              + ((sprLSBshifter[i] & 0x80)? 0x01 : 0x00);
                if (color)
                {
                    sprColorAddr = (((sprAttrLatch[i] & OAMmask::byte2PaletteID) + 0x04) << 2) + color;
                    currSprInShifter = i;
                    break;
                }
            }
        }
    }
    if ((screenY <= 239) || (screenY == 261))   // for visible scanlines
    {
        if ((registers[1] & PPUMASKmask::showSprites) && screenX && (screenX <= 257))
        {
            for (int i = 0; i < sprToRender; i++)
            {
                if (sprPosX[i] > 0)
                    sprPosX[i]--;
                else
                {
                    sprMSBshifter[i] <<= 1;
                    sprLSBshifter[i] <<= 1;
                }
            }
        }
        if (screenX == 1)       // ((screenX >= 1) && (screenX <= 64))      // clear secondary OAM
        {                       // (coalesced since everything is internal)
            memset(OAMsecondary, 0xFF, 8 * 4 * sizeof(uint8_t));
            nxtSprToRender = 0;
            nxtRenderSprite0 = false;
        }
        else if (screenX == 65) // ((screenX >= 65) && (screenX <= 256))    // load secondary OAM
        {                       // 192 ppu clock cycles (3 cycles per entry in primary OAM) (coalesced since everything is internal)
            uint16_t OAMoffset = registers[3];
            uint8_t n = 0, m = 0, n2 = 0;
            while ((n2 < 8) && ((OAMoffset + (n * 4) + 3) < 256))
            {
                OAMsecondary[n2 * 4] = OAMprimary[OAMoffset + (n * 4)];
                int16_t diffY = (int16_t)((screenY == 261)? 0 : (screenY + 1)) - (int16_t)(OAMprimary[n * 4]);
                if ((diffY >= 0) && (diffY < ((registers[0] & PPUCTRLmask::spriteSize)? 16 : 8)))
                {
                    OAMsecondary[(n2 * 4) + 1] = OAMprimary[OAMoffset + (n * 4) + 1];
                    OAMsecondary[(n2 * 4) + 2] = OAMprimary[OAMoffset + (n * 4) + 2];
                    OAMsecondary[(n2 * 4) + 3] = OAMprimary[OAMoffset + (n * 4) + 3];
                    nxtSprToRender++;
                    if (n == 0)
                        nxtRenderSprite0 = true;
                    n2++;
                }
                n++;
            }
            while ((OAMoffset + (n * 4) + m) < 256)     // note how we increment m for the hardware bug for sprite overflow check
            {
                int16_t diffY = (int16_t)((screenY == 261)? 0 : (screenY + 1)) - (int16_t)(OAMprimary[OAMoffset + (n * 4) + m]);
                m = (m + 1) & 0x03;
                if ((diffY >= 0) && (diffY < ((registers[0] & PPUCTRLmask::spriteSize)? 16 : 8)))
                {
                    registers[2] |= PPUSTATUSmask::spriteOverflowFlag;
                    m += 3;
                    if (m >= 4)
                    {
                        n++;
                        m &= 0x03;
                    }
                }
                else
                {
                    n++;
                    m = (m + 1) & 0x03;
                }
            }
        }
        else if ((screenX >= 257) && (screenX <= 320))  // load data from secondary OAM into shifters?
        {                                               // 64 ppu clock cycles (8 cycles per entry in secondary OAM) (not completely accurate; just assumed ppuRead occurs every 4 ppu cycles)
            if (screenX == 257)
            {
                sprToRender = nxtSprToRender;
                renderSprite0 = nxtRenderSprite0;
                currSpriteinOAM2 = 0;
            }
            if (currSpriteinOAM2 < nxtSprToRender)
            {
                switch (screenX & 0x0007)
                {
                    case 1:
                        tileRow = (uint8_t)((screenY == 261)? 0 : (screenY + 1)) - OAMsecondary[currSpriteinOAM2 * 4];
                        if (registers[0] & PPUCTRLmask::spriteSize) // 8x16 sprite mode
                        {
                            patTableAddr = ((OAMsecondary[(currSpriteinOAM2 * 4) + 1] & OAMmask::byte1Bank)? 0x1000 : 0x0000);
                            tileID = OAMsecondary[(currSpriteinOAM2 * 4) + 1] & OAMmask::byte1TileID;
                            if (OAMsecondary[(currSpriteinOAM2 * 4) + 2] & OAMmask::byte2FlipVert)     // flip sprite veritcally
                                tileRow = 15 - tileRow;
                            if (tileRow >= 8)
                            {
                                tileRow -= 8;
                                tileID++;
                            }
                        }
                        else    // 8x8 sprite mode
                        {
                            patTableAddr = (registers[0] & PPUCTRLmask::spritePatternTable8x8Addr)? 0x1000 : 0x0000;
                            tileID = OAMsecondary[(currSpriteinOAM2 * 4) + 1];
                            if (OAMsecondary[(currSpriteinOAM2 * 4) + 2] & OAMmask::byte2FlipVert)      // flip sprite vertically
                                tileRow = 7 - tileRow;
                        }
                        break;
                    case 4:
                        sprLSBshifter[currSpriteinOAM2] = mem->ppuRead(patTableAddr + (tileID << 4) + tileRow);
                        break;
                    case 0:
                        sprMSBshifter[currSpriteinOAM2] = mem->ppuRead(patTableAddr + (tileID << 4) + tileRow + 8);
                        if (OAMsecondary[(currSpriteinOAM2 * 4) + 2] & OAMmask::byte2FlipHoriz)         // flip sprite horizontally
                        {
                            uint8_t toFlip = sprLSBshifter[currSpriteinOAM2];
                            toFlip = (toFlip & 0xF0) >> 4 | (toFlip & 0x0F) << 4;
                            toFlip = (toFlip & 0xCC) >> 2 | (toFlip & 0x33) << 2;
                            toFlip = (toFlip & 0xAA) >> 1 | (toFlip & 0x55) << 1;
                            sprLSBshifter[currSpriteinOAM2] = toFlip;
                            toFlip = sprMSBshifter[currSpriteinOAM2];
                            toFlip = (toFlip & 0xF0) >> 4 | (toFlip & 0x0F) << 4;
                            toFlip = (toFlip & 0xCC) >> 2 | (toFlip & 0x33) << 2;
                            toFlip = (toFlip & 0xAA) >> 1 | (toFlip & 0x55) << 1;
                            sprMSBshifter[currSpriteinOAM2] = toFlip;
                        }
                        sprAttrLatch[currSpriteinOAM2] = OAMsecondary[(currSpriteinOAM2 * 4) + 2];
                        sprPosX[currSpriteinOAM2] = OAMsecondary[(currSpriteinOAM2 * 4) + 3];
                        currSpriteinOAM2++;
                        break;
                    default:
                        break;
                }
            }
            else    // edge case: is there is no sprite to load, scanline counting for Mapper 4 fails
            {       // ("dirty" trick to get Mapper 4 to work) (address line A12 actively in use even when no sprites are to be rendered next frame) (so, we use dummy writes to mimic this)
                switch(screenX & 0x0007)
                {
                    case 0:
                    case 4:
                        if (registers[0] & PPUCTRLmask::spriteSize) // 8x8 sprite mode
                            mem->ppuRead(0x1FF0);   // see "https://wiki.nesdev.org/w/index.php?title=MMC3#IRQ_Specifics"? (not 100% correct. but close enough)
                        else
                            mem->ppuRead((registers[0] & PPUCTRLmask::spritePatternTable8x8Addr)? 0x1000 : 0x0000);
                        break;
                    default:
                        break;
                }
            }
        }
    }

    // -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
    // pixel rendering here
    // -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
    uint8_t colorAddr = 0x00;
    if (bgColorAddr & 0x03)
    {
        if (sprColorAddr & 0x03)
        {
            if (sprAttrLatch[currSprInShifter] & OAMmask::byte2Priority)
                colorAddr = bgColorAddr;
            else
                colorAddr = sprColorAddr;
            bool renderBackgroundAndSprites = ((registers[1] & (PPUMASKmask::showBackground | PPUMASKmask::showSprites)) == (PPUMASKmask::showBackground | PPUMASKmask::showSprites));
            if (renderSprite0 && (currSprInShifter == 0) && renderBackgroundAndSprites)
            {
                // sprite 0 collision (https://wiki.nesdev.com/w/index.php?title=PPU_OAM&redirect=no#Sprite_zero_hits)
                uint8_t leftmostTile = (registers[1] & (PPUMASKmask::showLeftmostBackground | PPUMASKmask::showLeftmostSprite));
                if ((screenX != 256) && (screenX >= ((leftmostTile)? 1 : 9)))
                    registers[2] |= PPUSTATUSmask::sprite0HitFlag;
            }
        }
        else
            colorAddr = bgColorAddr;
    }
    else if (sprColorAddr & 0x03)
        colorAddr = sprColorAddr;

    if ((screenY < 240) && (screenX <= 256) && (screenX > 0)) // draw pixel using screenX, screenY, and palette (calculated from above conditional)
    {
        uint8_t paletteData = (mem->ppuRead(0x3F00 + colorAddr)) & 0x3F;
        if (registers[1] & PPUMASKmask::grayscale)
            paletteData &= 0x30;
        ((RGB*)screenBuffer)[(screenY * 256) + screenX - 1] = paletteRGB[paletteData];
    }

    // -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
    // other PPU clock cycle functions here
    // -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
    if ((screenY <= 239) || (screenY == 261))   // for visible scanlines
    {
        if (registers[1] & (PPUMASKmask::showBackground | PPUMASKmask::showSprites))
        {
            if ((((screenX >= 1) && (screenX <= 256)) || ((screenX >= 321) && (screenX <= 336))) && ((screenX & 0x07) == 0x07))   // && !(screenX & 0x07))   // update VRAM address
            {
                if (screenX != 255)     // increment v horizontally (256)
                {
                    if ((vramAddrCurr & VRAMmask::coarseX) == 31)
                    {
                        vramAddrCurr &= ~(VRAMmask::coarseX);
                        vramAddrCurr ^= VRAMmask::nametableID0;
                    }
                    else
                        vramAddrCurr++;
                }
                else                    // increment v vertically
                {
                    vramAddrCurr = (vramAddrCurr + 0x1000) & 0x7FFF;
                    if (!(vramAddrCurr & VRAMmask::fineY))
                    {
                        uint8_t coarseY = (vramAddrCurr & VRAMmask::coarseY) >> 5;
                        if (coarseY == 29)
                        {
                            vramAddrCurr &= ~(VRAMmask::coarseY);
                            vramAddrCurr ^= VRAMmask::nametableID1;
                        }
                        else if (coarseY == 31)
                        {
                            vramAddrCurr &= ~(VRAMmask::coarseY);
                        }
                        else
                            vramAddrCurr += 0x0020; // increment coarseY
                    }       
                }
            }
            else if (screenX == 257)
                vramAddrCurr = (vramAddrCurr & ~(VRAMmask::coarseX | VRAMmask::nametableID0)) | (vramAddrTemp & (VRAMmask::coarseX | VRAMmask::nametableID0));
            else if ((screenY == 261) && (screenX >= 280) && (screenX <= 304))
                vramAddrCurr = (vramAddrCurr & ~(VRAMmask::coarseY | VRAMmask::nametableID1 | VRAMmask::fineY)) | (vramAddrTemp & (VRAMmask::coarseY | VRAMmask::nametableID1 | VRAMmask::fineY));
        }

        if ((screenX >= 257) && (screenX <= 320))   // OAMADDR is set to 0 during each of ticks 257-320 (the sprite tile loading interval) of the pre-render and visible scanlines
            registers[3] = 0x00;

    }
    NMI = false;
    if ((screenY == 241) && (screenX == 1))
    {
        registers[2] |= PPUSTATUSmask::vblankFlag;
        if(registers[0] & PPUCTRLmask::vblankInterval)
            NMI = true;
    }

    // -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
    // clock cycle update here
    // -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
    screenX++;
    if (screenX > 340)
    {
        screenX = 0;
        screenY++;
        if (screenY > 261)
            screenY = 0;
    }
    frameDone = false;
    if ((screenY == 239) && (screenX == 256))
    {
        oddFrame = !oddFrame;
        frameDone = true;
    }
    else if ((screenY == 0) && (screenX == 0) && oddFrame && (registers[1] & (PPUMASKmask::showBackground | PPUMASKmask::showSprites))) // odd frame skip
    {
        screenX = 1;
    }
    if (PPUCTRLpost30000 < 30000)
        PPUCTRLpost30000++;
}

bool ricoh2C02::PPU::frameComplete()
{
    return frameDone;
}

uint8_t* const ricoh2C02::PPU::getScreen()
{
    return screenBuffer;
}

bool ricoh2C02::PPU::triggerNMI()
{
    return NMI;
}