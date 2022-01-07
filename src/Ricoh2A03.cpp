#include "../include/Ricoh2A03.hpp"
#include "../include/Memory.hpp"

// #include <iostream>
// #include <cstring>

#ifdef DEBUG
       #include <fstream>
       #include <iomanip>
#endif

ricoh2A03::CPU::CPU(NES::Memory *m) : mem(m)
{
        #ifdef DEBUG
              CPUlogfile = std::ofstream("CPUlogfile.txt", std::ios_base::out);  // debug
              CPUlogfile << std::uppercase << std::hex << std::setfill('0');
        #endif
}

ricoh2A03::CPU::~CPU()
{
       #ifdef DEBUG
              CPUlogfile.close();
       #endif
}

#ifdef DEBUG
       void ricoh2A03::CPU::enableLog(bool enable)
       {
              CPUlog = enable;
       }
#endif



void ricoh2A03::CPU::tick(bool functional)
{
       clock++;
       if (!functional)
              return;
       if (insClk <= 0)
       {
              if (pendingNMI)
              {
                     #ifdef DEBUG
                            if (CPUlog)                                             // debug
                                   CPUlogfile << "(NMI triggered)";                 // debug
                     #endif
                     mem->cpuWrite((0x0100 + SP), (uint8_t)((PC >> 8) & 0x00FF));
                     SP--;
                     mem->cpuWrite((0x0100 + SP), (uint8_t)(PC & 0x00FF));
                     SP--;
                     mem->cpuWrite((0x0100 + SP), STATUS);
                     SP--;
                     PC = (((uint16_t)(mem->cpuRead(0xFFFB)) << 8) | mem->cpuRead(0xFFFA));
                     insClk = 8;
              }
              else if (pendingIRQ && !(STATUS & interruptDisable))
              {
                     #ifdef DEBUG
                            if (CPUlog)                                             // debug
                                   CPUlogfile << "(IRQ triggered)";                 // debug
                     #endif
                     mem->cpuWrite((0x0100 + SP), (uint8_t)((PC >> 8) & 0x00FF));
                     SP--;
                     mem->cpuWrite((0x0100 + SP), (uint8_t)(PC & 0x00FF));
                     SP--;
                     mem->cpuWrite((0x0100 + SP), STATUS);
                     SP--;
                     STATUS |= interruptDisable;
                     PC = (((uint16_t)(mem->cpuRead(0xFFFF)) << 8) | mem->cpuRead(0xFFFE));
                     insClk = 7;
              }
              else
              {
                     #ifdef DEBUG
                            if (CPUlog)                                             // debug
                                   CPUlogfile << PC;                                // debug
                     #endif
                     currOp = &(instructionSet[mem->cpuRead(PC++)]);
                     #ifdef DEBUG
                            if (CPUlog)                                             // debug
                                   CPUlogfile << ' ' << currOp->operation;          // debug
                     #endif
                     operandClk = (this->*currOp->addrMode)();
                     processClk = (this->*currOp->operate)();
                     insClk = (currOp->cycles) + operandClk + processClk;
              }
              #ifdef DEBUG
                     if (CPUlog)                        // debug
                            CPUlogfile << std::endl;    // debug
              #endif
              pendingIRQ = false;
              pendingNMI = false;
       }
       insClk--;
}


uint64_t ricoh2A03::CPU::getClock()
{
       return clock;
}


void ricoh2A03::CPU::rst()
{
       PC = (((uint16_t)(mem->cpuRead(0xFFFD)) << 8) | mem->cpuRead(0xFFFC));
       SP = 0xFF;
       ACC = 0x00;
       REGX = 0x00;
       REGY = 0x00;
       STATUS = 0x00;
       pendingIRQ = false;
       pendingNMI = false;
       operandRef = nullptr;
       operandAddr = -1;
       insClk = 0;
       clock = 0;
}

void ricoh2A03::CPU::irq()
{
       pendingIRQ = true;
}

void ricoh2A03::CPU::nmi()
{
       pendingNMI = true;
}

bool ricoh2A03::CPU::instrDone()
{
       return (insClk == 0)? true : false;
}





uint8_t ricoh2A03::CPU::ACCUM()
{
       operandRef = &ACC;
       operandAddr = -1;
       return 0;
}

uint8_t ricoh2A03::CPU::IMM()
{
       operandRef = nullptr;
       operandAddr = PC++;
       #ifdef DEBUG
              if (CPUlog)
                     CPUlogfile << " #$" << std::setw(2) << (int)(mem->cpuReadDebug(operandAddr));
       #endif
       return 0;
}

uint8_t ricoh2A03::CPU::ABS()
{
       operandRef = nullptr;
       operandAddr = (((uint16_t)(mem->cpuRead(PC + 1)) << 8) | mem->cpuRead(PC));
       PC += 2;
       #ifdef DEBUG
              if (CPUlog)
                     CPUlogfile << " $" << std::setw(4) << (int)(operandAddr);
       #endif
       return 0;
}

uint8_t ricoh2A03::CPU::ZP()
{
       operandRef = nullptr;
       operandAddr = (0x0000 | mem->cpuRead(PC++));
       #ifdef DEBUG
              if (CPUlog)
                     CPUlogfile << " $" << std::setw(2) << (int)(operandAddr);
       #endif
       return 0;
}

uint8_t ricoh2A03::CPU::ZPX()
{
       operandRef = nullptr;
       operandAddr = (0x00FF & (mem->cpuRead(PC++) + REGX));
       #ifdef DEBUG
              if (CPUlog)
                     CPUlogfile << " $" << std::setw(2) << (int)(operandAddr - REGX) << ",X";
       #endif
       return 0;
}

uint8_t ricoh2A03::CPU::ZPY()
{
       operandRef = nullptr;
       operandAddr = (0x00FF & (mem->cpuRead(PC++) + REGY));
       #ifdef DEBUG
              if (CPUlog)
                     CPUlogfile << " $" << std::setw(2) << (int)(operandAddr - REGY) << ",Y";
       #endif
       return 0;
}

uint8_t ricoh2A03::CPU::ABSX()
{
       uint16_t buffer = (((uint16_t)(mem->cpuRead(PC + 1)) << 8) | mem->cpuRead(PC));
       PC += 2;
       operandRef = nullptr;
       operandAddr = buffer + REGX;
       #ifdef DEBUG
              if (CPUlog)
                     CPUlogfile << " $" << std::setw(4) << (int)(buffer) << ",X";
       #endif
       if ((operandAddr & 0xFF00) == (buffer & 0xFF00))
              return 0;
       else
              return currOp->pageDelay;
}

uint8_t ricoh2A03::CPU::ABSY()
{
       uint16_t buffer = (((uint16_t)(mem->cpuRead(PC + 1)) << 8) | mem->cpuRead(PC));
       PC += 2;
       operandRef = nullptr;
       operandAddr = buffer + REGY;
       #ifdef DEBUG
              if (CPUlog)
                     CPUlogfile << " $" << std::setw(4) << (int)(buffer) << ",Y";
       #endif
       if ((operandAddr & 0xFF00) == (buffer & 0xFF00))
              return 0;
       else
              return currOp->pageDelay;
}

uint8_t ricoh2A03::CPU::IMP()
{
       operandRef = nullptr;
       operandAddr = -1;
       return 0;
}

uint8_t ricoh2A03::CPU::REL()
{
       operandRef = nullptr;
       operandAddr = PC++;
       #ifdef DEBUG
              if (CPUlog)
                     CPUlogfile << ' ' << std::setw(2) << (int)(mem->cpuReadDebug(operandAddr)) << ",Y";
       #endif
       return 0;
}

uint8_t ricoh2A03::CPU::INDX()
{
       uint8_t temp = mem->cpuRead(PC++);
       uint8_t zpAddr = temp + REGX;
       operandRef = nullptr;
       operandAddr = (((uint16_t)(mem->cpuRead(zpAddr + 1)) << 8) | mem->cpuRead(zpAddr)) & 0x000000FF;
       #ifdef DEBUG
              if (CPUlog)
                     CPUlogfile << " ($" << std::setw(2) << (int)(temp) << ",X)";
       #endif
       return 0;
}

uint8_t ricoh2A03::CPU::INDY()
{
       uint8_t zpAddr = mem->cpuRead(PC++);
       uint16_t buffer = (((uint16_t)(mem->cpuRead((uint16_t)(zpAddr) + 1)) << 8) | mem->cpuRead(zpAddr));
       operandRef = nullptr;
       operandAddr = buffer + REGY;
       #ifdef DEBUG
              if (CPUlog)
                     CPUlogfile << " ($" << std::setw(2) << (int)(zpAddr) << "),Y";
       #endif
       if((operandAddr & 0xFF00) == (buffer & 0xFF00))
              return 0;
       else
              return currOp->pageDelay;
}

uint8_t ricoh2A03::CPU::IND()
{
       uint16_t addrToAddr = (((uint16_t)(mem->cpuRead(PC + 1)) << 8) | mem->cpuRead(PC));
       operandRef = nullptr;
       if(mem->cpuRead(PC) == 0xFF)
              operandAddr = (((uint16_t)(mem->cpuRead(addrToAddr | 0xFF00)) << 8) | mem->cpuRead(addrToAddr));  
       else
              operandAddr = (((uint16_t)(mem->cpuRead(addrToAddr + 1)) << 8) | mem->cpuRead(addrToAddr));
       PC += 2;
       #ifdef DEBUG
              if (CPUlog)
                     CPUlogfile << " ($" << std::setw(4) << (int)(addrToAddr) << ")";
       #endif
       return 0;
}





uint8_t ricoh2A03::CPU::LDA()
{
       ACC = (operandRef)? *operandRef : mem->cpuRead(operandAddr);
       STATUS = (ACC == 0x00)? (STATUS | zeroFlag) : (STATUS & ~(zeroFlag));
       STATUS = (ACC & 0x80)? (STATUS | negativeFlag) : (STATUS & ~(negativeFlag));
       return 0;
}

uint8_t ricoh2A03::CPU::LDX()
{
       REGX = (operandRef)? *operandRef : mem->cpuRead(operandAddr);
       STATUS = (REGX == 0x00)? (STATUS | zeroFlag) : (STATUS & ~(zeroFlag));
       STATUS = (REGX & 0x80)? (STATUS | negativeFlag) : (STATUS & ~(negativeFlag));
       return 0;
}

uint8_t ricoh2A03::CPU::LDY()
{
       REGY = (operandRef)? *operandRef : mem->cpuRead(operandAddr);
       STATUS = (REGY == 0x00)? (STATUS | zeroFlag) : (STATUS & ~(zeroFlag));
       STATUS = (REGY & 0x80)? (STATUS | negativeFlag) : (STATUS & ~(negativeFlag));
       return 0;
}

uint8_t ricoh2A03::CPU::STA()
{
       if (operandRef)
              *operandRef = ACC;
       else
              mem->cpuWrite(operandAddr, ACC);
       return 0;
}

uint8_t ricoh2A03::CPU::STX()
{
       if (operandRef)
              *operandRef = REGX;
       else
              mem->cpuWrite(operandAddr, REGX);
       return 0;
}

uint8_t ricoh2A03::CPU::STY()
{
       if (operandRef)
              *operandRef = REGY;
       else
              mem->cpuWrite(operandAddr, REGY);
       return 0;
}

uint8_t ricoh2A03::CPU::TAX()
{
       REGX = ACC;
       STATUS = (REGX == 0x00)? (STATUS | zeroFlag) : (STATUS & ~(zeroFlag));
       STATUS = (REGX & 0x80)? (STATUS | negativeFlag) : (STATUS & ~(negativeFlag));
       return 0;
}

uint8_t ricoh2A03::CPU::TAY()
{
       REGY = ACC;
       STATUS = (REGY == 0x00)? (STATUS | zeroFlag) : (STATUS & ~(zeroFlag));
       STATUS = (REGY & 0x80)? (STATUS | negativeFlag) : (STATUS & ~(negativeFlag));
       return 0;
}

uint8_t ricoh2A03::CPU::TXA()
{
       ACC = REGX;
       STATUS = (ACC == 0x00)? (STATUS | zeroFlag) : (STATUS & ~(zeroFlag));
       STATUS = (ACC & 0x80)? (STATUS | negativeFlag) : (STATUS & ~(negativeFlag));
       return 0;
}

uint8_t ricoh2A03::CPU::TYA()
{
       ACC = REGY;
       STATUS = (ACC == 0x00)? (STATUS | zeroFlag) : (STATUS & ~(zeroFlag));
       STATUS = (ACC & 0x80)? (STATUS | negativeFlag) : (STATUS & ~(negativeFlag));
       return 0;
}

uint8_t ricoh2A03::CPU::TSX()
{
       REGX = SP;
       STATUS = (REGX == 0x00)? (STATUS | zeroFlag) : (STATUS & ~(zeroFlag));
       STATUS = (REGX & 0x80)? (STATUS | negativeFlag) : (STATUS & ~(negativeFlag));
       return 0;
}

uint8_t ricoh2A03::CPU::TXS()
{
       SP = REGX;
       return 0;
}

uint8_t ricoh2A03::CPU::PHA()
{
       mem->cpuWrite(0x0100 + SP, ACC);
       SP--;
       return 0;
}

uint8_t ricoh2A03::CPU::PHP()
{
       mem->cpuWrite(0x0100 + SP, (STATUS | breakCommand | unused));
       SP--;
       return 0;
}

uint8_t ricoh2A03::CPU::PLA()
{
       SP++;
       ACC = mem->cpuRead(0x0100 + SP);
       STATUS = (ACC == 0x00)? (STATUS | zeroFlag) : (STATUS & ~(zeroFlag));
       STATUS = (ACC & 0x80)? (STATUS | negativeFlag) : (STATUS & ~(negativeFlag));
       return 0;
}

uint8_t ricoh2A03::CPU::PLP()
{
       SP++;
       STATUS = mem->cpuRead(0x0100 + SP);
       return 0;
}

uint8_t ricoh2A03::CPU::AND()
{
       ACC &= (operandRef)? *operandRef : mem->cpuRead(operandAddr);
       STATUS = (ACC == 0x00)? (STATUS | zeroFlag) : (STATUS & ~(zeroFlag));
       STATUS = (ACC & 0x80)? (STATUS | negativeFlag) : (STATUS & ~(negativeFlag));
       return 0;
}

uint8_t ricoh2A03::CPU::EOR()
{
       ACC ^= (operandRef)? *operandRef : mem->cpuRead(operandAddr);
       STATUS = (ACC == 0x00)? (STATUS | zeroFlag) : (STATUS & ~(zeroFlag));
       STATUS = (ACC & 0x80)? (STATUS | negativeFlag) : (STATUS & ~(negativeFlag));
       return 0;
}

uint8_t ricoh2A03::CPU::ORA()
{
       ACC |= (operandRef)? *operandRef : mem->cpuRead(operandAddr);
       STATUS = (ACC == 0x00)? (STATUS | zeroFlag) : (STATUS & ~(zeroFlag));
       STATUS = (ACC & 0x80)? (STATUS | negativeFlag) : (STATUS & ~(negativeFlag));
       return 0;
}

uint8_t ricoh2A03::CPU::BIT()
{
       uint8_t operand = (operandRef)? *operandRef : mem->cpuRead(operandAddr);
       STATUS = (ACC & operand)? (STATUS & ~(zeroFlag)) : (STATUS | zeroFlag);
       STATUS = (operand & 0x40)? (STATUS | overflowFlag) : (STATUS & ~(overflowFlag));
       STATUS = (operand & 0x80)? (STATUS | negativeFlag) : (STATUS & ~(negativeFlag));
       return 0;
}

uint8_t ricoh2A03::CPU::ADC()
{
       uint8_t operand = (operandRef)? *operandRef : mem->cpuRead(operandAddr);
       uint16_t buffer = (uint16_t)(ACC) + (uint16_t)(operand) + (uint16_t)((STATUS & carryFlag)? 1 : 0);
       STATUS = (buffer & 0xFF00)? (STATUS | carryFlag) : (STATUS & ~(carryFlag));
       STATUS = (buffer & 0x00FF)? (STATUS & ~(zeroFlag)) : (STATUS | zeroFlag);
       STATUS = (((uint16_t)(ACC) ^ ~(uint16_t)(operand)) & ((uint16_t)(ACC) ^ buffer) & 0x0080)? (STATUS | overflowFlag) : (STATUS & ~(overflowFlag));
       STATUS = (buffer & 0x0080)? (STATUS | negativeFlag) : (STATUS & ~(negativeFlag));
       ACC = (buffer & 0x00FF);
       return 0;
}

uint8_t ricoh2A03::CPU::SBC()
{
       uint8_t operand = (operandRef)? *operandRef : mem->cpuRead(operandAddr);
       uint16_t buffer = (uint16_t)(ACC) + (uint16_t)(operand ^ 0xFF) + (uint16_t)((STATUS & carryFlag)? 1 : 0);       // carryFlag is "reverse-carry" here
       STATUS = (buffer & 0xFF00)? (STATUS | carryFlag) : (STATUS & ~(carryFlag));
       STATUS = (buffer & 0x00FF)? (STATUS & ~(zeroFlag)) : (STATUS | zeroFlag);
       STATUS = (((uint16_t)(ACC) ^ (uint16_t)(operand)) & ((uint16_t)(ACC) ^ buffer) & 0x0080)? (STATUS | overflowFlag) : (STATUS & ~(overflowFlag));
       STATUS = (buffer & 0x0080)? (STATUS | negativeFlag) : (STATUS & ~(negativeFlag));
       ACC = (buffer & 0x00FF);
       return 0;
}

uint8_t ricoh2A03::CPU::CMP()
{
       uint8_t operand = (operandRef)? *operandRef : mem->cpuRead(operandAddr);
       STATUS = (ACC >= operand)? (STATUS | carryFlag) : (STATUS & ~(carryFlag));
       STATUS = (ACC == operand)? (STATUS | zeroFlag) : (STATUS & ~(zeroFlag));
       STATUS = ((ACC - operand) & 0x80)? (STATUS | negativeFlag) : (STATUS & ~(negativeFlag));
       return 0;
}

uint8_t ricoh2A03::CPU::CPX()
{
       uint8_t operand = (operandRef)? *operandRef : mem->cpuRead(operandAddr);
       STATUS = (REGX >= operand)? (STATUS | carryFlag) : (STATUS & ~(carryFlag));
       STATUS = (REGX == operand)? (STATUS | zeroFlag) : (STATUS & ~(zeroFlag));
       STATUS = ((REGX - operand) & 0x80)? (STATUS | negativeFlag) : (STATUS & ~(negativeFlag));
       return 0;
}

uint8_t ricoh2A03::CPU::CPY()
{
       uint8_t operand = (operandRef)? *operandRef : mem->cpuRead(operandAddr);
       STATUS = (REGY >= operand)? (STATUS | carryFlag) : (STATUS & ~(carryFlag));
       STATUS = (REGY == operand)? (STATUS | zeroFlag) : (STATUS & ~(zeroFlag));
       STATUS = ((REGY - operand) & 0x80)? (STATUS | negativeFlag) : (STATUS & ~(negativeFlag));
       return 0;
}

uint8_t ricoh2A03::CPU::INC()
{
       uint8_t operand;
       if (operandRef)
       {
              (*operandRef)++;
              operand = *operandRef;
       }
       else
       {
              operand = mem->cpuRead(operandAddr) + 1;
              mem->cpuWrite(operandAddr, operand);
       }
       STATUS = (operand)? (STATUS & ~(zeroFlag)) : (STATUS | zeroFlag);
       STATUS = (operand & 0x80)? (STATUS | negativeFlag) : (STATUS & ~(negativeFlag));
       return 0;
}

uint8_t ricoh2A03::CPU::INX()
{
       REGX++;
       STATUS = (REGX)? (STATUS & ~(zeroFlag)) : (STATUS | zeroFlag);
       STATUS = (REGX & 0x80)? (STATUS | negativeFlag) : (STATUS & ~(negativeFlag));
       return 0;
}

uint8_t ricoh2A03::CPU::INY()
{
       REGY++;
       STATUS = (REGY)? (STATUS & ~(zeroFlag)) : (STATUS | zeroFlag);
       STATUS = (REGY & 0x80)? (STATUS | negativeFlag) : (STATUS & ~(negativeFlag));
       return 0;
}

uint8_t ricoh2A03::CPU::DEC()
{
       uint8_t operand;
       if (operandRef)
       {
              (*operandRef)--;
              operand = *operandRef;
       }
       else
       {
              operand = mem->cpuRead(operandAddr) - 1;
              mem->cpuWrite(operandAddr, operand);
       }
       STATUS = (operand)? (STATUS & ~(zeroFlag)) : (STATUS | zeroFlag);
       STATUS = (operand & 0x80)? (STATUS | negativeFlag) : (STATUS & ~(negativeFlag));
       return 0;
}

uint8_t ricoh2A03::CPU::DEX()
{
       REGX--;
       STATUS = (REGX)? (STATUS & ~(zeroFlag)) : (STATUS | zeroFlag);
       STATUS = (REGX & 0x80)? (STATUS | negativeFlag) : (STATUS & ~(negativeFlag));
       return 0;
}

uint8_t ricoh2A03::CPU::DEY()
{
       REGY--;
       STATUS = (REGY)? (STATUS & ~(zeroFlag)) : (STATUS | zeroFlag);
       STATUS = (REGY & 0x80)? (STATUS | negativeFlag) : (STATUS & ~(negativeFlag));
       return 0;
}

uint8_t ricoh2A03::CPU::ASL()
{
       uint8_t operand = (operandRef)? *operandRef : mem->cpuRead(operandAddr);
       uint16_t buffer = ((uint16_t)(operand) << 1);
       STATUS = (buffer & 0xFF00)? (STATUS | carryFlag) : (STATUS & ~(carryFlag));
       STATUS = (buffer & 0x00FF)? (STATUS & ~(zeroFlag)) : (STATUS | zeroFlag);
       STATUS = (buffer & 0x0080)? (STATUS | negativeFlag) : (STATUS & ~(negativeFlag));
       if (operandRef)
              *operandRef = (buffer & 0x00FF);
       else
              mem->cpuWrite(operandAddr, (buffer & 0x00FF));
       return 0;
}

uint8_t ricoh2A03::CPU::LSR()
{
       uint8_t operand = (operandRef)? *operandRef : mem->cpuRead(operandAddr);
       uint16_t buffer = ((uint16_t)(operand) >> 1);
       STATUS = (operand & 0x01)? (STATUS | carryFlag) : (STATUS & ~(carryFlag));
       STATUS = (buffer & 0x00FF)? (STATUS & ~(zeroFlag)) : (STATUS | zeroFlag);
       STATUS = (buffer & 0x0080)? (STATUS | negativeFlag) : (STATUS & ~(negativeFlag));
       if (operandRef)
              *operandRef = (buffer & 0x00FF);
       else
              mem->cpuWrite(operandAddr, (buffer & 0x00FF));
       return 0;
}

uint8_t ricoh2A03::CPU::ROL()
{
       uint8_t operand = (operandRef)? *operandRef : mem->cpuRead(operandAddr);
       uint16_t buffer = (((uint16_t)(operand) << 1) | ((STATUS & carryFlag)? 0x0001 : 0x0000));
       STATUS = (buffer & 0xFF00)? (STATUS | carryFlag) : (STATUS & ~(carryFlag));
       STATUS = (buffer & 0x00FF)? (STATUS & ~(zeroFlag)) : (STATUS | zeroFlag);
       STATUS = (buffer & 0x0080)? (STATUS | negativeFlag) : (STATUS & ~(negativeFlag));
       if (operandRef)
              *operandRef = (buffer & 0x00FF);
       else
              mem->cpuWrite(operandAddr, (buffer & 0x00FF));
       return 0;
}

uint8_t ricoh2A03::CPU::ROR()
{
       uint8_t operand = (operandRef)? *operandRef : mem->cpuRead(operandAddr);
       uint16_t buffer = (((uint16_t)(operand) >> 1) | ((STATUS & carryFlag)? 0x0080 : 0x0000));
       STATUS = (operand & 0x01)? (STATUS | carryFlag) : (STATUS & ~(carryFlag));
       STATUS = (buffer & 0x00FF)? (STATUS & ~(zeroFlag)) : (STATUS | zeroFlag);
       STATUS = (buffer & 0x0080)? (STATUS | negativeFlag) : (STATUS & ~(negativeFlag));
       if (operandRef)
              *operandRef = (buffer & 0x00FF);
       else
              mem->cpuWrite(operandAddr, (buffer & 0x00FF));
       return 0;
}

uint8_t ricoh2A03::CPU::JMP()
{
       PC = (uint16_t)(operandAddr & 0x0000FFFF);
       return 0;
}

uint8_t ricoh2A03::CPU::JSR()
{
       PC--;
       mem->cpuWrite(0x0100 + SP, (uint8_t)((PC >> 8) & 0x00FF));
       SP--;
       mem->cpuWrite(0x0100 + SP, (uint8_t)(PC & 0x00FF));
       SP--;
       PC = operandAddr;
       return 0;
}

uint8_t ricoh2A03::CPU::RTS()
{
       SP++;
       PC = mem->cpuRead(0x0100 + SP);
       SP++;
       PC |= ((uint16_t)(mem->cpuRead(0x0100 + SP)) << 8);
       PC++;
       return 0;
}

uint8_t ricoh2A03::CPU::BCC()
{
       if ((STATUS & carryFlag) || operandRef)
              return 0;
       else
       {
              uint8_t addCycles = 1;
              uint8_t operand = mem->cpuRead(operandAddr);
              uint16_t operand16 = (operand & 0x80)? (0xFF00 | operand) : (0x0000 | operand);
              if (((PC + operand16) & 0XFF00) != (PC & 0xFF00))
                     addCycles++;
              PC += operand16;     // overflow intended
              return addCycles;
       }
}

uint8_t ricoh2A03::CPU::BCS()
{
       if ((STATUS & carryFlag) && !(operandRef))
       {
              uint8_t addCycles = 1;
              uint8_t operand = mem->cpuRead(operandAddr);
              uint16_t operand16 = (operand & 0x80)? (0xFF00 | operand) : (0x0000 | operand);
              if (((PC + operand16) & 0XFF00) != (PC & 0xFF00))
                     addCycles++;
              PC += operand16;     // overflow intended
              return addCycles;
       }
       else
              return 0;
}

uint8_t ricoh2A03::CPU::BEQ()
{
       if ((STATUS & zeroFlag) && !(operandRef))
       {
              uint8_t addCycles = 1;
              uint8_t operand = mem->cpuRead(operandAddr);
              uint16_t offset = (operand & 0x80)? (0xFF00 | operand) : (0x0000 | operand);
              if (((PC + offset) & 0XFF00) != (PC & 0xFF00))
                     addCycles++;
              PC += offset;     // overflow intended
              return addCycles;
       }
       else
              return 0;
}

uint8_t ricoh2A03::CPU::BMI()
{
       if ((STATUS & negativeFlag) && !(operandRef))
       {
              uint8_t addCycles = 1;
              uint8_t operand = mem->cpuRead(operandAddr);
              uint16_t offset = (operand & 0x80)? (0xFF00 | operand) : (0x0000 | operand);
              if (((PC + offset) & 0XFF00) != (PC & 0xFF00))
                     addCycles++;
              PC += offset;     // overflow intended
              return addCycles;
       }
       else
              return 0;
}

uint8_t ricoh2A03::CPU::BNE()
{
       if ((STATUS & zeroFlag) || operandRef)
              return 0;
       else
       {
              uint8_t addCycles = 1;
              uint8_t operand = mem->cpuRead(operandAddr);
              uint16_t offset = (operand & 0x80)? (0xFF00 | operand) : (0x0000 | operand);
              if (((PC + offset) & 0XFF00) != (PC & 0xFF00))
                     addCycles++;
              PC += offset;     // overflow intended
              return addCycles;
       }
}

uint8_t ricoh2A03::CPU::BPL()
{
       if ((STATUS & negativeFlag) || operandRef)
              return 0;
       else
       {
              uint8_t addCycles = 1;
              uint8_t operand = mem->cpuRead(operandAddr);
              uint16_t offset = (operand & 0x80)? (0xFF00 | operand) : (0x0000 | operand);
              if (((PC + offset) & 0XFF00) != (PC & 0xFF00))
                     addCycles++;
              PC += offset;     // overflow intended
              return addCycles;
       }
}

uint8_t ricoh2A03::CPU::BVC()
{
       if ((STATUS & overflowFlag) || operandRef)
              return 0;
       else
       {
              uint8_t addCycles = 1;
              uint8_t operand = mem->cpuRead(operandAddr);
              uint16_t offset = (operand & 0x80)? (0xFF00 | operand) : (0x0000 | operand);
              if (((PC + offset) & 0XFF00) != (PC & 0xFF00))
                     addCycles++;
              PC += offset;     // overflow intended
              return addCycles;
       }
}

uint8_t ricoh2A03::CPU::BVS()
{
       if ((STATUS & overflowFlag) && !(operandRef))
       {
              uint8_t addCycles = 1;
              uint8_t operand = mem->cpuRead(operandAddr);
              uint16_t offset = (operand & 0x80)? (0xFF00 | operand) : (0x0000 | operand);
              if (((PC + offset) & 0XFF00) != (PC & 0xFF00))
                     addCycles++;
              PC += offset;     // overflow intended
              return addCycles;
       }
       else
              return 0;
}

uint8_t ricoh2A03::CPU::CLC()
{
       STATUS &= ~(carryFlag);
       return 0;
}

uint8_t ricoh2A03::CPU::CLD()
{
       STATUS &= ~(decimalMode);
       return 0;
}

uint8_t ricoh2A03::CPU::CLI()
{
       STATUS &= ~(interruptDisable);
       return 0;
}

uint8_t ricoh2A03::CPU::CLV()
{
       STATUS &= ~(overflowFlag);
       return 0;
}

uint8_t ricoh2A03::CPU::SEC()
{
       STATUS |= carryFlag;
       return 0;
}

uint8_t ricoh2A03::CPU::SED()
{
       STATUS |= decimalMode;
       return 0;
}

uint8_t ricoh2A03::CPU::SEI()
{
       STATUS |= interruptDisable;
       return 0;
}

uint8_t ricoh2A03::CPU::BRK()
{
       PC++;  // RTI will go to the address of the BRK +2
       mem->cpuWrite(0x0100 + SP, (uint8_t)((PC >> 8) & 0x00FF));
       SP--;
       mem->cpuWrite(0x0100 + SP, (uint8_t)(PC & 0x00FF));
       SP--;
       mem->cpuWrite(0x0100 + SP, STATUS);
       SP--;
       PC = (((uint16_t)(mem->cpuRead(0xFFFF)) << 8) | (mem->cpuRead(0xFFFE)));
       STATUS |= breakCommand;
       return 0;
}

uint8_t ricoh2A03::CPU::NOP()
{
       return 0;
}

uint8_t ricoh2A03::CPU::RTI()
{
       SP++;
       STATUS = mem->cpuRead(0x0100 + SP);
       SP++;
       PC = (0x0000 | mem->cpuRead(0x0100 + SP));
       SP++;
       PC |= ((uint16_t)(mem->cpuRead(0x0100 + SP)) << 8);
       #ifdef DEBUG
              if (CPUlog)                               // debug
                     CPUlogfile << " (RTI here)";       // debug
       #endif
       return 0;
}

uint8_t ricoh2A03::CPU::XXX()
{
       return 0;
}


