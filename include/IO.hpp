#ifndef _IO
#define _IO

#include "SDL2/SDL.h"

#include <cstdint>

namespace NES
{
    class Memory;

    class IO
    {
    public:
        IO(Memory *m);
        ~IO();

        void displayScreen(uint8_t* screen);
        void updateInputs(bool *quit, bool *pause);

    private:
        Memory *mem;
        SDL_Window *window;
        SDL_Renderer *renderer;
        SDL_Texture *texture;

        SDL_Event event;
    };
}

#endif