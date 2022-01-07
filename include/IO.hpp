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

        #ifdef DEBUG
            void displayChrROM(uint8_t* screen);
            void displayOAM(uint8_t* screen);
            void displayNT(uint8_t* screen);
        #endif
        
        void updateInputs(bool *quit, bool *pause, bool *log);

    private:
        Memory *mem;

        SDL_Window *window0;
        SDL_Renderer *renderer0;
        SDL_Texture *texture0;

        #ifdef DEBUG
            SDL_Window *window1;
            SDL_Renderer *renderer1;
            SDL_Texture *texture1;

            SDL_Window *window2;
            SDL_Renderer *renderer2;
            SDL_Texture *texture2;

            SDL_Window *window3;
            SDL_Renderer *renderer3;
            SDL_Texture *texture3;
        #endif

        SDL_Event event;
    };
}

#endif