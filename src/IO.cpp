#include "../include/IO.hpp"
#include "../include/Memory.hpp"

NES::IO::IO(NES::Memory *m) : mem(m)
{
    SDL_Init(SDL_INIT_VIDEO);
    window = SDL_CreateWindow("NES Emulation", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 256, 240, SDL_WINDOW_RESIZABLE);
    renderer = SDL_CreateRenderer(window, -1, 0);
    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STATIC, 256, 240);
}

NES::IO::~IO()
{
    SDL_Quit();
}

void NES::IO::displayScreen(uint8_t *screen)
{
    SDL_UpdateTexture(texture, NULL, screen, 256 * sizeof(uint8_t) * 3);
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);
}

void NES::IO::updateInputs(bool *quit, bool *pause)
{
    uint8_t data0 = mem->controllerRead(0);
    uint8_t data1 = mem->controllerRead(1);
    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
            case SDL_QUIT:
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
                    case SDL_SCANCODE_P:    // PAUSE (custom input; not on NES controller)
                        *pause = !(*pause);
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


