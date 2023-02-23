#ifndef _RICOH2A03_APU
#define _RICOH2A03_APU

#define USE_LOOKUP_TABLE 1
#define USE_LINEAR_APPROX 0
#define USE_FILTER 0

#include <cstdint>
#include <cmath>
#include "../include/IO.hpp"
#include <iostream>

namespace NES
{
    class IO;
    class Memory;
}

namespace ricoh2A03
{
    class APU
    {
        template<unsigned totalSize=1, unsigned byteOffset=0, unsigned bitOffset=0, unsigned numBits=1> // assume (bitOffset + numBits <= 8), or no field overlaps multiple bytes
        struct regField
        {
            uint8_t data[totalSize];
            enum
            {
                shiftLeft = 8 - bitOffset - numBits,
                byteMask = (((0x0001 << numBits) - 1) << shiftLeft)
            };
            void set(uint8_t val)
            {
                data[byteOffset] = (data[byteOffset] & ~byteMask) | ((val << shiftLeft) & byteMask);
            }
            uint8_t operator()()
            {
                return ((data[byteOffset] & byteMask) >> shiftLeft);
            }
            /*
            void setBits(uint8_t val)
            {
                data[byteOffset] |= ((val << shiftLeft) & byteMask);
            }
            void unsetBits(uint8_t val)
            {
                data[byteOffset] &= ((~byteMask) | ((~(val << shiftLeft)) & byteMask));
            }
            void operator+=(uint8_t inc)
            {
                uint8_t val = ((data[byteOffset] + (inc << shiftLeft)) & byteMask);
                data[byteOffset] = (data[byteOffset] & ~byteMask) | val;
            }
            void operator-=(uint8_t dec)
            {
                uint8_t val = ((data[byteOffset] - (dec << shiftLeft)) & byteMask);
                data[byteOffset] = (data[byteOffset] & ~byteMask) | val;
            }
            */
        };

        /*
        // Pulse Channels
                       +---------+    +---------+
                       |  Sweep  |--->|Timer / 2|
                       +---------+    +---------+
                            |              |
                            |              v
                            |         +---------+    +---------+
                            |         |Sequencer|    | Length  |
                            |         +---------+    +---------+
                            |              |              |
                            v              v              v
        +---------+        |\             |\             |\          +---------+
        |Envelope |------->| >----------->| >----------->| >-------->|   DAC   |
        +---------+        |/             |/             |/          +---------+
        */
       
        struct PulseChannel
        {
            union regs
            {
                uint8_t reg[4];
                regField<4,0,0,2> duty;             // DD------ (unused)
                regField<4,0,2,1> lenCounterHalt;   // --L-----
                regField<4,0,3,1> constVol;         // ---C----
                regField<4,0,4,4> envelope;         // ----VVVV
                regField<4,1,0,1> enable;           // E-------
                regField<4,1,1,3> period;           // -PPP----
                regField<4,1,4,1> negate;           // ----N---
                regField<4,1,5,3> shift;            // -----SSS
                regField<4,2,0,8> timerLow;         // TTTTTTTT
                regField<4,3,0,5> lenCounterLoad;   // LLLLL---
                regField<4,3,5,3> timerHigh;        // -----TTT
            } regs = {0x30, 0x08, 0x00, 0x00};

            // envelope unit
            uint8_t envelopeDivider = 0;
            uint8_t envelopeCounter = 0;
            bool envelopeStart = false;

            // sweep unit
            uint8_t sweepDivider = 0;
            uint8_t sweepShifter = 0;
            uint8_t sweepTimer = 0;
            bool sweepReload = false;
            uint16_t sweepChange = 0x00;
            uint16_t sweepTargetPeriod = 0x00;

            // sequencer unit
            uint8_t sequenceValue = 0x00;
            uint8_t sequenceReload = 0x00;
            uint16_t sequenceTimer = 0x00;

            // length unit
            uint8_t lengthCounter = 0;

            // get "current period"
            uint16_t getSequencePeriod()
            {
                return (((uint16_t)(regs.timerHigh()) << 8) | regs.timerLow());
            }

            // set "current period"
            void setSequencePeriod(uint16_t val)
            {
                regs.timerHigh.set((uint8_t)(val >> 8));
                regs.timerLow.set((uint8_t)(val & 0x00FF));
            }

            // output 0x00-0x0F || (getSequencePeriod() < 8)
            uint8_t sample()
            {
                if (((regs.negate() == 0x00) && (sweepTargetPeriod > 0x7FF)) || ((sequenceValue & 0x01) == 0x00) || (sequenceTimer < 8) || (lengthCounter == 0))
                    return 0x00;
                return (regs.constVol())? regs.envelope() : envelopeCounter;
            }

        } PulseChannel1, PulseChannel2;     // $4000-4003, $4004-4007

        /*
        // Triangle Channel
                       +---------+    +---------+
                       |LinearCtr|    | Length  |
                       +---------+    +---------+
                            |              |
                            v              v
        +---------+        |\             |\         +---------+    +---------+
        |  Timer  |------->| >----------->| >------->|Sequencer|--->|   DAC   |
        +---------+        |/             |/         +---------+    +---------+
        */

        struct TriangleChannel
        {
            union regs
            {
                uint8_t reg[4];
                regField<4,0,0,1> lenCounterHalt;   // C-------
                regField<4,0,1,7> linCounterLoad;   // -RRRRRRR
                regField<4,2,0,8> timerLow;         // TTTTTTTT
                regField<4,3,0,5> lenCounterLoad;   // LLLLL---
                regField<4,3,5,3> timerHigh;        // -----TTT
            } regs = {0x80, 0x00, 0x00, 0x00};

            // linear unit
            uint8_t linearCounter = 0;
            bool linearHalt = false;

            // length unit
            uint8_t lengthCounter = 0;

            // sequencer unit
            uint8_t sequenceValue = 0x00;
            bool sequenceHalfPeriod = false;
            uint16_t sequenceTimer = 0x00;

            // get "current period"
            uint16_t getSequencePeriod()
            {
                return (((uint16_t)(regs.timerHigh()) << 8) | regs.timerLow());
            }

            // output 0x00-0x0F
            uint8_t sample()
            {
                return sequenceValue;       // sequencer is "silenced" if value doesn't increment/decrement
            }
        } TriangleChannel;                  // $4008-400B

        /*
        // Noise Channel
        +---------+    +---------+    +---------+
        |  Timer  |--->| Random  |    | Length  |
        +---------+    +---------+    +---------+
                            |              |
                            v              v
        +---------+        |\             |\         +---------+
        |Envelope |------->| >----------->| >------->|   DAC   |
        +---------+        |/             |/         +---------+
        */

        struct NoiseChannel
        {
            union regs
            {
                uint8_t reg[4];
                regField<4,0,2,1> lenCounterHalt;   // --L-----
                regField<4,0,3,1> constVol;         // ---C----
                regField<4,0,4,4> envelope;         // ----VVVV
                regField<4,2,0,1> loopNoise;        // L-------
                regField<4,2,4,4> noisePeriod;      // ----PPPP
                regField<4,3,0,5> lenCounterLoad;   // LLLLL---
            } regs = {0x30, 0x00, 0x00, 0x00};

            // envelope unit
            uint8_t envelopeDivider = 0;
            uint8_t envelopeCounter = 0;
            bool envelopeStart = false;

            // random unit
            uint16_t randomValue = 0x01;
            uint16_t randomTimer = 0;
            inline static const uint16_t randomPeriodTable[16] =
            {
                0x0004,
                0x0008,
                0x0010,
                0x0020,
                0x0040,
                0x0060,
                0x0080,
                0x00A0,
                0x00CA,
                0x00FE,
                0x017C,
                0x01FC,
                0x02FA,
                0x03F8,
                0x07F2,
                0x0FE4
            };

            // length unit
            uint8_t lengthCounter = 0;

            // output 0x00-0x0F
            uint8_t sample()
            {
                if (((randomValue & 0x0001) == 0x00) || (lengthCounter == 0))
                    return 0x00;
                return (regs.constVol())? regs.envelope() : envelopeCounter;
            }
        } NoiseChannel;                     // $400C-400F

        /*
        // DMC Channel
        // note difference between implementation and diagram naming
        +----------+    +---------+
        |DMA Reader|    |  Timer  |
        +----------+    +---------+
            |               |
            |               v
        +----------+    +---------+     +---------+     +---------+ 
        |  Buffer  |----| Output  |---->| Counter |---->|   DAC   |
        +----------+    +---------+     +---------+     +---------+
        */

        struct DMCChannel   // note: DMC channel has not been rigorously tested
        {
            union regs
            {
                uint8_t reg[4];
                regField<4,0,0,1> irqEnable;    // I-------
                regField<4,0,1,1> loop;         // -L------
                regField<4,0,4,4> freq;         // ----RRRR
                regField<4,1,1,7> loadCounter;  // -DDDDDDD 	
                regField<4,2,0,8> sampleAddr;   // AAAAAAAA
                regField<4,3,0,8> sampleLen;    // LLLLLLLL
            } regs = {0x00, 0x00, 0x00, 0x00};

            bool interruptFlag = false;

            // reader unit
            uint16_t readerAddr = 0x00;
            uint16_t readerBytesRemaining = 0;
            uint8_t readerDelay = 0;    // (note: simplied the CPU delay to a hard 4 cycles [originally 1-4 cycles to delay depending on too many conditions])
                                        // see differences in "https://www.nesdev.org/wiki/APU_DMC" vs "https://www.nesdev.org/apu_ref.txt"

            // buffer unit
            uint8_t sampleBuffer = 0x00;
            bool sampleEmpty = true;

            // output unit
            uint8_t outputBuffer = 0x00;
            uint8_t outputCounter = 1;
            bool outputSilence = true;
            uint16_t outputTimer = 0;

            // counter unit
            uint8_t counterOutput = 0x00;

            inline static const uint16_t dmcPeriodTable[16] =
            {
                0x01AC,
                0x017C,
                0x0154,
                0x0140,
                0x011E,
                0x00FE,
                0x00E2,
                0x00D6,
                0x00BE,
                0x00A0,
                0x008E,
                0x0080,
                0x006A,
                0x0054,
                0x0048,
                0x0036
            };

            // sample (re)start
            void sampleRestart()
            {
                readerAddr = ((uint16_t)(regs.sampleAddr()) << 6) + 0xC000;
                readerBytesRemaining = ((uint16_t)(regs.sampleLen()) << 4) + 1;
            }

            // output 0x00-0x7F
            uint8_t sample()
            {
                return (counterOutput & 0x7F);
            }
        } DMCChannel;                       // $4010-4013

        // length counter table (same for all channels)
        // taken directly from "https://www.nesdev.org/apu_ref.txt"
        inline static const uint8_t lengthCounterTable[] = {
            0x0A, 0xFE,
            0x14, 0x02,
            0x28, 0x04,
            0x50, 0x06,
            0xA0, 0x08,
            0x3C, 0x0A,
            0x0E, 0x0C,
            0x1A, 0x0E,
            0x0C, 0x10,
            0x18, 0x12,
            0x30, 0x14,
            0x60, 0x16,
            0xC0, 0x18,
            0x48, 0x1A,
            0x10, 0x1C,
            0x20, 0x1E
        };

        // "https://forums.nesdev.org/viewtopic.php?t=8602"
        // heavy aliasing as original NES outputs at 1.8MHz, and we sample much less than that
        // use low-pass filter to downsample audio to 1/2 frequency of sampling rate
        // lowpass FIR filter @ 22.05kHz
        // window-sinc method (impulse response calculation and windowing ripped from "https://rjeschke.tumblr.com/post/8382596050/fir-filters-in-practice")
        #if USE_FILTER
            struct lowpassFilter
            {
                #define order 200

                #define cutoff_freq 20000.0f    // (keep this under 22050.0f)
                #define sample_rate 44100.0f

                #define sinc(x) ((x != 0.0f)? (sin(x * M_PI) / (x * M_PI)) : 1.0f)

                lowpassFilter()
                {
                    if (!impulseResponseInit)
                    {
                        impulseResponseInit = true;
                        // maxAmplitude = 0.0f;
                        double cutoff = cutoff_freq / sample_rate;
                        double factor = 2.0f * cutoff;
                        int32_t half = order >> 1;
                        for(int i = 0; i <= order; i++)
                        {
                            impulseResponse[i] = factor * sinc(factor * (i - half));                                                                                                    // sinc() impulse response w/ no windowing
                            // std::cout << factor * sinc(factor * (i - half)) << std::endl;
                            impulseResponse[i] *= (0.42f - (0.5f * cos(2.0f * M_PI * (double)(i) / (double)(order))) + (0.08f * cos(4.0f * M_PI * (double)(i) / (double)(order))));     // applying Blackman window
                            // std::cout << (0.42f - (0.5f * cos(2.0f * M_PI * (double)(i) / (double)(order))) + (0.08f * cos(4.0f * M_PI * (double)(i) / (double)(order)))) << std::endl;
                            std::cout << impulseResponse[i] << std::endl;
                            // maxAmplitude += (255.0f * impulseResponse[i] * ((impulseResponse[i] >= 0.0f)? 1.0f : -1.0f));
                        }
                        // std::cout << maxAmplitude << std::endl;
                    }
                }

                ~lowpassFilter() {}

                inline static bool impulseResponseInit = false;
                inline static double impulseResponse[order + 1];    // for convolution
                // inline static double maxAmplitude = 0.0f;
                uint8_t sampleBuffer[order + 1] = {0};
                uint8_t currIndex = 0;

                uint8_t processSample(uint8_t sample)
                {
                    sampleBuffer[currIndex++] = sample;
                    if (currIndex > order)
                        currIndex = 0;
                    double result = 0.0f;
                    int convIndex = currIndex;
                    for (int i = 0; i <= order; i++)
                    {
                        convIndex--;
                        if (convIndex < 0)
                            convIndex = order;
                        result += impulseResponse[i] * (double)(sampleBuffer[convIndex]);
                    }
                    return (result > 0.0f)? (uint8_t)(floor(result)) : 0x00;
                    // std::cout << result << std::endl;
                    // return (result > 0.0f)? (uint8_t)((result / maxAmplitude) * 255.0f) : 0x00;
                    // return (uint8_t)(result);
                }

                #undef order
                #undef cutoff_freq
                #undef sample_rate
                #undef sinc
            } LowpassFilter;
        #endif

        #if USE_LOOKUP_TABLE
            inline static double pulseTable[31] = {0};
            inline static double tndTable[203] = {0};
        #endif

    public:
        APU(NES::Memory *m, NES::IO *io) : mem(m), IO(io), samplesPerTick(((double)(io->audioSampleRate()) * 1.5f) / (((341 * 262 * 2) - 1.0f) * 60.0f * 0.5f))
        {
            #if USE_LOOKUP_TABLE
                pulseTable[0] = 0.0f;
                for (int i = 1; i < 31; i++)
                {
                    pulseTable[i] = 95.52f / ((8128.0f / (double)(i)) + 100.0f);
                }
                tndTable[0] = 0.0f;
                for (int i = 1; i < 203; i++)
                {
                    tndTable[i] = 163.67f / ((24329.0f / (double)(i)) + 100);
                }
            #endif
        }

        ~APU() {}

        uint8_t cpuRead(uint16_t addr);
        bool cpuWrite(uint16_t addr, uint8_t data);

        void tick();

        bool irqReq() {return IRQ;}
        void irqReset() {IRQ = false;}

        uint8_t DMCReaderDelay() { return ((DMCChannel.readerDelay)? DMCChannel.readerDelay-- : 0); }

    private:
        // reader unit operation (called only if sampleBuffer is empty and readerBytesRemaining is not 0)
        void DMCReaderFetch();

        NES::Memory *mem = nullptr;
        NES::IO *IO = nullptr;

        double samplesToGenerateOffset = 0.0f;

        uint8_t statusReg = 0x00;                       // $4015 (IF-DNT21) (channel length counter enable flags)
        uint8_t frameCounterReg = 0x00;                 // $4017
        
        // frame sequencer variables
        uint8_t dividerCnt = 0;
        uint16_t dividerTick = 0;

        // timer count (note APU runs at twice CPU clock speed)
        uint8_t timerCount = 0;     // (0-3)

        bool IRQ = false;
        bool IRQset = false;

        // approx number of samples to buffer (may miss a single sample occasionally, but IO fills blanks and sample rate should be high enough it shouldn't be noticeable?)
        const double samplesPerTick;

        /*
        uint8_t reg[24] = {\x30, \x08, \x00, \x00,
                    \x30, \x08, \x00, \x00,
                    \x80, \x00, \x00, \x00,
                    \x30, \x00, \x00, \x00,
                    \x00, \x00, \x00, \x00,
                    \x30, \x00, \x00, \x00};     // $4000-$4017
        */
    };
}
#endif

/*
(https://www.nesdev.org/wiki/APU)
APU registers synopsis:
    Pulse 1/2:
    $4000 / $4004	DDLC VVVV	Duty (D), envelope loop / length counter halt (L), constant volume (C), volume/envelope (V)
    $4001 / $4005	EPPP NSSS	Sweep unit: enabled (E), period (P), negate (N), shift (S)
    $4002 / $4006	TTTT TTTT	Timer low (T)
    $4003 / $4007	LLLL LTTT	Length counter load (L), timer high (T)

    Triangle:
    $4008	CRRR RRRR	Length counter halt / linear counter control (C), linear counter load (R)
    $4009	---- ----	Unused
    $400A	TTTT TTTT	Timer low (T)
    $400B	LLLL LTTT	Length counter load (L), timer high (T)

    Noise:
    $400C	--LC VVVV	Envelope loop / length counter halt (L), constant volume (C), volume/envelope (V)
    $400D	---- ----	Unused
    $400E	L--- PPPP	Loop noise (L), noise period (P)
    $400F	LLLL L---	Length counter load (L)

    DMC:
    $4010	IL-- RRRR	IRQ enable (I), loop (L), frequency (R)
    $4011	-DDD DDDD	Load counter (D)
    $4012	AAAA AAAA	Sample address (A)
    $4013	LLLL LLLL	Sample length (L)

    Status:
    $4015 write	---D NT21	Enable DMC (D), noise (N), triangle (T), and pulse channels (2/1)

    Frame Counter:
    $4017	MI-- ----	Mode (M, 0 = 4-step, 1 = 5-step), IRQ inhibit flag (I)

(https://www.nesdev.org/wiki/APU_Mixer)
linear approximation formula: (using pulse table approximation)
    output = pulse_out + tnd_out
    pulse_table [n] = 95.52 / (8128.0 / n + 100)
    pulse_out = pulse_table [pulse1 + pulse2]
    tnd_table [n] = 163.67 / (24329.0 / n + 100)
    tnd_out = tnd_table [3 * triangle + 2 * noise + dmc]
    
    (pulse = pulse1 + pulse2)
    (tnd = triangle + noise + dmc)
    (tnd_output accuracy within 4% error)
    (dmc: 0-127)
    (all others: 0-15)
    ("When the values for one of the groups are all zero, the result for that group should be treated as zero rather than undefined due to the division by 0 that otherwise results.")
    (ie is pulse1 + pulse2 is 0, pulse_out ~= [95.52 / inf] = 0)
*/


// https://forums.nesdev.org/viewtopic.php?t=8602
// NOTE: because we need to downsample the original 1.8MHz signal to 22.05kHz, we need to add a low-pass filter