#include "../include/APU.hpp"
#include "../include/Memory.hpp"

#include <iostream>
#include <iomanip>

uint8_t ricoh2A03::APU::cpuRead(uint16_t addr)
{
    // all registers are write-only except for status register
    if (addr == 0x4015)
    {
        uint8_t ret = 0x00;
        if (PulseChannel1.lengthCounter)
            ret |= 0x01;
        if (PulseChannel2.lengthCounter)
            ret |= 0x02;
        if (TriangleChannel.lengthCounter)
            ret |= 0x04;
        if (NoiseChannel.lengthCounter)
            ret |= 0x08;
        if (DMCChannel.readerBytesRemaining)
            ret |= 0x10;
        if (IRQ)
        {
            ret |= 0x40;
            if (!IRQset)        // "if an interrupt flag was set at the same moment of the read, it will read back as 1 but it will not be cleared"
                IRQ = false;
        }
        if (DMCChannel.interruptFlag)
            ret |= 0x10;
        return ret;
    }
    return 0x00;

}

bool ricoh2A03::APU::cpuWrite(uint16_t addr, uint8_t data)
{
    if ((addr & 0xFFE0) == 0x4000)
    {
        uint8_t reg = addr & 0x001F;
        switch (reg)
        {
            case 0:
            case 1:
            case 2:
            case 3:
                PulseChannel1.regs.reg[reg] = data;
                if (reg == 0)
                {
                    switch (((data & 0xC0) >> 6))
                    {
                        case 0:
                            PulseChannel1.sequenceReload = 0x02;    // 0x40;
                            break;
                        case 1:
                            PulseChannel1.sequenceReload = 0x06;    // 0x60;
                            break;
                        case 2:
                            PulseChannel1.sequenceReload = 0x1E;    // 0x78;
                            break;
                        case 3:
                            PulseChannel1.sequenceReload = 0xF9;    // 0x9F;
                            break;
                        default:
                            break;
                    }
                    PulseChannel1.sequenceValue = PulseChannel1.sequenceReload;
                }
                else if (reg == 1)
                {
                    PulseChannel1.sweepReload = true;
                }
                else if (reg == 3)
                {
                    PulseChannel1.sequenceTimer = PulseChannel1.getSequencePeriod();
                    PulseChannel1.sequenceValue = PulseChannel1.sequenceReload;
                    PulseChannel1.envelopeStart = true;
                    if (statusReg & 0x01)
                        PulseChannel1.lengthCounter = lengthCounterTable[PulseChannel1.regs.lenCounterLoad()];
                }
                return true;
                break;
            case 4:
            case 5:
            case 6:
            case 7:
                PulseChannel2.regs.reg[reg & 0x03] = data;
                if ((reg & 0x03) == 0)
                {
                    switch (((data & 0xC0) >> 6))
                    {
                        case 0:
                            PulseChannel2.sequenceReload = 0x02;    // 0x40;
                            break;
                        case 1:
                            PulseChannel2.sequenceReload = 0x06;    // 0x60;
                            break;
                        case 2:
                            PulseChannel2.sequenceReload = 0x1E;    // 0x78;
                            break;
                        case 3:
                            PulseChannel2.sequenceReload = 0xF9;    // 0x9F;
                            break;
                        default:
                            break;
                    }
                    PulseChannel2.sequenceValue = PulseChannel2.sequenceReload;
                }
                else if ((reg & 0x03) == 1)
                {
                    PulseChannel2.sweepReload = true;
                }
                else if ((reg & 0x03) == 3)
                {
                    PulseChannel2.sequenceTimer = PulseChannel2.getSequencePeriod();
                    PulseChannel2.sequenceValue = PulseChannel2.sequenceReload;
                    PulseChannel2.envelopeStart = true;
                    if (statusReg & 0x02)
                        PulseChannel2.lengthCounter = lengthCounterTable[PulseChannel2.regs.lenCounterLoad()];
                }
                return true;
                break;
            case 8:
            case 9:
            case 10:
            case 11:
                TriangleChannel.regs.reg[reg & 0x03] = data;
                if ((reg & 0x03) == 3)
                {
                    TriangleChannel.linearHalt = true;
                    if (statusReg & 0x04)
                        TriangleChannel.lengthCounter = lengthCounterTable[TriangleChannel.regs.lenCounterLoad()];
                }
                return true;
                break;
            case 12:
            case 13:
            case 14:
            case 15:
                NoiseChannel.regs.reg[reg & 0x03] = data;
                if ((reg & 0x03) == 3)
                {
                    NoiseChannel.envelopeStart = true;
                    if (statusReg & 0x08)
                        NoiseChannel.lengthCounter = lengthCounterTable[NoiseChannel.regs.lenCounterLoad()];
                }
                return true;
                break;
            case 16:
            case 17:
            case 18:
            case 19:
                DMCChannel.regs.reg[reg & 0x03] = data;
                if ((reg & 0x03) == 0)
                {
                    if (DMCChannel.regs.irqEnable() == 0x00)
                        DMCChannel.interruptFlag = false;
                }
                else if ((reg & 0x03) == 1)
                    DMCChannel.counterOutput = (data & 0x7F);
                return true;
                break;
            case 21:
                statusReg = data;
                if ((data & 0x01) == 0x00)
                    PulseChannel1.lengthCounter = 0;
                if ((data & 0x02) == 0x00)
                    PulseChannel2.lengthCounter = 0;
                if ((data & 0x04) == 0x00)
                    TriangleChannel.lengthCounter = 0;
                if ((data & 0x08) == 0x00)
                    NoiseChannel.lengthCounter = 0;
                if ((data & 0x10) == 0x00)
                    DMCChannel.readerBytesRemaining = 0;
                else if (DMCChannel.readerBytesRemaining == 0)
                {
                    DMCChannel.sampleRestart();
                    DMCReaderFetch();
                }
                DMCChannel.interruptFlag = false;
                return true;
            case 23:                // not completely accurate, but close enough
                frameCounterReg = data;
                dividerTick = 14915;
                dividerCnt = (data & 0x80)? 0 : 4;
                return true;
                break;
            default:
                break;
        }
    }
    return false;
}

// run at twice CPU clock speed bc frame sequencer (some subunits will be operated by dividers)
// APU really runs on both master clock and cpu clock
// function runs at 3579545.334 Hz
void ricoh2A03::APU::tick()
{
    samplesToGenerateOffset += samplesPerTick;
    int samplesToGenerate = (int)(floor(samplesToGenerateOffset));
    samplesToGenerateOffset -= samplesToGenerate;
    
    // target period for sweeper is calculated CONSTANTLY (condition for sweep unit muting)
    PulseChannel1.sweepChange = (PulseChannel1.getSequencePeriod() >> PulseChannel1.regs.shift());
    PulseChannel1.sweepTargetPeriod = PulseChannel1.getSequencePeriod() + ((PulseChannel1.regs.negate())? ~(PulseChannel1.sweepChange) : PulseChannel1.sweepChange);        // 1's complement
    // std::cout << (int)(PulseChannel1.getSequencePeriod()) << ' ' << PulseChannel1.sweepTargetPeriod << std::endl;

    PulseChannel2.sweepChange = (PulseChannel2.getSequencePeriod() >> PulseChannel2.regs.shift());
    PulseChannel2.sweepTargetPeriod = PulseChannel2.getSequencePeriod() + ((PulseChannel2.regs.negate())? (~(PulseChannel2.sweepChange) + 1) : PulseChannel2.sweepChange);  // negative

    IRQset = false;

    // frame sequencer operation (entire tick function essentially run by this frame sequencer)
    // divider divides master clock by 89490 for ~240 Hz
    // or divide CPU clock by (89490/12); (89490/6) since we run this at twice CPU clock speed (could have run at CPU clock speed, but just wanted whole number here)
    if (dividerTick >= 14915)
    {
        dividerTick = 0;
        
        bool seq5step = (frameCounterReg & 0x80);
        bool halfSeqCheck = (seq5step)? ((dividerCnt == 0) || (dividerCnt == 2)) : ((dividerCnt == 1) || (dividerCnt == 3));
        bool quarterSeqCheck = (dividerCnt < 4);

        // set interrupt flag
        if (!seq5step && ((frameCounterReg & 0x40) == 0x00) && (dividerCnt == 3))
        {
            IRQ = true;
            IRQset = true;
        }

        if (halfSeqCheck)       // adjust note length and sweepers
        {
            // sweep units and clock length counters
            if (PulseChannel1.sweepTimer == 0)
            {
                if ((PulseChannel1.regs.enable() != 0x00) && (PulseChannel1.regs.shift() != 0x00) && (PulseChannel1.getSequencePeriod() >= 8) && (PulseChannel1.sweepChange < 0x7FF))
                    PulseChannel1.setSequencePeriod(PulseChannel1.sweepTargetPeriod);
                PulseChannel1.sweepTimer = PulseChannel1.regs.period();
                PulseChannel1.sweepReload = false;
            }
            else if (PulseChannel1.sweepReload)
            {
                PulseChannel1.sweepTimer = PulseChannel1.regs.period();
                PulseChannel1.sweepReload = false;
            }
            else
                PulseChannel1.sweepTimer--;
                
            if (PulseChannel2.sweepTimer == 0)
            {
                if ((PulseChannel2.regs.enable() != 0x00) && (PulseChannel2.regs.shift() != 0x00) && (PulseChannel2.getSequencePeriod() >= 8) && (PulseChannel2.sweepChange < 0x7FF))   // (PulseChannel2.sweepTargetPeriod <= 0x7FF))
                    PulseChannel2.setSequencePeriod(PulseChannel2.sweepTargetPeriod);
                PulseChannel2.sweepTimer = PulseChannel2.regs.period();
                PulseChannel2.sweepReload = false;
            }
            else if (PulseChannel2.sweepReload)
            {
                PulseChannel2.sweepTimer = PulseChannel2.regs.period();
                PulseChannel2.sweepReload = false;
            }
            else
                PulseChannel2.sweepTimer--;
                
            if ((statusReg & 0x01) == 0x00)
            {
                PulseChannel1.lengthCounter = 0;
            }
            else if (!(PulseChannel1.regs.lenCounterHalt()) && (PulseChannel1.lengthCounter > 0))
                PulseChannel1.lengthCounter--;

            if ((statusReg & 0x02) == 0x00)
            {
                PulseChannel2.lengthCounter = 0;
            }
            else if (!(PulseChannel2.regs.lenCounterHalt()) && (PulseChannel2.lengthCounter > 0))
                PulseChannel2.lengthCounter--;

            if ((statusReg & 0x04) == 0x00)
            {
                TriangleChannel.lengthCounter = 0;
            }
            else if (!(TriangleChannel.regs.lenCounterHalt()) && (TriangleChannel.lengthCounter > 0))
                TriangleChannel.lengthCounter--;

            if ((statusReg & 0x08) == 0x00)
            {
                NoiseChannel.lengthCounter = 0;
            }
            else if (!(NoiseChannel.regs.lenCounterHalt()) && (NoiseChannel.lengthCounter > 0))
                NoiseChannel.lengthCounter--;
        }

        if (quarterSeqCheck)    // adjust volume envelope
        {
            // clock envelopes and triangle's linear counter
            if (PulseChannel1.envelopeStart)
            {
                PulseChannel1.envelopeCounter = 15;
                PulseChannel1.envelopeDivider = PulseChannel1.regs.envelope();
                PulseChannel1.envelopeStart = false;
            }
            else
            {
                if (PulseChannel1.envelopeDivider == 0)
                {
                    if (PulseChannel1.envelopeCounter == 0x00)
                    {
                        if (PulseChannel1.regs.lenCounterHalt())
                            PulseChannel1.envelopeCounter = 15;
                    }
                    else
                        PulseChannel1.envelopeCounter--;
                    PulseChannel1.envelopeDivider = PulseChannel1.regs.envelope();
                }
                else
                    PulseChannel1.envelopeDivider--;
            }
            
            if (PulseChannel2.envelopeStart)
            {
                PulseChannel2.envelopeCounter = 15;
                PulseChannel2.envelopeDivider = PulseChannel2.regs.envelope();
                PulseChannel2.envelopeStart = false;
            }
            else
            {
                if (PulseChannel2.envelopeDivider == 0)
                {
                    if (PulseChannel1.envelopeCounter == 0x00)
                    {
                        if (PulseChannel2.regs.lenCounterHalt())
                            PulseChannel2.envelopeCounter = 15;
                    }
                    else
                        PulseChannel2.envelopeCounter--;
                    PulseChannel2.envelopeDivider = PulseChannel2.regs.envelope();
                }
                else
                    PulseChannel2.envelopeDivider--;
            }

            if (TriangleChannel.linearHalt)
                TriangleChannel.linearCounter = TriangleChannel.regs.linCounterLoad();
            else if (TriangleChannel.linearCounter)
                TriangleChannel.linearCounter--;
            if (TriangleChannel.regs.lenCounterHalt() == 0x00)
                TriangleChannel.linearHalt = false;

            
            if (NoiseChannel.envelopeStart)
            {
                NoiseChannel.envelopeCounter = 15;
                NoiseChannel.envelopeDivider = NoiseChannel.regs.envelope();
                NoiseChannel.envelopeStart = false;
            }
            else
            {
                if (NoiseChannel.envelopeDivider == 0)
                {
                    if (NoiseChannel.envelopeCounter == 0x00)
                    {
                        if (NoiseChannel.regs.lenCounterHalt())
                            NoiseChannel.envelopeCounter = 15;
                    }
                    else
                        NoiseChannel.envelopeCounter--;
                    NoiseChannel.envelopeDivider = NoiseChannel.regs.envelope();
                }
                else
                    NoiseChannel.envelopeDivider--;
            }
        }

        dividerCnt++;
        if (dividerCnt >= (seq5step)? 5 : 4)
            dividerCnt = 0;
    }

    // continuous operation @ CPU clock speed (not part of frame sequencer operation)
    if (timerCount & 0x01)
    {
        // half CPU clock speed (Timer / 2)
        if (timerCount == 0x03)
        {
            // pulse sequencers
            if (PulseChannel1.sequenceTimer == 0x0000)
            {
                PulseChannel1.sequenceTimer = PulseChannel1.getSequencePeriod();
                PulseChannel1.sequenceValue = ((PulseChannel1.sequenceValue & 0x01)? 0x80 : 0x00) | (PulseChannel1.sequenceValue >> 1);
            }
            else
                PulseChannel1.sequenceTimer--;

            if (PulseChannel2.sequenceTimer == 0x0000)
            {
                PulseChannel2.sequenceTimer = PulseChannel2.getSequencePeriod();
                PulseChannel2.sequenceValue = ((PulseChannel2.sequenceValue & 0x01)? 0x80 : 0x00) | (PulseChannel2.sequenceValue >> 1);
            }
            else
                PulseChannel2.sequenceTimer--;
        }

        // triangle sequencer
        if ((TriangleChannel.linearCounter > 0) && (TriangleChannel.lengthCounter > 0))
        {
                if (TriangleChannel.sequenceTimer == 0x0000)
                {
                    TriangleChannel.sequenceTimer = TriangleChannel.getSequencePeriod();
                    if (TriangleChannel.sequenceHalfPeriod)
                    {
                        if (TriangleChannel.sequenceValue == 0x00)
                            TriangleChannel.sequenceHalfPeriod = false;
                        else
                            TriangleChannel.sequenceValue--;
                    }
                    else
                    {
                        if (TriangleChannel.sequenceValue == 0x0F)
                            TriangleChannel.sequenceHalfPeriod = true;
                        else
                            TriangleChannel.sequenceValue++;
                    }
                }
                else
                    TriangleChannel.sequenceTimer--;
        }

        // noise sequencer
        if (NoiseChannel.randomTimer == 0x0000)
        {
            NoiseChannel.randomTimer = NoiseChannel.randomPeriodTable[NoiseChannel.regs.noisePeriod()];
            uint16_t NoiseChannelXorBitmask = (NoiseChannel.regs.loopNoise())? 0x0020 : 0x0002;
            NoiseChannel.randomValue = (((NoiseChannel.randomValue & NoiseChannelXorBitmask) ^ ((NoiseChannel.randomValue & 0x0001)? NoiseChannelXorBitmask : 0x0000))? 0x4000 : 0x0000) | (NoiseChannel.randomValue >> 1);;
        }
        else
            NoiseChannel.randomTimer--;

        // DMC output unit
        if (DMCChannel.outputTimer == 0x00)
        {
            DMCChannel.outputTimer = DMCChannel.dmcPeriodTable[DMCChannel.regs.freq()];
            if (!(DMCChannel.outputSilence))
            {
                if (DMCChannel.outputBuffer & 0x01)
                {
                    if (DMCChannel.counterOutput <= 125)
                        DMCChannel.counterOutput += 2;
                }
                else
                {
                    if (DMCChannel.counterOutput >= 2)
                        DMCChannel.counterOutput -= 2;
                }
            }
            DMCChannel.outputBuffer >>= 1;
            if ((--DMCChannel.outputCounter) == 0)
            {
                DMCChannel.outputCounter = 8;
                if (DMCChannel.sampleEmpty)
                    DMCChannel.outputSilence = true;
                else
                {
                    DMCChannel.outputSilence = false;
                    DMCChannel.outputBuffer = DMCChannel.sampleBuffer;
                    DMCChannel.sampleEmpty = true;
                    DMCReaderFetch();
                }
            }
        }
        else
            DMCChannel.outputTimer--;
    }

    // if there is a sample to add, do so (only at most 1 sample anyways since 3.6MHz >>> 44.1kHz)
    for (int i = 0; i < samplesToGenerate; i++)
    {
        uint8_t pulse1Output = PulseChannel1.sample();
        uint8_t pulse2Output = PulseChannel2.sample();
        uint8_t triangleOutput = TriangleChannel.sample();
        uint8_t noiseOutput = NoiseChannel.sample();
        uint8_t dmcOutput = DMCChannel.sample();        // note: I did not rigorously test this; zero this out if any issues

        #if USE_LOOKUP_TABLE
            double pulseOutput = pulseTable[pulse1Output + pulse2Output];
            double tndOutput = tndTable[(3 * triangleOutput) + (2 * noiseOutput) + dmcOutput];
        #elif USE_LINEAR_APPROX
            double pulseOutput = 0.00752f * (double)(pulse1Output + pulse2Output);
            double tndOutput = (0.00851f * (double)(triangleOutput)) + (0.00494f * (double)(noiseOutput)) + (0.00335f * (double)(dmcOutput));
        #else
            double pulseOutput = (pulse1Output + pulse2Output)? (95.88f / ((double)(8128.0f / (pulse1Output + pulse2Output)) + 100.0f)) : 0.0f;
            double tndOutput = (triangleOutput | noiseOutput | dmcOutput)? (159.79f / ((1.0f / (((double)(triangleOutput) / 8227.0f) + ((double)(noiseOutput) / 12241.0f) + ((double)(dmcOutput) / 22638.0f))) + 100.0f)) : 0.0f;
        #endif

        uint8_t mixerOutput = (uint8_t)(floor((pulseOutput + tndOutput) * 255.0f));

        #if USE_FILTER
            IO->audioAddSample(LowpassFilter.processSample(mixerOutput));
        #else
            IO->audioAddSample(mixerOutput);
        #endif
    }
    
    dividerTick++;
    ++timerCount &= 0x03;
}

void ricoh2A03::APU::DMCReaderFetch()
{
    if (!(DMCChannel.sampleEmpty) || !(DMCChannel.readerBytesRemaining))
        return;
    DMCChannel.readerDelay = 4;
    DMCChannel.sampleBuffer = mem->cpuRead(DMCChannel.readerAddr);
    DMCChannel.sampleEmpty = false;
    if (DMCChannel.readerAddr == 0xFFFF)
        DMCChannel.readerAddr = 0x8000;
    else
        DMCChannel.readerAddr++;
    DMCChannel.readerBytesRemaining--;
    if (DMCChannel.readerBytesRemaining == 0)
    {
        if (DMCChannel.regs.loop())
            DMCChannel.sampleRestart();
        else if (DMCChannel.regs.irqEnable())
            DMCChannel.interruptFlag = true;
    }

}
