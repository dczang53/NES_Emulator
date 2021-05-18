#ifndef _GTEST
#define _GTEST

#include "gtest/gtest.h"
#include "../include/Memory.hpp"
#include "../include/Ricoh2A03.hpp"

#include <map>

namespace NES
{
    struct GTESTmemory : public Memory  // used only for gtests; not be used in actual application
    {
        GTESTmemory()
        {
            cpuMemory = new uint8_t[0xFFFF]{0};  // straight-up all cpu memory
        }

        ~GTESTmemory()
        {
            delete[] cpuMemory;
        }

        uint8_t cpuRead(uint16_t addr)
        {
            if (addr <= 0x1FFF)
                return cpuMemory[addr & 0x07FF];
            else if (addr <= 0x3FFF)
                return cpuMemory[(addr & 0x0007) + 0x2000];
            else // if (addr <= 0x401F) and if (addr <= 0xFFFF)
                return cpuMemory[addr];
        }

        bool cpuWrite(uint16_t addr, uint8_t data)
        {
            if (addr <= 0x1FFF)
            {
                cpuMemory[addr & 0x07FF] = data;
                return true;
            }
            else if (addr <= 0x3FFF)
            {
                cpuMemory[(addr & 0x0007) + 0x2000] = data;
                return true;
            }
            else // if (addr <= 0x401F) and if (addr <= 0xFFFF)
            {
                cpuMemory[addr] = data; // ???
                return true;            // ???
            }
        }

        // filler functions as no PPU tests
        uint8_t ppuRead(uint16_t addr) {return 0x00;}
        bool ppuWrite(uint16_t addr, uint8_t data) {return false;}
        void connectCPUandPPU(ricoh2A03::CPU *c, ricoh2C02::PPU *p) {}
        uint8_t controllerRead(uint8_t player) {}
        void controllerWrite(uint8_t player, uint8_t data) {}
        void toggleCpuCycle() {}
        void ppuRequestDMA() {}
        void finalizeDMAreq() {}
        bool DMAactive() {}
        void handleDMA() {}

        uint8_t *cpuMemory = nullptr;   // completely public memory to read/modify during debugging
        // no need for mmc; cpu/ppu only sees memory
    };



    class cpuTest : public ::testing::Test
    {
    public:
        cpuTest()
        {
            mem = new GTESTmemory();
            cpu = new ricoh2A03::CPU(mem);
            cpu->rst();
        }

        ~cpuTest()
        {
            delete cpu;
            delete mem;
        }

        void SetUp(){}
        void TearDown(){}

        struct cpuState
        {
            uint16_t PC;
            uint8_t SP;
            uint8_t ACC;
            uint8_t REGX;
            uint8_t REGY;
            uint8_t STATUS;
        };

        Memory *mem = nullptr;
        ricoh2A03::CPU *cpu = nullptr;

        void test(uint8_t ticks, cpuState initState, std::map<uint16_t, uint8_t> &initMem, cpuState finalState, std::map<uint16_t, uint8_t> &finalMem)
        {
            cpu->rst();
            cpu->PC = initState.PC;
            cpu->SP = initState.SP;
            cpu->ACC = initState.ACC;
            cpu->REGX = initState.REGX;
            cpu->REGY = initState.REGY;
            cpu->STATUS = initState.STATUS;
            for (std::map<uint16_t, uint8_t>::iterator iter = initMem.begin(), end = initMem.end(); iter != end; iter++)
                mem->cpuWrite(iter->first, iter->second);
            while (ticks > 0)
            {
                cpu->tick();
                ticks--;
            }
            EXPECT_EQ(cpu->instrDone(), true);
            EXPECT_EQ(cpu->PC, finalState.PC);
            EXPECT_EQ(cpu->SP, finalState.SP);
            EXPECT_EQ(cpu->ACC, finalState.ACC);
            EXPECT_EQ(cpu->REGX, finalState.REGX);
            EXPECT_EQ(cpu->REGY, finalState.REGY);
            EXPECT_EQ(cpu->STATUS, finalState.STATUS);
            for (std::map<uint16_t, uint8_t>::iterator iter = finalMem.begin(), end = finalMem.end(); iter != end; iter++)
                EXPECT_EQ(mem->cpuRead(iter->first), iter->second);
        }
    };





    // NOTE: tests only include offical opcodes

    TEST_F(cpuTest, testLDA)    // 0xA1 0xA5 0xA9 0xAD 0xB1 0xB5 0xB9 0xBD
    {
        // INDX
        cpuState initStateINDX {0x0000, 0xFF, 0x00, 0x04, 0x00, 0x00};
        std::map<uint16_t, uint8_t> initMemINDX {
            {0x0000, 0xA1}, {0x0001, 0xAA},
            {0x00AE, 0xBB}, {0x00AF, 0xCC},
            {0xCCBB, 0xDD}
        };
        cpuState finalStateINDX {0x0002, 0xFF, 0xDD, 0x04, 0x00, ricoh2A03::negativeFlag};
        std::map<uint16_t, uint8_t> finalMemINDX {};
        test(6, initStateINDX, initMemINDX, finalStateINDX, finalMemINDX);

        // INDX 2 (zeroFlag)
        cpuState initStateINDX2 {0x0000, 0xFF, 0xDD, 0x04, 0x00, 0x00};
        std::map<uint16_t, uint8_t> initMemINDX2 {
            {0x0000, 0xA1}, {0x0001, 0xAA},
            {0x00AE, 0xBB}, {0x00AF, 0xCC},
            {0xCCBB, 0x00}
        };
        cpuState finalStateINDX2 {0x0002, 0xFF, 0x00, 0x04, 0x00, ricoh2A03::zeroFlag};
        std::map<uint16_t, uint8_t> finalMemINDX2 {};
        test(6, initStateINDX2, initMemINDX2, finalStateINDX2, finalMemINDX2);

        // ZP
        cpuState initStateZP {0x0000, 0xFF, 0x00, 0x00, 0x00, 0x00};
        std::map<uint16_t, uint8_t> initMemZP {
            {0x0000, 0xA5}, {0x0001, 0xAA},
            {0x00AA, 0xBB}
        };
        cpuState finalStateZP {0x0002, 0xFF, 0xBB, 0x00, 0x00, ricoh2A03::negativeFlag};
        std::map<uint16_t, uint8_t> finalMemZP {};
        test(3, initStateZP, initMemZP, finalStateZP, finalMemZP);

        // IMM
        cpuState initStateIMM {0x0000, 0xFF, 0x00, 0x00, 0x00, 0x00};
        std::map<uint16_t, uint8_t> initMemIMM {
            {0x0000, 0xA9}, {0x0001, 0xAA}
        };
        cpuState finalStateIMM {0x0002, 0xFF, 0xAA, 0x00, 0x00, ricoh2A03::negativeFlag};
        std::map<uint16_t, uint8_t> finalMemIMM {};
        test(2, initStateIMM, initMemIMM, finalStateIMM, finalMemIMM);

        // ABS
        cpuState initStateABS {0x0000, 0xFF, 0x00, 0x00, 0x00, 0x00};
        std::map<uint16_t, uint8_t> initMemABS {
            {0x0000, 0xAD}, {0x0001, 0xAA}, {0x0002, 0xBB},
            {0xBBAA, 0xCC}
        };
        cpuState finalStateABS {0x0003, 0xFF, 0xCC, 0x00, 0x00, ricoh2A03::negativeFlag};
        std::map<uint16_t, uint8_t> finalMemABS {};
        test(4, initStateABS, initMemABS, finalStateABS, finalMemABS);

        // INDY
        cpuState initStateINDY {0x0000, 0xFF, 0x00, 0x00, 0x03, 0x00};
        std::map<uint16_t, uint8_t> initMemINDY {
            {0x0000, 0xB1}, {0x0001, 0xAA},
            {0x00AA, 0xBB}, {0x00AB, 0xCC},
            {0xCCBE, 0xDD}
        };
        cpuState finalStateINDY {0x0002, 0xFF, 0xDD, 0x00, 0x03, ricoh2A03::negativeFlag};
        std::map<uint16_t, uint8_t> finalMemINDY {};
        test(5, initStateINDY, initMemINDY, finalStateINDY, finalMemINDY);

        // INDY (page cross delay +1)
        cpuState initStateINDY2 {0x0000, 0xFF, 0x00, 0x00, 0x46, 0x00};
        std::map<uint16_t, uint8_t> initMemINDY2 {
            {0x0000, 0xB1}, {0x0001, 0xAA},
            {0x00AA, 0xBB}, {0x00AB, 0xCC},
            {0xCD01, 0xDD}
        };
        cpuState finalStateINDY2 {0x0002, 0xFF, 0xDD, 0x00, 0x46, ricoh2A03::negativeFlag};
        std::map<uint16_t, uint8_t> finalMemINDY2 {};
        test(6, initStateINDY2, initMemINDY2, finalStateINDY2, finalMemINDY2);

        // ZPX (also checks for no page cross with indexing)
        cpuState initStateZPX {0x0000, 0xFF, 0x00, 0x15, 0x00, 0x00};
        std::map<uint16_t, uint8_t> initMemZPX {
            {0x0000, 0xB5}, {0x0001, 0xFF},
            {0x0014, 0xBB}
        };
        cpuState finalStateZPX {0x0002, 0xFF, 0xBB, 0x15, 0x00, ricoh2A03::negativeFlag};
        std::map<uint16_t, uint8_t> finalMemZPX {};
        test(4, initStateZPX, initMemZPX, finalStateZPX, finalMemZPX);

        // ABSY
        cpuState initStateABSY {0x0000, 0xFF, 0x00, 0x00, 0x04, 0x00};
        std::map<uint16_t, uint8_t> initMemABSY {
            {0x0000, 0xB9}, {0x0001, 0xBB}, {0x0002, 0xAA},
            {0xAABF, 0xCC}
        };
        cpuState finalStateABSY {0x0003, 0xFF, 0xCC, 0x00, 0x04, ricoh2A03::negativeFlag};
        std::map<uint16_t, uint8_t> finalMemABSY {};
        test(4, initStateABSY, initMemABSY, finalStateABSY, finalMemABSY);

        // ABSY (page cross delay +1)
        cpuState initStateABSY2 {0x0000, 0xFF, 0x00, 0x00, 0x46, 0x00};
        std::map<uint16_t, uint8_t> initMemABSY2 {
            {0x0000, 0xB9}, {0x0001, 0xBB}, {0x0002, 0xAA},
            {0xAB01, 0xDD}
        };
        cpuState finalStateABSY2 {0x0003, 0xFF, 0xDD, 0x00, 0x46, ricoh2A03::negativeFlag};
        std::map<uint16_t, uint8_t> finalMemABSY2 {};
        test(5, initStateABSY2, initMemABSY2, finalStateABSY2, finalMemABSY2);

        // ABSX
        cpuState initStateABSX {0x0000, 0xFF, 0x00, 0x03, 0x00, 0x00};
        std::map<uint16_t, uint8_t> initMemABSX {
            {0x0000, 0xBD}, {0x0001, 0xAA}, {0x0002, 0xBB},
            {0xBBAD, 0xEE}
        };
        cpuState finalStateABSX {0x0003, 0xFF, 0xEE, 0x03, 0x00, ricoh2A03::negativeFlag};
        std::map<uint16_t, uint8_t> finalMemABSX {};
        test(4, initStateABSX, initMemABSX, finalStateABSX, finalMemABSX);

        // ABSX (page cross delay +1)
        cpuState initStateABSX2 {0x0000, 0xFF, 0x00, 0x64, 0x00, 0x00};
        std::map<uint16_t, uint8_t> initMemABSX2 {
            {0x0000, 0xBD}, {0x0001, 0xAA}, {0x0002, 0xBB},
            {0xBC0E, 0xFF}
        };
        cpuState finalStateABSX2 {0x0003, 0xFF, 0xFF, 0x64, 0x00, ricoh2A03::negativeFlag};
        std::map<uint16_t, uint8_t> finalMemABSX2 {};
        test(5, initStateABSX2, initMemABSX2, finalStateABSX2, finalMemABSX2);
    }



    TEST_F(cpuTest, testLDX)    // 0xA2 0xA6 0xAE 0xB6 0xBE
    {
        // IMM
        cpuState initStateIMM {0x0000, 0xFF, 0x00, 0x00, 0x00, 0x00};
        std::map<uint16_t, uint8_t> initMemIMM {
            {0x0000, 0xA2}, {0x0001, 0xAA}
        };
        cpuState finalStateIMM {0x0002, 0xFF, 0x00, 0xAA, 0x00, ricoh2A03::negativeFlag};
        std::map<uint16_t, uint8_t> finalMemIMM {};
        test(2, initStateIMM, initMemIMM, finalStateIMM, finalMemIMM);

        // IMM (zeroFlag)
        cpuState initStateIMM2 {0x0000, 0xFF, 0x00, 0x00, 0x00, 0x00};
        std::map<uint16_t, uint8_t> initMemIMM2 {
            {0x0000, 0xA2}, {0x0001, 0xAA}
        };
        cpuState finalStateIMM2 {0x0002, 0xFF, 0x00, 0xAA, 0x00, ricoh2A03::negativeFlag};
        std::map<uint16_t, uint8_t> finalMemIMM2 {};
        test(2, initStateIMM2, initMemIMM2, finalStateIMM2, finalMemIMM2);

        // ZP
        cpuState initStateZP {0x0000, 0xFF, 0x00, 0x00, 0x00, 0x00};
        std::map<uint16_t, uint8_t> initMemZP {
            {0x0000, 0xA6}, {0x0001, 0xAA},
            {0x00AA, 0xBB}
        };
        cpuState finalStateZP {0x0002, 0xFF, 0x00, 0xBB, 0x00, ricoh2A03::negativeFlag};
        std::map<uint16_t, uint8_t> finalMemZP {};
        test(3, initStateZP, initMemZP, finalStateZP, finalMemZP);

        // ABS
        cpuState initStateABS {0x0000, 0xFF, 0x00, 0x00, 0x00, 0x00};
        std::map<uint16_t, uint8_t> initMemABS {
            {0x0000, 0xAE}, {0x0001, 0xAA}, {0x0002, 0xBB},
            {0xBBAA, 0xCC}
        };
        cpuState finalStateABS {0x0003, 0xFF, 0x00, 0xCC, 0x00, ricoh2A03::negativeFlag};
        std::map<uint16_t, uint8_t> finalMemABS {};
        test(4, initStateABS, initMemABS, finalStateABS, finalMemABS);

        // ZPY (also checks for no page cross with indexing)
        cpuState initStateZPY {0x0000, 0xFF, 0x00, 0x00, 0x15, 0x00};
        std::map<uint16_t, uint8_t> initMemZPY {
            {0x0000, 0xB6}, {0x0001, 0xFF},
            {0x0014, 0xBB}
        };
        cpuState finalStateZPY {0x0002, 0xFF, 0x00, 0xBB, 0x15, ricoh2A03::negativeFlag};
        std::map<uint16_t, uint8_t> finalMemZPY {};
        test(4, initStateZPY, initMemZPY, finalStateZPY, finalMemZPY);

        // ABSY
        cpuState initStateABSY {0x0000, 0xFF, 0x00, 0x00, 0x04, 0x00};
        std::map<uint16_t, uint8_t> initMemABSY {
            {0x0000, 0xBE}, {0x0001, 0xBB}, {0x0002, 0xAA},
            {0xAABF, 0xCC}
        };
        cpuState finalStateABSY {0x0003, 0xFF, 0x00, 0xCC, 0x04, ricoh2A03::negativeFlag};
        std::map<uint16_t, uint8_t> finalMemABSY {};
        test(4, initStateABSY, initMemABSY, finalStateABSY, finalMemABSY);

        // ABSY (page cross delay +1)
        cpuState initStateABSY2 {0x0000, 0xFF, 0x00, 0x00, 0x46, 0x00};
        std::map<uint16_t, uint8_t> initMemABSY2 {
            {0x0000, 0xBE}, {0x0001, 0xBB}, {0x0002, 0xAA},
            {0xAB01, 0xDD}
        };
        cpuState finalStateABSY2 {0x0003, 0xFF, 0x00, 0xDD, 0x46, ricoh2A03::negativeFlag};
        std::map<uint16_t, uint8_t> finalMemABSY2 {};
        test(5, initStateABSY2, initMemABSY2, finalStateABSY2, finalMemABSY2);
    }



    TEST_F(cpuTest, testLDY)    // 0xA0 0xA4 0xAC 0xB4 0xBC
    {
        // IMM
        cpuState initStateIMM {0x0000, 0xFF, 0x00, 0x00, 0x00, 0x00};
        std::map<uint16_t, uint8_t> initMemIMM {
            {0x0000, 0xA0}, {0x0001, 0xAA}
        };
        cpuState finalStateIMM {0x0002, 0xFF, 0x00, 0x00, 0xAA, ricoh2A03::negativeFlag};
        std::map<uint16_t, uint8_t> finalMemIMM {};
        test(2, initStateIMM, initMemIMM, finalStateIMM, finalMemIMM);

        // IMM 2 (zeroFlag)
        cpuState initStateIMM2 {0x0000, 0xFF, 0x00, 0x00, 0xBB, 0x00};
        std::map<uint16_t, uint8_t> initMemIMM2 {
            {0x0000, 0xA0}, {0x0001, 0x00}
        };
        cpuState finalStateIMM2 {0x0002, 0xFF, 0x00, 0x00, 0x00, ricoh2A03::zeroFlag};
        std::map<uint16_t, uint8_t> finalMemIMM2 {};
        test(2, initStateIMM2, initMemIMM2, finalStateIMM2, finalMemIMM2);

        // ZP
        cpuState initStateZP {0x0000, 0xFF, 0x00, 0x00, 0x00, 0x00};
        std::map<uint16_t, uint8_t> initMemZP {
            {0x0000, 0xA4}, {0x0001, 0xAA},
            {0x00AA, 0xBB}
        };
        cpuState finalStateZP {0x0002, 0xFF, 0x00, 0x00, 0xBB, ricoh2A03::negativeFlag};
        std::map<uint16_t, uint8_t> finalMemZP {};
        test(3, initStateZP, initMemZP, finalStateZP, finalMemZP);

        // ABS
        cpuState initStateABS {0x0000, 0xFF, 0x00, 0x00, 0x00, 0x00};
        std::map<uint16_t, uint8_t> initMemABS {
            {0x0000, 0xAC}, {0x0001, 0xAA}, {0x0002, 0xBB},
            {0xBBAA, 0xCC}
        };
        cpuState finalStateABS {0x0003, 0xFF, 0x00, 0x00, 0xCC, ricoh2A03::negativeFlag};
        std::map<uint16_t, uint8_t> finalMemABS {};
        test(4, initStateABS, initMemABS, finalStateABS, finalMemABS);

        // ZPX (also checks for no page cross with indexing)
        cpuState initStateZPX {0x0000, 0xFF, 0x00, 0x15, 0x00, 0x00};
        std::map<uint16_t, uint8_t> initMemZPX {
            {0x0000, 0xB4}, {0x0001, 0xFF},
            {0x0014, 0xBB}
        };
        cpuState finalStateZPX {0x0002, 0xFF, 0x00, 0x15, 0xBB, ricoh2A03::negativeFlag};
        std::map<uint16_t, uint8_t> finalMemZPX {};
        test(4, initStateZPX, initMemZPX, finalStateZPX, finalMemZPX);

        // ABSX
        cpuState initStateABSX {0x0000, 0xFF, 0x00, 0x03, 0x00, 0x00};
        std::map<uint16_t, uint8_t> initMemABSX {
            {0x0000, 0xBC}, {0x0001, 0xAA}, {0x0002, 0xBB},
            {0xBBAD, 0xEE}
        };
        cpuState finalStateABSX {0x0003, 0xFF, 0x00, 0x03, 0xEE, ricoh2A03::negativeFlag};
        std::map<uint16_t, uint8_t> finalMemABSX {};
        test(4, initStateABSX, initMemABSX, finalStateABSX, finalMemABSX);

        // ABSX (page cross delay +1)
        cpuState initStateABSX2 {0x0000, 0xFF, 0x00, 0x64, 0x00, 0x00};
        std::map<uint16_t, uint8_t> initMemABSX2 {
            {0x0000, 0xBC}, {0x0001, 0xAA}, {0x0002, 0xBB},
            {0xBC0E, 0xFF}
        };
        cpuState finalStateABSX2 {0x0003, 0xFF, 0x00, 0x64, 0xFF, ricoh2A03::negativeFlag};
        std::map<uint16_t, uint8_t> finalMemABSX2 {};
        test(5, initStateABSX2, initMemABSX2, finalStateABSX2, finalMemABSX2);
    }



    TEST_F(cpuTest, testSTA)    // 0x81 0x85 0x8D 0x91 0x95 0x99 0x9D
    {
        // INDX
        cpuState initStateINDX {0x0000, 0xFF, 0xDD, 0x04, 0x00, 0x00};
        std::map<uint16_t, uint8_t> initMemINDX {
            {0x0000, 0x81}, {0x0001, 0xAA},
            {0x00AE, 0xBB}, {0x00AF, 0xCC},
            {0xCCBB, 0x00}
        };
        cpuState finalStateINDX {0x0002, 0xFF, 0xDD, 0x04, 0x00, 0x00};
        std::map<uint16_t, uint8_t> finalMemINDX {
            {0xCCBB, 0xDD}
        };
        test(6, initStateINDX, initMemINDX, finalStateINDX, finalMemINDX);

        // ZP
        cpuState initStateZP {0x0000, 0xFF, 0xBB, 0x00, 0x00, 0x00};
        std::map<uint16_t, uint8_t> initMemZP {
            {0x0000, 0x85}, {0x0001, 0xAA},
            {0x00AA, 0x00}
        };
        cpuState finalStateZP {0x0002, 0xFF, 0xBB, 0x00, 0x00, 0x00};
        std::map<uint16_t, uint8_t> finalMemZP {
            {0x00AA, 0xBB}
        };
        test(3, initStateZP, initMemZP, finalStateZP, finalMemZP);

        // ABS
        cpuState initStateABS {0x0000, 0xFF, 0xCC, 0x00, 0x00, 0x00};
        std::map<uint16_t, uint8_t> initMemABS {
            {0x0000, 0x8D}, {0x0001, 0xAA}, {0x0002, 0xBB},
            {0xBBAA, 0x00}
        };
        cpuState finalStateABS {0x0003, 0xFF, 0xCC, 0x00, 0x00, 0x00};
        std::map<uint16_t, uint8_t> finalMemABS {
            {0xBBAA, 0xCC}
        };
        test(4, initStateABS, initMemABS, finalStateABS, finalMemABS);

        // INDY
        cpuState initStateINDY {0x0000, 0xFF, 0xDD, 0x00, 0x03, 0x00};
        std::map<uint16_t, uint8_t> initMemINDY {
            {0x0000, 0x91}, {0x0001, 0xAA},
            {0x00AA, 0xBB}, {0x00AB, 0xCC},
            {0xCCBE, 0x00}
        };
        cpuState finalStateINDY {0x0002, 0xFF, 0xDD, 0x00, 0x03, 0x00};
        std::map<uint16_t, uint8_t> finalMemINDY {
            {0xCCBE, 0xDD}
        };
        test(6, initStateINDY, initMemINDY, finalStateINDY, finalMemINDY);

        // INDY (NO page cross delay +1)
        cpuState initStateINDY2 {0x0000, 0xFF, 0xDD, 0x00, 0x46, 0x00};
        std::map<uint16_t, uint8_t> initMemINDY2 {
            {0x0000, 0x91}, {0x0001, 0xAA},
            {0x00AA, 0xBB}, {0x00AB, 0xCC},
            {0xCD01, 0x00}
        };
        cpuState finalStateINDY2 {0x0002, 0xFF, 0xDD, 0x00, 0x46, 0x00};
        std::map<uint16_t, uint8_t> finalMemINDY2 {
            {0xCD01, 0xDD}
        };
        test(6, initStateINDY2, initMemINDY2, finalStateINDY2, finalMemINDY2);

        // ZPX (also checks for no page cross with indexing)
        cpuState initStateZPX {0x0000, 0xFF, 0xBB, 0x15, 0x00, 0x00};
        std::map<uint16_t, uint8_t> initMemZPX {
            {0x0000, 0x95}, {0x0001, 0xFF},
            {0x0014, 0x00}
        };
        cpuState finalStateZPX {0x0002, 0xFF, 0xBB, 0x15, 0x00, 0x00};
        std::map<uint16_t, uint8_t> finalMemZPX {
            {0x0014, 0xBB}
        };
        test(4, initStateZPX, initMemZPX, finalStateZPX, finalMemZPX);

        // ABSY
        cpuState initStateABSY {0x0000, 0xFF, 0xCC, 0x00, 0x04, 0x00};
        std::map<uint16_t, uint8_t> initMemABSY {
            {0x0000, 0x99}, {0x0001, 0xBB}, {0x0002, 0xAA},
            {0xAABF, 0x00}
        };
        cpuState finalStateABSY {0x0003, 0xFF, 0xCC, 0x00, 0x04, 0x00};
        std::map<uint16_t, uint8_t> finalMemABSY {
            {0xAABF, 0xCC}
        };
        test(5, initStateABSY, initMemABSY, finalStateABSY, finalMemABSY);

        // ABSY (NO page cross delay +1)
        cpuState initStateABSY2 {0x0000, 0xFF, 0xDD, 0x00, 0x46, 0x00};
        std::map<uint16_t, uint8_t> initMemABSY2 {
            {0x0000, 0x99}, {0x0001, 0xBB}, {0x0002, 0xAA},
            {0xAB01, 0x00}
        };
        cpuState finalStateABSY2 {0x0003, 0xFF, 0xDD, 0x00, 0x46, 0x00};
        std::map<uint16_t, uint8_t> finalMemABSY2 {
            {0xAB01, 0xDD}
        };
        test(5, initStateABSY2, initMemABSY2, finalStateABSY2, finalMemABSY2);

        // ABSX
        cpuState initStateABSX {0x0000, 0xFF, 0xEE, 0x03, 0x00, 0x00};
        std::map<uint16_t, uint8_t> initMemABSX {
            {0x0000, 0x9D}, {0x0001, 0xAA}, {0x0002, 0xBB},
            {0xBBAD, 0x00}
        };
        cpuState finalStateABSX {0x0003, 0xFF, 0xEE, 0x03, 0x00, 0x00};
        std::map<uint16_t, uint8_t> finalMemABSX {
            {0xBBAD, 0xEE}
        };
        test(5, initStateABSX, initMemABSX, finalStateABSX, finalMemABSX);

        // ABSX (NO page cross delay +1)
        cpuState initStateABSX2 {0x0000, 0xFF, 0xFF, 0x64, 0x00, 0x00};
        std::map<uint16_t, uint8_t> initMemABSX2 {
            {0x0000, 0x9D}, {0x0001, 0xAA}, {0x0002, 0xBB},
            {0xBC0E, 0x00}
        };
        cpuState finalStateABSX2 {0x0003, 0xFF, 0xFF, 0x64, 0x00, 0x00};
        std::map<uint16_t, uint8_t> finalMemABSX2 {
            {0xBC0E, 0xFF}
        };
        test(5, initStateABSX2, initMemABSX2, finalStateABSX2, finalMemABSX2);
    }



    TEST_F(cpuTest, testSTX)    // 0x86 0x8E 0x96
    {
        // ZP
        cpuState initStateZP {0x0000, 0xFF, 0x00, 0xBB, 0x00, 0x00};
        std::map<uint16_t, uint8_t> initMemZP {
            {0x0000, 0x86}, {0x0001, 0xAA},
            {0x00AA, 0x00}
        };
        cpuState finalStateZP {0x0002, 0xFF, 0x00, 0xBB, 0x00, 0x00};
        std::map<uint16_t, uint8_t> finalMemZP {
            {0x00AA, 0xBB}
        };
        test(3, initStateZP, initMemZP, finalStateZP, finalMemZP);

        // ABS
        cpuState initStateABS {0x0000, 0xFF, 0x00, 0xCC, 0x00, 0x00};
        std::map<uint16_t, uint8_t> initMemABS {
            {0x0000, 0x8E}, {0x0001, 0xAA}, {0x0002, 0xBB},
            {0xBBAA, 0x00}
        };
        cpuState finalStateABS {0x0003, 0xFF, 0x00, 0xCC, 0x00, 0x00};
        std::map<uint16_t, uint8_t> finalMemABS {
            {0xBBAA, 0xCC}
        };
        test(4, initStateABS, initMemABS, finalStateABS, finalMemABS);

        // ZPY (also checks for no page cross with indexing)
        cpuState initStateZPY {0x0000, 0xFF, 0x00, 0xBB, 0x15, 0x00};
        std::map<uint16_t, uint8_t> initMemZPY {
            {0x0000, 0x96}, {0x0001, 0xFF},
            {0x0014, 0x00}
        };
        cpuState finalStateZPY {0x0002, 0xFF, 0x00, 0xBB, 0x15, 0x00};
        std::map<uint16_t, uint8_t> finalMemZPY {
            {0x0014, 0xBB}
        };
        test(4, initStateZPY, initMemZPY, finalStateZPY, finalMemZPY);
    }



    TEST_F(cpuTest, testSTY)    // 0x84 0x8C 0x94
    {
        
        // ZP
        cpuState initStateZP {0x0000, 0xFF, 0x00, 0x00, 0xBB, 0x00};
        std::map<uint16_t, uint8_t> initMemZP {
            {0x0000, 0x84}, {0x0001, 0xAA},
            {0x00AA, 0x00}
        };
        cpuState finalStateZP {0x0002, 0xFF, 0x00, 0x00, 0xBB, 0x00};
        std::map<uint16_t, uint8_t> finalMemZP {
            {0x00AA, 0xBB}
        };
        test(3, initStateZP, initMemZP, finalStateZP, finalMemZP);

        // ABS
        cpuState initStateABS {0x0000, 0xFF, 0x00, 0x00, 0xCC, 0x00};
        std::map<uint16_t, uint8_t> initMemABS {
            {0x0000, 0x8C}, {0x0001, 0xAA}, {0x0002, 0xBB},
            {0xBBAA, 0x00}
        };
        cpuState finalStateABS {0x0003, 0xFF, 0x00, 0x00, 0xCC, 0x00};
        std::map<uint16_t, uint8_t> finalMemABS {
            {0xBBAA, 0xCC}
        };
        test(4, initStateABS, initMemABS, finalStateABS, finalMemABS);

        // ZPY (also checks for no page cross with indexing)
        cpuState initStateZPY {0x0000, 0xFF, 0x00, 0x15, 0xBB, 0x00};
        std::map<uint16_t, uint8_t> initMemZPY {
            {0x0000, 0x94}, {0x0001, 0xFF},
            {0x0014, 0x00}
        };
        cpuState finalStateZPY {0x0002, 0xFF, 0x00, 0x15, 0xBB, 0x00};
        std::map<uint16_t, uint8_t> finalMemZPY {
            {0x0014, 0xBB}
        };
        test(4, initStateZPY, initMemZPY, finalStateZPY, finalMemZPY);
    }



    TEST_F(cpuTest, testTAX)    // 0xAA
    {
        // IMP (negativeFlag)
        cpuState initStateIMP {0x0000, 0xFF, 0xCC, 0xBB, 0xAA, 0x00};
        std::map<uint16_t, uint8_t> initMemIMP {
            {0x0000, 0xAA}
        };
        cpuState finalStateIMP {0x0001, 0xFF, 0xCC, 0xCC, 0xAA, ricoh2A03::negativeFlag};
        std::map<uint16_t, uint8_t> finalMemIMP {};
        test(2, initStateIMP, initMemIMP, finalStateIMP, finalMemIMP);

        
        // IMP (zeroFlag)
        cpuState initStateIMP2 {0x0000, 0xFF, 0x00, 0xBB, 0xAA, 0x00};
        std::map<uint16_t, uint8_t> initMemIMP2 {
            {0x0000, 0xAA}
        };
        cpuState finalStateIMP2 {0x0001, 0xFF, 0x00, 0x00, 0xAA, ricoh2A03::zeroFlag};
        std::map<uint16_t, uint8_t> finalMemIMP2 {};
        test(2, initStateIMP2, initMemIMP2, finalStateIMP2, finalMemIMP2);
    }



    TEST_F(cpuTest, testTAY)    // 0xA8
    {
        // IMP (negativeFlag)
        cpuState initStateIMP {0x0000, 0xFF, 0xCC, 0xBB, 0xAA, 0x00};
        std::map<uint16_t, uint8_t> initMemIMP {
            {0x0000, 0xA8}
        };
        cpuState finalStateIMP {0x0001, 0xFF, 0xCC, 0xBB, 0xCC, ricoh2A03::negativeFlag};
        std::map<uint16_t, uint8_t> finalMemIMP {};
        test(2, initStateIMP, initMemIMP, finalStateIMP, finalMemIMP);

        
        // IMP (zeroFlag)
        cpuState initStateIMP2 {0x0000, 0xFF, 0x00, 0xBB, 0xAA, 0x00};
        std::map<uint16_t, uint8_t> initMemIMP2 {
            {0x0000, 0xA8}
        };
        cpuState finalStateIMP2 {0x0001, 0xFF, 0x00, 0xBB, 0x00, ricoh2A03::zeroFlag};
        std::map<uint16_t, uint8_t> finalMemIMP2 {};
        test(2, initStateIMP2, initMemIMP2, finalStateIMP2, finalMemIMP2);
    }



    TEST_F(cpuTest, testTXA)    // 0x8A
    {
        // IMP (negativeFlag)
        cpuState initStateIMP {0x0000, 0xFF, 0xCC, 0xBB, 0xAA, 0x00};
        std::map<uint16_t, uint8_t> initMemIMP {
            {0x0000, 0x8A}
        };
        cpuState finalStateIMP {0x0001, 0xFF, 0xBB, 0xBB, 0xAA, ricoh2A03::negativeFlag};
        std::map<uint16_t, uint8_t> finalMemIMP {};
        test(2, initStateIMP, initMemIMP, finalStateIMP, finalMemIMP);

        
        // IMP (zeroFlag)
        cpuState initStateIMP2 {0x0000, 0xFF, 0xBB, 0x00, 0xAA, 0x00};
        std::map<uint16_t, uint8_t> initMemIMP2 {
            {0x0000, 0x8A}
        };
        cpuState finalStateIMP2 {0x0001, 0xFF, 0x00, 0x00, 0xAA, ricoh2A03::zeroFlag};
        std::map<uint16_t, uint8_t> finalMemIMP2 {};
        test(2, initStateIMP2, initMemIMP2, finalStateIMP2, finalMemIMP2);
    }



    TEST_F(cpuTest, testTYA)    // 0x98
    {
        // IMP (negativeFlag)
        cpuState initStateIMP {0x0000, 0xFF, 0xCC, 0xBB, 0xAA, 0x00};
        std::map<uint16_t, uint8_t> initMemIMP {
            {0x0000, 0x98}
        };
        cpuState finalStateIMP {0x0001, 0xFF, 0xAA, 0xBB, 0xAA, ricoh2A03::negativeFlag};
        std::map<uint16_t, uint8_t> finalMemIMP {};
        test(2, initStateIMP, initMemIMP, finalStateIMP, finalMemIMP);

        
        // IMP (zeroFlag)
        cpuState initStateIMP2 {0x0000, 0xFF, 0xBB, 0xAA, 0x00, 0x00};
        std::map<uint16_t, uint8_t> initMemIMP2 {
            {0x0000, 0x98}
        };
        cpuState finalStateIMP2 {0x0001, 0xFF, 0x00, 0xAA, 0x00, ricoh2A03::zeroFlag};
        std::map<uint16_t, uint8_t> finalMemIMP2 {};
        test(2, initStateIMP2, initMemIMP2, finalStateIMP2, finalMemIMP2);
    }



    TEST_F(cpuTest, testTSX)    // 0xBA
    {
        // IMP (negativeFlag)
        cpuState initStateIMP {0x0000, 0xFF, 0xCC, 0xBB, 0xAA, 0x00};
        std::map<uint16_t, uint8_t> initMemIMP {
            {0x0000, 0xBA}
        };
        cpuState finalStateIMP {0x0001, 0xFF, 0xCC, 0xFF, 0xAA, ricoh2A03::negativeFlag};
        std::map<uint16_t, uint8_t> finalMemIMP {};
        test(2, initStateIMP, initMemIMP, finalStateIMP, finalMemIMP);

        
        // IMP (zeroFlag)
        cpuState initStateIMP2 {0x0000, 0x00, 0xCC, 0xBB, 0xAA, 0x00};
        std::map<uint16_t, uint8_t> initMemIMP2 {
            {0x0000, 0xBA}
        };
        cpuState finalStateIMP2 {0x0001, 0x00, 0xCC, 0x00, 0xAA, ricoh2A03::zeroFlag};
        std::map<uint16_t, uint8_t> finalMemIMP2 {};
        test(2, initStateIMP2, initMemIMP2, finalStateIMP2, finalMemIMP2);
    }



    TEST_F(cpuTest, testTXS)    // 0x9A
    {
        // IMP
        cpuState initStateIMP {0x0000, 0xFF, 0xCC, 0xBB, 0xAA, 0x00};
        std::map<uint16_t, uint8_t> initMemIMP {
            {0x0000, 0x9A}
        };
        cpuState finalStateIMP {0x0001, 0xBB, 0xCC, 0xBB, 0xAA, 0x00};
        std::map<uint16_t, uint8_t> finalMemIMP {};
        test(2, initStateIMP, initMemIMP, finalStateIMP, finalMemIMP);
    }



    TEST_F(cpuTest, testPHA)    // 0x48
    {
        // IMP
        cpuState initStateIMP {0x0000, 0xFF, 0xCC, 0xBB, 0xAA, 0x00};
        std::map<uint16_t, uint8_t> initMemIMP {
            {0x0000, 0x48}
        };
        cpuState finalStateIMP {0x0001, 0xFE, 0xCC, 0xBB, 0xAA, 0x00};
        std::map<uint16_t, uint8_t> finalMemIMP {
            {0x01FF, 0xCC}
        };
        test(3, initStateIMP, initMemIMP, finalStateIMP, finalMemIMP);
    }



    TEST_F(cpuTest, testPHP)    // 0x08
    {
        // IMP
        cpuState initStateIMP {0x0000, 0xFF, 0xCC, 0xBB, 0xAA, 0xEE};
        std::map<uint16_t, uint8_t> initMemIMP {
            {0x0000, 0x08}
        };
        cpuState finalStateIMP {0x0001, 0xFE, 0xCC, 0xBB, 0xAA, 0xEE};
        std::map<uint16_t, uint8_t> finalMemIMP {
            {0x01FF, 0xEE}
        };
        test(3, initStateIMP, initMemIMP, finalStateIMP, finalMemIMP);
    }



    TEST_F(cpuTest, testPLA)    // 0x68
    {
        // IMP (negativeFlag)
        cpuState initStateIMP {0x0000, 0xFD, 0xCC, 0xBB, 0xAA, 0x00};
        std::map<uint16_t, uint8_t> initMemIMP {
            {0x0000, 0x68},
            {0x01FE, 0x99}
        };
        cpuState finalStateIMP {0x0001, 0xFE, 0x99, 0xBB, 0xAA, ricoh2A03::negativeFlag};
        std::map<uint16_t, uint8_t> finalMemIMP {};
        test(4, initStateIMP, initMemIMP, finalStateIMP, finalMemIMP);

        // IMP (zeroFlag)
        cpuState initStateIMP2 {0x0000, 0xFD, 0xCC, 0xBB, 0xAA, 0x00};
        std::map<uint16_t, uint8_t> initMemIMP2 {
            {0x0000, 0x68},
            {0x01FE, 0x00}
        };
        cpuState finalStateIMP2 {0x0001, 0xFE, 0x00, 0xBB, 0xAA, ricoh2A03::zeroFlag};
        std::map<uint16_t, uint8_t> finalMemIMP2 {};
        test(4, initStateIMP2, initMemIMP2, finalStateIMP2, finalMemIMP2);
    }



    TEST_F(cpuTest, testPLP)    // 0x28
    {
        // IMP
        cpuState initStateIMP {0x0000, 0xFD, 0xCC, 0xBB, 0xAA, 0xDD};
        std::map<uint16_t, uint8_t> initMemIMP {
            {0x0000, 0x28},
            {0x01FE, 0xEE}
        };
        cpuState finalStateIMP {0x0001, 0xFE, 0xCC, 0xBB, 0xAA, 0xEE};
        std::map<uint16_t, uint8_t> finalMemIMP {};
        test(4, initStateIMP, initMemIMP, finalStateIMP, finalMemIMP);
    }



    TEST_F(cpuTest, testAND)    // 0x21 0x25 0x29 0x2D 0x31 0x35 0x39 0x3D
    {
        // INDX (negativeFlag)
        cpuState initStateINDX {0x0000, 0xFF, 0xB1, 0x04, 0x00, 0x00};
        std::map<uint16_t, uint8_t> initMemINDX {
            {0x0000, 0x21}, {0x0001, 0xAA},
            {0x00AE, 0xBB}, {0x00AF, 0xCC},
            {0xCCBB, 0x8A}
        };
        cpuState finalStateINDX {0x0002, 0xFF, 0x80, 0x04, 0x00, ricoh2A03::negativeFlag};
        std::map<uint16_t, uint8_t> finalMemINDX {};
        test(6, initStateINDX, initMemINDX, finalStateINDX, finalMemINDX);

        // INDX (zeroFlag)
        cpuState initStateINDX2 {0x0000, 0xFF, 0xFF, 0x04, 0x00, 0x00};
        std::map<uint16_t, uint8_t> initMemINDX2 {
            {0x0000, 0x21}, {0x0001, 0xAA},
            {0x00AE, 0xBB}, {0x00AF, 0xCC},
            {0xCCBB, 0x00}
        };
        cpuState finalStateINDX2 {0x0002, 0xFF, 0x00, 0x04, 0x00, ricoh2A03::zeroFlag};
        std::map<uint16_t, uint8_t> finalMemINDX2 {};
        test(6, initStateINDX2, initMemINDX2, finalStateINDX2, finalMemINDX2);
        
        // ZP
        cpuState initStateZP {0x0000, 0xFF, 0xB1, 0x00, 0x00, 0x00};
        std::map<uint16_t, uint8_t> initMemZP {
            {0x0000, 0x25}, {0x0001, 0xAA},
            {0x00AA, 0x8A}
        };
        cpuState finalStateZP {0x0002, 0xFF, 0x80, 0x00, 0x00, ricoh2A03::negativeFlag};
        std::map<uint16_t, uint8_t> finalMemZP {};
        test(3, initStateZP, initMemZP, finalStateZP, finalMemZP);

        // IMM
        cpuState initStateIMM {0x0000, 0xFF, 0xB1, 0x00, 0x00, 0x00};
        std::map<uint16_t, uint8_t> initMemIMM {
            {0x0000, 0x29}, {0x0001, 0x8A}
        };
        cpuState finalStateIMM {0x0002, 0xFF, 0x80, 0x00, 0x00, ricoh2A03::negativeFlag};
        std::map<uint16_t, uint8_t> finalMemIMM {};
        test(2, initStateIMM, initMemIMM, finalStateIMM, finalMemIMM);

        // ABS
        cpuState initStateABS {0x0000, 0xFF, 0xB1, 0x00, 0x00, 0x00};
        std::map<uint16_t, uint8_t> initMemABS {
            {0x0000, 0x2D}, {0x0001, 0xAA}, {0x0002, 0xBB},
            {0xBBAA, 0x8A}
        };
        cpuState finalStateABS {0x0003, 0xFF, 0x80, 0x00, 0x00, ricoh2A03::negativeFlag};
        std::map<uint16_t, uint8_t> finalMemABS {};
        test(4, initStateABS, initMemABS, finalStateABS, finalMemABS);

        // INDY
        cpuState initStateINDY {0x0000, 0xFF, 0xB1, 0x00, 0x03, 0x00};
        std::map<uint16_t, uint8_t> initMemINDY {
            {0x0000, 0x31}, {0x0001, 0xAA},
            {0x00AA, 0xBB}, {0x00AB, 0xCC},
            {0xCCBE, 0x8A}
        };
        cpuState finalStateINDY {0x0002, 0xFF, 0x80, 0x00, 0x03, ricoh2A03::negativeFlag};
        std::map<uint16_t, uint8_t> finalMemINDY {};
        test(5, initStateINDY, initMemINDY, finalStateINDY, finalMemINDY);

        // INDY (page cross delay +1)
        cpuState initStateINDY2 {0x0000, 0xFF, 0xB1, 0x00, 0x46, 0x00};
        std::map<uint16_t, uint8_t> initMemINDY2 {
            {0x0000, 0x31}, {0x0001, 0xAA},
            {0x00AA, 0xBB}, {0x00AB, 0xCC},
            {0xCD01, 0x8A}
        };
        cpuState finalStateINDY2 {0x0002, 0xFF, 0x80, 0x00, 0x46, ricoh2A03::negativeFlag};
        std::map<uint16_t, uint8_t> finalMemINDY2 {};
        test(6, initStateINDY2, initMemINDY2, finalStateINDY2, finalMemINDY2);

        // ZPX (also checks for no page cross with indexing)
        cpuState initStateZPX {0x0000, 0xFF, 0xB1, 0x15, 0x00, 0x00};
        std::map<uint16_t, uint8_t> initMemZPX {
            {0x0000, 0x35}, {0x0001, 0xFF},
            {0x0014, 0x8A}
        };
        cpuState finalStateZPX {0x0002, 0xFF, 0x80, 0x15, 0x00, ricoh2A03::negativeFlag};
        std::map<uint16_t, uint8_t> finalMemZPX {};
        test(4, initStateZPX, initMemZPX, finalStateZPX, finalMemZPX);

        // ABSY
        cpuState initStateABSY {0x0000, 0xFF, 0xB1, 0x00, 0x04, 0x00};
        std::map<uint16_t, uint8_t> initMemABSY {
            {0x0000, 0x39}, {0x0001, 0xBB}, {0x0002, 0xAA},
            {0xAABF, 0x8A}
        };
        cpuState finalStateABSY {0x0003, 0xFF, 0x80, 0x00, 0x04, ricoh2A03::negativeFlag};
        std::map<uint16_t, uint8_t> finalMemABSY {};
        test(4, initStateABSY, initMemABSY, finalStateABSY, finalMemABSY);

        // ABSY (NO page cross delay +1)
        cpuState initStateABSY2 {0x0000, 0xFF, 0xB1, 0x00, 0x46, 0x00};
        std::map<uint16_t, uint8_t> initMemABSY2 {
            {0x0000, 0x39}, {0x0001, 0xBB}, {0x0002, 0xAA},
            {0xAB01, 0x8A}
        };
        cpuState finalStateABSY2 {0x0003, 0xFF, 0x80, 0x00, 0x46, ricoh2A03::negativeFlag};
        std::map<uint16_t, uint8_t> finalMemABSY2 {};
        test(5, initStateABSY2, initMemABSY2, finalStateABSY2, finalMemABSY2);

        // ABSX
        cpuState initStateABSX {0x0000, 0xFF, 0xB1, 0x03, 0x00, 0x00};
        std::map<uint16_t, uint8_t> initMemABSX {
            {0x0000, 0x3D}, {0x0001, 0xAA}, {0x0002, 0xBB},
            {0xBBAD, 0x8A}
        };
        cpuState finalStateABSX {0x0003, 0xFF, 0x80, 0x03, 0x00, ricoh2A03::negativeFlag};
        std::map<uint16_t, uint8_t> finalMemABSX {};
        test(4, initStateABSX, initMemABSX, finalStateABSX, finalMemABSX);

        // ABSX (NO page cross delay +1)
        cpuState initStateABSX2 {0x0000, 0xFF, 0xB1, 0x64, 0x00, 0x00};
        std::map<uint16_t, uint8_t> initMemABSX2 {
            {0x0000, 0x3D}, {0x0001, 0xAA}, {0x0002, 0xBB},
            {0xBC0E, 0x8A}
        };
        cpuState finalStateABSX2 {0x0003, 0xFF, 0x80, 0x64, 0x00, ricoh2A03::negativeFlag};
        std::map<uint16_t, uint8_t> finalMemABSX2 {};
        test(5, initStateABSX2, initMemABSX2, finalStateABSX2, finalMemABSX2);
    }



    TEST_F(cpuTest, testEOR)    // 0x41 0x45 0x49 0x4D 0x51 0x55 0x59 0x5D
    {
        // INDX (negativeFlag)
        cpuState initStateINDX {0x0000, 0xFF, 0x55, 0x04, 0x00, 0x00};
        std::map<uint16_t, uint8_t> initMemINDX {
            {0x0000, 0x41}, {0x0001, 0xAA},
            {0x00AE, 0xBB}, {0x00AF, 0xCC},
            {0xCCBB, 0xA5}
        };
        cpuState finalStateINDX {0x0002, 0xFF, 0xF0, 0x04, 0x00, ricoh2A03::negativeFlag};
        std::map<uint16_t, uint8_t> finalMemINDX {};
        test(6, initStateINDX, initMemINDX, finalStateINDX, finalMemINDX);

        // INDX (zeroFlag)
        cpuState initStateINDX2 {0x0000, 0xFF, 0x55, 0x04, 0x00, 0x00};
        std::map<uint16_t, uint8_t> initMemINDX2 {
            {0x0000, 0x41}, {0x0001, 0xAA},
            {0x00AE, 0xBB}, {0x00AF, 0xCC},
            {0xCCBB, 0x55}
        };
        cpuState finalStateINDX2 {0x0002, 0xFF, 0x00, 0x04, 0x00, ricoh2A03::zeroFlag};
        std::map<uint16_t, uint8_t> finalMemINDX2 {};
        test(6, initStateINDX2, initMemINDX2, finalStateINDX2, finalMemINDX2);
        
        // ZP
        cpuState initStateZP {0x0000, 0xFF, 0x55, 0x00, 0x00, 0x00};
        std::map<uint16_t, uint8_t> initMemZP {
            {0x0000, 0x45}, {0x0001, 0xAA},
            {0x00AA, 0xA5}
        };
        cpuState finalStateZP {0x0002, 0xFF, 0xF0, 0x00, 0x00, ricoh2A03::negativeFlag};
        std::map<uint16_t, uint8_t> finalMemZP {};
        test(3, initStateZP, initMemZP, finalStateZP, finalMemZP);

        // IMM
        cpuState initStateIMM {0x0000, 0xFF, 0x55, 0x00, 0x00, 0x00};
        std::map<uint16_t, uint8_t> initMemIMM {
            {0x0000, 0x49}, {0x0001, 0xA5}
        };
        cpuState finalStateIMM {0x0002, 0xFF, 0xF0, 0x00, 0x00, ricoh2A03::negativeFlag};
        std::map<uint16_t, uint8_t> finalMemIMM {};
        test(2, initStateIMM, initMemIMM, finalStateIMM, finalMemIMM);

        // ABS
        cpuState initStateABS {0x0000, 0xFF, 0x55, 0x00, 0x00, 0x00};
        std::map<uint16_t, uint8_t> initMemABS {
            {0x0000, 0x4D}, {0x0001, 0xAA}, {0x0002, 0xBB},
            {0xBBAA, 0xA5}
        };
        cpuState finalStateABS {0x0003, 0xFF, 0xF0, 0x00, 0x00, ricoh2A03::negativeFlag};
        std::map<uint16_t, uint8_t> finalMemABS {};
        test(4, initStateABS, initMemABS, finalStateABS, finalMemABS);

        // INDY
        cpuState initStateINDY {0x0000, 0xFF, 0x55, 0x00, 0x03, 0x00};
        std::map<uint16_t, uint8_t> initMemINDY {
            {0x0000, 0x51}, {0x0001, 0xAA},
            {0x00AA, 0xBB}, {0x00AB, 0xCC},
            {0xCCBE, 0xA5}
        };
        cpuState finalStateINDY {0x0002, 0xFF, 0xF0, 0x00, 0x03, ricoh2A03::negativeFlag};
        std::map<uint16_t, uint8_t> finalMemINDY {};
        test(5, initStateINDY, initMemINDY, finalStateINDY, finalMemINDY);

        // INDY (page cross delay +1)
        cpuState initStateINDY2 {0x0000, 0xFF, 0x55, 0x00, 0x46, 0x00};
        std::map<uint16_t, uint8_t> initMemINDY2 {
            {0x0000, 0x51}, {0x0001, 0xAA},
            {0x00AA, 0xBB}, {0x00AB, 0xCC},
            {0xCD01, 0xA5}
        };
        cpuState finalStateINDY2 {0x0002, 0xFF, 0xF0, 0x00, 0x46, ricoh2A03::negativeFlag};
        std::map<uint16_t, uint8_t> finalMemINDY2 {};
        test(6, initStateINDY2, initMemINDY2, finalStateINDY2, finalMemINDY2);

        // ZPX (also checks for no page cross with indexing)
        cpuState initStateZPX {0x0000, 0xFF, 0x55, 0x15, 0x00, 0x00};
        std::map<uint16_t, uint8_t> initMemZPX {
            {0x0000, 0x55}, {0x0001, 0xFF},
            {0x0014, 0xA5}
        };
        cpuState finalStateZPX {0x0002, 0xFF, 0xF0, 0x15, 0x00, ricoh2A03::negativeFlag};
        std::map<uint16_t, uint8_t> finalMemZPX {};
        test(4, initStateZPX, initMemZPX, finalStateZPX, finalMemZPX);

        // ABSY
        cpuState initStateABSY {0x0000, 0xFF, 0x55, 0x00, 0x04, 0x00};
        std::map<uint16_t, uint8_t> initMemABSY {
            {0x0000, 0x59}, {0x0001, 0xBB}, {0x0002, 0xAA},
            {0xAABF, 0xA5}
        };
        cpuState finalStateABSY {0x0003, 0xFF, 0xF0, 0x00, 0x04, ricoh2A03::negativeFlag};
        std::map<uint16_t, uint8_t> finalMemABSY {};
        test(4, initStateABSY, initMemABSY, finalStateABSY, finalMemABSY);

        // ABSY (NO page cross delay +1)
        cpuState initStateABSY2 {0x0000, 0xFF, 0x55, 0x00, 0x46, 0x00};
        std::map<uint16_t, uint8_t> initMemABSY2 {
            {0x0000, 0x59}, {0x0001, 0xBB}, {0x0002, 0xAA},
            {0xAB01, 0xA5}
        };
        cpuState finalStateABSY2 {0x0003, 0xFF, 0xF0, 0x00, 0x46, ricoh2A03::negativeFlag};
        std::map<uint16_t, uint8_t> finalMemABSY2 {};
        test(5, initStateABSY2, initMemABSY2, finalStateABSY2, finalMemABSY2);

        // ABSX
        cpuState initStateABSX {0x0000, 0xFF, 0x55, 0x03, 0x00, 0x00};
        std::map<uint16_t, uint8_t> initMemABSX {
            {0x0000, 0x5D}, {0x0001, 0xAA}, {0x0002, 0xBB},
            {0xBBAD, 0xA5}
        };
        cpuState finalStateABSX {0x0003, 0xFF, 0xF0, 0x03, 0x00, ricoh2A03::negativeFlag};
        std::map<uint16_t, uint8_t> finalMemABSX {};
        test(4, initStateABSX, initMemABSX, finalStateABSX, finalMemABSX);

        // ABSX (NO page cross delay +1)
        cpuState initStateABSX2 {0x0000, 0xFF, 0x55, 0x64, 0x00, 0x00};
        std::map<uint16_t, uint8_t> initMemABSX2 {
            {0x0000, 0x5D}, {0x0001, 0xAA}, {0x0002, 0xBB},
            {0xBC0E, 0xA5}
        };
        cpuState finalStateABSX2 {0x0003, 0xFF, 0xF0, 0x64, 0x00, ricoh2A03::negativeFlag};
        std::map<uint16_t, uint8_t> finalMemABSX2 {};
        test(5, initStateABSX2, initMemABSX2, finalStateABSX2, finalMemABSX2);
    }



    TEST_F(cpuTest, testORA)    // 0x01 0x05 0x09 0x0D 0x11 0x15 0x19 0x1D
    {
        // INDX (negativeFlag)
        cpuState initStateINDX {0x0000, 0xFF, 0xB1, 0x04, 0x00, 0x00};
        std::map<uint16_t, uint8_t> initMemINDX {
            {0x0000, 0x01}, {0x0001, 0xAA},
            {0x00AE, 0xBB}, {0x00AF, 0xCC},
            {0xCCBB, 0x8A}
        };
        cpuState finalStateINDX {0x0002, 0xFF, 0xBB, 0x04, 0x00, ricoh2A03::negativeFlag};
        std::map<uint16_t, uint8_t> finalMemINDX {};
        test(6, initStateINDX, initMemINDX, finalStateINDX, finalMemINDX);

        // INDX (zeroFlag)
        cpuState initStateINDX2 {0x0000, 0xFF, 0x00, 0x04, 0x00, 0x00};
        std::map<uint16_t, uint8_t> initMemINDX2 {
            {0x0000, 0x01}, {0x0001, 0xAA},
            {0x00AE, 0xBB}, {0x00AF, 0xCC},
            {0xCCBB, 0x00}
        };
        cpuState finalStateINDX2 {0x0002, 0xFF, 0x00, 0x04, 0x00, ricoh2A03::zeroFlag};
        std::map<uint16_t, uint8_t> finalMemINDX2 {};
        test(6, initStateINDX2, initMemINDX2, finalStateINDX2, finalMemINDX2);
        
        // ZP
        cpuState initStateZP {0x0000, 0xFF, 0xB1, 0x00, 0x00, 0x00};
        std::map<uint16_t, uint8_t> initMemZP {
            {0x0000, 0x05}, {0x0001, 0xAA},
            {0x00AA, 0x8A}
        };
        cpuState finalStateZP {0x0002, 0xFF, 0xBB, 0x00, 0x00, ricoh2A03::negativeFlag};
        std::map<uint16_t, uint8_t> finalMemZP {};
        test(3, initStateZP, initMemZP, finalStateZP, finalMemZP);

        // IMM
        cpuState initStateIMM {0x0000, 0xFF, 0xB1, 0x00, 0x00, 0x00};
        std::map<uint16_t, uint8_t> initMemIMM {
            {0x0000, 0x09}, {0x0001, 0x8A}
        };
        cpuState finalStateIMM {0x0002, 0xFF, 0xBB, 0x00, 0x00, ricoh2A03::negativeFlag};
        std::map<uint16_t, uint8_t> finalMemIMM {};
        test(2, initStateIMM, initMemIMM, finalStateIMM, finalMemIMM);

        // ABS
        cpuState initStateABS {0x0000, 0xFF, 0xB1, 0x00, 0x00, 0x00};
        std::map<uint16_t, uint8_t> initMemABS {
            {0x0000, 0x0D}, {0x0001, 0xAA}, {0x0002, 0xBB},
            {0xBBAA, 0x8A}
        };
        cpuState finalStateABS {0x0003, 0xFF, 0xBB, 0x00, 0x00, ricoh2A03::negativeFlag};
        std::map<uint16_t, uint8_t> finalMemABS {};
        test(4, initStateABS, initMemABS, finalStateABS, finalMemABS);

        // INDY
        cpuState initStateINDY {0x0000, 0xFF, 0xB1, 0x00, 0x03, 0x00};
        std::map<uint16_t, uint8_t> initMemINDY {
            {0x0000, 0x11}, {0x0001, 0xAA},
            {0x00AA, 0xBB}, {0x00AB, 0xCC},
            {0xCCBE, 0x8A}
        };
        cpuState finalStateINDY {0x0002, 0xFF, 0xBB, 0x00, 0x03, ricoh2A03::negativeFlag};
        std::map<uint16_t, uint8_t> finalMemINDY {};
        test(5, initStateINDY, initMemINDY, finalStateINDY, finalMemINDY);

        // INDY (page cross delay +1) (this test case has error)
        cpuState initStateINDY2 {0x0000, 0xFF, 0xB1, 0x00, 0x46, 0x00};
        std::map<uint16_t, uint8_t> initMemINDY2 {
            {0x0000, 0x11}, {0x0001, 0xAA},
            {0x00AA, 0xBB}, {0x00AB, 0xCC},
            {0xCD01, 0x8A}
        };
        cpuState finalStateINDY2 {0x0002, 0xFF, 0xBB, 0x00, 0x46, ricoh2A03::negativeFlag};
        std::map<uint16_t, uint8_t> finalMemINDY2 {};
        test(6, initStateINDY2, initMemINDY2, finalStateINDY2, finalMemINDY2);

        // ZPX (also checks for no page cross with indexing)
        cpuState initStateZPX {0x0000, 0xFF, 0xB1, 0x15, 0x00, 0x00};
        std::map<uint16_t, uint8_t> initMemZPX {
            {0x0000, 0x15}, {0x0001, 0xFF},
            {0x0014, 0x8A}
        };
        cpuState finalStateZPX {0x0002, 0xFF, 0xBB, 0x15, 0x00, ricoh2A03::negativeFlag};
        std::map<uint16_t, uint8_t> finalMemZPX {};
        test(4, initStateZPX, initMemZPX, finalStateZPX, finalMemZPX);

        // ABSY
        cpuState initStateABSY {0x0000, 0xFF, 0xB1, 0x00, 0x04, 0x00};
        std::map<uint16_t, uint8_t> initMemABSY {
            {0x0000, 0x19}, {0x0001, 0xBB}, {0x0002, 0xAA},
            {0xAABF, 0x8A}
        };
        cpuState finalStateABSY {0x0003, 0xFF, 0xBB, 0x00, 0x04, ricoh2A03::negativeFlag};
        std::map<uint16_t, uint8_t> finalMemABSY {};
        test(4, initStateABSY, initMemABSY, finalStateABSY, finalMemABSY);

        // ABSY (NO page cross delay +1)
        cpuState initStateABSY2 {0x0000, 0xFF, 0xB1, 0x00, 0x46, 0x00};
        std::map<uint16_t, uint8_t> initMemABSY2 {
            {0x0000, 0x19}, {0x0001, 0xBB}, {0x0002, 0xAA},
            {0xAB01, 0x8A}
        };
        cpuState finalStateABSY2 {0x0003, 0xFF, 0xBB, 0x00, 0x46, ricoh2A03::negativeFlag};
        std::map<uint16_t, uint8_t> finalMemABSY2 {};
        test(5, initStateABSY2, initMemABSY2, finalStateABSY2, finalMemABSY2);

        // ABSX
        cpuState initStateABSX {0x0000, 0xFF, 0xB1, 0x03, 0x00, 0x00};
        std::map<uint16_t, uint8_t> initMemABSX {
            {0x0000, 0x1D}, {0x0001, 0xAA}, {0x0002, 0xBB},
            {0xBBAD, 0x8A}
        };
        cpuState finalStateABSX {0x0003, 0xFF, 0xBB, 0x03, 0x00, ricoh2A03::negativeFlag};
        std::map<uint16_t, uint8_t> finalMemABSX {};
        test(4, initStateABSX, initMemABSX, finalStateABSX, finalMemABSX);

        // ABSX (NO page cross delay +1)
        cpuState initStateABSX2 {0x0000, 0xFF, 0xB1, 0x64, 0x00, 0x00};
        std::map<uint16_t, uint8_t> initMemABSX2 {
            {0x0000, 0x1D}, {0x0001, 0xAA}, {0x0002, 0xBB},
            {0xBC0E, 0x8A}
        };
        cpuState finalStateABSX2 {0x0003, 0xFF, 0xBB, 0x64, 0x00, ricoh2A03::negativeFlag};
        std::map<uint16_t, uint8_t> finalMemABSX2 {};
        test(5, initStateABSX2, initMemABSX2, finalStateABSX2, finalMemABSX2);
    }



    TEST_F(cpuTest, testBIT)    // 0x24 0x2C
    {
        // ZP (bit 7)
        cpuState initStateZP {0x0000, 0xFF, 0xFF, 0x00, 0x00, ricoh2A03::zeroFlag};
        std::map<uint16_t, uint8_t> initMemZP {
            {0x0000, 0x24}, {0x0001, 0xAA},
            {0x00AA, 0x40}
        };
        cpuState finalStateZP {0x0002, 0xFF, 0xFF, 0x00, 0x00, ricoh2A03::overflowFlag};
        std::map<uint16_t, uint8_t> finalMemZP {};
        test(3, initStateZP, initMemZP, finalStateZP, finalMemZP);

        // ZP (bit 7)
        cpuState initStateZP2 {0x0000, 0xFF, 0xFF, 0x00, 0x00, ricoh2A03::zeroFlag};
        std::map<uint16_t, uint8_t> initMemZP2 {
            {0x0000, 0x24}, {0x0001, 0xAA},
            {0x00AA, 0x80}
        };
        cpuState finalStateZP2 {0x0002, 0xFF, 0xFF, 0x00, 0x00, ricoh2A03::negativeFlag};
        std::map<uint16_t, uint8_t> finalMemZP2 {};
        test(3, initStateZP2, initMemZP2, finalStateZP2, finalMemZP2);

        // ZP (zero)
        cpuState initStateZP3 {0x0000, 0xFF, 0xCC, 0x00, 0x00, (ricoh2A03::overflowFlag | ricoh2A03::negativeFlag)};
        std::map<uint16_t, uint8_t> initMemZP3 {
            {0x0000, 0x24}, {0x0001, 0xAA},
            {0x00AA, 0x33}
        };
        cpuState finalStateZP3 {0x0002, 0xFF, 0xCC, 0x00, 0x00, ricoh2A03::zeroFlag};
        std::map<uint16_t, uint8_t> finalMemZP3 {};
        test(3, initStateZP3, initMemZP3, finalStateZP3, finalMemZP3);

        // ABS (zero)
        cpuState initStateABS {0x0000, 0xFF, 0xCC, 0x00, 0x00, (ricoh2A03::overflowFlag | ricoh2A03::negativeFlag)};
        std::map<uint16_t, uint8_t> initMemABS {
            {0x0000, 0x2C}, {0x0001, 0xAA}, {0x0002, 0xBB},
            {0xBBAA, 0x33}
        };
        cpuState finalStateABS {0x0003, 0xFF, 0xCC, 0x00, 0x00, ricoh2A03::zeroFlag};
        std::map<uint16_t, uint8_t> finalMemABS {};
        test(4, initStateABS, initMemABS, finalStateABS, finalMemABS);
    }



    TEST_F(cpuTest, testADC)    // 0x61 0x65 0x69 0x6D 0x71 0x75 0x79 0x7D
    {
        // INDX
        cpuState initStateINDX {0x0000, 0xFF, 0x22, 0x04, 0x00, 0x00};
        std::map<uint16_t, uint8_t> initMemINDX {
            {0x0000, 0x61}, {0x0001, 0xAA},
            {0x00AE, 0xBB}, {0x00AF, 0xCC},
            {0xCCBB, 0x38}
        };
        cpuState finalStateINDX {0x0002, 0xFF, 0x5A, 0x04, 0x00, 0x00};
        std::map<uint16_t, uint8_t> finalMemINDX {};
        test(6, initStateINDX, initMemINDX, finalStateINDX, finalMemINDX);

        // INDX (using carryFlag)
        cpuState initStateINDX2 {0x0000, 0xFF, 0x22, 0x04, 0x00, ricoh2A03::carryFlag};
        std::map<uint16_t, uint8_t> initMemINDX2 {
            {0x0000, 0x61}, {0x0001, 0xAA},
            {0x00AE, 0xBB}, {0x00AF, 0xCC},
            {0xCCBB, 0x38}
        };
        cpuState finalStateINDX2 {0x0002, 0xFF, 0x5B, 0x04, 0x00, 0x00};
        std::map<uint16_t, uint8_t> finalMemINDX2 {};
        test(6, initStateINDX2, initMemINDX2, finalStateINDX2, finalMemINDX2);

        // INDX (setting carryFlag)
        cpuState initStateINDX3 {0x0000, 0xFF, 0xFF, 0x04, 0x00, ricoh2A03::carryFlag};
        std::map<uint16_t, uint8_t> initMemINDX3 {
            {0x0000, 0x61}, {0x0001, 0xAA},
            {0x00AE, 0xBB}, {0x00AF, 0xCC},
            {0xCCBB, 0x01}
        };
        cpuState finalStateINDX3 {0x0002, 0xFF, 0x01, 0x04, 0x00, ricoh2A03::carryFlag};
        std::map<uint16_t, uint8_t> finalMemINDX3 {};
        test(6, initStateINDX3, initMemINDX3, finalStateINDX3, finalMemINDX3);

        // INDX (zeroFlag)
        cpuState initStateINDX4 {0x0000, 0xFF, 0xFF, 0x04, 0x00, 0x00};
        std::map<uint16_t, uint8_t> initMemINDX4 {
            {0x0000, 0x61}, {0x0001, 0xAA},
            {0x00AE, 0xBB}, {0x00AF, 0xCC},
            {0xCCBB, 0x01}
        };
        cpuState finalStateINDX4 {0x0002, 0xFF, 0x00, 0x04, 0x00, (ricoh2A03::carryFlag | ricoh2A03::zeroFlag)};
        std::map<uint16_t, uint8_t> finalMemINDX4 {};
        test(6, initStateINDX4, initMemINDX4, finalStateINDX4, finalMemINDX4);

        // INDX (overflowFlag) (+ + -) (carryFlag should be zeroed)
        cpuState initStateINDX5 {0x0000, 0xFF, 0x54, 0x04, 0x00, ricoh2A03::carryFlag};
        std::map<uint16_t, uint8_t> initMemINDX5 {
            {0x0000, 0x61}, {0x0001, 0xAA},
            {0x00AE, 0xBB}, {0x00AF, 0xCC},
            {0xCCBB, 0x55}
        };
        cpuState finalStateINDX5 {0x0002, 0xFF, 0xAA, 0x04, 0x00, (ricoh2A03::overflowFlag | ricoh2A03::negativeFlag)};
        std::map<uint16_t, uint8_t> finalMemINDX5 {};
        test(6, initStateINDX5, initMemINDX5, finalStateINDX5, finalMemINDX5);

        // INDX (overflowFlag) (- - +) (carryFlag should be set)
        cpuState initStateINDX6 {0x0000, 0xFF, 0xAA, 0x04, 0x00, 0x00};
        std::map<uint16_t, uint8_t> initMemINDX6 {
            {0x0000, 0x61}, {0x0001, 0xAA},
            {0x00AE, 0xBB}, {0x00AF, 0xCC},
            {0xCCBB, 0x87}
        };
        cpuState finalStateINDX6 {0x0002, 0xFF, 0x31, 0x04, 0x00, (ricoh2A03::carryFlag | ricoh2A03::overflowFlag)};
        std::map<uint16_t, uint8_t> finalMemINDX6 {};
        test(6, initStateINDX6, initMemINDX6, finalStateINDX6, finalMemINDX6);

        // INDX (negativeFlag)
        cpuState initStateINDX7 {0x0000, 0xFF, 0xFE, 0x04, 0x00, 0x00};
        std::map<uint16_t, uint8_t> initMemINDX7 {
            {0x0000, 0x61}, {0x0001, 0xAA},
            {0x00AE, 0xBB}, {0x00AF, 0xCC},
            {0xCCBB, 0x01}
        };
        cpuState finalStateINDX7 {0x0002, 0xFF, 0xFF, 0x04, 0x00, ricoh2A03::negativeFlag};
        std::map<uint16_t, uint8_t> finalMemINDX7 {};
        test(6, initStateINDX7, initMemINDX7, finalStateINDX7, finalMemINDX7);

        // ZP
        cpuState initStateZP {0x0000, 0xFF, 0x22, 0x00, 0x00, 0x00};
        std::map<uint16_t, uint8_t> initMemZP {
            {0x0000, 0x65}, {0x0001, 0xAA},
            {0x00AA, 0x38}
        };
        cpuState finalStateZP {0x0002, 0xFF, 0x5A, 0x00, 0x00, 0x00};
        std::map<uint16_t, uint8_t> finalMemZP {};
        test(3, initStateZP, initMemZP, finalStateZP, finalMemZP);

        // IMM
        cpuState initStateIMM {0x0000, 0xFF, 0x22, 0x00, 0x00, 0x00};
        std::map<uint16_t, uint8_t> initMemIMM {
            {0x0000, 0x69}, {0x0001, 0xA38}
        };
        cpuState finalStateIMM {0x0002, 0xFF, 0x5A, 0x00, 0x00, 0x00};
        std::map<uint16_t, uint8_t> finalMemIMM {};
        test(2, initStateIMM, initMemIMM, finalStateIMM, finalMemIMM);

        // ABS
        cpuState initStateABS {0x0000, 0xFF, 0x22, 0x00, 0x00, 0x00};
        std::map<uint16_t, uint8_t> initMemABS {
            {0x0000, 0x6D}, {0x0001, 0xAA}, {0x0002, 0xBB},
            {0xBBAA, 0x38}
        };
        cpuState finalStateABS {0x0003, 0xFF, 0x5A, 0x00, 0x00, 0x00};
        std::map<uint16_t, uint8_t> finalMemABS {};
        test(4, initStateABS, initMemABS, finalStateABS, finalMemABS);

        // INDY
        cpuState initStateINDY {0x0000, 0xFF, 0x22, 0x00, 0x03, 0x00};
        std::map<uint16_t, uint8_t> initMemINDY {
            {0x0000, 0x71}, {0x0001, 0xAA},
            {0x00AA, 0xBB}, {0x00AB, 0xCC},
            {0xCCBE, 0x38}
        };
        cpuState finalStateINDY {0x0002, 0xFF, 0x5A, 0x00, 0x03, 0x00};
        std::map<uint16_t, uint8_t> finalMemINDY {};
        test(5, initStateINDY, initMemINDY, finalStateINDY, finalMemINDY);

        // INDY (page cross delay +1)
        cpuState initStateINDY2 {0x0000, 0xFF, 0x22, 0x00, 0x46, 0x00};
        std::map<uint16_t, uint8_t> initMemINDY2 {
            {0x0000, 0x71}, {0x0001, 0xAA},
            {0x00AA, 0xBB}, {0x00AB, 0xCC},
            {0xCD01, 0x38}
        };
        cpuState finalStateINDY2 {0x0002, 0xFF, 0x5A, 0x00, 0x46, 0x00};
        std::map<uint16_t, uint8_t> finalMemINDY2 {};
        test(6, initStateINDY2, initMemINDY2, finalStateINDY2, finalMemINDY2);

        // ZPX (also checks for no page cross with indexing)
        cpuState initStateZPX {0x0000, 0xFF, 0x22, 0x15, 0x00, 0x00};
        std::map<uint16_t, uint8_t> initMemZPX {
            {0x0000, 0x75}, {0x0001, 0xFF},
            {0x0014, 0x38}
        };
        cpuState finalStateZPX {0x0002, 0xFF, 0x5A, 0x15, 0x00, 0x00};
        std::map<uint16_t, uint8_t> finalMemZPX {};
        test(4, initStateZPX, initMemZPX, finalStateZPX, finalMemZPX);

        // ABSY
        cpuState initStateABSY {0x0000, 0xFF, 0x22, 0x00, 0x04, 0x00};
        std::map<uint16_t, uint8_t> initMemABSY {
            {0x0000, 0x79}, {0x0001, 0xBB}, {0x0002, 0xAA},
            {0xAABF, 0x38}
        };
        cpuState finalStateABSY {0x0003, 0xFF, 0x5A, 0x00, 0x04, 0x00};
        std::map<uint16_t, uint8_t> finalMemABSY {};
        test(4, initStateABSY, initMemABSY, finalStateABSY, finalMemABSY);

        // ABSY (page cross delay +1)
        cpuState initStateABSY2 {0x0000, 0xFF, 0x22, 0x00, 0x46, 0x00};
        std::map<uint16_t, uint8_t> initMemABSY2 {
            {0x0000, 0x79}, {0x0001, 0xBB}, {0x0002, 0xAA},
            {0xAB01, 0x38}
        };
        cpuState finalStateABSY2 {0x0003, 0xFF, 0x5A, 0x00, 0x46, 0x00};
        std::map<uint16_t, uint8_t> finalMemABSY2 {};
        test(5, initStateABSY2, initMemABSY2, finalStateABSY2, finalMemABSY2);

        // ABSX
        cpuState initStateABSX {0x0000, 0xFF, 0x22, 0x03, 0x00, 0x00};
        std::map<uint16_t, uint8_t> initMemABSX {
            {0x0000, 0x7D}, {0x0001, 0xAA}, {0x0002, 0xBB},
            {0xBBAD, 0x38}
        };
        cpuState finalStateABSX {0x0003, 0xFF, 0x5A, 0x03, 0x00, 0x00};
        std::map<uint16_t, uint8_t> finalMemABSX {};
        test(4, initStateABSX, initMemABSX, finalStateABSX, finalMemABSX);

        // ABSX (page cross delay +1)
        cpuState initStateABSX2 {0x0000, 0xFF, 0x22, 0x64, 0x00, 0x00};
        std::map<uint16_t, uint8_t> initMemABSX2 {
            {0x0000, 0x7D}, {0x0001, 0xAA}, {0x0002, 0xBB},
            {0xBC0E, 0x38}
        };
        cpuState finalStateABSX2 {0x0003, 0xFF, 0x5A, 0x64, 0x00, 0x00};
        std::map<uint16_t, uint8_t> finalMemABSX2 {};
        test(5, initStateABSX2, initMemABSX2, finalStateABSX2, finalMemABSX2);
    }



    TEST_F(cpuTest, testSBC)    // 0xE1 0xE5 0xE9 0xED 0xF1 0xF5 0xF9 0xFD
    {
        // INDX
        cpuState initStateINDX {0x0000, 0xFF, 0x22, 0x04, 0x00, ricoh2A03::carryFlag};
        std::map<uint16_t, uint8_t> initMemINDX {
            {0x0000, 0xE1}, {0x0001, 0xAA},
            {0x00AE, 0xBB}, {0x00AF, 0xCC},
            {0xCCBB, 0xEA}
        };
        cpuState finalStateINDX {0x0002, 0xFF, 0x38, 0x04, 0x00, ricoh2A03::carryFlag};
        std::map<uint16_t, uint8_t> finalMemINDX {};
        test(6, initStateINDX, initMemINDX, finalStateINDX, finalMemINDX);

        // INDX (not using carryFlag)
        cpuState initStateINDX2 {0x0000, 0xFF, 0x22, 0x04, 0x00, 0x00};
        std::map<uint16_t, uint8_t> initMemINDX2 {
            {0x0000, 0xE1}, {0x0001, 0xAA},
            {0x00AE, 0xBB}, {0x00AF, 0xCC},
            {0xCCBB, 0xEA}
        };
        cpuState finalStateINDX2 {0x0002, 0xFF, 0x37, 0x04, 0x00, ricoh2A03::carryFlag};
        std::map<uint16_t, uint8_t> finalMemINDX2 {};
        test(6, initStateINDX2, initMemINDX2, finalStateINDX2, finalMemINDX2);

        // INDX (not setting carryFlag)
        cpuState initStateINDX3 {0x0000, 0xFF, 0x5A, 0x04, 0x00, ricoh2A03::carryFlag};
        std::map<uint16_t, uint8_t> initMemINDX3 {
            {0x0000, 0xE1}, {0x0001, 0xAA},
            {0x00AE, 0xBB}, {0x00AF, 0xCC},
            {0xCCBB, 0x38}
        };
        cpuState finalStateINDX3 {0x0002, 0xFF, 0x22, 0x04, 0x00, 0x00};
        std::map<uint16_t, uint8_t> finalMemINDX3 {};
        test(6, initStateINDX3, initMemINDX3, finalStateINDX3, finalMemINDX3);

        // INDX (zeroFlag)
        cpuState initStateINDX4 {0x0000, 0xFF, 0xEE, 0x04, 0x00, ricoh2A03::carryFlag};
        std::map<uint16_t, uint8_t> initMemINDX4 {
            {0x0000, 0xE1}, {0x0001, 0xAA},
            {0x00AE, 0xBB}, {0x00AF, 0xCC},
            {0xCCBB, 0xEE}
        };
        cpuState finalStateINDX4 {0x0002, 0xFF, 0x00, 0x04, 0x00, ricoh2A03::zeroFlag};
        std::map<uint16_t, uint8_t> finalMemINDX4 {};
        test(6, initStateINDX4, initMemINDX4, finalStateINDX4, finalMemINDX4);

        // INDX (overflowFlag) (+ - -) (carryFlag should be set)
        cpuState initStateINDX5 {0x0000, 0xFF, 0x7F, 0x04, 0x00, 0x00};
        std::map<uint16_t, uint8_t> initMemINDX5 {
            {0x0000, 0xE1}, {0x0001, 0xAA},
            {0x00AE, 0xBB}, {0x00AF, 0xCC},
            {0xCCBB, 0xFD}
        };
        cpuState finalStateINDX5 {0x0002, 0xFF, 0x81, 0x04, 0x00, (ricoh2A03::carryFlag | ricoh2A03::overflowFlag | ricoh2A03::negativeFlag)};
        std::map<uint16_t, uint8_t> finalMemINDX5 {};
        test(6, initStateINDX5, initMemINDX5, finalStateINDX5, finalMemINDX5);

        // INDX (overflowFlag) (- + +) (carryFlag should be zeroed)
        cpuState initStateINDX6 {0x0000, 0xFF, 0x82, 0x04, 0x00, ricoh2A03::carryFlag};
        std::map<uint16_t, uint8_t> initMemINDX6 {
            {0x0000, 0xE1}, {0x0001, 0xAA},
            {0x00AE, 0xBB}, {0x00AF, 0xCC},
            {0xCCBB, 0x04}
        };
        cpuState finalStateINDX6 {0x0002, 0xFF, 0x7E, 0x04, 0x00, ricoh2A03::overflowFlag};
        std::map<uint16_t, uint8_t> finalMemINDX6 {};
        test(6, initStateINDX6, initMemINDX6, finalStateINDX6, finalMemINDX6);

        // INDX (negativeFlag)
        cpuState initStateINDX7 {0x0000, 0xFF, 0x01, 0x04, 0x00, ricoh2A03::carryFlag};
        std::map<uint16_t, uint8_t> initMemINDX7 {
            {0x0000, 0xE1}, {0x0001, 0xAA},
            {0x00AE, 0xBB}, {0x00AF, 0xCC},
            {0xCCBB, 0x03}
        };
        cpuState finalStateINDX7 {0x0002, 0xFF, 0xFE, 0x04, 0x00, (ricoh2A03::carryFlag | ricoh2A03::negativeFlag)};
        std::map<uint16_t, uint8_t> finalMemINDX7 {};
        test(6, initStateINDX7, initMemINDX7, finalStateINDX7, finalMemINDX7);

        // ZP
        cpuState initStateZP {0x0000, 0xFF, 0x22, 0x00, 0x00, ricoh2A03::carryFlag};
        std::map<uint16_t, uint8_t> initMemZP {
            {0x0000, 0xE5}, {0x0001, 0xAA},
            {0x00AA, 0xEA}
        };
        cpuState finalStateZP {0x0002, 0xFF, 0x38, 0x00, 0x00, ricoh2A03::carryFlag};
        std::map<uint16_t, uint8_t> finalMemZP {};
        test(3, initStateZP, initMemZP, finalStateZP, finalMemZP);

        // IMM
        cpuState initStateIMM {0x0000, 0xFF, 0x22, 0x00, 0x00, ricoh2A03::carryFlag};
        std::map<uint16_t, uint8_t> initMemIMM {
            {0x0000, 0xE9}, {0x0001, 0xEA}
        };
        cpuState finalStateIMM {0x0002, 0xFF, 0x38, 0x00, 0x00, ricoh2A03::carryFlag};
        std::map<uint16_t, uint8_t> finalMemIMM {};
        test(2, initStateIMM, initMemIMM, finalStateIMM, finalMemIMM);

        // ABS
        cpuState initStateABS {0x0000, 0xFF, 0x22, 0x00, 0x00, ricoh2A03::carryFlag};
        std::map<uint16_t, uint8_t> initMemABS {
            {0x0000, 0xED}, {0x0001, 0xAA}, {0x0002, 0xBB},
            {0xBBAA, 0xEA}
        };
        cpuState finalStateABS {0x0003, 0xFF, 0x38, 0x00, 0x00, ricoh2A03::carryFlag};
        std::map<uint16_t, uint8_t> finalMemABS {};
        test(4, initStateABS, initMemABS, finalStateABS, finalMemABS);

        // INDY
        cpuState initStateINDY {0x0000, 0xFF, 0x22, 0x00, 0x03, ricoh2A03::carryFlag};
        std::map<uint16_t, uint8_t> initMemINDY {
            {0x0000, 0xF1}, {0x0001, 0xAA},
            {0x00AA, 0xBB}, {0x00AB, 0xCC},
            {0xCCBE, 0xEA}
        };
        cpuState finalStateINDY {0x0002, 0xFF, 0x38, 0x00, 0x03, ricoh2A03::carryFlag};
        std::map<uint16_t, uint8_t> finalMemINDY {};
        test(5, initStateINDY, initMemINDY, finalStateINDY, finalMemINDY);

        // INDY (page cross delay +1)
        cpuState initStateINDY2 {0x0000, 0xFF, 0x22, 0x00, 0x46, ricoh2A03::carryFlag};
        std::map<uint16_t, uint8_t> initMemINDY2 {
            {0x0000, 0xF1}, {0x0001, 0xAA},
            {0x00AA, 0xBB}, {0x00AB, 0xCC},
            {0xCD01, 0xEA}
        };
        cpuState finalStateINDY2 {0x0002, 0xFF, 0x38, 0x00, 0x46, ricoh2A03::carryFlag};
        std::map<uint16_t, uint8_t> finalMemINDY2 {};
        test(6, initStateINDY2, initMemINDY2, finalStateINDY2, finalMemINDY2);

        // ZPX (also checks for no page cross with indexing)
        cpuState initStateZPX {0x0000, 0xFF, 0x22, 0x15, 0x00, ricoh2A03::carryFlag};
        std::map<uint16_t, uint8_t> initMemZPX {
            {0x0000, 0xF5}, {0x0001, 0xFF},
            {0x0014, 0xEA}
        };
        cpuState finalStateZPX {0x0002, 0xFF, 0x38, 0x15, 0x00, ricoh2A03::carryFlag};
        std::map<uint16_t, uint8_t> finalMemZPX {};
        test(4, initStateZPX, initMemZPX, finalStateZPX, finalMemZPX);

        // ABSY
        cpuState initStateABSY {0x0000, 0xFF, 0x22, 0x00, 0x04, ricoh2A03::carryFlag};
        std::map<uint16_t, uint8_t> initMemABSY {
            {0x0000, 0xF9}, {0x0001, 0xBB}, {0x0002, 0xAA},
            {0xAABF, 0xEA}
        };
        cpuState finalStateABSY {0x0003, 0xFF, 0x38, 0x00, 0x04, ricoh2A03::carryFlag};
        std::map<uint16_t, uint8_t> finalMemABSY {};
        test(4, initStateABSY, initMemABSY, finalStateABSY, finalMemABSY);

        // ABSY (page cross delay +1)
        cpuState initStateABSY2 {0x0000, 0xFF, 0x22, 0x00, 0x46, ricoh2A03::carryFlag};
        std::map<uint16_t, uint8_t> initMemABSY2 {
            {0x0000, 0xF9}, {0x0001, 0xBB}, {0x0002, 0xAA},
            {0xAB01, 0xEA}
        };
        cpuState finalStateABSY2 {0x0003, 0xFF, 0x38, 0x00, 0x46, ricoh2A03::carryFlag};
        std::map<uint16_t, uint8_t> finalMemABSY2 {};
        test(5, initStateABSY2, initMemABSY2, finalStateABSY2, finalMemABSY2);

        // ABSX
        cpuState initStateABSX {0x0000, 0xFF, 0x22, 0x03, 0x00, ricoh2A03::carryFlag};
        std::map<uint16_t, uint8_t> initMemABSX {
            {0x0000, 0xFD}, {0x0001, 0xAA}, {0x0002, 0xBB},
            {0xBBAD, 0xEA}
        };
        cpuState finalStateABSX {0x0003, 0xFF, 0x38, 0x03, 0x00, ricoh2A03::carryFlag};
        std::map<uint16_t, uint8_t> finalMemABSX {};
        test(4, initStateABSX, initMemABSX, finalStateABSX, finalMemABSX);

        // ABSX (page cross delay +1)
        cpuState initStateABSX2 {0x0000, 0xFF, 0x22, 0x64, 0x00, ricoh2A03::carryFlag};
        std::map<uint16_t, uint8_t> initMemABSX2 {
            {0x0000, 0xFD}, {0x0001, 0xAA}, {0x0002, 0xBB},
            {0xBC0E, 0xEA}
        };
        cpuState finalStateABSX2 {0x0003, 0xFF, 0x38, 0x64, 0x00, ricoh2A03::carryFlag};
        std::map<uint16_t, uint8_t> finalMemABSX2 {};
        test(5, initStateABSX2, initMemABSX2, finalStateABSX2, finalMemABSX2);
    }



    TEST_F(cpuTest, testCMP)    // 0xC1 0xC5 0xC9 0xCD 0xD1 0xD5 0xD9 0xDD
    {
        // INDX (carryFlag)
        cpuState initStateINDX {0x0000, 0xFF, 0x38, 0x04, 0x00, 0x00};
        std::map<uint16_t, uint8_t> initMemINDX {
            {0x0000, 0xC1}, {0x0001, 0xAA},
            {0x00AE, 0xBB}, {0x00AF, 0xCC},
            {0xCCBB, 0x22}
        };
        cpuState finalStateINDX {0x0002, 0xFF, 0x38, 0x04, 0x00, ricoh2A03::carryFlag};
        std::map<uint16_t, uint8_t> finalMemINDX {};
        test(6, initStateINDX, initMemINDX, finalStateINDX, finalMemINDX);

        // INDX (zeroFlag)
        cpuState initStateINDX4 {0x0000, 0xFF, 0x38, 0x04, 0x00, 0x00};
        std::map<uint16_t, uint8_t> initMemINDX4 {
            {0x0000, 0xC1}, {0x0001, 0xAA},
            {0x00AE, 0xBB}, {0x00AF, 0xCC},
            {0xCCBB, 0x38}
        };
        cpuState finalStateINDX4 {0x0002, 0xFF, 0x38, 0x04, 0x00, (ricoh2A03::carryFlag | ricoh2A03::zeroFlag)};
        std::map<uint16_t, uint8_t> finalMemINDX4 {};
        test(6, initStateINDX4, initMemINDX4, finalStateINDX4, finalMemINDX4);

        // INDX (negativeFlag)
        cpuState initStateINDX7 {0x0000, 0xFF, 0x22, 0x04, 0x00, (ricoh2A03::carryFlag | ricoh2A03::zeroFlag)};
        std::map<uint16_t, uint8_t> initMemINDX7 {
            {0x0000, 0xC1}, {0x0001, 0xAA},
            {0x00AE, 0xBB}, {0x00AF, 0xCC},
            {0xCCBB, 0x38}
        };
        cpuState finalStateINDX7 {0x0002, 0xFF, 0x22, 0x04, 0x00, ricoh2A03::negativeFlag};
        std::map<uint16_t, uint8_t> finalMemINDX7 {};
        test(6, initStateINDX7, initMemINDX7, finalStateINDX7, finalMemINDX7);

        // ZP
        cpuState initStateZP {0x0000, 0xFF, 0x38, 0x00, 0x00, 0x00};
        std::map<uint16_t, uint8_t> initMemZP {
            {0x0000, 0xC5}, {0x0001, 0xAA},
            {0x00AA, 0x22}
        };
        cpuState finalStateZP {0x0002, 0xFF, 0x38, 0x00, 0x00, ricoh2A03::carryFlag};
        std::map<uint16_t, uint8_t> finalMemZP {};
        test(3, initStateZP, initMemZP, finalStateZP, finalMemZP);

        // IMM
        cpuState initStateIMM {0x0000, 0xFF, 0x38, 0x00, 0x00, 0x00};
        std::map<uint16_t, uint8_t> initMemIMM {
            {0x0000, 0xC9}, {0x0001, 0x22}
        };
        cpuState finalStateIMM {0x0002, 0xFF, 0x38, 0x00, 0x00, ricoh2A03::carryFlag};
        std::map<uint16_t, uint8_t> finalMemIMM {};
        test(2, initStateIMM, initMemIMM, finalStateIMM, finalMemIMM);

        // ABS
        cpuState initStateABS {0x0000, 0xFF, 0x38, 0x00, 0x00, 0x00};
        std::map<uint16_t, uint8_t> initMemABS {
            {0x0000, 0xCD}, {0x0001, 0xAA}, {0x0002, 0xBB},
            {0xBBAA, 0x22}
        };
        cpuState finalStateABS {0x0003, 0xFF, 0x38, 0x00, 0x00, ricoh2A03::carryFlag};
        std::map<uint16_t, uint8_t> finalMemABS {};
        test(4, initStateABS, initMemABS, finalStateABS, finalMemABS);

        // INDY
        cpuState initStateINDY {0x0000, 0xFF, 0x38, 0x00, 0x03, 0x00};
        std::map<uint16_t, uint8_t> initMemINDY {
            {0x0000, 0xD1}, {0x0001, 0xAA},
            {0x00AA, 0xBB}, {0x00AB, 0xCC},
            {0xCCBE, 0x22}
        };
        cpuState finalStateINDY {0x0002, 0xFF, 0x38, 0x00, 0x03, ricoh2A03::carryFlag};
        std::map<uint16_t, uint8_t> finalMemINDY {};
        test(5, initStateINDY, initMemINDY, finalStateINDY, finalMemINDY);

        // INDY (page cross delay +1)
        cpuState initStateINDY2 {0x0000, 0xFF, 0x38, 0x00, 0x46, 0x00};
        std::map<uint16_t, uint8_t> initMemINDY2 {
            {0x0000, 0xD1}, {0x0001, 0xAA},
            {0x00AA, 0xBB}, {0x00AB, 0xCC},
            {0xCD01, 0x22}
        };
        cpuState finalStateINDY2 {0x0002, 0xFF, 0x38, 0x00, 0x46, ricoh2A03::carryFlag};
        std::map<uint16_t, uint8_t> finalMemINDY2 {};
        test(6, initStateINDY2, initMemINDY2, finalStateINDY2, finalMemINDY2);

        // ZPX (also checks for no page cross with indexing)
        cpuState initStateZPX {0x0000, 0xFF, 0x38, 0x15, 0x00, 0x00};
        std::map<uint16_t, uint8_t> initMemZPX {
            {0x0000, 0xD5}, {0x0001, 0xFF},
            {0x0014, 0x22}
        };
        cpuState finalStateZPX {0x0002, 0xFF, 0x38, 0x15, 0x00, ricoh2A03::carryFlag};
        std::map<uint16_t, uint8_t> finalMemZPX {};
        test(4, initStateZPX, initMemZPX, finalStateZPX, finalMemZPX);

        // ABSY
        cpuState initStateABSY {0x0000, 0xFF, 0x38, 0x00, 0x04, 0x00};
        std::map<uint16_t, uint8_t> initMemABSY {
            {0x0000, 0xD9}, {0x0001, 0xBB}, {0x0002, 0xAA},
            {0xAABF, 0x22}
        };
        cpuState finalStateABSY {0x0003, 0xFF, 0x38, 0x00, 0x04, ricoh2A03::carryFlag};
        std::map<uint16_t, uint8_t> finalMemABSY {};
        test(4, initStateABSY, initMemABSY, finalStateABSY, finalMemABSY);

        // ABSY (page cross delay +1)
        cpuState initStateABSY2 {0x0000, 0xFF, 0x38, 0x00, 0x46, 0x00};
        std::map<uint16_t, uint8_t> initMemABSY2 {
            {0x0000, 0xD9}, {0x0001, 0xBB}, {0x0002, 0xAA},
            {0xAB01, 0x22}
        };
        cpuState finalStateABSY2 {0x0003, 0xFF, 0x38, 0x00, 0x46, ricoh2A03::carryFlag};
        std::map<uint16_t, uint8_t> finalMemABSY2 {};
        test(5, initStateABSY2, initMemABSY2, finalStateABSY2, finalMemABSY2);

        // ABSX
        cpuState initStateABSX {0x0000, 0xFF, 0x38, 0x03, 0x00, 0x00};
        std::map<uint16_t, uint8_t> initMemABSX {
            {0x0000, 0xDD}, {0x0001, 0xAA}, {0x0002, 0xBB},
            {0xBBAD, 0x22}
        };
        cpuState finalStateABSX {0x0003, 0xFF, 0x38, 0x03, 0x00, ricoh2A03::carryFlag};
        std::map<uint16_t, uint8_t> finalMemABSX {};
        test(4, initStateABSX, initMemABSX, finalStateABSX, finalMemABSX);

        // ABSX (page cross delay +1)
        cpuState initStateABSX2 {0x0000, 0xFF, 0x38, 0x64, 0x00, 0x00};
        std::map<uint16_t, uint8_t> initMemABSX2 {
            {0x0000, 0xDD}, {0x0001, 0xAA}, {0x0002, 0xBB},
            {0xBC0E, 0x22}
        };
        cpuState finalStateABSX2 {0x0003, 0xFF, 0x38, 0x64, 0x00, ricoh2A03::carryFlag};
        std::map<uint16_t, uint8_t> finalMemABSX2 {};
        test(5, initStateABSX2, initMemABSX2, finalStateABSX2, finalMemABSX2);
    }



    TEST_F(cpuTest, testCPX)    // 0xE0 0xE4 0xEC
    {
        // IMM (carryFlag)
        cpuState initStateIMM {0x0000, 0xFF, 0x00, 0x38, 0x00, 0x00};
        std::map<uint16_t, uint8_t> initMemIMM {
            {0x0000, 0xE0}, {0x0001, 0x22}
        };
        cpuState finalStateIMM {0x0002, 0xFF, 0x00, 0x38, 0x00, ricoh2A03::carryFlag};
        std::map<uint16_t, uint8_t> finalMemIMM {};
        test(2, initStateIMM, initMemIMM, finalStateIMM, finalMemIMM);

        // IMM (zeroFlag)
        cpuState initStateIMM2 {0x0000, 0xFF, 0x00, 0x38, 0x00, 0x00};
        std::map<uint16_t, uint8_t> initMemIMM2 {
            {0x0000, 0xE0}, {0x0001, 0x38}
        };
        cpuState finalStateIMM2 {0x0002, 0xFF, 0x00, 0x38, 0x00, (ricoh2A03::carryFlag | ricoh2A03::zeroFlag)};
        std::map<uint16_t, uint8_t> finalMemIMM2 {};
        test(2, initStateIMM2, initMemIMM2, finalStateIMM2, finalMemIMM2);

        // IMM (negativeFlag)
        cpuState initStateIMM3 {0x0000, 0xFF, 0x00, 0x22, 0x00, (ricoh2A03::carryFlag | ricoh2A03::zeroFlag)};
        std::map<uint16_t, uint8_t> initMemIMM3 {
            {0x0000, 0xE0}, {0x0001, 0x38}
        };
        cpuState finalStateIMM3 {0x0002, 0xFF, 0x00, 0x22, 0x00, ricoh2A03::negativeFlag};
        std::map<uint16_t, uint8_t> finalMemIMM3 {};
        test(2, initStateIMM3, initMemIMM3, finalStateIMM3, finalMemIMM3);

        // ZP
        cpuState initStateZP {0x0000, 0xFF, 0x00, 0x38, 0x00, 0x00};
        std::map<uint16_t, uint8_t> initMemZP {
            {0x0000, 0xE4}, {0x0001, 0xAA},
            {0x00AA, 0x22}
        };
        cpuState finalStateZP {0x0002, 0xFF, 0x00, 0x38, 0x00, ricoh2A03::carryFlag};
        std::map<uint16_t, uint8_t> finalMemZP {};
        test(3, initStateZP, initMemZP, finalStateZP, finalMemZP);

        // ABS
        cpuState initStateABS {0x0000, 0xFF, 0x00, 0x38, 0x00, 0x00};
        std::map<uint16_t, uint8_t> initMemABS {
            {0x0000, 0xEC}, {0x0001, 0xAA}, {0x0002, 0xBB},
            {0xBBAA, 0x22}
        };
        cpuState finalStateABS {0x0003, 0xFF, 0x00, 0x38, 0x00, ricoh2A03::carryFlag};
        std::map<uint16_t, uint8_t> finalMemABS {};
        test(4, initStateABS, initMemABS, finalStateABS, finalMemABS);
    }



    TEST_F(cpuTest, testCPY)    // 0xC0 0xC4 0xCC
    {
        // IMM (carryFlag)
        cpuState initStateIMM {0x0000, 0xFF, 0x00, 0x00, 0x38, 0x00};
        std::map<uint16_t, uint8_t> initMemIMM {
            {0x0000, 0xC0}, {0x0001, 0x22}
        };
        cpuState finalStateIMM {0x0002, 0xFF, 0x00, 0x00, 0x38, ricoh2A03::carryFlag};
        std::map<uint16_t, uint8_t> finalMemIMM {};
        test(2, initStateIMM, initMemIMM, finalStateIMM, finalMemIMM);

        // IMM (zeroFlag)
        cpuState initStateIMM2 {0x0000, 0xFF, 0x00, 0x00, 0x38, 0x00};
        std::map<uint16_t, uint8_t> initMemIMM2 {
            {0x0000, 0xC0}, {0x0001, 0x38}
        };
        cpuState finalStateIMM2 {0x0002, 0xFF, 0x00, 0x00, 0x38, (ricoh2A03::carryFlag | ricoh2A03::zeroFlag)};
        std::map<uint16_t, uint8_t> finalMemIMM2 {};
        test(2, initStateIMM2, initMemIMM2, finalStateIMM2, finalMemIMM2);

        // IMM (negativeFlag)
        cpuState initStateIMM3 {0x0000, 0xFF, 0x00, 0x00, 0x22, (ricoh2A03::carryFlag | ricoh2A03::zeroFlag)};
        std::map<uint16_t, uint8_t> initMemIMM3 {
            {0x0000, 0xC0}, {0x0001, 0x38}
        };
        cpuState finalStateIMM3 {0x0002, 0xFF, 0x00, 0x00, 0x22, ricoh2A03::negativeFlag};
        std::map<uint16_t, uint8_t> finalMemIMM3 {};
        test(2, initStateIMM3, initMemIMM3, finalStateIMM3, finalMemIMM3);

        // ZP
        cpuState initStateZP {0x0000, 0xFF, 0x00, 0x00, 0x38, 0x00};
        std::map<uint16_t, uint8_t> initMemZP {
            {0x0000, 0xC4}, {0x0001, 0xAA},
            {0x00AA, 0x22}
        };
        cpuState finalStateZP {0x0002, 0xFF, 0x00, 0x00, 0x38, ricoh2A03::carryFlag};
        std::map<uint16_t, uint8_t> finalMemZP {};
        test(3, initStateZP, initMemZP, finalStateZP, finalMemZP);

        // ABS
        cpuState initStateABS {0x0000, 0xFF, 0x00, 0x00, 0x38, 0x00};
        std::map<uint16_t, uint8_t> initMemABS {
            {0x0000, 0xCC}, {0x0001, 0xAA}, {0x0002, 0xBB},
            {0xBBAA, 0x22}
        };
        cpuState finalStateABS {0x0003, 0xFF, 0x00, 0x00, 0x38, ricoh2A03::carryFlag};
        std::map<uint16_t, uint8_t> finalMemABS {};
        test(4, initStateABS, initMemABS, finalStateABS, finalMemABS);
    }



    TEST_F(cpuTest, testINC)    // 0xE6 0xEE 0xF6 0xFE
    {
        // ZP
        cpuState initStateZP {0x0000, 0xFF, 0x00, 0x00, 0x00, 0x00};
        std::map<uint16_t, uint8_t> initMemZP {
            {0x0000, 0xE6}, {0x0001, 0xAA},
            {0x00AA, 0x22}
        };
        cpuState finalStateZP {0x0002, 0xFF, 0x00, 0x00, 0x00, 0x00};
        std::map<uint16_t, uint8_t> finalMemZP {
            {0x00AA, 0x23}
        };
        test(5, initStateZP, initMemZP, finalStateZP, finalMemZP);

        // ZP (zeroFlag)
        cpuState initStateZP2 {0x0000, 0xFF, 0x00, 0x00, 0x00, 0x00};
        std::map<uint16_t, uint8_t> initMemZP2 {
            {0x0000, 0xE6}, {0x0001, 0xAA},
            {0x00AA, 0xFF}
        };
        cpuState finalStateZP2 {0x0002, 0xFF, 0x00, 0x00, 0x00, ricoh2A03::zeroFlag};
        std::map<uint16_t, uint8_t> finalMemZP2 {
            {0x00AA, 0x00}
        };
        test(5, initStateZP2, initMemZP2, finalStateZP2, finalMemZP2);

        // ZP (negativeFlag)
        cpuState initStateZP3 {0x0000, 0xFF, 0x00, 0x00, 0x00, 0x00};
        std::map<uint16_t, uint8_t> initMemZP3 {
            {0x0000, 0xE6}, {0x0001, 0xAA},
            {0x00AA, 0xFE}
        };
        cpuState finalStateZP3 {0x0002, 0xFF, 0x00, 0x00, 0x00, ricoh2A03::negativeFlag};
        std::map<uint16_t, uint8_t> finalMemZP3 {
            {0x00AA, 0xFF}
        };
        test(5, initStateZP3, initMemZP3, finalStateZP3, finalMemZP3);

        // ABS
        cpuState initStateABS {0x0000, 0xFF, 0x00, 0x00, 0x00, 0x00};
        std::map<uint16_t, uint8_t> initMemABS {
            {0x0000, 0xEE}, {0x0001, 0xAA}, {0x0002, 0xBB},
            {0xBBAA, 0x22}
        };
        cpuState finalStateABS {0x0003, 0xFF, 0x00, 0x00, 0x00, 0x00};
        std::map<uint16_t, uint8_t> finalMemABS {
            {0xBBAA, 0x23}
        };
        test(6, initStateABS, initMemABS, finalStateABS, finalMemABS);

        // ZPX (also checks for no page cross with indexing)
        cpuState initStateZPX {0x0000, 0xFF, 0x00, 0x15, 0x00, 0x00};
        std::map<uint16_t, uint8_t> initMemZPX {
            {0x0000, 0xF6}, {0x0001, 0xFF},
            {0x0014, 0x22}
        };
        cpuState finalStateZPX {0x0002, 0xFF, 0x00, 0x15, 0x00, 0x00};
        std::map<uint16_t, uint8_t> finalMemZPX {
            {0xBBAA, 0x23}
        };
        test(6, initStateZPX, initMemZPX, finalStateZPX, finalMemZPX);

        // ABSX
        cpuState initStateABSX {0x0000, 0xFF, 0x00, 0x03, 0x00, 0x00};
        std::map<uint16_t, uint8_t> initMemABSX {
            {0x0000, 0xFE}, {0x0001, 0xAA}, {0x0002, 0xBB},
            {0xBBAD, 0x22}
        };
        cpuState finalStateABSX {0x0003, 0xFF, 0x00, 0x03, 0x00, 0x00};
        std::map<uint16_t, uint8_t> finalMemABSX {
            {0xBBAA, 0x23}
        };
        test(7, initStateABSX, initMemABSX, finalStateABSX, finalMemABSX);

        // ABSX (NO page cross delay +1)
        cpuState initStateABSX2 {0x0000, 0xFF, 0x00, 0x64, 0x00, 0x00};
        std::map<uint16_t, uint8_t> initMemABSX2 {
            {0x0000, 0xFE}, {0x0001, 0xAA}, {0x0002, 0xBB},
            {0xBC0E, 0x22}
        };
        cpuState finalStateABSX2 {0x0003, 0xFF, 0x00, 0x64, 0x00, 0x00};
        std::map<uint16_t, uint8_t> finalMemABSX2 {
            {0xBBAA, 0x23}
        };
        test(7, initStateABSX2, initMemABSX2, finalStateABSX2, finalMemABSX2);
    }



    TEST_F(cpuTest, testINX)    // 0xE8
    {
        // IMP
        cpuState initStateIMP {0x0000, 0xFF, 0xCC, 0x7B, 0xAA, 0x00};
        std::map<uint16_t, uint8_t> initMemIMP {
            {0x0000, 0xE8}
        };
        cpuState finalStateIMP {0x0001, 0xFF, 0xCC, 0x7C, 0xAA, 0x00};
        std::map<uint16_t, uint8_t> finalMemIMP {};
        test(2, initStateIMP, initMemIMP, finalStateIMP, finalMemIMP);

        // IMP (zeroFlag)
        cpuState initStateIMP2 {0x0000, 0xFF, 0xCC, 0xFF, 0xAA, 0x00};
        std::map<uint16_t, uint8_t> initMemIMP2 {
            {0x0000, 0xE8}
        };
        cpuState finalStateIMP2 {0x0001, 0xFF, 0xCC, 0x00, 0xAA, ricoh2A03::zeroFlag};
        std::map<uint16_t, uint8_t> finalMemIMP2 {};
        test(2, initStateIMP2, initMemIMP2, finalStateIMP2, finalMemIMP2);

        // IMP (negativeFlag)
        cpuState initStateIMP3 {0x0000, 0xFF, 0xCC, 0xBB, 0xAA, 0x00};
        std::map<uint16_t, uint8_t> initMemIMP3 {
            {0x0000, 0xE8}
        };
        cpuState finalStateIMP3 {0x0001, 0xFF, 0xCC, 0xBC, 0xAA, ricoh2A03::negativeFlag};
        std::map<uint16_t, uint8_t> finalMemIMP3 {};
        test(2, initStateIMP3, initMemIMP3, finalStateIMP3, finalMemIMP3);
    }



    TEST_F(cpuTest, testINY)    // 0xC8
    {
        // IMP
        cpuState initStateIMP {0x0000, 0xFF, 0xCC, 0xBB, 0x7A, 0x00};
        std::map<uint16_t, uint8_t> initMemIMP {
            {0x0000, 0xC8}
        };
        cpuState finalStateIMP {0x0001, 0xFF, 0xCC, 0xBB, 0x7B, 0x00};
        std::map<uint16_t, uint8_t> finalMemIMP {};
        test(2, initStateIMP, initMemIMP, finalStateIMP, finalMemIMP);

        // IMP (zeroFlag)
        cpuState initStateIMP2 {0x0000, 0xFF, 0xCC, 0xBB, 0xFF, 0x00};
        std::map<uint16_t, uint8_t> initMemIMP2 {
            {0x0000, 0xC8}
        };
        cpuState finalStateIMP2 {0x0001, 0xFF, 0xCC, 0xBB, 0x00, ricoh2A03::zeroFlag};
        std::map<uint16_t, uint8_t> finalMemIMP2 {};
        test(2, initStateIMP2, initMemIMP2, finalStateIMP2, finalMemIMP2);

        // IMP (negativeFlag)
        cpuState initStateIMP3 {0x0000, 0xFF, 0xCC, 0xBB, 0xAA, 0x00};
        std::map<uint16_t, uint8_t> initMemIMP3 {
            {0x0000, 0xC8}
        };
        cpuState finalStateIMP3 {0x0001, 0xFF, 0xCC, 0xBB, 0xAB, ricoh2A03::negativeFlag};
        std::map<uint16_t, uint8_t> finalMemIMP3 {};
        test(2, initStateIMP3, initMemIMP3, finalStateIMP3, finalMemIMP3);
    }



    TEST_F(cpuTest, testDEC)    // 0xC6 0xCE 0xD6 0xDE
    {
        // ZP
        cpuState initStateZP {0x0000, 0xFF, 0x00, 0x00, 0x00, 0x00};
        std::map<uint16_t, uint8_t> initMemZP {
            {0x0000, 0xC6}, {0x0001, 0xAA},
            {0x00AA, 0x22}
        };
        cpuState finalStateZP {0x0002, 0xFF, 0x00, 0x00, 0x00, 0x00};
        std::map<uint16_t, uint8_t> finalMemZP {
            {0x00AA, 0x21}
        };
        test(5, initStateZP, initMemZP, finalStateZP, finalMemZP);

        // ZP (zeroFlag)
        cpuState initStateZP2 {0x0000, 0xFF, 0x00, 0x00, 0x00, 0x00};
        std::map<uint16_t, uint8_t> initMemZP2 {
            {0x0000, 0xC6}, {0x0001, 0xAA},
            {0x00AA, 0x01}
        };
        cpuState finalStateZP2 {0x0002, 0xFF, 0x00, 0x00, 0x00, ricoh2A03::zeroFlag};
        std::map<uint16_t, uint8_t> finalMemZP2 {
            {0x00AA, 0x00}
        };
        test(5, initStateZP2, initMemZP2, finalStateZP2, finalMemZP2);

        // ZP (negativeFlag)
        cpuState initStateZP3 {0x0000, 0xFF, 0x00, 0x00, 0x00, 0x00};
        std::map<uint16_t, uint8_t> initMemZP3 {
            {0x0000, 0xC6}, {0x0001, 0xAA},
            {0x00AA, 0xFF}
        };
        cpuState finalStateZP3 {0x0002, 0xFF, 0x00, 0x00, 0x00, ricoh2A03::negativeFlag};
        std::map<uint16_t, uint8_t> finalMemZP3 {
            {0x00AA, 0xFE}
        };
        test(5, initStateZP3, initMemZP3, finalStateZP3, finalMemZP3);

        // ABS
        cpuState initStateABS {0x0000, 0xFF, 0x00, 0x00, 0x00, 0x00};
        std::map<uint16_t, uint8_t> initMemABS {
            {0x0000, 0xCE}, {0x0001, 0xAA}, {0x0002, 0xBB},
            {0xBBAA, 0x22}
        };
        cpuState finalStateABS {0x0003, 0xFF, 0x00, 0x00, 0x00, 0x00};
        std::map<uint16_t, uint8_t> finalMemABS {
            {0xBBAA, 0x21}
        };
        test(6, initStateABS, initMemABS, finalStateABS, finalMemABS);

        // ZPX (also checks for no page cross with indexing)
        cpuState initStateZPX {0x0000, 0xFF, 0x00, 0x15, 0x00, 0x00};
        std::map<uint16_t, uint8_t> initMemZPX {
            {0x0000, 0xD6}, {0x0001, 0xFF},
            {0x0014, 0x22}
        };
        cpuState finalStateZPX {0x0002, 0xFF, 0x00, 0x15, 0x00, 0x00};
        std::map<uint16_t, uint8_t> finalMemZPX {
            {0xBBAA, 0x21}
        };
        test(6, initStateZPX, initMemZPX, finalStateZPX, finalMemZPX);

        // ABSX
        cpuState initStateABSX {0x0000, 0xFF, 0x00, 0x03, 0x00, 0x00};
        std::map<uint16_t, uint8_t> initMemABSX {
            {0x0000, 0xDE}, {0x0001, 0xAA}, {0x0002, 0xBB},
            {0xBBAD, 0x22}
        };
        cpuState finalStateABSX {0x0003, 0xFF, 0x00, 0x03, 0x00, 0x00};
        std::map<uint16_t, uint8_t> finalMemABSX {
            {0xBBAA, 0x21}
        };
        test(7, initStateABSX, initMemABSX, finalStateABSX, finalMemABSX);

        // ABSX (NO page cross delay +1)
        cpuState initStateABSX2 {0x0000, 0xFF, 0x00, 0x64, 0x00, 0x00};
        std::map<uint16_t, uint8_t> initMemABSX2 {
            {0x0000, 0xDE}, {0x0001, 0xAA}, {0x0002, 0xBB},
            {0xBC0E, 0x22}
        };
        cpuState finalStateABSX2 {0x0003, 0xFF, 0x00, 0x64, 0x00, 0x00};
        std::map<uint16_t, uint8_t> finalMemABSX2 {
            {0xBBAA, 0x21}
        };
        test(7, initStateABSX2, initMemABSX2, finalStateABSX2, finalMemABSX2);
    }



    TEST_F(cpuTest, testDEX)    // 0xCA
    {
        // IMP
        cpuState initStateIMP {0x0000, 0xFF, 0xCC, 0x7C, 0xAA, 0x00};
        std::map<uint16_t, uint8_t> initMemIMP {
            {0x0000, 0xCA}
        };
        cpuState finalStateIMP {0x0001, 0xFF, 0xCC, 0x7B, 0xAA, 0x00};
        std::map<uint16_t, uint8_t> finalMemIMP {};
        test(2, initStateIMP, initMemIMP, finalStateIMP, finalMemIMP);

        // IMP (zeroFlag)
        cpuState initStateIMP2 {0x0000, 0xFF, 0xCC, 0x01, 0xAA, 0x00};
        std::map<uint16_t, uint8_t> initMemIMP2 {
            {0x0000, 0xCA}
        };
        cpuState finalStateIMP2 {0x0001, 0xFF, 0xCC, 0x00, 0xAA, ricoh2A03::zeroFlag};
        std::map<uint16_t, uint8_t> finalMemIMP2 {};
        test(2, initStateIMP2, initMemIMP2, finalStateIMP2, finalMemIMP2);

        // IMP (negativeFlag)
        cpuState initStateIMP3 {0x0000, 0xFF, 0xCC, 0xBB, 0xAA, 0x00};
        std::map<uint16_t, uint8_t> initMemIMP3 {
            {0x0000, 0xCA}
        };
        cpuState finalStateIMP3 {0x0001, 0xFF, 0xCC, 0xBA, 0xAA, ricoh2A03::negativeFlag};
        std::map<uint16_t, uint8_t> finalMemIMP3 {};
        test(2, initStateIMP3, initMemIMP3, finalStateIMP3, finalMemIMP3);
    }



    TEST_F(cpuTest, testDEY)    // 0x88
    {
        // IMP
        cpuState initStateIMP {0x0000, 0xFF, 0xCC, 0xBB, 0x7A, 0x00};
        std::map<uint16_t, uint8_t> initMemIMP {
            {0x0000, 0x88}
        };
        cpuState finalStateIMP {0x0001, 0xFF, 0xCC, 0xBB, 0x79, 0x00};
        std::map<uint16_t, uint8_t> finalMemIMP {};
        test(2, initStateIMP, initMemIMP, finalStateIMP, finalMemIMP);

        // IMP (zeroFlag)
        cpuState initStateIMP2 {0x0000, 0xFF, 0xCC, 0xBB, 0x01, 0x00};
        std::map<uint16_t, uint8_t> initMemIMP2 {
            {0x0000, 0x88}
        };
        cpuState finalStateIMP2 {0x0001, 0xFF, 0xCC, 0xBB, 0x00, ricoh2A03::zeroFlag};
        std::map<uint16_t, uint8_t> finalMemIMP2 {};
        test(2, initStateIMP2, initMemIMP2, finalStateIMP2, finalMemIMP2);

        // IMP (negativeFlag)
        cpuState initStateIMP3 {0x0000, 0xFF, 0xCC, 0xBB, 0xAA, 0x00};
        std::map<uint16_t, uint8_t> initMemIMP3 {
            {0x0000, 0x88}
        };
        cpuState finalStateIMP3 {0x0001, 0xFF, 0xCC, 0xBB, 0xA9, ricoh2A03::negativeFlag};
        std::map<uint16_t, uint8_t> finalMemIMP3 {};
        test(2, initStateIMP3, initMemIMP3, finalStateIMP3, finalMemIMP3);
    }



    TEST_F(cpuTest, testASL)    // 0x06 0x0A 0x0E 0x16 0x1E
    {
        // ZP
        cpuState initStateZP {0x0000, 0xFF, 0x00, 0x00, 0x00, 0x00};
        std::map<uint16_t, uint8_t> initMemZP {
            {0x0000, 0x06}, {0x0001, 0xAA},
            {0x00AA, 0x3C}
        };
        cpuState finalStateZP {0x0002, 0xFF, 0x00, 0x00, 0x00, 0x00};
        std::map<uint16_t, uint8_t> finalMemZP {
            {0x00AA, 0x78}
        };
        test(5, initStateZP, initMemZP, finalStateZP, finalMemZP);

        // ZP (carryFlag)
        cpuState initStateZP2 {0x0000, 0xFF, 0x00, 0x00, 0x00, 0x00};
        std::map<uint16_t, uint8_t> initMemZP2 {
            {0x0000, 0x06}, {0x0001, 0xAA},
            {0x00AA, 0x81}
        };
        cpuState finalStateZP2 {0x0002, 0xFF, 0x00, 0x00, 0x00, ricoh2A03::carryFlag};
        std::map<uint16_t, uint8_t> finalMemZP2 {
            {0x00AA, 0x02}
        };
        test(5, initStateZP2, initMemZP2, finalStateZP2, finalMemZP2);

        // ZP (zeroFlag)
        cpuState initStateZP3 {0x0000, 0xFF, 0x00, 0x00, 0x00, 0x00};
        std::map<uint16_t, uint8_t> initMemZP3 {
            {0x0000, 0x06}, {0x0001, 0xAA},
            {0x00AA, 0x00}
        };
        cpuState finalStateZP3 {0x0002, 0xFF, 0x00, 0x00, 0x00, ricoh2A03::zeroFlag};
        std::map<uint16_t, uint8_t> finalMemZP3 {
            {0x00AA, 0x00}
        };
        test(5, initStateZP3, initMemZP3, finalStateZP3, finalMemZP3);

        // ZP (negativeFlag)
        cpuState initStateZP4 {0x0000, 0xFF, 0x00, 0x00, 0x00, 0x00};
        std::map<uint16_t, uint8_t> initMemZP4 {
            {0x0000, 0x06}, {0x0001, 0xAA},
            {0x00AA, 0x40}
        };
        cpuState finalStateZP4 {0x0002, 0xFF, 0x00, 0x00, 0x00, ricoh2A03::negativeFlag};
        std::map<uint16_t, uint8_t> finalMemZP4 {
            {0x00AA, 0x80}
        };
        test(5, initStateZP4, initMemZP4, finalStateZP4, finalMemZP4);

        // ACC
        cpuState initStateACC {0x0000, 0xFF, 0x3C, 0x00, 0x00, 0x00};
        std::map<uint16_t, uint8_t> initMemACC {
            {0x0000, 0x0A}
        };
        cpuState finalStateACC {0x0001, 0xFF, 0x78, 0x00, 0x00, 0x00};
        std::map<uint16_t, uint8_t> finalMemACC {};
        test(2, initStateACC, initMemACC, finalStateACC, finalMemACC);

        // ABS
        cpuState initStateABS {0x0000, 0xFF, 0x00, 0x00, 0x00, 0x00};
        std::map<uint16_t, uint8_t> initMemABS {
            {0x0000, 0x0E}, {0x0001, 0xAA}, {0x0002, 0xBB},
            {0xBBAA, 0x3C}
        };
        cpuState finalStateABS {0x0003, 0xFF, 0x00, 0x00, 0x00, 0x00};
        std::map<uint16_t, uint8_t> finalMemABS {
            {0xBBAA, 0x78}
        };
        test(6, initStateABS, initMemABS, finalStateABS, finalMemABS);

        // ZPX (also checks for no page cross with indexing)
        cpuState initStateZPX {0x0000, 0xFF, 0x00, 0x15, 0x00, 0x00};
        std::map<uint16_t, uint8_t> initMemZPX {
            {0x0000, 0x16}, {0x0001, 0xFF},
            {0x0014, 0x3C}
        };
        cpuState finalStateZPX {0x0002, 0xFF, 0x00, 0x15, 0x00, 0x00};
        std::map<uint16_t, uint8_t> finalMemZPX {
            {0xBBAA, 0x78}
        };
        test(6, initStateZPX, initMemZPX, finalStateZPX, finalMemZPX);

        // ABSX
        cpuState initStateABSX {0x0000, 0xFF, 0x00, 0x03, 0x00, 0x00};
        std::map<uint16_t, uint8_t> initMemABSX {
            {0x0000, 0x1E}, {0x0001, 0xAA}, {0x0002, 0xBB},
            {0xBBAD, 0x3C}
        };
        cpuState finalStateABSX {0x0003, 0xFF, 0x00, 0x03, 0x00, 0x00};
        std::map<uint16_t, uint8_t> finalMemABSX {
            {0xBBAA, 0x78}
        };
        test(7, initStateABSX, initMemABSX, finalStateABSX, finalMemABSX);

        // ABSX (NO page cross delay +1)
        cpuState initStateABSX2 {0x0000, 0xFF, 0x00, 0x64, 0x00, 0x00};
        std::map<uint16_t, uint8_t> initMemABSX2 {
            {0x0000, 0x1E}, {0x0001, 0xAA}, {0x0002, 0xBB},
            {0xBC0E, 0x3C}
        };
        cpuState finalStateABSX2 {0x0003, 0xFF, 0x00, 0x64, 0x00, 0x00};
        std::map<uint16_t, uint8_t> finalMemABSX2 {
            {0xBBAA, 0x78}
        };
        test(7, initStateABSX2, initMemABSX2, finalStateABSX2, finalMemABSX2);
    }



    TEST_F(cpuTest, testLSR)    // 0x46 0x4A 0x4E 0x56 0x5E
    {
        // ZP
        cpuState initStateZP {0x0000, 0xFF, 0x00, 0x00, 0x00, 0x00};
        std::map<uint16_t, uint8_t> initMemZP {
            {0x0000, 0x46}, {0x0001, 0xAA},
            {0x00AA, 0x3C}
        };
        cpuState finalStateZP {0x0002, 0xFF, 0x00, 0x00, 0x00, 0x00};
        std::map<uint16_t, uint8_t> finalMemZP {
            {0x00AA, 0x1E}
        };
        test(5, initStateZP, initMemZP, finalStateZP, finalMemZP);

        // ZP (carryFlag)
        cpuState initStateZP2 {0x0000, 0xFF, 0x00, 0x00, 0x00, 0x00};
        std::map<uint16_t, uint8_t> initMemZP2 {
            {0x0000, 0x46}, {0x0001, 0xAA},
            {0x00AA, 0x81}
        };
        cpuState finalStateZP2 {0x0002, 0xFF, 0x00, 0x00, 0x00, ricoh2A03::carryFlag};
        std::map<uint16_t, uint8_t> finalMemZP2 {
            {0x00AA, 0x40}
        };
        test(5, initStateZP2, initMemZP2, finalStateZP2, finalMemZP2);

        // ZP (zeroFlag)
        cpuState initStateZP3 {0x0000, 0xFF, 0x00, 0x00, 0x00, 0x00};
        std::map<uint16_t, uint8_t> initMemZP3 {
            {0x0000, 0x46}, {0x0001, 0xAA},
            {0x00AA, 0x00}
        };
        cpuState finalStateZP3 {0x0002, 0xFF, 0x00, 0x00, 0x00, ricoh2A03::zeroFlag};
        std::map<uint16_t, uint8_t> finalMemZP3 {
            {0x00AA, 0x00}
        };
        test(5, initStateZP3, initMemZP3, finalStateZP3, finalMemZP3);

        // ZP (negativeFlag impossible)
        cpuState initStateZP4 {0x0000, 0xFF, 0x00, 0x00, 0x00, 0x00};
        std::map<uint16_t, uint8_t> initMemZP4 {
            {0x0000, 0x46}, {0x0001, 0xAA},
            {0x00AA, 0xFE}
        };
        cpuState finalStateZP4 {0x0002, 0xFF, 0x00, 0x00, 0x00, 0x00};
        std::map<uint16_t, uint8_t> finalMemZP4 {
            {0x00AA, 0x7F}
        };
        test(5, initStateZP4, initMemZP4, finalStateZP4, finalMemZP4);

        // ACC
        cpuState initStateACC {0x0000, 0xFF, 0x3C, 0x00, 0x00, 0x00};
        std::map<uint16_t, uint8_t> initMemACC {
            {0x0000, 0x4A}
        };
        cpuState finalStateACC {0x0001, 0xFF, 0x1E, 0x00, 0x00, 0x00};
        std::map<uint16_t, uint8_t> finalMemACC {};
        test(2, initStateACC, initMemACC, finalStateACC, finalMemACC);

        // ABS
        cpuState initStateABS {0x0000, 0xFF, 0x00, 0x00, 0x00, 0x00};
        std::map<uint16_t, uint8_t> initMemABS {
            {0x0000, 0x4E}, {0x0001, 0xAA}, {0x0002, 0xBB},
            {0xBBAA, 0x3C}
        };
        cpuState finalStateABS {0x0003, 0xFF, 0x00, 0x00, 0x00, 0x00};
        std::map<uint16_t, uint8_t> finalMemABS {
            {0xBBAA, 0x1E}
        };
        test(6, initStateABS, initMemABS, finalStateABS, finalMemABS);

        // ZPX (also checks for no page cross with indexing)
        cpuState initStateZPX {0x0000, 0xFF, 0x00, 0x15, 0x00, 0x00};
        std::map<uint16_t, uint8_t> initMemZPX {
            {0x0000, 0x56}, {0x0001, 0xFF},
            {0x0014, 0x3C}
        };
        cpuState finalStateZPX {0x0002, 0xFF, 0x00, 0x15, 0x00, 0x00};
        std::map<uint16_t, uint8_t> finalMemZPX {
            {0xBBAA, 0x1E}
        };
        test(6, initStateZPX, initMemZPX, finalStateZPX, finalMemZPX);

        // ABSX
        cpuState initStateABSX {0x0000, 0xFF, 0x00, 0x03, 0x00, 0x00};
        std::map<uint16_t, uint8_t> initMemABSX {
            {0x0000, 0x5E}, {0x0001, 0xAA}, {0x0002, 0xBB},
            {0xBBAD, 0x3C}
        };
        cpuState finalStateABSX {0x0003, 0xFF, 0x00, 0x03, 0x00, 0x00};
        std::map<uint16_t, uint8_t> finalMemABSX {
            {0xBBAA, 0x1E}
        };
        test(7, initStateABSX, initMemABSX, finalStateABSX, finalMemABSX);

        // ABSX (NO page cross delay +1)
        cpuState initStateABSX2 {0x0000, 0xFF, 0x00, 0x64, 0x00, 0x00};
        std::map<uint16_t, uint8_t> initMemABSX2 {
            {0x0000, 0x5E}, {0x0001, 0xAA}, {0x0002, 0xBB},
            {0xBC0E, 0x3C}
        };
        cpuState finalStateABSX2 {0x0003, 0xFF, 0x00, 0x64, 0x00, 0x00};
        std::map<uint16_t, uint8_t> finalMemABSX2 {
            {0xBBAA, 0x1E}
        };
        test(7, initStateABSX2, initMemABSX2, finalStateABSX2, finalMemABSX2);
    }



    TEST_F(cpuTest, testROL)    // 0x26 0x2A 0x2E 0x36 0x3E
    {
        // ZP
        cpuState initStateZP {0x0000, 0xFF, 0x00, 0x00, 0x00, 0x00};
        std::map<uint16_t, uint8_t> initMemZP {
            {0x0000, 0x26}, {0x0001, 0xAA},
            {0x00AA, 0x3C}
        };
        cpuState finalStateZP {0x0002, 0xFF, 0x00, 0x00, 0x00, 0x00};
        std::map<uint16_t, uint8_t> finalMemZP {
            {0x00AA, 0x78}
        };
        test(5, initStateZP, initMemZP, finalStateZP, finalMemZP);

        // ZP (using carryFlag)
        cpuState initStateZP5 {0x0000, 0xFF, 0x00, 0x00, 0x00, ricoh2A03::carryFlag};
        std::map<uint16_t, uint8_t> initMemZP5 {
            {0x0000, 0x26}, {0x0001, 0xAA},
            {0x00AA, 0x11}
        };
        cpuState finalStateZP5 {0x0002, 0xFF, 0x00, 0x00, 0x00, 0x00};
        std::map<uint16_t, uint8_t> finalMemZP5 {
            {0x00AA, 0x23}
        };
        test(5, initStateZP5, initMemZP5, finalStateZP5, finalMemZP5);

        // ZP (setting carryFlag)
        cpuState initStateZP2 {0x0000, 0xFF, 0x00, 0x00, 0x00, 0x00};
        std::map<uint16_t, uint8_t> initMemZP2 {
            {0x0000, 0x26}, {0x0001, 0xAA},
            {0x00AA, 0x81}
        };
        cpuState finalStateZP2 {0x0002, 0xFF, 0x00, 0x00, 0x00, ricoh2A03::carryFlag};
        std::map<uint16_t, uint8_t> finalMemZP2 {
            {0x00AA, 0x02}
        };
        test(5, initStateZP2, initMemZP2, finalStateZP2, finalMemZP2);

        // ZP (zeroFlag)
        cpuState initStateZP3 {0x0000, 0xFF, 0x00, 0x00, 0x00, 0x00};
        std::map<uint16_t, uint8_t> initMemZP3 {
            {0x0000, 0x26}, {0x0001, 0xAA},
            {0x00AA, 0x00}
        };
        cpuState finalStateZP3 {0x0002, 0xFF, 0x00, 0x00, 0x00, ricoh2A03::zeroFlag};
        std::map<uint16_t, uint8_t> finalMemZP3 {
            {0x00AA, 0x00}
        };
        test(5, initStateZP3, initMemZP3, finalStateZP3, finalMemZP3);

        // ZP (negativeFlag)
        cpuState initStateZP4 {0x0000, 0xFF, 0x00, 0x00, 0x00, 0x00};
        std::map<uint16_t, uint8_t> initMemZP4 {
            {0x0000, 0x26}, {0x0001, 0xAA},
            {0x00AA, 0x40}
        };
        cpuState finalStateZP4 {0x0002, 0xFF, 0x00, 0x00, 0x00, ricoh2A03::negativeFlag};
        std::map<uint16_t, uint8_t> finalMemZP4 {
            {0x00AA, 0x80}
        };
        test(5, initStateZP4, initMemZP4, finalStateZP4, finalMemZP4);

        // ACC
        cpuState initStateACC {0x0000, 0xFF, 0x3C, 0x00, 0x00, 0x00};
        std::map<uint16_t, uint8_t> initMemACC {
            {0x0000, 0x2A}
        };
        cpuState finalStateACC {0x0001, 0xFF, 0x78, 0x00, 0x00, 0x00};
        std::map<uint16_t, uint8_t> finalMemACC {};
        test(2, initStateACC, initMemACC, finalStateACC, finalMemACC);

        // ABS
        cpuState initStateABS {0x0000, 0xFF, 0x00, 0x00, 0x00, 0x00};
        std::map<uint16_t, uint8_t> initMemABS {
            {0x0000, 0x2E}, {0x0001, 0xAA}, {0x0002, 0xBB},
            {0xBBAA, 0x3C}
        };
        cpuState finalStateABS {0x0003, 0xFF, 0x00, 0x00, 0x00, 0x00};
        std::map<uint16_t, uint8_t> finalMemABS {
            {0xBBAA, 0x78}
        };
        test(6, initStateABS, initMemABS, finalStateABS, finalMemABS);

        // ZPX (also checks for no page cross with indexing)
        cpuState initStateZPX {0x0000, 0xFF, 0x00, 0x15, 0x00, 0x00};
        std::map<uint16_t, uint8_t> initMemZPX {
            {0x0000, 0x36}, {0x0001, 0xFF},
            {0x0014, 0x3C}
        };
        cpuState finalStateZPX {0x0002, 0xFF, 0x00, 0x15, 0x00, 0x00};
        std::map<uint16_t, uint8_t> finalMemZPX {
            {0xBBAA, 0x78}
        };
        test(6, initStateZPX, initMemZPX, finalStateZPX, finalMemZPX);

        // ABSX
        cpuState initStateABSX {0x0000, 0xFF, 0x00, 0x03, 0x00, 0x00};
        std::map<uint16_t, uint8_t> initMemABSX {
            {0x0000, 0x3E}, {0x0001, 0xAA}, {0x0002, 0xBB},
            {0xBBAD, 0x3C}
        };
        cpuState finalStateABSX {0x0003, 0xFF, 0x00, 0x03, 0x00, 0x00};
        std::map<uint16_t, uint8_t> finalMemABSX {
            {0xBBAA, 0x78}
        };
        test(7, initStateABSX, initMemABSX, finalStateABSX, finalMemABSX);

        // ABSX (NO page cross delay +1)
        cpuState initStateABSX2 {0x0000, 0xFF, 0x00, 0x64, 0x00, 0x00};
        std::map<uint16_t, uint8_t> initMemABSX2 {
            {0x0000, 0x3E}, {0x0001, 0xAA}, {0x0002, 0xBB},
            {0xBC0E, 0x3C}
        };
        cpuState finalStateABSX2 {0x0003, 0xFF, 0x00, 0x64, 0x00, 0x00};
        std::map<uint16_t, uint8_t> finalMemABSX2 {
            {0xBBAA, 0x78}
        };
        test(7, initStateABSX2, initMemABSX2, finalStateABSX2, finalMemABSX2);
    }



    TEST_F(cpuTest, testROR)    // 0x66 0x6A 0x6E 0x76 0x7E
    {
        // ZP
        cpuState initStateZP {0x0000, 0xFF, 0x00, 0x00, 0x00, 0x00};
        std::map<uint16_t, uint8_t> initMemZP {
            {0x0000, 0x66}, {0x0001, 0xAA},
            {0x00AA, 0x3C}
        };
        cpuState finalStateZP {0x0002, 0xFF, 0x00, 0x00, 0x00, 0x00};
        std::map<uint16_t, uint8_t> finalMemZP {
            {0x00AA, 0x1E}
        };
        test(5, initStateZP, initMemZP, finalStateZP, finalMemZP);

        // ZP (using carryFlag)
        cpuState initStateZP5 {0x0000, 0xFF, 0x00, 0x00, 0x00, ricoh2A03::carryFlag};
        std::map<uint16_t, uint8_t> initMemZP5 {
            {0x0000, 0x66}, {0x0001, 0xAA},
            {0x00AA, 0x82}
        };
        cpuState finalStateZP5 {0x0002, 0xFF, 0x00, 0x00, 0x00, ricoh2A03::negativeFlag};
        std::map<uint16_t, uint8_t> finalMemZP5 {
            {0x00AA, 0xC1}
        };
        test(5, initStateZP5, initMemZP5, finalStateZP5, finalMemZP5);

        // ZP (setting carryFlag)
        cpuState initStateZP2 {0x0000, 0xFF, 0x00, 0x00, 0x00, 0x00};
        std::map<uint16_t, uint8_t> initMemZP2 {
            {0x0000, 0x66}, {0x0001, 0xAA},
            {0x00AA, 0x81}
        };
        cpuState finalStateZP2 {0x0002, 0xFF, 0x00, 0x00, 0x00, ricoh2A03::carryFlag};
        std::map<uint16_t, uint8_t> finalMemZP2 {
            {0x00AA, 0x40}
        };
        test(5, initStateZP2, initMemZP2, finalStateZP2, finalMemZP2);

        // ZP (zeroFlag)
        cpuState initStateZP3 {0x0000, 0xFF, 0x00, 0x00, 0x00, 0x00};
        std::map<uint16_t, uint8_t> initMemZP3 {
            {0x0000, 0x66}, {0x0001, 0xAA},
            {0x00AA, 0x00}
        };
        cpuState finalStateZP3 {0x0002, 0xFF, 0x00, 0x00, 0x00, ricoh2A03::zeroFlag};
        std::map<uint16_t, uint8_t> finalMemZP3 {
            {0x00AA, 0x00}
        };
        test(5, initStateZP3, initMemZP3, finalStateZP3, finalMemZP3);

        // ZP (negativeFlag impossible)
        cpuState initStateZP4 {0x0000, 0xFF, 0x00, 0x00, 0x00, 0x00};
        std::map<uint16_t, uint8_t> initMemZP4 {
            {0x0000, 0x66}, {0x0001, 0xAA},
            {0x00AA, 0xFE}
        };
        cpuState finalStateZP4 {0x0002, 0xFF, 0x00, 0x00, 0x00, 0x00};
        std::map<uint16_t, uint8_t> finalMemZP4 {
            {0x00AA, 0x7F}
        };
        test(5, initStateZP4, initMemZP4, finalStateZP4, finalMemZP4);

        // ACC
        cpuState initStateACC {0x0000, 0xFF, 0x3C, 0x00, 0x00, 0x00};
        std::map<uint16_t, uint8_t> initMemACC {
            {0x0000, 0x6A}
        };
        cpuState finalStateACC {0x0001, 0xFF, 0x1E, 0x00, 0x00, 0x00};
        std::map<uint16_t, uint8_t> finalMemACC {};
        test(2, initStateACC, initMemACC, finalStateACC, finalMemACC);

        // ABS
        cpuState initStateABS {0x0000, 0xFF, 0x00, 0x00, 0x00, 0x00};
        std::map<uint16_t, uint8_t> initMemABS {
            {0x0000, 0x6E}, {0x0001, 0xAA}, {0x0002, 0xBB},
            {0xBBAA, 0x3C}
        };
        cpuState finalStateABS {0x0003, 0xFF, 0x00, 0x00, 0x00, 0x00};
        std::map<uint16_t, uint8_t> finalMemABS {
            {0xBBAA, 0x1E}
        };
        test(6, initStateABS, initMemABS, finalStateABS, finalMemABS);

        // ZPX (also checks for no page cross with indexing)
        cpuState initStateZPX {0x0000, 0xFF, 0x00, 0x15, 0x00, 0x00};
        std::map<uint16_t, uint8_t> initMemZPX {
            {0x0000, 0x76}, {0x0001, 0xFF},
            {0x0014, 0x3C}
        };
        cpuState finalStateZPX {0x0002, 0xFF, 0x00, 0x15, 0x00, 0x00};
        std::map<uint16_t, uint8_t> finalMemZPX {
            {0xBBAA, 0x1E}
        };
        test(6, initStateZPX, initMemZPX, finalStateZPX, finalMemZPX);

        // ABSX
        cpuState initStateABSX {0x0000, 0xFF, 0x00, 0x03, 0x00, 0x00};
        std::map<uint16_t, uint8_t> initMemABSX {
            {0x0000, 0x7E}, {0x0001, 0xAA}, {0x0002, 0xBB},
            {0xBBAD, 0x3C}
        };
        cpuState finalStateABSX {0x0003, 0xFF, 0x00, 0x03, 0x00, 0x00};
        std::map<uint16_t, uint8_t> finalMemABSX {
            {0xBBAA, 0x1E}
        };
        test(7, initStateABSX, initMemABSX, finalStateABSX, finalMemABSX);

        // ABSX (NO page cross delay +1)
        cpuState initStateABSX2 {0x0000, 0xFF, 0x00, 0x64, 0x00, 0x00};
        std::map<uint16_t, uint8_t> initMemABSX2 {
            {0x0000, 0x7E}, {0x0001, 0xAA}, {0x0002, 0xBB},
            {0xBC0E, 0x3C}
        };
        cpuState finalStateABSX2 {0x0003, 0xFF, 0x00, 0x64, 0x00, 0x00};
        std::map<uint16_t, uint8_t> finalMemABSX2 {
            {0xBBAA, 0x1E}
        };
        test(7, initStateABSX2, initMemABSX2, finalStateABSX2, finalMemABSX2);
    }



    TEST_F(cpuTest, testJMP)    // 0x4C 0x6C
    {
        // ABS
        // IND

        // can also add tests for page boundary errors in the future

        // ABS
        cpuState initStateABS {0x0000, 0xFF, 0x00, 0x00, 0x00, 0x00};
        std::map<uint16_t, uint8_t> initMemABS {
            {0x0000, 0x4C}, {0x0001, 0xAA}, {0x0002, 0xBB},
            {0xBBAA, 0x3C}
        };
        cpuState finalStateABS {0xBBAA, 0xFF, 0x00, 0x00, 0x00, 0x00};
        std::map<uint16_t, uint8_t> finalMemABS {};
        test(3, initStateABS, initMemABS, finalStateABS, finalMemABS);

        // IND
        cpuState initStateINDX {0x0000, 0xFF, 0x00, 0x00, 0x00, 0x00};
        std::map<uint16_t, uint8_t> initMemINDX {
            {0x0000, 0x6C}, {0x0001, 0xAA}, {0x0002, 0xBB},
            {0xBBAA, 0xCC}, {0xBBAB, 0xDD}, 
            {0xDDCC, 0x3C}
        };
        cpuState finalStateINDX {0xDDCC, 0xFF, 0x00, 0x00, 0x00, 0x00};
        std::map<uint16_t, uint8_t> finalMemINDX {};
        test(5, initStateINDX, initMemINDX, finalStateINDX, finalMemINDX);

    }



    TEST_F(cpuTest, testJSR)    // 0x20
    {
        // ABS
        cpuState initStateABS {0x02AA, 0xFF, 0x00, 0x00, 0x00, 0x00};
        std::map<uint16_t, uint8_t> initMemABS {
            {0x02AA, 0x20}, {0x02AB, 0xAA}, {0x02AC, 0xBB},
            {0xBBAA, 0x3C}
        };
        cpuState finalStateABS {0xBBAA, 0xFD, 0x00, 0x00, 0x00, 0x00};
        std::map<uint16_t, uint8_t> finalMemABS {
            {0x01FF, 0x02}, {0x01FE, 0xAC}
        };
        test(6, initStateABS, initMemABS, finalStateABS, finalMemABS);
    }



    TEST_F(cpuTest, testRTS)    // 0x60
    {
        // (JSR ABS)
        cpuState initStateABS {0x02AA, 0xFF, 0x00, 0x00, 0x00, 0x00};
        std::map<uint16_t, uint8_t> initMemABS {
            {0x02AA, 0x20}, {0x02AB, 0xAA}, {0x02AC, 0xBB},
            {0xBBAA, 0x3C}
        };
        cpuState finalStateABS {0xBBAA, 0xFD, 0x00, 0x00, 0x00, 0x00};
        std::map<uint16_t, uint8_t> finalMemABS {
            {0x01FE, 0xAC}, {0x01FF, 0x02}
        };
        test(6, initStateABS, initMemABS, finalStateABS, finalMemABS);

        // IMP
        cpuState initStateIMP {0xBBAA, 0xFD, 0x00, 0x00, 0x00, 0x00};
        std::map<uint16_t, uint8_t> initMemIMP {
            {0xBBAA, 0x60}
        };
        cpuState finalStateIMP {0x02AD, 0xFF, 0x00, 0x00, 0x00, 0x00};
        std::map<uint16_t, uint8_t> finalMemIMP {};
        test(6, initStateIMP, initMemIMP, finalStateIMP, finalMemIMP);
    }



    TEST_F(cpuTest, testBCC)    // 0x90
    {
        // REL (no succeed)
        cpuState initStateREL {0x0000, 0xFF, 0x00, 0x00, 0x00, ricoh2A03::carryFlag};
        std::map<uint16_t, uint8_t> initMemREL {
            {0x0000, 0x90}, {0x0001, 0x0F}
        };
        cpuState finalStateREL {0x0002, 0xFF, 0x00, 0x00, 0x00, ricoh2A03::carryFlag};
        std::map<uint16_t, uint8_t> finalMemREL {};
        test(2, initStateREL, initMemREL, finalStateREL, finalMemREL);

        // REL (succeed positive displaement)
        cpuState initStateREL2 {0x0000, 0xFF, 0x00, 0x00, 0x00, 0x00};
        std::map<uint16_t, uint8_t> initMemREL2 {
            {0x0000, 0x90}, {0x0001, 0x0F}
        };
        cpuState finalStateREL2 {0x0011, 0xFF, 0x00, 0x00, 0x00, 0x00};
        std::map<uint16_t, uint8_t> finalMemREL2 {};
        test(3, initStateREL2, initMemREL2, finalStateREL2, finalMemREL2);

        // REL (succeed negative displaement)
        cpuState initStateREL3 {0xAAAA, 0xFF, 0x00, 0x00, 0x00, 0x00};
        std::map<uint16_t, uint8_t> initMemREL3 {
            {0xAAAA, 0x90}, {0xAAAB, 0xAF}
        };
        cpuState finalStateREL3 {0xAA5B, 0xFF, 0x00, 0x00, 0x00, 0x00};
        std::map<uint16_t, uint8_t> finalMemREL3 {};
        test(3, initStateREL3, initMemREL3, finalStateREL3, finalMemREL3);

        // REL (succeed new page delay)
        cpuState initStateREL4 {0x00F0, 0xFF, 0x00, 0x00, 0x00, 0x00};
        std::map<uint16_t, uint8_t> initMemREL4 {
            {0x00F0, 0x90}, {0x00F1, 0x7F}
        };
        cpuState finalStateREL4 {0x0171, 0xFF, 0x00, 0x00, 0x00, 0x00};
        std::map<uint16_t, uint8_t> finalMemREL4 {};
        test(4, initStateREL4, initMemREL4, finalStateREL4, finalMemREL4);
    }



    TEST_F(cpuTest, testBCS)    // 0xB0
    {
        // REL (no succeed)
        cpuState initStateREL {0x0000, 0xFF, 0x00, 0x00, 0x00, 0x00};
        std::map<uint16_t, uint8_t> initMemREL {
            {0x0000, 0xB0}, {0x0001, 0x0F}
        };
        cpuState finalStateREL {0x0002, 0xFF, 0x00, 0x00, 0x00, 0x00};
        std::map<uint16_t, uint8_t> finalMemREL {};
        test(2, initStateREL, initMemREL, finalStateREL, finalMemREL);

        // REL (succeed positive displaement)
        cpuState initStateREL2 {0x0000, 0xFF, 0x00, 0x00, 0x00, ricoh2A03::carryFlag};
        std::map<uint16_t, uint8_t> initMemREL2 {
            {0x0000, 0xB0}, {0x0001, 0x0F}
        };
        cpuState finalStateREL2 {0x0011, 0xFF, 0x00, 0x00, 0x00, ricoh2A03::carryFlag};
        std::map<uint16_t, uint8_t> finalMemREL2 {};
        test(3, initStateREL2, initMemREL2, finalStateREL2, finalMemREL2);

        // REL (succeed negative displaement)
        cpuState initStateREL3 {0xAAAA, 0xFF, 0x00, 0x00, 0x00, ricoh2A03::carryFlag};
        std::map<uint16_t, uint8_t> initMemREL3 {
            {0xAAAA, 0xB0}, {0xAAAB, 0xAF}
        };
        cpuState finalStateREL3 {0xAA5B, 0xFF, 0x00, 0x00, 0x00, ricoh2A03::carryFlag};
        std::map<uint16_t, uint8_t> finalMemREL3 {};
        test(3, initStateREL3, initMemREL3, finalStateREL3, finalMemREL3);

        // REL (succeed new page delay)
        cpuState initStateREL4 {0x00F0, 0xFF, 0x00, 0x00, 0x00, ricoh2A03::carryFlag};
        std::map<uint16_t, uint8_t> initMemREL4 {
            {0x00F0, 0xB0}, {0x00F1, 0x7F}
        };
        cpuState finalStateREL4 {0x0171, 0xFF, 0x00, 0x00, 0x00, ricoh2A03::carryFlag};
        std::map<uint16_t, uint8_t> finalMemREL4 {};
        test(4, initStateREL4, initMemREL4, finalStateREL4, finalMemREL4);
    }



    TEST_F(cpuTest, testBEQ)    // 0xF0
    {
        // REL (no succeed)
        cpuState initStateREL {0x0000, 0xFF, 0x00, 0x00, 0x00, 0x00};
        std::map<uint16_t, uint8_t> initMemREL {
            {0x0000, 0xF0}, {0x0001, 0x0F}
        };
        cpuState finalStateREL {0x0002, 0xFF, 0x00, 0x00, 0x00, 0x00};
        std::map<uint16_t, uint8_t> finalMemREL {};
        test(2, initStateREL, initMemREL, finalStateREL, finalMemREL);

        // REL (succeed positive displaement)
        cpuState initStateREL2 {0x0000, 0xFF, 0x00, 0x00, 0x00, ricoh2A03::zeroFlag};
        std::map<uint16_t, uint8_t> initMemREL2 {
            {0x0000, 0xF0}, {0x0001, 0x0F}
        };
        cpuState finalStateREL2 {0x0011, 0xFF, 0x00, 0x00, 0x00, ricoh2A03::zeroFlag};
        std::map<uint16_t, uint8_t> finalMemREL2 {};
        test(3, initStateREL2, initMemREL2, finalStateREL2, finalMemREL2);

        // REL (succeed negative displaement)
        cpuState initStateREL3 {0xAAAA, 0xFF, 0x00, 0x00, 0x00, ricoh2A03::zeroFlag};
        std::map<uint16_t, uint8_t> initMemREL3 {
            {0xAAAA, 0xF0}, {0xAAAB, 0xAF}
        };
        cpuState finalStateREL3 {0xAA5B, 0xFF, 0x00, 0x00, 0x00, ricoh2A03::zeroFlag};
        std::map<uint16_t, uint8_t> finalMemREL3 {};
        test(3, initStateREL3, initMemREL3, finalStateREL3, finalMemREL3);

        // REL (succeed new page delay)
        cpuState initStateREL4 {0x00F0, 0xFF, 0x00, 0x00, 0x00, ricoh2A03::zeroFlag};
        std::map<uint16_t, uint8_t> initMemREL4 {
            {0x00F0, 0xF0}, {0x00F1, 0x7F}
        };
        cpuState finalStateREL4 {0x0171, 0xFF, 0x00, 0x00, 0x00, ricoh2A03::zeroFlag};
        std::map<uint16_t, uint8_t> finalMemREL4 {};
        test(4, initStateREL4, initMemREL4, finalStateREL4, finalMemREL4);
    }



    TEST_F(cpuTest, testBMI)    // 0x30
    {
        // REL (no succeed)
        cpuState initStateREL {0x0000, 0xFF, 0x00, 0x00, 0x00, 0x00};
        std::map<uint16_t, uint8_t> initMemREL {
            {0x0000, 0x30}, {0x0001, 0x0F}
        };
        cpuState finalStateREL {0x0002, 0xFF, 0x00, 0x00, 0x00, 0x00};
        std::map<uint16_t, uint8_t> finalMemREL {};
        test(2, initStateREL, initMemREL, finalStateREL, finalMemREL);

        // REL (succeed positive displaement)
        cpuState initStateREL2 {0x0000, 0xFF, 0x00, 0x00, 0x00, ricoh2A03::negativeFlag};
        std::map<uint16_t, uint8_t> initMemREL2 {
            {0x0000, 0x30}, {0x0001, 0x0F}
        };
        cpuState finalStateREL2 {0x0011, 0xFF, 0x00, 0x00, 0x00, ricoh2A03::negativeFlag};
        std::map<uint16_t, uint8_t> finalMemREL2 {};
        test(3, initStateREL2, initMemREL2, finalStateREL2, finalMemREL2);

        // REL (succeed negative displaement)
        cpuState initStateREL3 {0xAAAA, 0xFF, 0x00, 0x00, 0x00, ricoh2A03::negativeFlag};
        std::map<uint16_t, uint8_t> initMemREL3 {
            {0xAAAA, 0x30}, {0xAAAB, 0xAF}
        };
        cpuState finalStateREL3 {0xAA5B, 0xFF, 0x00, 0x00, 0x00, ricoh2A03::negativeFlag};
        std::map<uint16_t, uint8_t> finalMemREL3 {};
        test(3, initStateREL3, initMemREL3, finalStateREL3, finalMemREL3);

        // REL (succeed new page delay)
        cpuState initStateREL4 {0x00F0, 0xFF, 0x00, 0x00, 0x00, ricoh2A03::negativeFlag};
        std::map<uint16_t, uint8_t> initMemREL4 {
            {0x00F0, 0x30}, {0x00F1, 0x7F}
        };
        cpuState finalStateREL4 {0x0171, 0xFF, 0x00, 0x00, 0x00, ricoh2A03::negativeFlag};
        std::map<uint16_t, uint8_t> finalMemREL4 {};
        test(4, initStateREL4, initMemREL4, finalStateREL4, finalMemREL4);
    }



    TEST_F(cpuTest, testBNE)    // 0xD0
    {
        // REL (no succeed)
        cpuState initStateREL {0x0000, 0xFF, 0x00, 0x00, 0x00, ricoh2A03::zeroFlag};
        std::map<uint16_t, uint8_t> initMemREL {
            {0x0000, 0xD0}, {0x0001, 0x0F}
        };
        cpuState finalStateREL {0x0002, 0xFF, 0x00, 0x00, 0x00, ricoh2A03::zeroFlag};
        std::map<uint16_t, uint8_t> finalMemREL {};
        test(2, initStateREL, initMemREL, finalStateREL, finalMemREL);

        // REL (succeed positive displaement)
        cpuState initStateREL2 {0x0000, 0xFF, 0x00, 0x00, 0x00, 0x00};
        std::map<uint16_t, uint8_t> initMemREL2 {
            {0x0000, 0xD0}, {0x0001, 0x0F}
        };
        cpuState finalStateREL2 {0x0011, 0xFF, 0x00, 0x00, 0x00, 0x00};
        std::map<uint16_t, uint8_t> finalMemREL2 {};
        test(3, initStateREL2, initMemREL2, finalStateREL2, finalMemREL2);

        // REL (succeed negative displaement)
        cpuState initStateREL3 {0xAAAA, 0xFF, 0x00, 0x00, 0x00, 0x00};
        std::map<uint16_t, uint8_t> initMemREL3 {
            {0xAAAA, 0xD0}, {0xAAAB, 0xAF}
        };
        cpuState finalStateREL3 {0xAA5B, 0xFF, 0x00, 0x00, 0x00, 0x00};
        std::map<uint16_t, uint8_t> finalMemREL3 {};
        test(3, initStateREL3, initMemREL3, finalStateREL3, finalMemREL3);

        // REL (succeed new page delay)
        cpuState initStateREL4 {0x00F0, 0xFF, 0x00, 0x00, 0x00, 0x00};
        std::map<uint16_t, uint8_t> initMemREL4 {
            {0x00F0, 0xD0}, {0x00F1, 0x7F}
        };
        cpuState finalStateREL4 {0x0171, 0xFF, 0x00, 0x00, 0x00, 0x00};
        std::map<uint16_t, uint8_t> finalMemREL4 {};
        test(4, initStateREL4, initMemREL4, finalStateREL4, finalMemREL4);
    }



    TEST_F(cpuTest, testBPL)    // 0x10
    {
        // REL (no succeed)
        cpuState initStateREL {0x0000, 0xFF, 0x00, 0x00, 0x00, ricoh2A03::negativeFlag};
        std::map<uint16_t, uint8_t> initMemREL {
            {0x0000, 0x10}, {0x0001, 0x0F}
        };
        cpuState finalStateREL {0x0002, 0xFF, 0x00, 0x00, 0x00, ricoh2A03::negativeFlag};
        std::map<uint16_t, uint8_t> finalMemREL {};
        test(2, initStateREL, initMemREL, finalStateREL, finalMemREL);

        // REL (succeed positive displaement)
        cpuState initStateREL2 {0x0000, 0xFF, 0x00, 0x00, 0x00, 0x00};
        std::map<uint16_t, uint8_t> initMemREL2 {
            {0x0000, 0x10}, {0x0001, 0x0F}
        };
        cpuState finalStateREL2 {0x0011, 0xFF, 0x00, 0x00, 0x00, 0x00};
        std::map<uint16_t, uint8_t> finalMemREL2 {};
        test(3, initStateREL2, initMemREL2, finalStateREL2, finalMemREL2);

        // REL (succeed negative displaement)
        cpuState initStateREL3 {0xAAAA, 0xFF, 0x00, 0x00, 0x00, 0x00};
        std::map<uint16_t, uint8_t> initMemREL3 {
            {0xAAAA, 0x10}, {0xAAAB, 0xAF}
        };
        cpuState finalStateREL3 {0xAA5B, 0xFF, 0x00, 0x00, 0x00, 0x00};
        std::map<uint16_t, uint8_t> finalMemREL3 {};
        test(3, initStateREL3, initMemREL3, finalStateREL3, finalMemREL3);

        // REL (succeed new page delay)
        cpuState initStateREL4 {0x00F0, 0xFF, 0x00, 0x00, 0x00, 0x00};
        std::map<uint16_t, uint8_t> initMemREL4 {
            {0x00F0, 0x10}, {0x00F1, 0x7F}
        };
        cpuState finalStateREL4 {0x0171, 0xFF, 0x00, 0x00, 0x00, 0x00};
        std::map<uint16_t, uint8_t> finalMemREL4 {};
        test(4, initStateREL4, initMemREL4, finalStateREL4, finalMemREL4);
    }



    TEST_F(cpuTest, testBVC)    // 0x50
    {
        // REL (no succeed)
        cpuState initStateREL {0x0000, 0xFF, 0x00, 0x00, 0x00, ricoh2A03::overflowFlag};
        std::map<uint16_t, uint8_t> initMemREL {
            {0x0000, 0x50}, {0x0001, 0x0F}
        };
        cpuState finalStateREL {0x0002, 0xFF, 0x00, 0x00, 0x00, ricoh2A03::overflowFlag};
        std::map<uint16_t, uint8_t> finalMemREL {};
        test(2, initStateREL, initMemREL, finalStateREL, finalMemREL);

        // REL (succeed positive displaement)
        cpuState initStateREL2 {0x0000, 0xFF, 0x00, 0x00, 0x00, 0x00};
        std::map<uint16_t, uint8_t> initMemREL2 {
            {0x0000, 0x50}, {0x0001, 0x0F}
        };
        cpuState finalStateREL2 {0x0011, 0xFF, 0x00, 0x00, 0x00, 0x00};
        std::map<uint16_t, uint8_t> finalMemREL2 {};
        test(3, initStateREL2, initMemREL2, finalStateREL2, finalMemREL2);

        // REL (succeed negative displaement)
        cpuState initStateREL3 {0xAAAA, 0xFF, 0x00, 0x00, 0x00, 0x00};
        std::map<uint16_t, uint8_t> initMemREL3 {
            {0xAAAA, 0x50}, {0xAAAB, 0xAF}
        };
        cpuState finalStateREL3 {0xAA5B, 0xFF, 0x00, 0x00, 0x00, 0x00};
        std::map<uint16_t, uint8_t> finalMemREL3 {};
        test(3, initStateREL3, initMemREL3, finalStateREL3, finalMemREL3);

        // REL (succeed new page delay)
        cpuState initStateREL4 {0x00F0, 0xFF, 0x00, 0x00, 0x00, 0x00};
        std::map<uint16_t, uint8_t> initMemREL4 {
            {0x00F0, 0x50}, {0x00F1, 0x7F}
        };
        cpuState finalStateREL4 {0x0171, 0xFF, 0x00, 0x00, 0x00, 0x00};
        std::map<uint16_t, uint8_t> finalMemREL4 {};
        test(4, initStateREL4, initMemREL4, finalStateREL4, finalMemREL4);
    }



    TEST_F(cpuTest, testBVS)    // 0x70
    {
        // REL (no succeed)
        cpuState initStateREL {0x0000, 0xFF, 0x00, 0x00, 0x00, 0x00};
        std::map<uint16_t, uint8_t> initMemREL {
            {0x0000, 0x70}, {0x0001, 0x0F}
        };
        cpuState finalStateREL {0x0002, 0xFF, 0x00, 0x00, 0x00, 0x00};
        std::map<uint16_t, uint8_t> finalMemREL {};
        test(2, initStateREL, initMemREL, finalStateREL, finalMemREL);

        // REL (succeed positive displaement)
        cpuState initStateREL2 {0x0000, 0xFF, 0x00, 0x00, 0x00, ricoh2A03::overflowFlag};
        std::map<uint16_t, uint8_t> initMemREL2 {
            {0x0000, 0x70}, {0x0001, 0x0F}
        };
        cpuState finalStateREL2 {0x0011, 0xFF, 0x00, 0x00, 0x00, ricoh2A03::overflowFlag};
        std::map<uint16_t, uint8_t> finalMemREL2 {};
        test(3, initStateREL2, initMemREL2, finalStateREL2, finalMemREL2);

        // REL (succeed negative displaement)
        cpuState initStateREL3 {0xAAAA, 0xFF, 0x00, 0x00, 0x00, ricoh2A03::overflowFlag};
        std::map<uint16_t, uint8_t> initMemREL3 {
            {0xAAAA, 0x70}, {0xAAAB, 0xAF}
        };
        cpuState finalStateREL3 {0xAA5B, 0xFF, 0x00, 0x00, 0x00, ricoh2A03::overflowFlag};
        std::map<uint16_t, uint8_t> finalMemREL3 {};
        test(3, initStateREL3, initMemREL3, finalStateREL3, finalMemREL3);

        // REL (succeed new page delay)
        cpuState initStateREL4 {0x00F0, 0xFF, 0x00, 0x00, 0x00, ricoh2A03::overflowFlag};
        std::map<uint16_t, uint8_t> initMemREL4 {
            {0x00F0, 0x70}, {0x00F1, 0x7F}
        };
        cpuState finalStateREL4 {0x0171, 0xFF, 0x00, 0x00, 0x00, ricoh2A03::overflowFlag};
        std::map<uint16_t, uint8_t> finalMemREL4 {};
        test(4, initStateREL4, initMemREL4, finalStateREL4, finalMemREL4);
    }



    TEST_F(cpuTest, testCLC)    // 0x18
    {
        // IMP
        cpuState initStateIMP {0x0000, 0xFF, 0xCC, 0xBB, 0xAA, 0xFF};
        std::map<uint16_t, uint8_t> initMemIMP {
            {0x0000, 0x18}
        };
        cpuState finalStateIMP {0x0001, 0xFF, 0xCC, 0xBB, 0xAA, (0xFF ^ ricoh2A03::carryFlag)};
        std::map<uint16_t, uint8_t> finalMemIMP {};
        test(2, initStateIMP, initMemIMP, finalStateIMP, finalMemIMP);
    }



    TEST_F(cpuTest, testCLD)    // 0xD8
    {
        // IMP
        cpuState initStateIMP {0x0000, 0xFF, 0xCC, 0xBB, 0xAA, 0xFF};
        std::map<uint16_t, uint8_t> initMemIMP {
            {0x0000, 0xD8}
        };
        cpuState finalStateIMP {0x0001, 0xFF, 0xCC, 0xBB, 0xAA, (0xFF ^ ricoh2A03::decimalMode)};
        std::map<uint16_t, uint8_t> finalMemIMP {};
        test(2, initStateIMP, initMemIMP, finalStateIMP, finalMemIMP);
    }



    TEST_F(cpuTest, testCLI)    // 0x58
    {
        // IMP
        cpuState initStateIMP {0x0000, 0xFF, 0xCC, 0xBB, 0xAA, 0xFF};
        std::map<uint16_t, uint8_t> initMemIMP {
            {0x0000, 0x58}
        };
        cpuState finalStateIMP {0x0001, 0xFF, 0xCC, 0xBB, 0xAA, (0xFF ^ ricoh2A03::interruptDisable)};
        std::map<uint16_t, uint8_t> finalMemIMP {};
        test(2, initStateIMP, initMemIMP, finalStateIMP, finalMemIMP);
    }



    TEST_F(cpuTest, testCLV)    // 0xB8
    {
        // IMP
        cpuState initStateIMP {0x0000, 0xFF, 0xCC, 0xBB, 0xAA, 0xFF};
        std::map<uint16_t, uint8_t> initMemIMP {
            {0x0000, 0xB8}
        };
        cpuState finalStateIMP {0x0001, 0xFF, 0xCC, 0xBB, 0xAA, (0xFF ^ ricoh2A03::overflowFlag)};
        std::map<uint16_t, uint8_t> finalMemIMP {};
        test(2, initStateIMP, initMemIMP, finalStateIMP, finalMemIMP);
    }



    TEST_F(cpuTest, testSEC)    // 0x38
    {
        // IMP
        cpuState initStateIMP {0x0000, 0xFF, 0xCC, 0xBB, 0xAA, 0x00};
        std::map<uint16_t, uint8_t> initMemIMP {
            {0x0000, 0x38}
        };
        cpuState finalStateIMP {0x0001, 0xFF, 0xCC, 0xBB, 0xAA, ricoh2A03::carryFlag};
        std::map<uint16_t, uint8_t> finalMemIMP {};
        test(2, initStateIMP, initMemIMP, finalStateIMP, finalMemIMP);
    }



    TEST_F(cpuTest, testSED)    // 0xF8
    {
        // IMP
        cpuState initStateIMP {0x0000, 0xFF, 0xCC, 0xBB, 0xAA, 0x00};
        std::map<uint16_t, uint8_t> initMemIMP {
            {0x0000, 0xF8}
        };
        cpuState finalStateIMP {0x0001, 0xFF, 0xCC, 0xBB, 0xAA, ricoh2A03::decimalMode};
        std::map<uint16_t, uint8_t> finalMemIMP {};
        test(2, initStateIMP, initMemIMP, finalStateIMP, finalMemIMP);
    }



    TEST_F(cpuTest, testSEI)    // 0x78
    {
        // IMP
        cpuState initStateIMP {0x0000, 0xFF, 0xCC, 0xBB, 0xAA, 0x00};
        std::map<uint16_t, uint8_t> initMemIMP {
            {0x0000, 0x78}
        };
        cpuState finalStateIMP {0x0001, 0xFF, 0xCC, 0xBB, 0xAA, ricoh2A03::interruptDisable};
        std::map<uint16_t, uint8_t> finalMemIMP {};
        test(2, initStateIMP, initMemIMP, finalStateIMP, finalMemIMP);
    }



    TEST_F(cpuTest, testBRK)    // 0x00
    {
        // IMP
        cpuState initStateIMP {0x02AA, 0xFF, 0xCC, 0xBB, 0xAA, (0xFF ^ ricoh2A03::breakCommand)};
        std::map<uint16_t, uint8_t> initMemIMP {
            {0x02AA, 0x00},
            {0xFFFE, 0xEE}, {0xFFFF, 0xFF}
        };
        cpuState finalStateIMP {0xFFEE, 0xFC, 0xCC, 0xBB, 0xAA, 0xFF};
        std::map<uint16_t, uint8_t> finalMemIMP {
            {0x01FE, 0xAC}, {0x01FF, 0x02},
            {0x01FD, (0xFF ^ ricoh2A03::breakCommand)}
        };
        test(7, initStateIMP, initMemIMP, finalStateIMP, finalMemIMP);
    }



    TEST_F(cpuTest, testNOP)    // 0xEA
    {
        // IMP
        cpuState initStateIMP {0x0000, 0xFF, 0xCC, 0xBB, 0xAA, 0x00};
        std::map<uint16_t, uint8_t> initMemIMP {
            {0x0000, 0xEA}
        };
        cpuState finalStateIMP {0x0001, 0xFF, 0xCC, 0xBB, 0xAA, 0x00};
        std::map<uint16_t, uint8_t> finalMemIMP {};
        test(2, initStateIMP, initMemIMP, finalStateIMP, finalMemIMP);
    }



    TEST_F(cpuTest, testRTI)    // 0x40
    {
        // (BRK IMP)
        cpuState initStateIMP {0x02AA, 0xFF, 0xCC, 0xBB, 0xAA, (0xFF ^ ricoh2A03::breakCommand)};
        std::map<uint16_t, uint8_t> initMemIMP {
            {0x02AA, 0x00},
            {0xFFFE, 0xEE}, {0xFFFF, 0xFF}
        };
        cpuState finalStateIMP {0xFFEE, 0xFC, 0xCC, 0xBB, 0xAA, 0xFF};
        std::map<uint16_t, uint8_t> finalMemIMP {
            {0x01FE, 0xAC}, {0x01FF, 0x02},
            {0x01FD, (0xFF ^ ricoh2A03::breakCommand)}
        };
        test(7, initStateIMP, initMemIMP, finalStateIMP, finalMemIMP);

        // IMP
        cpuState initStateIMP2 {0xFFEE, 0xFC, 0xCC, 0xBB, 0xAA, 0xFF};
        std::map<uint16_t, uint8_t> initMemIMP2 {
            {0xFFEE, 0x40}
        };
        cpuState finalStateIMP2 {0x02AC, 0xFF, 0xCC, 0xBB, 0xAA, (0xFF ^ ricoh2A03::breakCommand)};
        std::map<uint16_t, uint8_t> finalMemIMP2 {};
        test(6, initStateIMP2, initMemIMP2, finalStateIMP2, finalMemIMP2);
    }
}



#endif