#pragma once
#include "Bus.h"

#include "Mnemonics.h"

// MACROS
#define IS_NEGATIVE(c) ((c & 0x80) == 0x80)
#define IS_POSITIVE(c) ((c & 0x80) != 0x80)

extern bool debugOutput;

typedef union FLAGS
{
    struct
    {
        bool carry : 1;
        bool zero : 1;
        bool irqDisable : 1;
        bool decimal : 1;
        bool breakCommand : 1;
        bool ignored : 1;
        bool overflow : 1;
        bool negative : 1;
    };

    char allFlags;
}FLAGS;

class CPU_6502;
typedef void (CPU_6502::*opcodeFuncPtr)(void);
typedef void (*opcodeFuncPtrThis)(void);
#define CALL_MEMBER_FN(object,ptrToMember)  ((object).*(ptrToMember))

class CPU_6502
{
public:
    CPU_6502();
    ~CPU_6502();

    Bus bus;

    void Reset();

    bool Step();

    void TriggerNMI();

    // registers
    uint8_t a;
    uint8_t x;
    uint8_t y;

    // program counter
    uint16_t PC;

    // Stack pointer
    uint8_t SP;

    FLAGS flags;
    bool running;
    bool nmi;

protected:
    // operations
    void UnhandledOpcode();
    void ADC_Generic(uint8_t value);
    void EOR_Generic(uint8_t value);
    void ORA_Generic(uint8_t value);
    void SBC_Generic(uint8_t value);
    void BRK();             // 00
    void ORA_zp_x_ind();    // 01 test
    void ORA_zp();          // 05 test
    void ASL_zp();          // 06 test
    void PHP();             // 08 test
    void ORA_imm();         // 09 test
    void ASL();             // 0A test
    void ORA_a();           // 0D test
    void ASL_a();           // 0E test
    void BPL_r();           // 10 test
    void ORA_zp_ind_y();    // 11 test
    void ORA_zp_x();        // 15 test
    void ASL_zp_x();        // 16 test
    void CLC();             // 18 test
    void ORA_a_y();         // 19 test
    void ORA_a_x();         // 1D test
    void ASL_a_x();         // 1E test
    void JSR();             // 20
    void AND_zp_x_ind();    // 21
    void BIT_zp();          // 24 test
    void AND_zp();          // 25 test
    void ROL_zp();          // 26 test
    void PLP();             // 28 test
    void AND_imm();         // 29 test
    void ROL_A();           // 2A test
    void BIT_a();           // 2C test
    void AND_a();           // 2D test
    void ROL_a();           // 2E test
    void BMI_r();           // 30 test
    void AND_zp_ind_y();    // 31 test
    void AND_zp_x();        // 35 test
    void ROL_zp_x();        // 36 test
    void SEC();             // 38
    void AND_a_y();         // 39 test
    void AND_a_x();         // 3D test
    void ROL_a_x();         // 3E test
    void RTI();             // 40 test
    void EOR_zp_x_ind();    // 41 test
    void EOR_zp();          // 45 test
    void LSR_zp();          // 46 test
    void PHA();             // 48
    void EOR_imm();         // 49
    void LSR();             // 4A test
    void JMP_a();           // 4C
    void EOR_a();           // 4D
    void LSR_a();           // 4E test
    void BVC_r();           // 50 test
    void EOR_zp_ind_y();    // 51 test
    void EOR_zp_x();        // 55
    void LSR_zp_x();        // 56
    void CLI();             // 58
    void EOR_a_y();         // 59
    void EOR_a_x();         // 5D
    void LSR_a_x();         // 5E
    void RTS();             // 60
    void ADC_zp_x_ind();    // 61
    void ADC_zp();          // 65
    void ROR_zp();          // 66
    void PLA();             // 68
    void ADC_imm();         // 69
    void ROR_A();           // 6A
    void JMP_ind();         // 6C
    void ADC_a();           // 6D
    void ROR_a();           // 6E
    void BVS_r();           // 70 test
    void ADC_zp_ind_y();    // 71 test
    void ADC_zp_x();        // 75 test
    void ROR_zp_x();        // 76 test
    void SEI();             // 78
    void ADC_a_y();         // 79 test
    void ADC_a_x();         // 7D test
    void ROR_a_x();         // 7E test
    void STA_zp_x_ind();    // 81 test
    void STY_zp();          // 84
    void STA_zp();          // 85
    void STX_zp();          // 86 test
    void DEY();             // 88
    void TYA();             // 89
    void TXA();             // 8A
    void STY_a();           // 8C
    void STA_a();           // 8D
    void STX_a();           // 8E
    void BCC_r();           // 90
    void STA_zp_ind_y();    // 91
    void STY_zp_x();        // 94
    void STA_zp_x();        // 95
    void STX_zp_y();        // 96 test
    void STA_a_y();         // 99
    void STA_a_x();         // 9D test
    void TXS();             // 9A test
    void LDY_imm();         // A0
    void LDA_zp_x_ind();    // A1
    void LDX_imm();         // A2
    void LDY_zp();          // A4 test
    void LDA_zp();          // A5 test
    void LDX_zp();          // A6 test
    void TAY();             // A8 test
    void LDA_imm();         // A9
    void TAX();             // AA
    void LDY_a();           // AC test
    void LDA_a();           // AD test
    void LDX_a();           // AE test
    void BCS_r();           // B0 test
    void LDA_zp_ind_y();    // B1
    void LDY_zp_x();        // B4 test
    void LDA_zp_x();        // B5 test
    void LDX_zp_y();        // B6 test
    void CLV();             // B8 test
    void LDA_a_y();         // B9 test
    void TSX();             // BA test
    void LDY_a_x();         // BC test
    void LDA_a_x();         // BD test
    void LDX_a_y();         // BE test
    void CPY_imm();         // C0
    void CPY_zp();          // C4 test
    void CMP_zp_x_ind();    // C1 test
    void CMP_zp();          // C5 test
    void DEC_zp();          // C6 test
    void INY();             // C8
    void CMP_imm();         // C9
    void DEX();             // CA
    void CPY_a();           // CC test
    void CMP_a();           // CD test
    void DEC_a();           // CE test
    void BRNE_r();          // D0
    void CMP_zp_ind_y();    // D1 test
    void CMP_zp_x();        // D5 test
    void DEC_zp_x();        // D6 test
    void CMP_a_y();         // D9 test
    void CLD();             // D8 test
    void CMP_a_x();         // DD test
    void DEC_a_x();         // DE test
    void CPX_imm();         // E0
    void SBC_zp_x_ind();    // E1 test
    void CPX_zp();          // E4 test
    void SBC_zp();          // E5 test
    void INC_zp();          // E6 test
    void INX();             // E8
    void SBC_imm();         // E9 test more
    void NOP();             // EA
    void CPX_a();           // EC test
    void SBC_a();           // ED test
    void INC_a();           // EE test
    void BEQ_r();           // F0 test
    void SBC_zp_ind_y();    // F1 test
    void SBC_zp_x();        // F5 test
    void INC_zp_x();        // F6 test
    void SED();             // F8 test
    void SBC_a_y();         // F9 test
    void SBC_a_x();         // FD test
    void INC_a_x();         // FE test
    // end of operations

    void SetupOpcodes();
    void SetupOpCode(uint8_t op, opcodeFuncPtr ptr, const char *mnemonic, uint8_t bytes);

    uint16_t operand;   // operand for the current instruction
    uint8_t opcode;     // opcode of the current instruction

    opcodeFuncPtr opcodes[256];
    opcodeFuncPtrThis ops[256];
    uint8_t opcodeBytes[256];
    const char *mnemonics[256];

    // temp
    int opsHandled;
};

