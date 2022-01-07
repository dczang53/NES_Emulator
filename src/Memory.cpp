#include "../include/Memory.hpp"
#include "../include/Mapper.hpp"
#include "../include/Ricoh2A03.hpp"
#include "../include/Ricoh2C02.hpp"

#include <iostream>

NES::NESmemory::NESmemory()
{
       cpuMemory = new uint8_t[0x401F]{0};
       // ppuMemory = new uint8_t[0x4000]{0};
       // set palette table to indices
       // palette stored in PPU
       ppuPalette = new uint8_t[0x001F]{0};
}

NES::NESmemory::~NESmemory()
{
       delete[] cpuMemory;
       delete[] ppuPalette;
       delete mapper;
}

void NES::NESmemory::initCartridge(std::string filename)
{
       mapper = createMapper(filename, cpu);
}

uint8_t NES::NESmemory::cpuRead(uint16_t addr)
{
    if (addr <= 0x1FFF)
        return cpuMemory[addr & 0x07FF];
    else if (addr <= 0x3FFF)        // remember to update PPU latch
        return ppu->cpuRead(addr);
    else if (addr <= 0x4015)
    {
        if (addr == 0x4014)
            return ppu->cpuRead(addr);
        return cpuMemory[addr];
    }
    else if (addr <= 0x4017)        // controller serial read
    {
        uint8_t data = (cpuMemory[addr] & 0x80)? 1 : 0;
        cpuMemory[addr] <<= 1;
        return data;
    }
    else if (addr <= 0x401F)
        return cpuMemory[addr];
    else
        return mapper->cpuRead(addr);
}

bool NES::NESmemory::cpuWrite(uint16_t addr, uint8_t data)
{
    if (addr <= 0x1FFF)
    {
        cpuMemory[addr & 0x07FF] = data;
        return true;
    }
    else if (addr <= 0x3FFF)
        return ppu->cpuWrite(addr, data);
    else if (addr <= 0x401F)
    {
        if (addr == 0x4014)
            return ppu->cpuWrite(addr, data);
        else if (addr == 0x4016)
            cpuMemory[addr] = controllerBuffer1;
        else if (addr == 0x4017)
            cpuMemory[addr] = controllerBuffer2;
        else
            cpuMemory[addr] = data;
        return true;
    }
    else
        return mapper->cpuWrite(addr, data);
}

uint8_t NES::NESmemory::ppuRead(uint16_t addr)
{
    addr &= 0x3FFF;
    if (addr <= 0x1FFF)
        return mapper->ppuRead(addr);           // get from cartridge
    else if (addr <= 0x3EFF)
        return mapper->ppuRead(addr & 0x2FFF);  // need nametable mirroring
    else
    {
        addr &= 0x001F;
        if ((addr & 0x0010) && !(addr & 0x0003))    // "addresses $3F04/$3F08/$3F0C can contain unique data"
            addr &= ~(0x0010);                      // "addresses $3F10/$3F14/$3F18/$3F1C are mirrors of $3F00/$3F04/$3F08/$3F0C"
        return ppuPalette[addr];                    // NOTE: need to convert raw byte to RGB color in palette
    }
}

bool NES::NESmemory::ppuWrite(uint16_t addr, uint8_t data)
{
    addr &= 0x3FFF;
    if (addr <= 0x1FFF)
        return mapper->ppuWrite(addr, data);            // get from cartridge
    else if (addr <= 0x3EFF)
        return mapper->ppuWrite((addr & 0x2FFF), data); // need nametable mirroring
    else
    {
        addr &= 0x001F;
        if ((addr & 0x0010) && !(addr & 0x0003))    // "addresses $3F04/$3F08/$3F0C can contain unique data"
            addr &= ~(0x0010);                      // "addresses $3F10/$3F14/$3F18/$3F1C are mirrors of $3F00/$3F04/$3F08/$3F0C"
        ppuPalette[addr] = data;                    // NOTE: need to convert raw byte to RGB color in palette
        return true;                    
    }
}

#ifdef DEBUG
    uint8_t NES::NESmemory::cpuReadDebug(uint16_t addr)
    {
        if (addr <= 0x1FFF)
            return cpuMemory[addr & 0x07FF];
        else if (addr <= 0x3FFF)
            return ppu->cpuReadDebug(addr);
        else if (addr <= 0x4015)
        {
            if (addr == 0x4014)
                return ppu->cpuReadDebug(addr);
            return cpuMemory[addr];
        }
        else if (addr <= 0x4017)
            return cpuMemory[addr];
        else if (addr <= 0x401F)
            return cpuMemory[addr];
        else
            return mapper->cpuReadDebug(addr);
    }

    uint8_t NES::NESmemory::ppuReadDebug(uint16_t addr)
    {
        addr &= 0x3FFF;
        if (addr <= 0x1FFF)
            return mapper->ppuReadDebug(addr);
        else if (addr <= 0x3EFF)
            return mapper->ppuReadDebug(addr & 0x2FFF);
        else
        {
            addr &= 0x001F;
            if ((addr & 0x0010) && !(addr & 0x0003))
                addr &= ~(0x0010);
            return ppuPalette[addr];
        }
    }
#endif

void NES::NESmemory::connectCPUandPPU(ricoh2A03::CPU *c, ricoh2C02::PPU *p)
{
    cpu = c;
    ppu = p;
}

uint8_t NES::NESmemory::controllerRead(uint8_t player)
{
    if (!player)
        return controllerBuffer1;
    return controllerBuffer2;
}

void NES::NESmemory::controllerWrite(uint8_t player, uint8_t data)
{
    if (!player)
        controllerBuffer1 = data;
    else
        controllerBuffer2 = data;
}



bool NES::NESmemory::mapperIrqReq()
{
    return mapper->IRQcheck();
}

void NES::NESmemory::mapperIrqReset()
{
    mapper->IRQreset();
}

void NES::NESmemory::toggleCpuCycle()
{
    cpuOddCycle = !cpuOddCycle;
}

void NES::NESmemory::ppuRequestDMA()
{
    reqDMA = true;
}

void NES::NESmemory::finalizeDMAreq()
{
    if(reqDMA)
        DMAcycles = 513;
    reqDMA = false;
}

bool NES::NESmemory::DMAactive()
{
    return (DMAcycles > 0);
}

void NES::NESmemory::handleDMA()
{
    if (DMAcycles > 0)
    {
        if ((DMAcycles == 513) && cpuOddCycle)              // extra idle cycle for odd cpu cycles
            return;
        if ((DMAcycles < 513) && (DMAcycles & 0x0001))      // takes into account idle cycle at beginning; also only happens on write cycles
            ppu->DMAtransfer();
        DMAcycles--;
    }
}

/*
bool NES::NESmemory::mapperIRQ() {return mapper->IRQcheck();}

void NES::NESmemory::mapperResetIRQ() {mapper->IRQreset();}
*/