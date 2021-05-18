#include "../include/Ricoh2C02.hpp"
#include "../include/Memory.hpp"
#include <cstring>

ricoh2C02::PPU::PPU(NES::Memory *m) : mem(m)
{
    screenBuffer = new uint8_t[256 * 240 * 3]{0};   // 341 * 262 cycles though
    chr = new uint8_t[128 * 256 * 3]{0};            // delete this
    OAMprimary = new uint8_t[64 * 4]{0};
    OAMsecondary = new uint8_t[8 * 4]{0};
}

ricoh2C02::PPU::~PPU()
{
    delete[] screenBuffer;
    delete[] chr;           // delete this
    delete[] OAMprimary;
    delete[] OAMsecondary;
}

uint8_t* const ricoh2C02::PPU::getChrROM()      // delete this after debugging
{
    const uint8_t palette = 0x00;
    for (int y = 0; y < 16; y++)
    {
        for (int x = 0; x < 16; x++)
        {
            uint16_t byteOffset = ((y * 16) + x) * 16;
            for (uint16_t yy = 0; yy < 8; yy++)
            {
                uint8_t lsb = mem->ppuRead(byteOffset + yy);
                uint8_t msb = mem->ppuRead(byteOffset + yy + 8);
                for (uint16_t xx = 0; xx < 8; xx++)
                {
                    uint8_t color = (((msb & (0x80 >> xx)) >> (7 - xx)) << 1) | ((lsb & (0x80 >> xx)) >> (7 - xx));
                    ((RGB*)chr)[(((y * 8) + yy) * 128) + ((x * 8) + xx)] = paletteRGB[(mem->ppuRead(0x3F00 + (palette << 2) + color)) & 0x3F];
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
                uint8_t lsb = mem->ppuRead(0x1000 + byteOffset + yy);
                uint8_t msb = mem->ppuRead(0x1000 + byteOffset + yy + 8);
                for (uint16_t xx = 0; xx < 8; xx++)
                {
                    uint8_t color = (((msb & (0x80 >> xx)) >> (7 - xx)) << 1) | ((lsb & (0x80 >> xx)) >> (7 - xx));
                    ((RGB*)chr)[(((y * 8) + yy) * 128) + ((x * 8) + xx) + (16 * 16 * 64)] = paletteRGB[(mem->ppuRead(0x3F00 + (palette << 2) + color)) & 0x3F];
                }
            }
        }
    }
    return chr;
}

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
                vramAddrTemp = (vramAddrTemp & ~(VRAMmask::nametableID)) & ((uint16_t)(data & PPUCTRLmask::nametableIndex) << 10);  // set nametable from register
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
                vramAddrCurr += (registers[0] & PPUCTRLmask::vramIncrement)? 32 : 1;
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



bool ricoh2C02::PPU::DMAtransfer()
{
    if(DMAaddr < 256)
    {
        OAMprimary[DMAaddr] = mem->cpuRead((registers[8] << 8) | DMAaddr);
        DMAaddr++;
        return true;
    }
    return false;
}



// see https://wiki.nesdev.com/w/index.php/PPU_rendering
// and https://wiki.nesdev.com/w/images/d/d1/Ntsc_timing.png
// and https://wiki.nesdev.com/w/index.php/PPU_sprite_evaluation
void ricoh2C02::PPU::tick()
{
    // pixel rendering here
    uint8_t bgColorAddr = 0x00;   // note in real PPU address, there is +0x3F00
    uint8_t sprColorAddr = 0x00;
    int currSprInShifter = -1;
    if (registers[1] & PPUMASKmask::showBackground)
    {
        bgColorAddr = ((bgPalette1shifter & (0x80 >> fineX))? 0x08 : 0x00)
                    + ((bgPalette0shifter & (0x80 >> fineX))? 0x04 : 0x00)
                    + ((bgMSBshifter & (0x8000 >> fineX))? 0x02 : 0x00)
                    + ((bgLSBshifter & (0x8000 >> fineX))? 0x01 : 0x00);
    }
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
                    sprColorAddr = ((sprAttrLatch[i] & OAMmask::byte2PaletteID) << 2) + color;
                    currSprInShifter = i;
                    break;
                }
            }
        }
    }
    uint8_t colorAddr = 0x00;
    if (bgColorAddr & 0x03)
    {
        if (sprColorAddr & 0x03)
        {
            if (sprAttrLatch[currSprInShifter] & OAMmask::byte2Priority)
                colorAddr = bgColorAddr;
            else
                colorAddr = sprColorAddr;
            if (renderSprite0 && (currSprInShifter == 0) && (registers[1] & (PPUMASKmask::showBackground & PPUMASKmask::showSprites)))
            {
                // sprite 0 collision (https://wiki.nesdev.com/w/index.php?title=PPU_OAM&redirect=no#Sprite_zero_hits)
                bool leftmostTile = ((registers[1] & (PPUMASKmask::showLeftmostBackground | PPUMASKmask::showLeftmostSprite)) == (PPUMASKmask::showLeftmostBackground | PPUMASKmask::showLeftmostSprite));
                if ((screenX != 256) && (screenX >= ((leftmostTile)? 9 : 1)))
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



    // update registers and shifters for future clock ticks
    NMI = false;
    if ((screenY <= 239) || (screenY == 261))   // for visible scanlines
    {
        // this entire mess for background
        if (((screenX >= 1) && (screenX <= 256)) || ((screenX >= 321) && (screenX <= 336)))
        {
            // should prob move shifter updates to end of loop
            if (registers[1] & (PPUMASKmask::showBackground | PPUMASKmask::showSprites))
            {
                bgPalette1shifter = (bgPalette1shifter << 1) | ((bgPalette1Latch)? 0x01 : 0x00);
                bgPalette0shifter = (bgPalette0shifter << 1) | ((bgPalette0Latch)? 0x01 : 0x00);
                bgMSBshifter <<= 1;
                bgLSBshifter <<= 1;
            }
            switch ((screenX - 1) & 0x0007)     // preloading data for next tile / 8 pixels
            {
                case 0:
                    bgPalette1Latch = (bgNextTileAttr & 0x02)? true : false;
                    bgPalette0Latch = (bgNextTileAttr & 0x01)? true : false;
                    bgMSBshifter |= bgNextMSB;
                    bgLSBshifter |= bgNextLSB;
                    if ((screenY == 261) && (screenX == 1)) // clear vblank, sprite 0, and overflow flags
                        registers[2] &= ~(PPUSTATUSmask::sprite0HitFlag | PPUSTATUSmask::spriteOverflowFlag | PPUSTATUSmask::vblankFlag);
                    // get next tile sprite ID
                    bgNextTileID = mem->ppuRead(0x2000 | (vramAddrCurr & 0x0FFF));
                    break;
                case 2:
                    // AT byte
                    bgNextTileAttr = mem->ppuRead(0x23C0 | (vramAddrCurr & VRAMmask::nametableID)
                                                         | (((vramAddrCurr & VRAMmask::coarseY) >> 4) & 0x0038)
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
                case 7:
                    if (registers[1] & (PPUMASKmask::showBackground | PPUMASKmask::showSprites))
                    {
                        if (screenX != 256)
                        {
                            // increment v horizontally
                            if ((vramAddrCurr & VRAMmask::coarseX) == 31)
                            {
                                vramAddrCurr &= ~(VRAMmask::coarseX);
                                vramAddrCurr ^= VRAMmask::nametableID0;
                            }
                            else
                                vramAddrCurr++;
                        }
                        else
                        {
                            // increment v vertically
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
                    break;
                default:
                    break;
            }
        }
        else if (screenX == 257)
        {
            if (registers[1] & (PPUMASKmask::showBackground | PPUMASKmask::showSprites))
            {
                vramAddrCurr = (vramAddrCurr & ~(VRAMmask::coarseX | VRAMmask::nametableID0)) | (vramAddrTemp & (VRAMmask::coarseX | VRAMmask::nametableID0));
                bgPalette1Latch = (bgNextTileAttr & 0x02)? true : false;
                bgPalette0Latch = (bgNextTileAttr & 0x01)? true : false;
                bgMSBshifter |= bgNextMSB;
                bgLSBshifter |= bgNextLSB;
            }
        }
        else if ((screenX == 337) || (screenX == 339))
        {
            bgNextTileID = mem->ppuRead(0x2000 | (vramAddrCurr & 0x0FFF));
        }
        else if ((screenY == 261) && (screenX == 340) && oddFrame)      // odd frame skip
        {
            if (registers[1] & (PPUMASKmask::showBackground | PPUMASKmask::showSprites))
            {
                screenX = 0;
                screenY = 0;
                oddFrame = false;
            }
        }
        else if ((screenY == 261) && (screenX >= 280) && (screenX <= 304))
        {
            if (registers[1] & (PPUMASKmask::showBackground | PPUMASKmask::showSprites))
                vramAddrCurr = (vramAddrCurr & ~(VRAMmask::coarseY | VRAMmask::nametableID1 | VRAMmask::fineY)) | (vramAddrTemp & (VRAMmask::coarseY | VRAMmask::nametableID1 | VRAMmask::fineY));
        }

        // this entire mess for sprites for visible scanlines
        // (not cycle accurate, just coalesces all operations into single specific cycle of given cycle period)
        if (registers[1] & (PPUMASKmask::showBackground | PPUMASKmask::showSprites))
        {
            if (screenX <= 256)
            {
                for (int i = 0; i < sprToRender; i++)
                {
                    // std::cout << i << ':' << std::endl;
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
            {
                memset(OAMsecondary, 0xFF, 8 * 4 * sizeof(uint8_t));
                nxtSprToRender = 0;
                nxtRenderSprite0 = false;
            }
            else if (screenX == 65) // ((screenX >= 65) && (screenX <= 256))    // load secondary OAM
            {
                uint8_t n = 0, m = 0,
                        n2 = 0;
                while (n2 < 8)
                {
                    OAMsecondary[n2 * 4] = OAMprimary[n * 4];
                    int16_t diffY = (int16_t)(screenY) - (int16_t)(OAMprimary[n * 4]);
                    if ((diffY >= 0) && (diffY < ((registers[0] & PPUCTRLmask::spriteSize)? 16 : 8)))
                    {
                        OAMsecondary[(n2 * 4) + 1] = OAMprimary[(n * 4) + 1];
                        OAMsecondary[(n2 * 4) + 2] = OAMprimary[(n * 4) + 2];
                        OAMsecondary[(n2 * 4) + 3] = OAMprimary[(n * 4) + 3];
                        nxtSprToRender++;
                        if (n == 0)
                            nxtRenderSprite0 = true;
                        n2++;
                    }
                    n++;
                    if (n >= 64)
                        break;
                }
                while (n < 64)  // note how we increment m for the hardware bug for sprite overflow check
                {
                    int16_t diffY = (int16_t)(screenY) - (int16_t)(OAMprimary[(n * 4) + m]);
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
            else if (screenX >= 257)    // ((screenX >= 257) && (screenX <= 320))  // load data from secondary OAM into shifters?
            {
                sprToRender = nxtSprToRender;
                renderSprite0 = renderSprite0;
                for (uint8_t i = 0; i < nxtSprToRender; i++)
                {
                    uint16_t patTableAddr = 0x0000;
                    uint16_t tileID = 0x00;
                    uint8_t tileRow = screenY - OAMsecondary[i * 4];
                    if (registers[0] & PPUCTRLmask::spriteSize) // 8x16 sprite mode
                    {
                        patTableAddr = ((OAMsecondary[(i * 4) + 1] & OAMmask::byte1Bank)? 0x1000 : 0x0000);
                        tileID = ((OAMsecondary[(i * 4) + 1] & OAMmask::byte1TileID) >> 1);
                        if (OAMsecondary[(i * 4) + 2] & OAMmask::byte2FlipVert)     // flip sprite veritcally
                            tileRow = 15 - tileRow;
                        if (tileRow > 8)
                        {
                            tileRow -= 8;
                            tileID++;
                        }
                    }
                    else    // 8x8 sprite mode
                    {
                        patTableAddr = (registers[0] & PPUCTRLmask::spritePatternTable8x8Addr)? 0x1000 : 0x0000;
                        tileID = OAMsecondary[(i * 4) + 1];
                        if (OAMsecondary[(i * 4) + 2] & OAMmask::byte2FlipVert)     // flip sprite veritcally
                            tileRow = 7 - tileRow;
                    }
                    sprLSBshifter[i] = mem->ppuRead(patTableAddr + (tileID << 4) + tileRow);
                    sprMSBshifter[i] = mem->ppuRead(patTableAddr + (tileID << 4) + tileRow + 8);
                    if (OAMsecondary[(i * 4) + 2] & OAMmask::byte2FlipHoriz)        // flip sprite horizontally
                    {
                        uint8_t toFlip = sprLSBshifter[i];
                        toFlip = (toFlip & 0xF0) >> 4 | (toFlip & 0x0F) << 4;
                        toFlip = (toFlip & 0xCC) >> 2 | (toFlip & 0x33) << 2;
                        toFlip = (toFlip & 0xAA) >> 1 | (toFlip & 0x55) << 1;
                        sprLSBshifter[i] = toFlip;
                        toFlip = sprMSBshifter[i];
                        toFlip = (toFlip & 0xF0) >> 4 | (toFlip & 0x0F) << 4;
                        toFlip = (toFlip & 0xCC) >> 2 | (toFlip & 0x33) << 2;
                        toFlip = (toFlip & 0xAA) >> 1 | (toFlip & 0x55) << 1;
                        sprMSBshifter[i] = toFlip;
                    }
                    sprAttrLatch[i] = OAMsecondary[(i * 4) + 2];
                    sprPosX[i] = OAMsecondary[(i * 4) + 3];
                }
            }
            /*
            else if ((screenX >= 321) || (screenX == 0))
            {
                // TODO: dunno what to do here?
            }
            */
        }

        if ((screenX >= 257) && (screenX <= 320))   // OAMADDR is set to 0 during each of ticks 257-320 (the sprite tile loading interval) of the pre-render and visible scanlines
            registers[3] = 0x00;
    }
    else if ((screenY == 241) && (screenX == 1))
    {
        registers[2] |= PPUSTATUSmask::vblankFlag;
        if(registers[0] & PPUCTRLmask::vblankInterval)
            NMI = true;
    }



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