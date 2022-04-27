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
#elif defined(_WIN32) || defined(__WIN32__) || defined(WIN32) || defined(_WIN64)
    #define SDL_MAIN_HANDLED
    #include "SDL.h"
#else
    #include "SDL2/SDL.h"
#endif

#define FPS 60      // defined FPS; change this to whatever you see fit
                    // https://wiki.nesdev.org/w/index.php?title=Cycle_reference_chart#Clock_rates

int main(int argc, char **argv)
{
    #ifdef GTEST
        ::testing::InitGoogleTest(&argc, argv);
        return RUN_ALL_TESTS();
    #else
        std::string ROMfile;
        if (argc != 2)
        {
            std::cout << "enter file path to .nes cartridge file" << std::endl << "exiting" << std::endl;
            return 0;
        }
        else
            ROMfile = std::string(argv[1]);
        NES::Memory *memory = new NES::NESmemory();
        ricoh2A03::CPU cpu(memory);
        ricoh2C02::PPU ppu(memory);
        NES::IO io(memory);
        memory->connectCPUandPPU(&cpu, &ppu);
        memory->initCartridge(ROMfile);
        cpu.rst();
        ppu.rst();

        const uint32_t frameMS = 1000 / FPS;
        uint32_t now = SDL_GetTicks();
        uint32_t nextFrame;

        bool quit = false;
        bool pause = false;
        bool log = false;
        uint8_t clkMod3 = 0;
        while (!quit)
        {
            if (!pause)
            {
                nextFrame = now + frameMS;
                now = SDL_GetTicks();
                if (now < nextFrame)
                    SDL_Delay(nextFrame - now);
                do
                {
                    ppu.tick();
                    if (clkMod3 == 0)
                    {
                        if (memory->DMAactive())
                            memory->handleDMA();
                        cpu.tick(!(memory->DMAactive()));
                        memory->toggleCpuCycle();
                    }
                    if (ppu.triggerNMI())
                    {
                        cpu.nmi();
                    }
                    if (memory->mapperIrqReq())
                    {
                        cpu.irq();
                        memory->mapperIrqReset();
                    }
                    memory->finalizeDMAreq();
                    if (clkMod3 >= 2)
                        clkMod3 = 0;
                    else
                        clkMod3 += 1;
                } while (!ppu.frameComplete());
                io.displayScreen(ppu.getScreen());
                #ifdef DEBUG
                    io.displayChrROM(ppu.getChrROM());
                    io.displayOAM(ppu.getOAM());
                    io.displayNT(ppu.getNT());
                #endif
            }
            else
                SDL_Delay(frameMS);
            io.updateInputs(&quit, &pause, &log);
            #ifdef DEBUG
                cpu.enableLog(log);
            #endif
        }
        delete memory;
        return 0;
    #endif
}

// http://nesdev.com/NESDoc.pdf


