#include "../include/Mapper.hpp"
#include "../include/Cartridge.hpp"
#include "../include/Memory.hpp"

NES::Mapper::Mapper(Cartridge *c) : mapperID(c->mapperID), cart(c)
{
    EXPROM = new uint8_t[0x5FFF - 0x4020 + 1]{0};
    SRAM = new uint8_t[0x7FFF - 0x6000 + 1]{0};
    if (cart->trainerPresent)
    {
        for(int i = 0; i < 512; i++)
            SRAM[0x7000 + i] = cart->trainer[i];    // load trainer data to 0x7000
    }
    NAMETABLE = new uint8_t[0x2FFF - 0x2000 + 1]{0};
}

NES::Mapper::~Mapper()
{
    delete cart;
    delete[] EXPROM;
    delete[] SRAM;
    delete[] NAMETABLE;
}



NES::Mapper* NES::createMapper(std::string filename)
{
    Cartridge *c = new Cartridge(filename);
    Mapper *m;
    if (c->inesFormat == 0)
        return nullptr;
    switch(c->mapperID)
    {
        case 1:
            m = new Mapper1(c);
            break;
        case 2:
            m = new Mapper2(c);
            break;
        case 3:
            m = new Mapper3(c);
            break;
        case 4:
            m = new Mapper4(c);
            break;
        default:
            m = new Mapper0(c);
            break;
    }
    return m;
}



NES::Mapper0::Mapper0(Cartridge *c) : Mapper(c)
{
    ntMirror = (cart->vertMirror)? mirror::vertical : mirror::horizontal;
}

NES::Mapper0::~Mapper0() {}

uint8_t NES::Mapper0::cpuRead(uint16_t addr)
{
    if (addr < 0x4020)
        return 0x00;
    else if (addr < 0x6000)
        return EXPROM[addr - 0x4020];
    else if (addr < 0x8000)
        return SRAM[addr - 0x6000];
    else
    {
        if (addr < 0xC000)
            return cart->prgROM[addr & 0x3FFF];
        else
            return cart->prgROM[((cart->nPrgROM - 1) * 0x4000) + (addr & 0x3FFF)];
    }
}

bool NES::Mapper0::cpuWrite(uint16_t addr, uint8_t data)
{
    if (addr < 0x4020)
        return false;
    else if (addr < 0x6000)
    {
        EXPROM[addr - 0x4020] = data;
        return true;
    }
    else if (addr < 0x8000)
    {
        SRAM[addr - 0x6000] = data;
        return true;
    }
    else
    {
        return false;
    }
}

uint8_t NES::Mapper0::ppuRead(uint16_t addr)
{
    if (addr <= 0x1FFF)
        return cart->chrROM[addr];
    else if (addr <= 0x3EFF)
    {
        addr &= 0x0FFF;
        if (ntMirror == mirror::horizontal)
        {
            if (addr <= 0x03FF)
                return NAMETABLE[addr];
            else if (addr <= 0x07FF)
                return NAMETABLE[addr & 0x03FF];
            else if (addr <= 0x0BFF)
                return NAMETABLE[addr];
            else
                return NAMETABLE[addr & 0x0BFF];
        }
        else if (ntMirror == mirror::vertical)
        {
            if (addr <= 0x07FF)
                return NAMETABLE[addr];             // 1st 2 nametables
            else
                return NAMETABLE[addr & 0x07FF];    // mirrors
        }
    }
    return 0x00;
}

bool NES::Mapper0::ppuWrite(uint16_t addr, uint8_t data)
{
    if ((addr >= 0x2000) && (addr <= 0x3EFF))
    {
        addr &= 0x0FFF;
        if (ntMirror == mirror::horizontal)
        {
            if (addr <= 0x03FF)
                NAMETABLE[addr] = data;
            else if (addr <= 0x07FF)
                NAMETABLE[addr & 0x03FF] = data;
            else if (addr <= 0x0BFF)
                NAMETABLE[addr] = data;
            else
                NAMETABLE[addr & 0x0BFF] = data;
            return true;
        }
        else if (ntMirror == mirror::vertical)
        {
            if (addr <= 0x07FF)
                NAMETABLE[addr] = data;             // 1st 2 nametables
            else
                NAMETABLE[addr & 0x07FF] = data;    // mirrors
            return true;
        }
    }
    return false;
}



NES::Mapper1::Mapper1(Cartridge *c) : Mapper(c) {}

NES::Mapper1::~Mapper1() {}

uint8_t NES::Mapper1::cpuRead(uint16_t addr)
{
    if (addr < 0x4020)
        return 0x00;
    else if (addr < 0x6000)
        return EXPROM[addr - 0x4020];
    else if (addr < 0x8000)
        return SRAM[addr - 0x6000];
    else    // mapper specific functionality
    {
        if (addr < 0xC000)
        {
            
        }
        else
        {

        }
        return 0x00;
    }
    
}

bool NES::Mapper1::cpuWrite(uint16_t addr, uint8_t data)
{
    return false;
}

uint8_t NES::Mapper1::ppuRead(uint16_t addr)
{
    return 0x00;
}

bool NES::Mapper1::ppuWrite(uint16_t addr, uint8_t data)
{
    return false;
}



NES::Mapper2::Mapper2(Cartridge *c) : Mapper(c)
{
    ntMirror = (cart->vertMirror)? mirror::vertical : mirror::horizontal;
}

NES::Mapper2::~Mapper2() {}

uint8_t NES::Mapper2::cpuRead(uint16_t addr)
{
    if (addr < 0x4020)
        return 0x00;
    else if (addr < 0x6000)
        return EXPROM[addr - 0x4020];
    else if (addr < 0x8000)
        return SRAM[addr - 0x6000];
    else    // mapper specific functionality
    {
        if (addr < 0xC000)
            return cart->prgROM[(regBankSelect * 0x4000) + (addr & 0x3FFF)];
        else
            return cart->prgROM[((cart->nPrgROM - 1) * 0x4000) + (addr & 0x3FFF)];
    }
}

bool NES::Mapper2::cpuWrite(uint16_t addr, uint8_t data)
{
    if (addr < 0x4020)
        return false;
    else if (addr < 0x6000)
    {
        EXPROM[addr - 0x4020] = data;
        return true;
    }
    else if (addr < 0x8000)
    {
        SRAM[addr - 0x6000] = data;
        return true;
    }
    else    // mapper specific functionality
    {
        regBankSelect = data & 0x0F;
        return true;
    }
}

uint8_t NES::Mapper2::ppuRead(uint16_t addr)
{
    if (addr <= 0x1FFF)
        return cart->chrROM[addr];
    else if (addr <= 0x3EFF)
    {
        addr &= 0x0FFF;
        if (ntMirror == mirror::horizontal)
        {
            if (addr <= 0x03FF)
                return NAMETABLE[addr];
            else if (addr <= 0x07FF)
                return NAMETABLE[addr & 0x03FF];
            else if (addr <= 0x0BFF)
                return NAMETABLE[addr];
            else
                return NAMETABLE[addr & 0x0BFF];
        }
        else if (ntMirror == mirror::vertical)
        {
            if (addr <= 0x07FF)
                return NAMETABLE[addr];             // 1st 2 nametables
            else
                return NAMETABLE[addr & 0x07FF];    // mirrors
        }
    }
    return 0x00;
}

bool NES::Mapper2::ppuWrite(uint16_t addr, uint8_t data)
{
    if ((addr >= 0x2000) && (addr <= 0x3EFF))
    {
        addr &= 0x0FFF;
        if (ntMirror == mirror::horizontal)
        {
            if (addr <= 0x03FF)
                NAMETABLE[addr] = data;
            else if (addr <= 0x07FF)
                NAMETABLE[addr & 0x03FF] = data;
            else if (addr <= 0x0BFF)
                NAMETABLE[addr] = data;
            else
                NAMETABLE[addr & 0x0BFF] = data;
            return true;
        }
        else if (ntMirror == mirror::vertical)
        {
            if (addr <= 0x07FF)
                NAMETABLE[addr] = data;             // 1st 2 nametables
            else
                NAMETABLE[addr & 0x07FF] = data;    // mirrors
            return true;
        }
    }
    return false;
}



NES::Mapper3::Mapper3(Cartridge *c) : Mapper(c)
{
    ntMirror = (cart->vertMirror)? mirror::vertical : mirror::horizontal;
}

NES::Mapper3::~Mapper3() {}

uint8_t NES::Mapper3::cpuRead(uint16_t addr)
{
    if (addr < 0x4020)
        return 0x00;
    else if (addr < 0x6000)
        return EXPROM[addr - 0x4020];
    else if (addr < 0x8000)
        return SRAM[addr - 0x6000];
    else
    {
        if (addr < 0xC000)
            return cart->prgROM[addr & 0x3FFF];
        else
            return cart->prgROM[((cart->nPrgROM - 1) * 0x4000) + (addr & 0x3FFF)];
    }
}

bool NES::Mapper3::cpuWrite(uint16_t addr, uint8_t data)
{
    if (addr < 0x4020)
        return false;
    else if (addr < 0x6000)
    {
        EXPROM[addr - 0x4020] = data;
        return true;
    }
    else if (addr < 0x8000)
    {
        SRAM[addr - 0x6000] = data;
        return true;
    }
    else    // mapper specific functionality
    {
        regBankSelect = data & 0x03;
        return true;
    }
}

uint8_t NES::Mapper3::ppuRead(uint16_t addr)
{
    if (addr <= 0x1FFF) // mapper specific functionality
        return cart->chrROM[(regBankSelect * 0x2000) + addr];
    else if (addr <= 0x3EFF)
    {
        addr &= 0x0FFF;
        if (ntMirror == mirror::horizontal)
        {
            if (addr <= 0x03FF)
                return NAMETABLE[addr];
            else if (addr <= 0x07FF)
                return NAMETABLE[addr & 0x03FF];
            else if (addr <= 0x0BFF)
                return NAMETABLE[addr];
            else
                return NAMETABLE[addr & 0x0BFF];
        }
        else if (ntMirror == mirror::vertical)
        {
            if (addr <= 0x07FF)
                return NAMETABLE[addr];             // 1st 2 nametables
            else
                return NAMETABLE[addr & 0x07FF];    // mirrors
        }
    }
    return 0x00;
}

bool NES::Mapper3::ppuWrite(uint16_t addr, uint8_t data)
{
    if ((addr >= 0x2000) && (addr <= 0x3EFF))
    {
        addr &= 0x0FFF;
        if (ntMirror == mirror::horizontal)
        {
            if (addr <= 0x03FF)
                NAMETABLE[addr] = data;
            else if (addr <= 0x07FF)
                NAMETABLE[addr & 0x03FF] = data;
            else if (addr <= 0x0BFF)
                NAMETABLE[addr] = data;
            else
                NAMETABLE[addr & 0x0BFF] = data;
            return true;
        }
        else if (ntMirror == mirror::vertical)
        {
            if (addr <= 0x07FF)
                NAMETABLE[addr] = data;             // 1st 2 nametables
            else
                NAMETABLE[addr & 0x07FF] = data;    // mirrors
            return true;
        }
    }
    return false;
}



NES::Mapper4::Mapper4(Cartridge *c) : Mapper(c), VRAM4screen(c->VRAM4screen) {}

NES::Mapper4::~Mapper4() {}

uint8_t NES::Mapper4::cpuRead(uint16_t addr)
{
    if (addr < 0x4020)
        return 0x00;
    else if (addr < 0x6000)
            return EXPROM[addr - 0x4020];
    else if (addr < 0x8000)
            return (prgRamEnable)? SRAM[addr - 0x6000] : 0x00;
    else    // mapper specific functionality
    {
        if (addr < 0xA000)
        {
            if (prgROMbankMode)
                return cart->prgROM[((cart->nPrgROM - 2) * 0x4000) + (addr & 0x1FFF)];
            else
                return cart->prgROM[(bankRegisters[6] * 0x4000) + (addr & 0x1FFF)];
        }
        else if (addr < 0xC000)
            return cart->prgROM[(bankRegisters[7] * 0x4000) + (addr & 0x1FFF)];
        else if (addr < 0xE000)
        {
            if (prgROMbankMode)
                return cart->prgROM[(bankRegisters[6] * 0x4000) + (addr & 0x1FFF)];
            else
                return cart->prgROM[((cart->nPrgROM - 2) * 0x4000) + (addr & 0x1FFF)];
        }
        else
            return cart->prgROM[((cart->nPrgROM - 1) * 0x4000) + (addr & 0x1FFF)];
    }
}

bool NES::Mapper4::cpuWrite(uint16_t addr, uint8_t data)
{
    if (addr < 0x4020)
        return false;
    else if (addr < 0x6000)
    {
        EXPROM[addr - 0x4020] = data;
        return true;
    }
    else if (addr < 0x8000)
    {
        if (prgRamEnable && prgRamWriteEnable)
        {
            SRAM[addr - 0x6000] = data;
            return true;
        }
        else
            return false;
    }
    else    // mapper specific functionality
    {
        if (addr < 0xA000)
        {
            if (addr & 0x0001)  // regBankData ($8001-$9FFF, odd)
            {
                if (updateRegister < 2)
                    bankRegisters[updateRegister] = (data & 0xFE);
                else
                    bankRegisters[updateRegister] = data;
                return true;
            }
            else                // regBankSelect ($8000-$9FFE, even)
            {
                updateRegister = data & 0x07;
                // data & 0x20 for MMC6
                prgROMbankMode = (data & 0x40)? true : false;
                chrROMinversion = (data & 0x80)? true : false;
                return true;
            }
        }
        else if (addr < 0xC000)
        {
            if (addr & 0x0001)  // regPrgRamProtect ($A001-$BFFF, odd)
            {
                // (data & 0x30) for MMC6
                prgRamWriteEnable = (data & 0x40)? false : true;
                prgRamEnable = (data & 0x80)? true : false;
            }
            else    // regMirroring ($A001-$BFFF, odd)
            {
                horizontalMirroring = (data & 0x01)? true : false;
                return true;
            }
        }
        else if (addr < 0xE000)
        {
            if (addr & 0x0001)  // regIRQreload ($C001-$DFFF, odd)
            {

            }
            else    // regIRQlatch ($C000-$DFFE, even)
            {
                regIRQlatch = data;
                return true;
            }
        }
        else
        {
            if (addr & 0x0001)  // regIRQenable ($E001-$FFFF, odd)
            {

            }
            else    // regIRQdisable ($E000-$FFFE, even)
            {
                
            }
        }
        return false;
    }
}

uint8_t NES::Mapper4::ppuRead(uint16_t addr)
{
    if (chrROMinversion)    // mapper specific functionality
    {
        if (addr < 0x0400)
            return cart->chrROM[(bankRegisters[2] * 0x0400) + (addr & 0x03FF)];
        else if (addr < 0x0800)
            return cart->chrROM[(bankRegisters[3] * 0x0400) + (addr & 0x03FF)];
        else if (addr < 0x0C00)
            return cart->chrROM[(bankRegisters[4] * 0x0400) + (addr & 0x03FF)];
        else if (addr < 0x1000)
            return cart->chrROM[(bankRegisters[5] * 0x0400) + (addr & 0x03FF)];
        else if (addr < 0x1800)
            return cart->chrROM[(bankRegisters[0] * 0x0400) + (addr & 0x07FF)];
        else
            return cart->chrROM[(bankRegisters[1] * 0x0400) + (addr & 0x07FF)];
    }
    else
    {
        if (addr < 0x0800)
            return cart->chrROM[(bankRegisters[0] * 0x0400) + (addr & 0x07FF)];
        else if (addr < 0x1000)
            return cart->chrROM[(bankRegisters[1] * 0x0400) + (addr & 0x07FF)];
        else if (addr < 0x1400)
            return cart->chrROM[(bankRegisters[2] * 0x0400) + (addr & 0x03FF)];
        else if (addr < 0x1800)
            return cart->chrROM[(bankRegisters[3] * 0x0400) + (addr & 0x03FF)];
        else if (addr < 0x1C00)
            return cart->chrROM[(bankRegisters[4] * 0x0400) + (addr & 0x03FF)];
        else
            return cart->chrROM[(bankRegisters[5] * 0x0400) + (addr & 0x03FF)];
    }
}

bool NES::Mapper4::ppuWrite(uint16_t addr, uint8_t data)
{
    return false;
}