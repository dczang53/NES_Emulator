#include "../include/Mapper.hpp"
#include "../include/Cartridge.hpp"
#include "../include/Memory.hpp"
#include "../include/Ricoh2A03.hpp"

#include <iostream>

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



NES::Mapper* NES::createMapper(std::string filename, ricoh2A03::CPU *cpu)
{
    Cartridge *c = new Cartridge(filename);
    Mapper *m;
    if (c->inesFormat == 0)
        return nullptr;
    std::cout << "Cartridge Mapper ID: " << (int)(c->mapperID) << std::endl << "Generating ";
    switch(c->mapperID)
    {
        case 1:
            m = new Mapper1(c);
            std::cout << "Mapper 1" << std::endl;
            break;
        case 2:
            m = new Mapper2(c);
            std::cout << "Mapper 2" << std::endl;
            break;
        case 3:
            m = new Mapper3(c);
            std::cout << "Mapper 3" << std::endl;
            break;
        case 4:
            m = new Mapper4(c, cpu);
            std::cout << "Mapper 4" << std::endl;
            break;
        default:
            m = new Mapper0(c);
            std::cout << "Mapper 0" << std::endl;
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
            return cart->prgROM[((cart->nPrgROM - 1) * 0x4000) + (addr & 0x3FFF)];  // just set to last PRGROM bank
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
    return false;
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
            if (addr <= 0x07FF)
                return NAMETABLE[addr & 0x03FF];
            else
                return NAMETABLE[addr & 0x0BFF];
        }
        else if (ntMirror == mirror::vertical)
            return NAMETABLE[addr & 0x07FF];
    }
    return 0x00;
}

bool NES::Mapper0::ppuWrite(uint16_t addr, uint8_t data)
{
    if ((addr <= 0x1FFF) && !(cart->nChrROM))       // CHR-RAM functionality; see "https://wiki.nesdev.com/w/index.php/Category:Mappers_with_CHR_RAM"
    {
        cart->chrROM[addr] = data;
        return true;
    }
    else if (addr <= 0x3EFF)
    {
        addr &= 0x0FFF;
        if (ntMirror == mirror::horizontal)
        {
            if (addr <= 0x07FF)
                NAMETABLE[addr & 0x03FF] = data;
            else
                NAMETABLE[addr & 0x0BFF] = data;
            return true;
        }
        else if (ntMirror == mirror::vertical)
        {
            NAMETABLE[addr & 0x07FF] = data;
            return true;
        }
    }
    return false;
}

#ifdef DEBUG
    uint8_t NES::Mapper0::cpuReadDebug(uint16_t addr)
    {
        return cpuRead(addr);
    }

    uint8_t NES::Mapper0::ppuReadDebug(uint16_t addr)
    {
        return ppuRead(addr);
    }
#endif



NES::Mapper1::Mapper1(Cartridge *c) : Mapper(c) {}      // ntMirror is unused for this mapper

NES::Mapper1::~Mapper1() {}

uint8_t NES::Mapper1::cpuRead(uint16_t addr)
{
    if (addr < 0x4020)
        return 0x00;
    else if (addr < 0x6000)
        return (regPrgBank & 0x0010)? 0x00 : EXPROM[addr - 0x4020];
    else if (addr < 0x8000)
        return (regPrgBank & 0x0010)? 0x00 : SRAM[addr - 0x6000];
    else    // mapper specific functionality
    {
        switch ((regCtrl & 0x000C) >> 2)
        {
            case 0:
            case 1:
                return cart->prgROM[(((regPrgBank & 0x0E) >> 1) * 0x8000) + (addr & 0x7FFF)];
            case 2:
                if (addr < 0xC000)
                    return cart->prgROM[addr & 0x3FFF];
                else
                    return cart->prgROM[((regPrgBank & 0x0F) * 0x4000) + (addr & 0x3FFF)];
            case 3:
                if (addr < 0xC000)
                    return cart->prgROM[((regPrgBank & 0x0F) * 0x4000) + (addr & 0x3FFF)];
                else
                    return cart->prgROM[((cart->nPrgROM - 1) * 0x4000) + (addr & 0x3FFF)];
        }
        return 0x00;
    }
}

bool NES::Mapper1::cpuWrite(uint16_t addr, uint8_t data)
{
    if (addr < 0x4020)
        return false;
    else if (addr < 0x6000)
    {
        if (regPrgBank & 0x0010)
            return false;
        EXPROM[addr - 0x4020] = data;
        return true;
    }
    else if (addr < 0x8000)
    {
        if (regPrgBank & 0x0010)
            return false;
        SRAM[addr - 0x6000] = data;
        return true;
    }
    else    // mapper specific functionality
    {
        if (data & 0x80)
        {
            regLoad = 0x00;
            loadCount = 0;
        }
        else
        {
            if (loadCount < 4)
            {
                regLoad = ((regLoad >> 1) | ((data & 0x01) << 4));
                loadCount++;
            }
            else
            {
                regLoad = ((regLoad >> 1) | ((data & 0x01) << 4));
                switch ((addr & 0x6000) >> 13)
                {
                    case 0:     // 0x8000
                        regCtrl = regLoad;
                        break;
                    case 1:     // 0xA000
                        regChrBank0 = regLoad;
                        break;
                    case 2:     // 0xC000
                        regChrBank1 = regLoad;
                        break;
                    case 3:     // 0xE000
                        regPrgBank = regLoad;
                        break;
                }
                regLoad = 0x00;
                loadCount = 0;
            }
        }
        return true;
    }
}

uint8_t NES::Mapper1::ppuRead(uint16_t addr)
{
    if (addr <= 0x0FFF)
    {
        if (regCtrl & 0x10)
            return cart->chrROM[((regChrBank0 & 0x1F) * 0x1000) + addr];
        else
            return cart->chrROM[(((regChrBank0 & 0x1E) >> 1) * 0x2000) + addr];
    }
    else if (addr <= 0x1FFF)
    {
        if (regCtrl & 0x10)
            return cart->chrROM[((regChrBank1 & 0x1F) * 0x1000) + (addr & 0x0FFF)];
        else
            return cart->chrROM[(((regChrBank0 & 0x1E) >> 1) * 0x2000) + addr];
    }
    else if (addr <= 0x3EFF)
    {
        addr &= 0x0FFF;
        switch (regCtrl & 0x03)
        {
            case 0:     // one-screen lower bank
                return NAMETABLE[0x0400 + (addr & 0x03FF)];
            case 1:     // one-screen upper bank
                return NAMETABLE[addr & 0x03FF];
            case 2:     // vertical
                return NAMETABLE[addr & 0x07FF];
            case 3:     // horizontal
                if (addr <= 0x07FF)
                    return NAMETABLE[addr & 0x03FF];
                else
                    return NAMETABLE[addr & 0x0BFF];
        }
    }
    return 0x00;
}

bool NES::Mapper1::ppuWrite(uint16_t addr, uint8_t data)
{
    if ((addr <= 0x0FFF) && !(cart->nChrROM))       // CHR-RAM functionality; see "https://wiki.nesdev.com/w/index.php/Category:Mappers_with_CHR_RAM"
    {
        if (regCtrl & 0x10)
            cart->chrROM[((regChrBank0 & 0x1F) * 0x1000) + addr] = data;
        else
            cart->chrROM[addr] = data;              // note: there is only 8kB worth of CHR RAM
        return true;
    }
    if ((addr <= 0x1FFF) && !(cart->nChrROM))       // CHR-RAM functionality; see "https://wiki.nesdev.com/w/index.php/Category:Mappers_with_CHR_RAM"
    {
        if (regCtrl & 0x10)
            cart->chrROM[((regChrBank1 & 0x1F) * 0x1000) + (addr & 0x0FFF)] = data;
        else
            cart->chrROM[addr] = data;              // note: there is only 8kB worth of CHR RAM
        return true;
    }
    else if (addr <= 0x3EFF)
    {
        addr &= 0x0FFF;
        switch (regCtrl & 0x03)
        {
            case 0:     // one-screen lower bank
                NAMETABLE[0x0400 + (addr & 0x03FF)] = data;
                return true;
            case 1:     // one-screen upper bank
                NAMETABLE[addr & 0x03FF] = data;
                return true;
            case 2:     // vertical
                NAMETABLE[addr & 0x07FF] = data;
                return true;
            case 3:     // horizontal
                if (addr <= 0x07FF)
                    NAMETABLE[addr & 0x03FF] = data;
                else
                    NAMETABLE[addr & 0x0BFF] = data;
                return true;
        }
    }
    return false;
}

#ifdef DEBUG
    uint8_t NES::Mapper1::cpuReadDebug(uint16_t addr)
    {
        return cpuRead(addr);
    }

    uint8_t NES::Mapper1::ppuReadDebug(uint16_t addr)
    {
        return ppuRead(addr);
    }
#endif



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
    // mapper specific functionality
    regBankSelect = data & 0x0F;
    return true;
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
            if (addr <= 0x07FF)
                return NAMETABLE[addr & 0x03FF];
            else
                return NAMETABLE[addr & 0x0BFF];
        }
        else if (ntMirror == mirror::vertical)
            return NAMETABLE[addr & 0x07FF];
    }
    return 0x00;
}

bool NES::Mapper2::ppuWrite(uint16_t addr, uint8_t data)
{
    if ((addr <= 0x1FFF) && !(cart->nChrROM))       // CHR-RAM functionality; see "https://wiki.nesdev.com/w/index.php/Category:Mappers_with_CHR_RAM"
    {
        cart->chrROM[addr] = data;
        return true;
    }
    else if (addr <= 0x3EFF)
    {
        addr &= 0x0FFF;
        if (ntMirror == mirror::horizontal)
        {
            if (addr <= 0x07FF)
                NAMETABLE[addr & 0x03FF] = data;
            else
                NAMETABLE[addr & 0x0BFF] = data;
            return true;
        }
        else if (ntMirror == mirror::vertical)
        {
            NAMETABLE[addr & 0x07FF] = data;
            return true;
        }
    }
    return false;
}

#ifdef DEBUG
    uint8_t NES::Mapper2::cpuReadDebug(uint16_t addr)
    {
        return cpuRead(addr);
    }

    uint8_t NES::Mapper2::ppuReadDebug(uint16_t addr)
    {
        return ppuRead(addr);
    }
#endif



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
            return cart->prgROM[((cart->nPrgROM - 1) * 0x4000) + (addr & 0x3FFF)];  // just set to last PRGROM bank
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
    // mapper specific functionality
    regBankSelect = data & 0x03;
    return true;
}

uint8_t NES::Mapper3::ppuRead(uint16_t addr)
{
    if (addr <= 0x1FFF)                             // mapper specific functionality
        return cart->chrROM[(regBankSelect * 0x2000) + addr];
    else if (addr <= 0x3EFF)
    {
        addr &= 0x0FFF;
        if (ntMirror == mirror::horizontal)
        {
            if (addr <= 0x07FF)
                return NAMETABLE[addr & 0x03FF];
            else
                return NAMETABLE[addr & 0x0BFF];
        }
        else if (ntMirror == mirror::vertical)
            return NAMETABLE[addr & 0x07FF];
    }
    return 0x00;
}

bool NES::Mapper3::ppuWrite(uint16_t addr, uint8_t data)
{
    if ((addr <= 0x1FFF) && !(cart->nChrROM))       // CHR-RAM functionality; see "https://wiki.nesdev.com/w/index.php/Category:Mappers_with_CHR_RAM"
    {
        cart->chrROM[addr] = data;                  // note: there is only 8kB worth of CHR RAM
        return true;
    }
    else if (addr <= 0x3EFF)
    {
        addr &= 0x0FFF;
        if (ntMirror == mirror::horizontal)
        {
            if (addr <= 0x07FF)
                NAMETABLE[addr & 0x03FF] = data;
            else
                NAMETABLE[addr & 0x0BFF] = data;
            return true;
        }
        else if (ntMirror == mirror::vertical)
        {
            NAMETABLE[addr & 0x07FF] = data;
            return true;
        }
    }
    return false;
}

#ifdef DEBUG
    uint8_t NES::Mapper3::cpuReadDebug(uint16_t addr)
    {
        return cpuRead(addr);
    }

    uint8_t NES::Mapper3::ppuReadDebug(uint16_t addr)
    {
        return ppuRead(addr);
    }
#endif



NES::Mapper4::Mapper4(Cartridge *c, ricoh2A03::CPU *cpu) : Mapper(c), cpu(cpu)
{
    ntMirror = (cart->vertMirror)? mirror::vertical : mirror::horizontal;
}

NES::Mapper4::~Mapper4() {}

uint8_t NES::Mapper4::cpuRead(uint16_t addr)
{
    if (addr < 0x4020)
        return 0x00;
    else if ((addr < 0x6000) && (regPrgRamProtect & 0x80))
        return EXPROM[addr - 0x4020];
    else if (addr < 0x8000)
        return SRAM[addr - 0x6000];
    else
    {
        switch ((addr & 0x6000) >> 13)
        {
            case 0:     // 0x8000
                return (regBankSelect & 0x40)? cart->prgROM[((cart->nPrgROM - 1) * 0x4000) + (addr & 0x1FFF)]
                                             : cart->prgROM[((bankRegisters[6] & 0x3F) * 0x2000) + (addr & 0x1FFF)];
            case 1:     // 0xA000
                return cart->prgROM[((bankRegisters[7] & 0x3F) * 0x2000) + (addr & 0x1FFF)];
            case 2:     // 0xC000
                return (regBankSelect & 0x40)? cart->prgROM[((bankRegisters[6] & 0x3F) * 0x2000) + (addr & 0x1FFF)]
                                             : cart->prgROM[((cart->nPrgROM - 1) * 0x4000) + (addr & 0x1FFF)];
            case 3:     // 0xE000
                return cart->prgROM[((cart->nPrgROM - 1) * 0x4000) + (addr & 0x3FFF)];
        }
        return 0x00;
    }
}

bool NES::Mapper4::cpuWrite(uint16_t addr, uint8_t data)
{
    if (addr < 0x4020)
        return false;
    else if ((addr < 0x6000) && (regPrgRamProtect & 0x80) && (regPrgRamProtect & 0x60))
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
        switch ((addr & 0x6000) >> 13)
        {
            case 0:     // 0x8000
                if (addr & 0x0001)
                    bankRegisters[regBankSelect & 0x07] = data;
                else
                    regBankSelect = data;
                return true;
            case 1:     // 0xA000
                if (addr & 0x0001)
                    regPrgRamProtect = data;
                else
                    regMirror = data;
                return true;
            case 2:     // 0xC000
                if (addr & 0x0001)
                    irqCounter = 0;
                else
                    regIrqLatch = data;
                return true;
            case 3:     // 0xE000
                if (addr & 0x0001)
                    irqEnable = true;
                else
                    irqEnable = false;
                return true;
        }
    }
    return false;
}

uint8_t NES::Mapper4::ppuRead(uint16_t addr)
{
    updateIrqCounter(addr);
    if (addr <= 0x1FFF)
    {
        if (regBankSelect & 0x80)
        {
            switch ((addr & 0x1C00) >> 10)
            {
                case 0:     // 0x0000
                    return cart->chrROM[(bankRegisters[2] * 0x0400) + (addr & 0x03FF)];
                case 1:     // 0x0400
                    return cart->chrROM[(bankRegisters[3] * 0x0400) + (addr & 0x03FF)];
                case 2:     // 0x0800
                    return cart->chrROM[(bankRegisters[4] * 0x0400) + (addr & 0x03FF)];
                case 3:     // 0x0C00
                    return cart->chrROM[(bankRegisters[5] * 0x0400) + (addr & 0x03FF)];
                case 4:     // 0x1000
                case 5:
                    return cart->chrROM[((bankRegisters[0] & 0xFE) * 0x0400) + (addr & 0x07FF)];
                case 6:     // 0x1800
                case 7:
                    return cart->chrROM[((bankRegisters[1] & 0xFE) * 0x0400) + (addr & 0x07FF)];
            }
        }
        else
        {
            switch ((addr & 0x1C00) >> 10)
            {
                case 0:     // 0x0000
                case 1:
                    return cart->chrROM[((bankRegisters[0] & 0xFE) * 0x0400) + (addr & 0x07FF)];
                case 2:     // 0x0800
                case 3:
                    return cart->chrROM[((bankRegisters[1] & 0xFE) * 0x0400) + (addr & 0x07FF)];
                case 4:     // 0x1000
                    return cart->chrROM[(bankRegisters[2] * 0x0400) + (addr & 0x03FF)];
                case 5:     // 0x1400
                    return cart->chrROM[(bankRegisters[3] * 0x0400) + (addr & 0x03FF)];
                case 6:     // 0x1800
                    return cart->chrROM[(bankRegisters[4] * 0x0400) + (addr & 0x03FF)];
                case 7:     // 0x1C00
                    return cart->chrROM[(bankRegisters[5] * 0x0400) + (addr & 0x03FF)];
            }
        }
    }
    else if (addr <= 0x3EFF)
    {
        addr &= 0x0FFF;
        if (regMirror & 0x01)   // horizontal
        {
            if (addr <= 0x07FF)
                return NAMETABLE[addr & 0x03FF];
            else
                return NAMETABLE[addr & 0x0BFF];
        }
        else                    // vertical
            return NAMETABLE[addr & 0x07FF];
    }
    return 0x00;
}

bool NES::Mapper4::ppuWrite(uint16_t addr, uint8_t data)
{
    // updateIrqCounter(addr);
    if ((addr <= 0x1FFF) && !(cart->nChrROM))       // CHR-RAM functionality; see "https://wiki.nesdev.com/w/index.php/Category:Mappers_with_CHR_RAM"
    {
        if (regBankSelect & 0x80)
        {
            switch ((addr & 0x1C00) >> 10)
            {
                case 0:     // 0x0000
                    cart->chrROM[(bankRegisters[2] * 0x0400) + (addr & 0x03FF)] = data;
                    return true;
                case 1:     // 0x0400
                    cart->chrROM[(bankRegisters[3] * 0x0400) + (addr & 0x03FF)] = data;
                    return true;
                case 2:     // 0x0800
                    cart->chrROM[(bankRegisters[4] * 0x0400) + (addr & 0x03FF)] = data;
                    return true;
                case 3:     // 0x0C00
                    cart->chrROM[(bankRegisters[5] * 0x0400) + (addr & 0x03FF)] = data;
                    return true;
                case 4:     // 0x1000
                case 5:
                    cart->chrROM[((bankRegisters[0] & 0xFE) * 0x0400) + (addr & 0x07FF)] = data;
                    return true;
                case 6:     // 0x1800
                case 7:
                    cart->chrROM[((bankRegisters[1] & 0xFE) * 0x0400) + (addr & 0x07FF)] = data;
                    return true;
            }
        }
        else
        {
            switch ((addr & 0x1C00) >> 10)
            {
                case 0:     // 0x0000
                case 1:
                    cart->chrROM[((bankRegisters[0] & 0xFE) * 0x0400) + (addr & 0x07FF)] = data;
                    return true;
                case 2:     // 0x0800
                case 3:
                    cart->chrROM[((bankRegisters[1] & 0xFE) * 0x0400) + (addr & 0x07FF)] = data;
                    return true;
                case 4:     // 0x1000
                    cart->chrROM[(bankRegisters[2] * 0x0400) + (addr & 0x03FF)] = data;
                    return true;
                case 5:     // 0x1400
                    cart->chrROM[(bankRegisters[3] * 0x0400) + (addr & 0x03FF)] = data;
                    return true;
                case 6:     // 0x1800
                    cart->chrROM[(bankRegisters[4] * 0x0400) + (addr & 0x03FF)] = data;
                    return true;
                case 7:     // 0x1C00
                    cart->chrROM[(bankRegisters[5] * 0x0400) + (addr & 0x03FF)] = data;
                    return true;
            }
        }
    }
    else if (addr <= 0x3EFF)
    {
        addr &= 0x0FFF;
        if (regMirror & 0x01)   // horizontal
        {
            if (addr <= 0x07FF)
                NAMETABLE[addr & 0x03FF] = data;
            else
                NAMETABLE[addr & 0x0BFF] = data;
            return true;
        }
        else                    // vertical
        {
            NAMETABLE[addr & 0x07FF] = data;
            return true;
        }
    }
    return false;
}

void NES::Mapper4::updateIrqCounter(uint16_t addr)
{
    if (addr & 0xE000)
        return;
    if ((addr & 0x1000) == 0x0000)
    {
        if (!A12down)
        {
            A12down = true;
            A12FirstDown = cpu->getClock();
        }
    }
    else
    {
        if (A12down)
        {
            uint64_t currClock = cpu->getClock();
            if ((currClock < A12FirstDown) || ((currClock - A12FirstDown) >= (uint64_t)(3)))
            {
                if (!irqCounter)
                    irqCounter = regIrqLatch;
                else
                    irqCounter--;
                if ((!irqCounter) && irqEnable)
                    IRQ = true;
            }
            A12down = false;
        }
    }
}

#ifdef DEBUG
    uint8_t NES::Mapper4::cpuReadDebug(uint16_t addr)
    {
        return cpuRead(addr);
    }

    uint8_t NES::Mapper4::ppuReadDebug(uint16_t addr)
    {
        if (addr <= 0x1FFF)
        {
            if (regBankSelect & 0x80)
            {
                switch ((addr & 0x1C00) >> 10)
                {
                    case 0:     // 0x0000
                        return cart->chrROM[(bankRegisters[2] * 0x0400) + (addr & 0x03FF)];
                    case 1:     // 0x0400
                        return cart->chrROM[(bankRegisters[3] * 0x0400) + (addr & 0x03FF)];
                    case 2:     // 0x0800
                        return cart->chrROM[(bankRegisters[4] * 0x0400) + (addr & 0x03FF)];
                    case 3:     // 0x0C00
                        return cart->chrROM[(bankRegisters[5] * 0x0400) + (addr & 0x03FF)];
                    case 4:     // 0x1000
                    case 5:
                        return cart->chrROM[((bankRegisters[0] & 0xFE) * 0x0400) + (addr & 0x07FF)];
                    case 6:     // 0x1800
                    case 7:
                        return cart->chrROM[((bankRegisters[1] & 0xFE) * 0x0400) + (addr & 0x07FF)];
                }
            }
            else
            {
                switch ((addr & 0x1C00) >> 10)
                {
                    case 0:     // 0x0000
                    case 1:
                        return cart->chrROM[((bankRegisters[0] & 0xFE) * 0x0400) + (addr & 0x07FF)];
                    case 2:     // 0x0800
                    case 3:
                        return cart->chrROM[((bankRegisters[1] & 0xFE) * 0x0400) + (addr & 0x07FF)];
                    case 4:     // 0x1000
                        return cart->chrROM[(bankRegisters[2] * 0x0400) + (addr & 0x03FF)];
                    case 5:     // 0x1400
                        return cart->chrROM[(bankRegisters[3] * 0x0400) + (addr & 0x03FF)];
                    case 6:     // 0x1800
                        return cart->chrROM[(bankRegisters[4] * 0x0400) + (addr & 0x03FF)];
                    case 7:     // 0x1C00
                        return cart->chrROM[(bankRegisters[5] * 0x0400) + (addr & 0x03FF)];
                }
            }
        }
        else if (addr <= 0x3EFF)
        {
            addr &= 0x0FFF;
            if (regMirror & 0x01)   // horizontal
            {
                if (addr <= 0x07FF)
                    return NAMETABLE[addr & 0x03FF];
                else
                    return NAMETABLE[addr & 0x0BFF];
            }
            else                    // vertical
                return NAMETABLE[addr & 0x07FF];
        }
        return 0x00;
    }
#endif