#include "../include/IO.hpp"
#include "../include/Memory.hpp"



NES::IO::IO(NES::Memory *m) : mem(m)
{
    SDL_Init(SDL_INIT_VIDEO);
    window0 = SDL_CreateWindow("NES Emulation", 0, SDL_WINDOWPOS_UNDEFINED, 512, 480, SDL_WINDOW_RESIZABLE);
    renderer0 = SDL_CreateRenderer(window0, -1, 0);
    texture0 = SDL_CreateTexture(renderer0, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STATIC, 256, 240);
    SDL_SetWindowTitle(window0, "NES Emulator");

    #ifdef DEBUG
        window1 = SDL_CreateWindow("NES Emulation", 512, SDL_WINDOWPOS_UNDEFINED, 256, 512, SDL_WINDOW_RESIZABLE);
        renderer1 = SDL_CreateRenderer(window1, -1, 0);
        texture1 = SDL_CreateTexture(renderer1, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STATIC, 128, 256);
        SDL_SetWindowTitle(window1, "CHR ROM");
        window2 = SDL_CreateWindow("NES Emulation", 768, SDL_WINDOWPOS_UNDEFINED, 128, 256, SDL_WINDOW_RESIZABLE);
        renderer2 = SDL_CreateRenderer(window2, -1, 0);
        texture2 = SDL_CreateTexture(renderer2, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STATIC, 64, 128);
        SDL_SetWindowTitle(window2, "OAM");
        window3 = SDL_CreateWindow("NES Emulation", 896, SDL_WINDOWPOS_UNDEFINED, 512, 480, SDL_WINDOW_RESIZABLE);
        renderer3 = SDL_CreateRenderer(window3, -1, 0);
        texture3 = SDL_CreateTexture(renderer3, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STATIC, 512, 480);
        SDL_SetWindowTitle(window3, "NT");
    #endif
}

NES::IO::~IO()
{
    SDL_DestroyTexture(texture0);
    SDL_DestroyRenderer(renderer0);
    SDL_DestroyWindow(window0);

    #ifdef DEBUG
        SDL_DestroyTexture(texture1);
        SDL_DestroyRenderer(renderer1);
        SDL_DestroyWindow(window1);
        SDL_DestroyTexture(texture2);
        SDL_DestroyRenderer(renderer2);
        SDL_DestroyWindow(window2);
        SDL_DestroyTexture(texture3);
        SDL_DestroyRenderer(renderer3);
        SDL_DestroyWindow(window3);
    #endif

    SDL_Quit();
}

void NES::IO::displayScreen(uint8_t *screen)
{
    SDL_UpdateTexture(texture0, NULL, screen, 256 * sizeof(uint8_t) * 3);
    SDL_RenderClear(renderer0);
    SDL_RenderCopy(renderer0, texture0, NULL, NULL);
    SDL_RenderPresent(renderer0);
}


#ifdef DEBUG
    void NES::IO::displayChrROM(uint8_t *screen)
    {
        SDL_UpdateTexture(texture1, NULL, screen, 128 * sizeof(uint8_t) * 3);
        SDL_RenderClear(renderer1);
        SDL_RenderCopy(renderer1, texture1, NULL, NULL);
        SDL_RenderPresent(renderer1);
    }

    void NES::IO::displayOAM(uint8_t *screen)
    {
        SDL_UpdateTexture(texture2, NULL, screen, 64 * sizeof(uint8_t) * 3);
        SDL_RenderClear(renderer2);
        SDL_RenderCopy(renderer2, texture2, NULL, NULL);
        SDL_RenderPresent(renderer2);
    }

    void NES::IO::displayNT(uint8_t *screen)
    {
        SDL_UpdateTexture(texture3, NULL, screen, 512 * sizeof(uint8_t) * 3);
        SDL_RenderClear(renderer3);
        SDL_RenderCopy(renderer3, texture3, NULL, NULL);
        SDL_RenderPresent(renderer3);
    }
#endif

void NES::IO::updateInputs(bool *quit, bool *pause, bool *log)
{
    uint8_t data0 = mem->controllerRead(0);
    uint8_t data1 = mem->controllerRead(1);
    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
            case SDL_WINDOWEVENT:     // case SDL_QUIT:
                if (event.window.event == SDL_WINDOWEVENT_CLOSE)    // needed to multiple windows
                    *quit = true;
                break;
            case SDL_KEYDOWN:
                switch (event.key.keysym.scancode)
                {
                    case SDL_SCANCODE_G:    // A (1)
                        data0 |= 0x80;
                        break;
                    case SDL_SCANCODE_H:    // B (1)
                        data0 |= 0x40;
                        break;
                    case SDL_SCANCODE_T:    // SELECT (1)
                        data0 |= 0x20;
                        break;
                    case SDL_SCANCODE_Y:    // START (1)
                        data0 |= 0x10;
                        break;
                    case SDL_SCANCODE_W:    // UP (1)
                        data0 |= 0x08;
                        break;
                    case SDL_SCANCODE_S:    // DOWN (1)
                        data0 |= 0x04;
                        break;
                    case SDL_SCANCODE_A:    // LEFT (1)
                        data0 |= 0x02;
                        break;
                    case SDL_SCANCODE_D:    // RIGHT (1)
                        data0 |= 0x01;
                        break;
                    case SDL_SCANCODE_KP_2:     // A (2)
                        data1 |= 0x80;
                        break;
                    case SDL_SCANCODE_KP_3:     // B (2)
                        data1 |= 0x40;
                        break;
                    case SDL_SCANCODE_KP_5:     // SELECT (2)
                        data1 |= 0x20;
                        break;
                    case SDL_SCANCODE_KP_6:     // START (2)
                        data1 |= 0x10;
                        break;
                    case SDL_SCANCODE_UP:       // UP (2)
                        data1 |= 0x08;
                        break;
                    case SDL_SCANCODE_DOWN:     // DOWN (2)
                        data1 |= 0x04;
                        break;
                    case SDL_SCANCODE_LEFT:     // LEFT (2)
                        data1 |= 0x02;
                        break;
                    case SDL_SCANCODE_RIGHT:    // RIGHT (2)
                        data1 |= 0x01;
                        break;
                    case SDL_SCANCODE_P:        // PAUSE (custom input; not on NES controller)
                        *pause = !(*pause);
                        break;
                    case SDL_SCANCODE_L:        // debug toggle (debugging)
                        *log = !(*log);
                        break;
                    default:
                        break;
                }
                break;
            case SDL_KEYUP:
                switch (event.key.keysym.scancode)
                {
                    case SDL_SCANCODE_G:    // A (1)
                        data0 &= ~(0x80);
                        break;
                    case SDL_SCANCODE_H:    // B (1)
                        data0 &= ~(0x40);
                        break;
                    case SDL_SCANCODE_T:    // SELECT (1)
                        data0 &= ~(0x20);
                        break;
                    case SDL_SCANCODE_Y:    // START (1)
                        data0 &= ~(0x10);
                        break;
                    case SDL_SCANCODE_W:    // UP (1)
                        data0 &= ~(0x08);
                        break;
                    case SDL_SCANCODE_S:    // DOWN (1)
                        data0 &= ~(0x04);
                        break;
                    case SDL_SCANCODE_A:    // LEFT (1)
                        data0 &= ~(0x02);
                        break;
                    case SDL_SCANCODE_D:    // RIGHT (1)
                        data0 &= ~(0x01);
                        break;
                    case SDL_SCANCODE_KP_2:     // A (2)
                        data1 &= ~(0x80);
                        break;
                    case SDL_SCANCODE_KP_3:     // B (2)
                        data1 &= ~(0x40);
                        break;
                    case SDL_SCANCODE_KP_5:     // SELECT (2)
                        data1 &= ~(0x20);
                        break;
                    case SDL_SCANCODE_KP_6:     // START (2)
                        data1 &= ~(0x10);
                        break;
                    case SDL_SCANCODE_UP:       // UP (2)
                        data1 &= ~(0x08);
                        break;
                    case SDL_SCANCODE_DOWN:     // DOWN (2)
                        data1 &= ~(0x04);
                        break;
                    case SDL_SCANCODE_LEFT:     // LEFT (2)
                        data1 &= ~(0x02);
                        break;
                    case SDL_SCANCODE_RIGHT:    // RIGHT (2)
                        data1 &= ~(0x01);
                        break;
                    default:
                        break;
                }
                break;
            default:
                break;
        }
    }
    mem->controllerWrite(0, data0);
    mem->controllerWrite(1, data1);
}


