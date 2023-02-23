#include <cstdint>
#include <iostream>
#include <string>
#include "include/Mapper.hpp"
#include "include/Memory.hpp"
#include "include/Ricoh2A03.hpp"
#include "include/Ricoh2C02.hpp"
#include "include/APU.hpp"
#include "include/IO.hpp"

#ifdef GTEST
    #include "testModules/gtestModules.hpp"
#elif defined(_WIN32) || defined(__WIN32__) || defined(WIN32) || defined(_WIN64)
    #define SDL_MAIN_HANDLED
    #include "SDL.h"
#else
    #include "SDL2/SDL.h"
#endif

/*
#define MASTER_CLOCK_SPEED  21477272            // (Hz)
#define PPU_CLOCK_SPEED     5369318             // (/4)
#define CPU_CLOCK_SPEED     1789772.667         // (/12)
#define APU_CLOCK_SPEED     CPU_CLOCK_SPEED     // (/12) (additional /2 for APU triangle channel)
*/
#define FPS 60      // defined FPS ~(PPU_CLOCK_SPEED / ((341 * 262) - 0.5))
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
        ricoh2A03::APU apu(memory, &io);
        memory->connect(&cpu, &ppu, &apu);
        memory->initCartridge(ROMfile);
        cpu.rst();
        ppu.rst();

        const float frameMS = 1000.0f / FPS;

        // float processingTime = 0;
        // float renderingTime = 0;

        bool quit = false;
        bool pause = false;
        bool log = false;
        uint8_t clkMod6 = 0;
        while (!quit)
        {
            io.audioPause(pause);
            if (!pause)
            {
                Uint64 begin = SDL_GetPerformanceCounter();
                do
                {
                    if (clkMod6 & 0x01)
                        ppu.tick();
                    if ((clkMod6 == 2) || (clkMod6 == 5))
                    {
                        apu.tick();
                    }
                    if (clkMod6 == 5)
                    {
                        if (apu.DMCReaderDelay() == 0x00)
                        {
                            if (memory->DMAactive())
                                memory->handleDMA();
                            cpu.tick(!(memory->DMAactive()));
                            memory->toggleCpuCycle();
                        }
                    }
                    if (ppu.triggerNMI())
                        cpu.nmi();
                    if ((memory->mapperIrqReq()) || (apu.irqReq()))
                    {
                        cpu.irq();
                        memory->mapperIrqReset();
                        apu.irqReset();
                    }
                    memory->finalizeDMAreq();
                    clkMod6++;
                    if (clkMod6 >= 6)
                        clkMod6 = 0;
                } while ((!ppu.frameComplete()) || ((clkMod6 & 0x01) == 0x00));
                // float elapsedProcess = (((float)(SDL_GetPerformanceCounter() - begin) * 1000.0f) / SDL_GetPerformanceFrequency());
                // processingTime += elapsedProcess;
                io.displayScreen(ppu.getScreen());
                // elapsedProcess = (((float)(SDL_GetPerformanceCounter() - begin) * 1000.0f) / SDL_GetPerformanceFrequency()) - elapsedProcess;
                // renderingTime += elapsedProcess;
                #ifdef DEBUG
                    io.displayChrROM(ppu.getChrROM());
                    io.displayOAM(ppu.getOAM());
                    io.displayNT(ppu.getNT());
                #endif
                float elapsed = (((float)(SDL_GetPerformanceCounter() - begin) * 1000.0f) / SDL_GetPerformanceFrequency());
                if (elapsed < frameMS)
                    SDL_Delay(floor(frameMS - elapsed));
            }
            else
            {
                SDL_Delay(frameMS);
            }
            io.updateInputs(&quit, &pause, &log);
            #ifdef DEBUG
                cpu.enableLog(log);
            #endif
        }
        // no point in multithreading as processing takes significantly more time than rendering
        // std::cout << "processing time: " << processingTime << std::endl;
        // std::cout << "rendering time: " << renderingTime << std::endl;
        io.audioPause(true);
        delete memory;
        return 0;
    #endif
}

// http://nesdev.com/NESDoc.pdf


