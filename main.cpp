#include <cstdint>
#include <iostream>
#include <string>
#include "include/Mapper.hpp"
#include "include/Memory.hpp"
#include "include/Ricoh2A03.hpp"
#include "include/IO.hpp"
#include "include/Ricoh2C02.hpp"


#ifdef GTEST
    #include "testModules/gtestModules.hpp"
#else
    #include "SDL2/SDL.h"
#endif

int main(int argc, char **argv)
{
    #ifdef GTEST
        ::testing::InitGoogleTest(&argc, argv);
        return RUN_ALL_TESTS();
    #else
        if (argc != 2)
            return 0;
        std::string ROMfile(argv[1]);
        ROMfile = "ROM/" + ROMfile;
        NES::Memory *memory = new NES::NESmemory(ROMfile);  // testing with "dk.nes"
        ricoh2A03::CPU cpu(memory);
        ricoh2C02::PPU ppu(memory);
        NES::IO io(memory);
        memory->connectCPUandPPU(&cpu, &ppu);
        cpu.rst();
        ppu.rst();

        const uint32_t frameMS = 1000 / 60;     // definitely not even close to original clock speed
        uint32_t nextFrame = SDL_GetTicks();    // just want to run at 60fps; change the 60 in line above for fps

        bool quit = false;
        bool pause = false;
        uint8_t clkMod3 = 0;
        while (!quit)
        {
            uint32_t now = SDL_GetTicks();
            if (now < nextFrame)
                SDL_Delay(nextFrame - now);
            if (!pause)
            {
                do
                {
                    ppu.tick();
                    if (clkMod3 == 0)
                    {
                        if (memory->DMAactive())
                            memory->handleDMA();
                        else
                            cpu.tick();
                        memory->toggleCpuCycle();
                    }
                    if (ppu.triggerNMI())
                        cpu.nmi();
                    memory->finalizeDMAreq();
                    if (clkMod3 >= 2)
                        clkMod3 = 0;
                    else
                        clkMod3 += 1;
                } while (!ppu.frameComplete());
                io.displayScreen(ppu.getScreen());
            }
            io.updateInputs(&quit, &pause);
            nextFrame += frameMS;
        }
        delete memory;
        return 0;
    #endif
}

// http://nesdev.com/NESDoc.pdf


