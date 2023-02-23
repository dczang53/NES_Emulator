#ifndef _IO
#define _IO

#if defined(_WIN32) || defined(__WIN32__) || defined(WIN32) || defined(_WIN64)
    #define SDL_MAIN_HANDLED
    #include "SDL.h"
#else
    #include "SDL2/SDL.h"
#endif

#define AUDIO_LATENCY_SAMPLES   (44100 / 20)    // 1/20 second of latency -> 1/20 * 44100 samples of latency
#define AUDIO_FRAME_SAMPLES     1024

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
        
        void audioAddSample(uint8_t sample);
        void audioPause(bool p);

        static void audioCallback(void* userdata, uint8_t* stream, int len);

        int audioSampleRate();

    private:
        Memory *mem;

        SDL_Window *window0;
        SDL_Renderer *renderer0;
        SDL_Texture *texture0;

        // APU
        SDL_AudioSpec audioTarget, audioHave;
        SDL_AudioDeviceID audioHandler;

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
        
        // SDL sound callback data (circular buffer)
        inline static uint8_t soundBuffer[AUDIO_LATENCY_SAMPLES * 2] = {0};     // sound buffer
        inline static int32_t soundBufferIndex = 0;                             // position of buffer write position
        inline static int32_t soundBufferStart = 0;
        inline static bool soundBufferLoop = false;

        inline static bool audioPaused = true;              // explicit pause from game loop
        inline static bool audioPlaybackPaused = true;      // brief pause from not enough samples in sound buffer
    };
}

#endif
