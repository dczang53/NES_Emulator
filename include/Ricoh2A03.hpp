#ifndef _RICOH2A03_CPU
#define _RICOH2A03_CPU

#include <cstdint>

#ifdef DEBUG
    #include <fstream>
#endif

namespace NES
{
    class Memory;
};

namespace ricoh2A03
{
    enum statusMask
    {
        carryFlag =         (uint8_t)(1),
        zeroFlag =          ((uint8_t)(1) << 1),
        interruptDisable =  ((uint8_t)(1) << 2),
        decimalMode =       ((uint8_t)(1) << 3),
        breakCommand =      ((uint8_t)(1) << 4),
        unused =            ((uint8_t)(1) << 5),
        overflowFlag =      ((uint8_t)(1) << 6),
        negativeFlag =      ((uint8_t)(1) << 7)
    };

    class CPU
    {
        struct instruction
        {
            uint8_t (ricoh2A03::CPU::*operate) (void);
            uint8_t (ricoh2A03::CPU::*addrMode)(void);
            uint8_t cycles;
            uint8_t pageDelay;          // "1" if page cross delay for all ABSX, ABSY, and INDY is present (if implemented), else "0"
            const char operation[5];
        };

    public:
        CPU(NES::Memory *m);
        ~CPU();

        // CPU registers
        uint16_t PC = 0x0000;   // program counter
        uint8_t SP = 0xFF;      // stack pointer (256 byte stack located between $0100 and $01FF)
        uint8_t ACC = 0x00;     // accumulator
        uint8_t REGX = 0x00;    // register X
        uint8_t REGY = 0x00;    // register Y
        uint8_t STATUS = 0x00;  // processor status register

        // clock function to call for one clock cycle
        void tick(bool functional);

        uint64_t getClock();

        // interrupt functions
        void rst();
	    void irq();
	    void nmi();

        // extra function to see if current instruction is done (UNUSED; remove this)
        bool instrDone();

        #ifdef DEBUG
            void enableLog(bool enable);    // debug
        #endif

    private:
        // memory interface for addresses up to 2^16
        NES::Memory *mem = nullptr;

        // remaining duration of current instruction
        uint8_t insClk = 0;

        // current operation and subparts to total operation duration
        const instruction *currOp = nullptr;
        uint8_t operandClk = 0;
        uint8_t processClk = 0;

        // pending interrupt request
        bool pendingIRQ = false;

        // pending non-maskable interrupt
        bool pendingNMI = false;

        // private buffer variables used by addressing mode and instruction functions
        uint8_t *operandRef = nullptr;  // set to pointer to CPU internal register
        int32_t operandAddr = -1;       // else set to address of operand

        uint64_t clock = 0;

        #ifdef DEBUG
            std::ofstream CPUlogfile;   // debug
            bool CPUlog = false;        // debug
        #endif

        // all addressing modes (http://www.obelisk.me.uk/6502/addressing.html) (http://archive.6502.org/datasheets/rockwell_r650x_r651x.pdf)
        uint8_t ACCUM();   // accumulator addressing
        uint8_t IMM();     // immediate address
        uint8_t ABS();     // absolute addressing
        uint8_t ZP();      // zero page addressing
        uint8_t ZPX();     // indexed zero page addressing from regX
        uint8_t ZPY();     // indexed zero page addressing from regY
        uint8_t ABSX();    // indexed absolute addressing from regX (potential +1 delay)
        uint8_t ABSY();    // indexed absolute addressing from regX (potential +1 delay)
        uint8_t IMP();     // implied addressing
        uint8_t REL();     // relative addressing (+1/+2 on success for branches)
        uint8_t INDX();    // indexed indirect addressing from regX
        uint8_t INDY();    // indexed indirect addressing from regY (potential +1 delay)
        uint8_t IND();     // absolute indirect

        // all instruction functions (http://6502.org/tutorials/6502opcodes.html) (http://www.obelisk.me.uk/6502/instructions.html)
        // note: 56 valid opcodes + 1 filler opcode
        uint8_t LDA();
        uint8_t LDX();
        uint8_t LDY();
        uint8_t STA();
        uint8_t STX();
        uint8_t STY();
        uint8_t TAX();
        uint8_t TAY();
        uint8_t TXA();
        uint8_t TYA();
        uint8_t TSX();
        uint8_t TXS();
        uint8_t PHA();
        uint8_t PHP();
        uint8_t PLA();
        uint8_t PLP();
        uint8_t AND();
        uint8_t EOR();
        uint8_t ORA();
        uint8_t BIT();
        uint8_t ADC();
        uint8_t SBC();
        uint8_t CMP();
        uint8_t CPX();
        uint8_t CPY();
        uint8_t INC();
        uint8_t INX();
        uint8_t INY();
        uint8_t DEC();
        uint8_t DEX();
        uint8_t DEY();
        uint8_t ASL();
        uint8_t LSR();
        uint8_t ROL();
        uint8_t ROR();
        uint8_t JMP();
        uint8_t JSR();
        uint8_t RTS();
        uint8_t BCC();
        uint8_t BCS();
        uint8_t BEQ();
        uint8_t BMI();
        uint8_t BNE();
        uint8_t BPL();
        uint8_t BVC();
        uint8_t BVS();
        uint8_t CLC();
        uint8_t CLD();
        uint8_t CLI();
        uint8_t CLV();
        uint8_t SEC();
        uint8_t SED();
        uint8_t SEI();
        uint8_t BRK();
        uint8_t NOP();
        uint8_t RTI();
        uint8_t XXX();  // filler funtion for illegal opcodes

        // opcode-to-instruction list (http://www.oxyron.de/html/opcodes02.html) (http://wiki.nesdev.com/w/index.php/CPU_unofficial_opcodes) (http://archive.6502.org/datasheets/rockwell_r65c00_microprocessors.pdf)
        // note: 4-letter operations beginning with 'X' are illegal/unoffical opcodes
        // note: for illegal/unofficial opcodes, 1 additional cycle on crossing page boundary or branch taken for certain instructions are not implemented
        inline static const instruction instructionSet[] = {
            {&ricoh2A03::CPU::BRK, &ricoh2A03::CPU::IMP, 7, 0, "BRK"},
            {&ricoh2A03::CPU::ORA, &ricoh2A03::CPU::INDX, 6, 1, "ORA"},
            {&ricoh2A03::CPU::XXX, &ricoh2A03::CPU::IMP, 2, 0, "XSTP"},
            {&ricoh2A03::CPU::XXX, &ricoh2A03::CPU::INDX, 8, 0, "XSLO"},
            {&ricoh2A03::CPU::XXX, &ricoh2A03::CPU::ZP, 3, 0, "XNOP"},
            {&ricoh2A03::CPU::ORA, &ricoh2A03::CPU::ZP, 3, 1, "ORA"},
            {&ricoh2A03::CPU::ASL, &ricoh2A03::CPU::ZP, 5, 0, "ASL"},
            {&ricoh2A03::CPU::XXX, &ricoh2A03::CPU::ZP, 5, 0, "XSLO"},
            {&ricoh2A03::CPU::PHP, &ricoh2A03::CPU::IMP, 3, 0, "PHP"},
            {&ricoh2A03::CPU::ORA, &ricoh2A03::CPU::IMM, 2, 1, "ORA"},
            {&ricoh2A03::CPU::ASL, &ricoh2A03::CPU::ACCUM, 2, 0, "ASL"},
            {&ricoh2A03::CPU::XXX, &ricoh2A03::CPU::IMM, 2, 0, "XANC"},
            {&ricoh2A03::CPU::XXX, &ricoh2A03::CPU::ABS, 4, 0, "XNOP"},
            {&ricoh2A03::CPU::ORA, &ricoh2A03::CPU::ABS, 4, 1, "ORA"},
            {&ricoh2A03::CPU::ASL, &ricoh2A03::CPU::ABS, 6, 0, "ASL"},
            {&ricoh2A03::CPU::XXX, &ricoh2A03::CPU::ABS, 6, 0, "XSLO"},

            {&ricoh2A03::CPU::BPL, &ricoh2A03::CPU::REL, 2, 0, "BPL"},
            {&ricoh2A03::CPU::ORA, &ricoh2A03::CPU::INDY, 5, 1, "ORA"},
            {&ricoh2A03::CPU::XXX, &ricoh2A03::CPU::IMP, 2, 0, "XSTP"},
            {&ricoh2A03::CPU::XXX, &ricoh2A03::CPU::INDY, 8, 0, "XSLO"},
            {&ricoh2A03::CPU::XXX, &ricoh2A03::CPU::ZPX, 4, 0, "XNOP"},
            {&ricoh2A03::CPU::ORA, &ricoh2A03::CPU::ZPX, 4, 1, "ORA"},
            {&ricoh2A03::CPU::ASL, &ricoh2A03::CPU::ZPX, 6, 0, "ASL"},
            {&ricoh2A03::CPU::XXX, &ricoh2A03::CPU::ZPX, 6, 0, "XSLO"},
            {&ricoh2A03::CPU::CLC, &ricoh2A03::CPU::IMP, 2, 0, "CLC"},
            {&ricoh2A03::CPU::ORA, &ricoh2A03::CPU::ABSY, 4, 1, "ORA"},
            {&ricoh2A03::CPU::XXX, &ricoh2A03::CPU::IMP, 2, 0, "XNOP"},
            {&ricoh2A03::CPU::XXX, &ricoh2A03::CPU::ABSY, 7, 0, "XSLO"},
            {&ricoh2A03::CPU::XXX, &ricoh2A03::CPU::ABSX, 4, 0, "XNOP"},
            {&ricoh2A03::CPU::ORA, &ricoh2A03::CPU::ABSX, 4, 1, "ORA"},
            {&ricoh2A03::CPU::ASL, &ricoh2A03::CPU::ABSX, 7, 0, "ASL"},
            {&ricoh2A03::CPU::XXX, &ricoh2A03::CPU::ABSX, 7, 0, "XSLO"},

            {&ricoh2A03::CPU::JSR, &ricoh2A03::CPU::ABS, 6, 0, "JSR"},
            {&ricoh2A03::CPU::AND, &ricoh2A03::CPU::INDX, 6, 1, "AND"},
            {&ricoh2A03::CPU::XXX, &ricoh2A03::CPU::IMP, 2, 0, "XSTP"},
            {&ricoh2A03::CPU::XXX, &ricoh2A03::CPU::INDX, 8, 0, "XRLA"},
            {&ricoh2A03::CPU::BIT, &ricoh2A03::CPU::ZP, 3, 0, "BIT"},
            {&ricoh2A03::CPU::AND, &ricoh2A03::CPU::ZP, 3, 1, "AND"},
            {&ricoh2A03::CPU::ROL, &ricoh2A03::CPU::ZP, 5, 0, "ROL"},
            {&ricoh2A03::CPU::XXX, &ricoh2A03::CPU::ZP, 5, 0, "XRLA"},
            {&ricoh2A03::CPU::PLP, &ricoh2A03::CPU::IMP, 4, 0, "PLP"},
            {&ricoh2A03::CPU::AND, &ricoh2A03::CPU::IMM, 2, 1, "AND"},
            {&ricoh2A03::CPU::ROL, &ricoh2A03::CPU::ACCUM, 2, 0, "ROL"},
            {&ricoh2A03::CPU::XXX, &ricoh2A03::CPU::IMM, 2, 0, "XANC"},
            {&ricoh2A03::CPU::BIT, &ricoh2A03::CPU::ABS, 4, 0, "BIT"},
            {&ricoh2A03::CPU::AND, &ricoh2A03::CPU::ABS, 4, 1, "AND"},
            {&ricoh2A03::CPU::ROL, &ricoh2A03::CPU::ABS, 6, 0, "ROL"},
            {&ricoh2A03::CPU::XXX, &ricoh2A03::CPU::ABS, 6, 0, "XRLA"},

            {&ricoh2A03::CPU::BMI, &ricoh2A03::CPU::REL, 2, 0, "BMI"},
            {&ricoh2A03::CPU::AND, &ricoh2A03::CPU::INDY, 5, 1, "AND"},
            {&ricoh2A03::CPU::XXX, &ricoh2A03::CPU::IMP, 2, 0, "XSTP"},
            {&ricoh2A03::CPU::XXX, &ricoh2A03::CPU::INDY, 8, 0, "XRLA"},
            {&ricoh2A03::CPU::XXX, &ricoh2A03::CPU::ZPX, 4, 0, "XNOP"},
            {&ricoh2A03::CPU::AND, &ricoh2A03::CPU::ZPX, 4, 1, "AND"},
            {&ricoh2A03::CPU::ROL, &ricoh2A03::CPU::ZPX, 6, 0, "ROL"},
            {&ricoh2A03::CPU::XXX, &ricoh2A03::CPU::ZPX, 6, 0, "XRLA"},
            {&ricoh2A03::CPU::SEC, &ricoh2A03::CPU::IMP, 2, 0, "SEC"},
            {&ricoh2A03::CPU::AND, &ricoh2A03::CPU::ABSY, 4, 1, "AND"},
            {&ricoh2A03::CPU::XXX, &ricoh2A03::CPU::IMP, 2, 0, "XNOP"},
            {&ricoh2A03::CPU::XXX, &ricoh2A03::CPU::ABSY, 7, 0, "XRLA"},
            {&ricoh2A03::CPU::XXX, &ricoh2A03::CPU::ABSX, 4, 0, "XNOP"},
            {&ricoh2A03::CPU::AND, &ricoh2A03::CPU::ABSX, 4, 1, "AND"},
            {&ricoh2A03::CPU::ROL, &ricoh2A03::CPU::ABSX, 7, 0, "ROL"},
            {&ricoh2A03::CPU::XXX, &ricoh2A03::CPU::ABSX, 7, 0, "XRLA"},

            {&ricoh2A03::CPU::RTI, &ricoh2A03::CPU::IMP, 6, 0, "RTI"},
            {&ricoh2A03::CPU::EOR, &ricoh2A03::CPU::INDX, 6, 1, "EOR"},
            {&ricoh2A03::CPU::XXX, &ricoh2A03::CPU::IMP, 2, 0, "XSTP"},
            {&ricoh2A03::CPU::XXX, &ricoh2A03::CPU::INDX, 8, 0, "XSRE"},
            {&ricoh2A03::CPU::XXX, &ricoh2A03::CPU::ZP, 3, 0, "XNOP"},
            {&ricoh2A03::CPU::EOR, &ricoh2A03::CPU::ZP, 3, 1, "EOR"},
            {&ricoh2A03::CPU::LSR, &ricoh2A03::CPU::ZP, 5, 0, "LSR"},
            {&ricoh2A03::CPU::XXX, &ricoh2A03::CPU::ZP, 4, 0, "XSRE"},
            {&ricoh2A03::CPU::PHA, &ricoh2A03::CPU::IMP, 3, 0, "PHA"},
            {&ricoh2A03::CPU::EOR, &ricoh2A03::CPU::IMM, 2, 1, "EOR"},
            {&ricoh2A03::CPU::LSR, &ricoh2A03::CPU::ACCUM, 2, 0, "LSR"},
            {&ricoh2A03::CPU::XXX, &ricoh2A03::CPU::IMM, 2, 0, "XALR"},
            {&ricoh2A03::CPU::JMP, &ricoh2A03::CPU::ABS, 3, 0, "JMP"},
            {&ricoh2A03::CPU::EOR, &ricoh2A03::CPU::ABS, 4, 1, "EOR"},
            {&ricoh2A03::CPU::LSR, &ricoh2A03::CPU::ABS, 6, 0, "LSR"},
            {&ricoh2A03::CPU::XXX, &ricoh2A03::CPU::ABS, 6, 0, "XSRE"},

            {&ricoh2A03::CPU::BVC, &ricoh2A03::CPU::REL, 2, 0, "BVC"},
            {&ricoh2A03::CPU::EOR, &ricoh2A03::CPU::INDY, 5, 1, "EOR"},
            {&ricoh2A03::CPU::XXX, &ricoh2A03::CPU::IMP, 2, 0, "XSTP"},
            {&ricoh2A03::CPU::XXX, &ricoh2A03::CPU::INDY, 8, 0, "XSRE"},
            {&ricoh2A03::CPU::XXX, &ricoh2A03::CPU::ZPX, 4, 0, "XNOP"},
            {&ricoh2A03::CPU::EOR, &ricoh2A03::CPU::ZPX, 4, 1, "EOR"},
            {&ricoh2A03::CPU::LSR, &ricoh2A03::CPU::ZPX, 6, 0, "LSR"},
            {&ricoh2A03::CPU::XXX, &ricoh2A03::CPU::ZPX, 6, 0, "XSRE"},
            {&ricoh2A03::CPU::CLI, &ricoh2A03::CPU::IMP, 2, 0, "CLI"},
            {&ricoh2A03::CPU::EOR, &ricoh2A03::CPU::ABSY, 4, 1, "EOR"},
            {&ricoh2A03::CPU::XXX, &ricoh2A03::CPU::IMP, 2, 0, "XNOP"},
            {&ricoh2A03::CPU::XXX, &ricoh2A03::CPU::ABSY, 7, 0, "XSRE"},
            {&ricoh2A03::CPU::XXX, &ricoh2A03::CPU::ABSX, 4, 0, "XNOP"},
            {&ricoh2A03::CPU::EOR, &ricoh2A03::CPU::ABSX, 4, 1, "EOR"},
            {&ricoh2A03::CPU::LSR, &ricoh2A03::CPU::ABSX, 7, 0, "LSR"},
            {&ricoh2A03::CPU::XXX, &ricoh2A03::CPU::ABSX, 7, 0, "XSRE"},

            {&ricoh2A03::CPU::RTS, &ricoh2A03::CPU::IMP, 6, 0, "RTS"},
            {&ricoh2A03::CPU::ADC, &ricoh2A03::CPU::INDX, 6, 1, "ADC"},
            {&ricoh2A03::CPU::XXX, &ricoh2A03::CPU::IMP, 2, 0, "XSTP"},
            {&ricoh2A03::CPU::XXX, &ricoh2A03::CPU::INDX, 8, 0, "XRRA"},
            {&ricoh2A03::CPU::XXX, &ricoh2A03::CPU::ZP, 3, 0, "XNOP"},
            {&ricoh2A03::CPU::ADC, &ricoh2A03::CPU::ZP, 3, 1, "ADC"},
            {&ricoh2A03::CPU::ROR, &ricoh2A03::CPU::ZP, 5, 0, "ROR"},
            {&ricoh2A03::CPU::XXX, &ricoh2A03::CPU::ZP, 5, 0, "XRRA"},
            {&ricoh2A03::CPU::PLA, &ricoh2A03::CPU::IMP, 4, 0, "PLA"},
            {&ricoh2A03::CPU::ADC, &ricoh2A03::CPU::IMM, 2, 1, "ADC"},
            {&ricoh2A03::CPU::ROR, &ricoh2A03::CPU::ACCUM, 2, 0, "ROR"},
            {&ricoh2A03::CPU::XXX, &ricoh2A03::CPU::IMM, 2, 0, "XARR"},
            {&ricoh2A03::CPU::JMP, &ricoh2A03::CPU::IND, 5, 0, "JMP"},
            {&ricoh2A03::CPU::ADC, &ricoh2A03::CPU::ABS, 4, 1, "ADC"},
            {&ricoh2A03::CPU::ROR, &ricoh2A03::CPU::ABS, 6, 0, "ROR"},
            {&ricoh2A03::CPU::XXX, &ricoh2A03::CPU::ABS, 6, 0, "XRRA"},

            {&ricoh2A03::CPU::BVS, &ricoh2A03::CPU::REL, 2, 0, "BVS"},
            {&ricoh2A03::CPU::ADC, &ricoh2A03::CPU::INDY, 5, 1, "ADC"},
            {&ricoh2A03::CPU::XXX, &ricoh2A03::CPU::IMP, 2, 0, "XSTP"},
            {&ricoh2A03::CPU::XXX, &ricoh2A03::CPU::INDY, 8, 0, "XRRA"},
            {&ricoh2A03::CPU::XXX, &ricoh2A03::CPU::ZPX, 4, 0, "XNOP"},
            {&ricoh2A03::CPU::ADC, &ricoh2A03::CPU::ZPX, 4, 1, "ADC"},
            {&ricoh2A03::CPU::ROR, &ricoh2A03::CPU::ZPX, 6, 0, "ROR"},
            {&ricoh2A03::CPU::XXX, &ricoh2A03::CPU::ZPX, 6, 0, "XRRA"},
            {&ricoh2A03::CPU::SEI, &ricoh2A03::CPU::IMP, 2, 0, "SEI"},
            {&ricoh2A03::CPU::ADC, &ricoh2A03::CPU::ABSY, 4, 1, "ADC"},
            {&ricoh2A03::CPU::XXX, &ricoh2A03::CPU::IMP, 2, 0, "XNOP"},
            {&ricoh2A03::CPU::XXX, &ricoh2A03::CPU::ABSY, 7, 0, "XRRA"},
            {&ricoh2A03::CPU::XXX, &ricoh2A03::CPU::ABSX, 4, 0, "XNOP"},
            {&ricoh2A03::CPU::ADC, &ricoh2A03::CPU::ABSX, 4, 1, "ADC"},
            {&ricoh2A03::CPU::ROR, &ricoh2A03::CPU::ABSX, 7, 0, "ROR"},
            {&ricoh2A03::CPU::XXX, &ricoh2A03::CPU::ABSX, 7, 0, "XRRA"},

            {&ricoh2A03::CPU::XXX, &ricoh2A03::CPU::IMM, 2, 0, "XNOP"},
            {&ricoh2A03::CPU::STA, &ricoh2A03::CPU::INDX, 6, 0, "STA"},
            {&ricoh2A03::CPU::XXX, &ricoh2A03::CPU::IMM, 2, 0, "XNOP"},
            {&ricoh2A03::CPU::XXX, &ricoh2A03::CPU::INDX, 6, 0, "XSAX"},
            {&ricoh2A03::CPU::STY, &ricoh2A03::CPU::ZP, 3, 0, "STY"},
            {&ricoh2A03::CPU::STA, &ricoh2A03::CPU::ZP, 3, 0, "STA"},
            {&ricoh2A03::CPU::STX, &ricoh2A03::CPU::ZP, 3, 0, "STX"},
            {&ricoh2A03::CPU::XXX, &ricoh2A03::CPU::ZP, 3, 0, "XSAX"},
            {&ricoh2A03::CPU::DEY, &ricoh2A03::CPU::IMP, 2, 0, "DEY"},
            {&ricoh2A03::CPU::XXX, &ricoh2A03::CPU::IMM, 2, 0, "XNOP"},
            {&ricoh2A03::CPU::TXA, &ricoh2A03::CPU::IMP, 2, 0, "TXA"},
            {&ricoh2A03::CPU::XXX, &ricoh2A03::CPU::IMM, 2, 0, "XXAA"},
            {&ricoh2A03::CPU::STY, &ricoh2A03::CPU::ABS, 4, 0, "STY"},
            {&ricoh2A03::CPU::STA, &ricoh2A03::CPU::ABS, 4, 0, "STA"},
            {&ricoh2A03::CPU::STX, &ricoh2A03::CPU::ABS, 4, 0, "STX"},
            {&ricoh2A03::CPU::XXX, &ricoh2A03::CPU::ABS, 4, 0, "XSAX"},

            {&ricoh2A03::CPU::BCC, &ricoh2A03::CPU::REL, 2, 0, "BCC"},
            {&ricoh2A03::CPU::STA, &ricoh2A03::CPU::INDY, 6, 0, "STA"},
            {&ricoh2A03::CPU::XXX, &ricoh2A03::CPU::IMP, 2, 0, "XSTP"},
            {&ricoh2A03::CPU::XXX, &ricoh2A03::CPU::INDY, 6, 0, "XAHX"},
            {&ricoh2A03::CPU::STY, &ricoh2A03::CPU::ZPX, 4, 0, "STY"},
            {&ricoh2A03::CPU::STA, &ricoh2A03::CPU::ZPX, 4, 0, "STA"},
            {&ricoh2A03::CPU::STX, &ricoh2A03::CPU::ZPY, 4, 0, "STX"},
            {&ricoh2A03::CPU::XXX, &ricoh2A03::CPU::ZPY, 4, 0, "XSAX"},
            {&ricoh2A03::CPU::TYA, &ricoh2A03::CPU::IMP, 2, 0, "TYA"},
            {&ricoh2A03::CPU::STA, &ricoh2A03::CPU::ABSY, 5, 0, "STA"},
            {&ricoh2A03::CPU::TXS, &ricoh2A03::CPU::IMP, 2, 0, "TXS"},
            {&ricoh2A03::CPU::XXX, &ricoh2A03::CPU::ABSY, 5, 0, "XTAS"},
            {&ricoh2A03::CPU::XXX, &ricoh2A03::CPU::ABSX, 5, 0, "XSHY"},
            {&ricoh2A03::CPU::STA, &ricoh2A03::CPU::ABSX, 5, 0, "STA"},
            {&ricoh2A03::CPU::XXX, &ricoh2A03::CPU::ABSY, 5, 0, "XSHX"},
            {&ricoh2A03::CPU::XXX, &ricoh2A03::CPU::ABSY, 5, 0, "XAHX"},

            {&ricoh2A03::CPU::LDY, &ricoh2A03::CPU::IMM, 2, 1, "LDY"},
            {&ricoh2A03::CPU::LDA, &ricoh2A03::CPU::INDX, 6, 1, "LDA"},
            {&ricoh2A03::CPU::LDX, &ricoh2A03::CPU::IMM, 2, 1, "LDX"},
            {&ricoh2A03::CPU::XXX, &ricoh2A03::CPU::INDX, 6, 0, "XLAX"},
            {&ricoh2A03::CPU::LDY, &ricoh2A03::CPU::ZP, 3, 1, "LDY"},
            {&ricoh2A03::CPU::LDA, &ricoh2A03::CPU::ZP, 3, 1, "LDA"},
            {&ricoh2A03::CPU::LDX, &ricoh2A03::CPU::ZP, 3, 1, "LDX"},
            {&ricoh2A03::CPU::XXX, &ricoh2A03::CPU::ZP, 3, 0, "XLAX"},
            {&ricoh2A03::CPU::TAY, &ricoh2A03::CPU::IMP, 2, 0, "TAY"},
            {&ricoh2A03::CPU::LDA, &ricoh2A03::CPU::IMM, 2, 1, "LDA"},
            {&ricoh2A03::CPU::TAX, &ricoh2A03::CPU::IMP, 2, 0, "TAX"},
            {&ricoh2A03::CPU::XXX, &ricoh2A03::CPU::IMM, 2, 0, "XLAX"},
            {&ricoh2A03::CPU::LDY, &ricoh2A03::CPU::ABS, 4, 1, "LDY"},
            {&ricoh2A03::CPU::LDA, &ricoh2A03::CPU::ABS, 4, 1, "LDA"},
            {&ricoh2A03::CPU::LDX, &ricoh2A03::CPU::ABS, 4, 1, "LDX"},
            {&ricoh2A03::CPU::XXX, &ricoh2A03::CPU::ABS, 4, 0, "XLAX"},

            {&ricoh2A03::CPU::BCS, &ricoh2A03::CPU::REL, 2, 0, "BCS"},
            {&ricoh2A03::CPU::LDA, &ricoh2A03::CPU::INDY, 5, 1, "LDA"},
            {&ricoh2A03::CPU::XXX, &ricoh2A03::CPU::IMP, 2, 0, "XSTP"},
            {&ricoh2A03::CPU::XXX, &ricoh2A03::CPU::INDY, 5, 0, "XLAX"},
            {&ricoh2A03::CPU::LDY, &ricoh2A03::CPU::ZPX, 4, 1, "LDY"},
            {&ricoh2A03::CPU::LDA, &ricoh2A03::CPU::ZPX, 4, 1, "LDA"},
            {&ricoh2A03::CPU::LDX, &ricoh2A03::CPU::ZPY, 4, 1, "LDX"},
            {&ricoh2A03::CPU::XXX, &ricoh2A03::CPU::ZPY, 4, 0, "XLAX"},
            {&ricoh2A03::CPU::CLV, &ricoh2A03::CPU::IMP, 2, 0, "CLV"},
            {&ricoh2A03::CPU::LDA, &ricoh2A03::CPU::ABSY, 4, 1, "LDA"},
            {&ricoh2A03::CPU::TSX, &ricoh2A03::CPU::IMP, 2, 0, "TSX"},
            {&ricoh2A03::CPU::XXX, &ricoh2A03::CPU::ABSY, 4, 0, "XLAS"},
            {&ricoh2A03::CPU::LDY, &ricoh2A03::CPU::ABSX, 4, 1, "LDY"},
            {&ricoh2A03::CPU::LDA, &ricoh2A03::CPU::ABSX, 4, 1, "LDA"},
            {&ricoh2A03::CPU::LDX, &ricoh2A03::CPU::ABSY, 4, 1, "LDX"},
            {&ricoh2A03::CPU::XXX, &ricoh2A03::CPU::ABSY, 4, 0, "XLAX"},

            {&ricoh2A03::CPU::CPY, &ricoh2A03::CPU::IMM, 2, 0, "CPY"},
            {&ricoh2A03::CPU::CMP, &ricoh2A03::CPU::INDX, 6, 1, "CMP"},
            {&ricoh2A03::CPU::XXX, &ricoh2A03::CPU::IMM, 2, 0, "XNOP"},
            {&ricoh2A03::CPU::XXX, &ricoh2A03::CPU::INDX, 8, 0, "XDCP"},
            {&ricoh2A03::CPU::CPY, &ricoh2A03::CPU::ZP, 3, 0, "CPY"},
            {&ricoh2A03::CPU::CMP, &ricoh2A03::CPU::ZP, 3, 1, "CMP"},
            {&ricoh2A03::CPU::DEC, &ricoh2A03::CPU::ZP, 5, 0, "DEC"},
            {&ricoh2A03::CPU::XXX, &ricoh2A03::CPU::ZP, 5, 0, "XDCP"},
            {&ricoh2A03::CPU::INY, &ricoh2A03::CPU::IMP, 2, 0, "INY"},
            {&ricoh2A03::CPU::CMP, &ricoh2A03::CPU::IMM, 2, 1, "CMP"},
            {&ricoh2A03::CPU::DEX, &ricoh2A03::CPU::IMP, 2, 0, "DEX"},
            {&ricoh2A03::CPU::XXX, &ricoh2A03::CPU::IMM, 2, 0, "XAXS"},
            {&ricoh2A03::CPU::CPY, &ricoh2A03::CPU::ABS, 4, 0, "CPY"},
            {&ricoh2A03::CPU::CMP, &ricoh2A03::CPU::ABS, 4, 1, "CMP"},
            {&ricoh2A03::CPU::DEC, &ricoh2A03::CPU::ABS, 6, 0, "DEC"},
            {&ricoh2A03::CPU::XXX, &ricoh2A03::CPU::ABS, 6, 0, "XDCP"},

            {&ricoh2A03::CPU::BNE, &ricoh2A03::CPU::REL, 2, 0, "BNE"},
            {&ricoh2A03::CPU::CMP, &ricoh2A03::CPU::INDY, 5, 1, "CMP"},
            {&ricoh2A03::CPU::XXX, &ricoh2A03::CPU::IMP, 2, 0, "XSTP"},
            {&ricoh2A03::CPU::XXX, &ricoh2A03::CPU::INDY, 8, 0, "XDCP"},
            {&ricoh2A03::CPU::XXX, &ricoh2A03::CPU::ZPX, 4, 0, "XNOP"},
            {&ricoh2A03::CPU::CMP, &ricoh2A03::CPU::ZPX, 4, 1, "CMP"},
            {&ricoh2A03::CPU::DEC, &ricoh2A03::CPU::ZPX, 6, 0, "DEC"},
            {&ricoh2A03::CPU::XXX, &ricoh2A03::CPU::ZPX, 6, 0, "XDCP"},
            {&ricoh2A03::CPU::CLD, &ricoh2A03::CPU::IMP, 2, 0, "CLD"},
            {&ricoh2A03::CPU::CMP, &ricoh2A03::CPU::ABSY, 4, 1, "CMP"},
            {&ricoh2A03::CPU::XXX, &ricoh2A03::CPU::IMP, 2, 0, "XNOP"},
            {&ricoh2A03::CPU::XXX, &ricoh2A03::CPU::ABSY, 7, 0, "XDCP"},
            {&ricoh2A03::CPU::XXX, &ricoh2A03::CPU::ABSX, 4, 0, "XNOP"},
            {&ricoh2A03::CPU::CMP, &ricoh2A03::CPU::ABSX, 4, 1, "CMP"},
            {&ricoh2A03::CPU::DEC, &ricoh2A03::CPU::ABSX, 7, 0, "DEC"},
            {&ricoh2A03::CPU::XXX, &ricoh2A03::CPU::ABSX, 7, 0, "XDCP"},

            {&ricoh2A03::CPU::CPX, &ricoh2A03::CPU::IMM, 2, 0, "CPX"},
            {&ricoh2A03::CPU::SBC, &ricoh2A03::CPU::INDX, 6, 1, "SBC"},
            {&ricoh2A03::CPU::XXX, &ricoh2A03::CPU::IMM, 2, 0, "XNOP"},
            {&ricoh2A03::CPU::XXX, &ricoh2A03::CPU::INDX, 8, 0, "XISC"},
            {&ricoh2A03::CPU::CPX, &ricoh2A03::CPU::ZP, 3, 0, "CPX"},
            {&ricoh2A03::CPU::SBC, &ricoh2A03::CPU::ZP, 3, 1, "SBC"},
            {&ricoh2A03::CPU::INC, &ricoh2A03::CPU::ZP, 5, 0, "INC"},
            {&ricoh2A03::CPU::XXX, &ricoh2A03::CPU::ZP, 5, 0, "XISC"},
            {&ricoh2A03::CPU::INX, &ricoh2A03::CPU::IMP, 2, 0, "INX"},
            {&ricoh2A03::CPU::SBC, &ricoh2A03::CPU::IMM, 2, 1, "SBC"},
            {&ricoh2A03::CPU::NOP, &ricoh2A03::CPU::IMP, 2, 0, "NOP"},
            {&ricoh2A03::CPU::XXX, &ricoh2A03::CPU::IMM, 2, 0, "XSBC"},
            {&ricoh2A03::CPU::CPX, &ricoh2A03::CPU::ABS, 4, 0, "CPX"},
            {&ricoh2A03::CPU::SBC, &ricoh2A03::CPU::ABS, 4, 1, "SBC"},
            {&ricoh2A03::CPU::INC, &ricoh2A03::CPU::ABS, 6, 0, "INC"},
            {&ricoh2A03::CPU::XXX, &ricoh2A03::CPU::ABS, 6, 0, "XISC"},

            {&ricoh2A03::CPU::BEQ, &ricoh2A03::CPU::REL, 2, 0, "BEQ"},
            {&ricoh2A03::CPU::SBC, &ricoh2A03::CPU::INDY, 5, 1, "SBC"},
            {&ricoh2A03::CPU::XXX, &ricoh2A03::CPU::IMP, 2, 0, "XSTP"},
            {&ricoh2A03::CPU::XXX, &ricoh2A03::CPU::INDY, 8, 0, "XISC"},
            {&ricoh2A03::CPU::XXX, &ricoh2A03::CPU::ZPX, 4, 0, "XNOP"},
            {&ricoh2A03::CPU::SBC, &ricoh2A03::CPU::ZPX, 4, 1, "SBC"},
            {&ricoh2A03::CPU::INC, &ricoh2A03::CPU::ZPX, 6, 0, "INC"},
            {&ricoh2A03::CPU::XXX, &ricoh2A03::CPU::ZPX, 6, 0, "XISC"},
            {&ricoh2A03::CPU::SED, &ricoh2A03::CPU::IMP, 2, 0, "SED"},
            {&ricoh2A03::CPU::SBC, &ricoh2A03::CPU::ABSY, 4, 1, "SBC"},
            {&ricoh2A03::CPU::XXX, &ricoh2A03::CPU::IMP, 2, 0, "XNOP"},
            {&ricoh2A03::CPU::XXX, &ricoh2A03::CPU::ABSY, 7, 0, "XISC"},
            {&ricoh2A03::CPU::XXX, &ricoh2A03::CPU::ABSX, 4, 0, "XNOP"},
            {&ricoh2A03::CPU::SBC, &ricoh2A03::CPU::ABSX, 4, 1, "SBC"},
            {&ricoh2A03::CPU::INC, &ricoh2A03::CPU::ABSX, 7, 0, "INC"},
            {&ricoh2A03::CPU::XXX, &ricoh2A03::CPU::ABSX, 7, 0, "XISC"}
        };
    };
}

#endif