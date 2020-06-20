#include "CPU_6502.h"
#include <stdio.h>

CPU_6502::CPU_6502()
{
    opsHandled = 0;

    SetupOpcodes();

    printf("%d of 151 opcodes implemented, %02.1f%%\n", opsHandled, opsHandled * 100.0 / 151);

    running = true;
    nmi = false;

    bus.pCPU = this;
}


CPU_6502::~CPU_6502()
{
}

void CPU_6502::Reset()
{
    // Even though hardware doesn't initialize these, we'll initialize them
    a = x = y = 0;
    SP = 0xFF;

    /* "All registers are initialized by software except Decimal and Interrupt disable mode
    select bits of the Processor Status Register(P) */
    flags.allFlags = 0x34;

    PC = bus.read(0xFFFD) << 8;
    PC += bus.read(0xFFFC);
}

bool CPU_6502::Step()
{
    // Check for NMI
    if (nmi)
    {
        printf("Handling NMI\n");
        printf("PC - 0x%X\n", PC);

        // Push return value onto stack, high byte then low byte
        bus.write(0x100 + SP, PC >> 8);
        --SP;
        bus.write(0x100 + SP, (uint8_t)(PC & 0xFF));
        --SP;

        // Push processor status onto stack
        // Set bit 5 but leave bit 4 clear (See https://wiki.nesdev.com/w/index.php/Status_flags#The_B_flag)
        bus.write(0x100 + SP, flags.allFlags & 0x10);
        --SP;

        // Load PC from $FFFA-$FFFB
        PC = bus.read(0xFFFA);
        PC |= (uint16_t)(bus.read(0xFFFB)) << 8;
        
        printf("New PC - 0x%X\n", PC);

        //debugOutput = true;
        nmi = false;
        return true;
    }

    // get the current opcode
    opcode = bus.read(PC++);

    if(debugOutput)
        printf("opcode: 0x%X - %s ", opcode, mnemonics[opcode]);
    
    // see how many bytes are in the operand
    int operandBytes = opcodeBytes[opcode] - 1;

    //printf("Operand bytes: %d\n", operandBytes);

    if (operandBytes >= 1)
    {
        // read the low byte of the operand
        operand = bus.read(PC++);
        if (operandBytes == 2)
        {
            // read the high byte of the operand
            operand += bus.read(PC++) << 8;
        }
        else if (operandBytes != 1)
            printf("Invalid operand size for opcode 0x%X!\n", opcode);
        
        if (debugOutput)
            printf(" 0x%X\n", operand);
    }
    else
    {
        if (debugOutput)
            printf("\n");
    }

    
    // Call the function associated with this opcode
    ((*this).*(opcodes[opcode]))(); 
    // (The horror! It may be better to ditch OOP and go with globals for everything CPU related in the future)

    // return false if this is an unhandled opcode
    if (mnemonics[opcode] == UNHANDLED)
        return false;

    return true;
}

void CPU_6502::TriggerNMI()
{
    nmi = true;
}

// operations
void CPU_6502::UnhandledOpcode()
{
    printf("Opcode 0x%X is unhandled!\n", opcode);
}

void CPU_6502::ADC_Generic(uint8_t value)
{
    uint16_t result = a + value;
    if (flags.carry)
        ++result;

    flags.carry = (result > 0xFF);
    flags.negative = IS_NEGATIVE(result);
    flags.zero = ((result & 0xFF) == 0);

    // Calculate overflow flag
    // if one operand is negative and the other is positive, overflow will be cleared
    if (IS_NEGATIVE(a) != IS_NEGATIVE(value))
        flags.overflow = false;
    else
    {
        // both a and value are the same sign
        if (IS_NEGATIVE(a) != IS_NEGATIVE(result))
            flags.overflow = true;
        else
            flags.overflow = false;
    }

    a = (result & 0xFF);
}

// Perform an exclusive or between a and value and store result in a
void CPU_6502::EOR_Generic(uint8_t value)
{
    a ^= value;

    flags.negative = IS_NEGATIVE(a);
    flags.zero = (a == 0);
}

// Inclusive OR of accumulator and value
void CPU_6502::ORA_Generic(uint8_t value)
{
    a |= value;

    flags.negative = IS_NEGATIVE(a);
    flags.zero = (a == 0);
}

// a = a - value - !c
void CPU_6502::SBC_Generic(uint8_t value)
{
    // This is almost certainly less difficult than I'm making it
    int signedA = (int8_t)a;
    int signedValue = (int8_t)value;
    int c = (flags.carry ? 1 : 0);
    int res = signedA - signedValue - c;

    // I read this online but it's probably wrong
    if (res > 127 || res < -127)
        flags.overflow = true;
    else
        flags.overflow = false;

    value = ~value;
    value += (flags.carry ? 1 : 0);

    uint16_t result = (uint16_t)a + value;

    flags.carry = ((result & 0x100) == 0x100);
    
    a = (uint8_t)result;

    flags.zero = (a == 0);
    flags.negative = IS_NEGATIVE(a);
}

// 00: BRK - break - 7, 1
void CPU_6502::BRK()
{
    printf("BRK encountered\n");
    running = false;
}

// 01: ORA (zp, x) - Perform an inclusive or between a and an indexed indirect memory location - 6, 2
void CPU_6502::ORA_zp_x_ind()
{
    uint16_t addr1 = (operand + x) & 0xFF;
    //printf("addr1: 0x%X\n", addr1);

    uint16_t address = bus.read(addr1);
    address += (uint16_t)bus.read(addr1 + 1) << 8;

    ORA_Generic(bus.read(address));
}

// 05: ORA zp - perform inclusive or between a and value stored in zero page - 3, 2
void CPU_6502::ORA_zp()
{
    ORA_Generic(bus.read(operand));
}

// 06: ASL zp - shift memory value in zp to the left one bit  - 5, 2
void CPU_6502::ASL_zp()
{
    uint8_t value = bus.read(operand);

    flags.carry = IS_NEGATIVE(value);

    value <<= 1;

    flags.negative = IS_NEGATIVE(value);
    flags.zero = IS_NEGATIVE(value);

    bus.write(operand, value);
}

// 08: PHP i - Push processor status flags onto stack - 3, 1
void CPU_6502::PHP()
{
    uint8_t flagValues = flags.allFlags;

    // Set bits 4 and 5 (See https://wiki.nesdev.com/w/index.php/Status_flags#The_B_flag)
    flagValues |= 0x30;

    bus.write(0x100 + SP, flagValues);
    --SP;
}

// 09: ORA # - inclusive OR between a nd immediate value - 2, 2
void CPU_6502::ORA_imm()
{
    ORA_Generic((uint8_t)operand);
}

// 0A: ASL - shift accumulator value one to the left - 2, 1
void CPU_6502::ASL()
{
    flags.carry = IS_NEGATIVE(a);

    a <<= 1;

    flags.negative = IS_NEGATIVE(a);
    flags.zero = (a == 0);
}

// 0D: ORA a - Inclusive OR between a and value in memory - 4, 3
void CPU_6502::ORA_a()
{
    ORA_Generic(bus.read(operand));
}

// 0E: ASL a - shift absolute memory value to the left one bit  - 6, 3
void CPU_6502::ASL_a()
{
    uint8_t value = bus.read(operand);

    flags.carry = IS_NEGATIVE(value);

    value <<= 1;

    flags.negative = IS_NEGATIVE(value);
    flags.zero = IS_NEGATIVE(value);

    bus.write(operand, value);
}

// 10: BPL r - branch relative if negative flag is clear - 2, 2
void CPU_6502::BPL_r()
{
    if (!flags.negative)
        PC += (int8_t)operand;
}

// 11: ORA (zp),y - Inclusive OR between a and an indirectly indexed value in memory - 5, 2
void CPU_6502::ORA_zp_ind_y()
{
    // TODO: if operand is 0xFF, should high byte of address be 0x0 or 0x100?
    uint16_t address = bus.read(operand);
    address += (uint16_t)bus.read((operand + 1) & 0xFF) << 8;
    address += y;

    ORA_Generic(bus.read(address));
}

// 15: Inclusive OR between a and a value in ZP memory offset by x - 4, 2
void CPU_6502::ORA_zp_x()
{
    uint16_t address = (operand + x) & 0xFF;

    ORA_Generic(bus.read(address));
}

// 16: ASL zp,x - shift memory value in zp offset by x to the left one bit  - 6, 2
void CPU_6502::ASL_zp_x()
{
    // handle zero-page wraparound
    uint16_t address = (operand + x) & 0xFF;

    uint8_t value = bus.read(address);

    flags.carry = IS_NEGATIVE(value);

    value <<= 1;

    flags.negative = IS_NEGATIVE(value);
    flags.zero = IS_NEGATIVE(value);

    bus.write(address, value);
}

// 18: CLC i - clear carry - 2, 1
void CPU_6502::CLC()
{
    flags.carry = false;
}

// 19: ORA a,y - Inclusive OR between a and a value stored in memory offset by y - 4, 3
void CPU_6502::ORA_a_y()
{
    ORA_Generic(bus.read(operand + y));
}

// 1D: ORA a,x - Inclusive OR between a and a value stored in memory offset by x - 4, 3
void CPU_6502::ORA_a_x()
{
    ORA_Generic(bus.read(operand + x));
}

// 1E: ASL a,x - shift absolute memory offset by x value to the left one bit  - 7, 3
void CPU_6502::ASL_a_x()
{
    uint8_t value = bus.read(operand + x);

    flags.carry = IS_NEGATIVE(value);

    value <<= 1;

    flags.negative = IS_NEGATIVE(value);
    flags.zero = IS_NEGATIVE(value);

    bus.write(operand + x, value);
}

// 20: JSR a - pushes PC - 1 then jumps to absolute address - 6, 3
void CPU_6502::JSR()
{
    --PC;
    // push high byte of PC - 1
    bus.write(0x100 + SP, (PC >> 8));
    SP--;

    // push low byte of PC - 1
    bus.write(0x100 + SP, (PC & 0xFF));
    SP--;

    PC = operand;
}

// 21: AND (zp, x) - Perform an AND between a and an indexed indirect memory location - 6, 2
void CPU_6502::AND_zp_x_ind()
{
    uint16_t addr1 = (operand + x) & 0xFF;
    //printf("addr1: 0x%X\n", addr1);

    uint16_t address = bus.read(addr1);
    address += (uint16_t)bus.read(addr1 + 1) << 8;

    a &= bus.read(address);

    flags.negative = IS_NEGATIVE(a);
    flags.zero = (a == 0);
}

// 24: BIT zp - Perform a bit test with a value in zp memory - 3, 2
void CPU_6502::BIT_zp()
{
    uint8_t testValue = bus.read(operand);

    uint8_t results = a & testValue;

    flags.zero = (results == 0);
    flags.negative = ((testValue & 0x80) == 0x80);
    flags.overflow = ((testValue & 0x40) == 0x40);
}

// 25: AND zp - read memory from zp and perform bitwise AND with accumulator - 3, 2
void CPU_6502::AND_zp()
{
    a &= bus.read(operand);

    flags.zero = (a == 0);
    flags.negative = IS_NEGATIVE(a);
}

// 26: ROL zp - rotate value in zero page memory one bit to the left - 5, 2
void CPU_6502::ROL_zp()
{
    uint8_t value = bus.read(operand);
    uint8_t newValue = value << 1;

    if (flags.carry)
        newValue |= 1;

    flags.carry = IS_NEGATIVE(newValue);

    value = newValue;

    flags.negative = IS_NEGATIVE(value);
    flags.zero = (value == 0);

    bus.write(operand, value);
}

// 28: PLP i - pull processor status flags from stack - 4, 1
void CPU_6502::PLP()
{
    ++SP;

    // ignore bits 4 and 5 of pushed values (see PHP)
    uint8_t flagValues = bus.read(0x100 + SP);
    flagValues &= 0xCF;
    flagValues += flags.allFlags & 0x30;

    flags.allFlags = flagValues;
}

// 29: AND # - bitwise AND with accumulator - 2, 2
void CPU_6502::AND_imm()
{
    a &= operand;

    flags.zero = (a == 0);
    flags.negative = IS_NEGATIVE(a);
}

// 2A: ROL A - rotate accumulator one bit to the left - 2, 1
void CPU_6502::ROL_A()
{
    uint8_t newA = a << 1;

    if (flags.carry)
        newA |= 1;

    flags.carry = IS_NEGATIVE(a);
    
    a = newA;

    flags.negative = IS_NEGATIVE(a);
    flags.zero = (a == 0);
}

// 2C: BIT a - Perform a bit test with a value in abs memory - 4, 3
void CPU_6502::BIT_a()
{
    uint8_t testValue = bus.read(operand);

    uint8_t results = a & testValue;

    flags.zero = (results == 0);
    flags.negative = ((testValue & 0x80) == 0x80);
    flags.overflow = ((testValue & 0x40) == 0x40);
}

// 2D: AND a - read memory from absolute address and perform bitwise AND with accumulator - 4, 3
void CPU_6502::AND_a()
{
    a &= bus.read(operand);

    flags.zero = (a == 0);
    flags.negative = IS_NEGATIVE(a);
}

// 2E: ROL a - rotate value in memory one bit to the left - 6, 3
void CPU_6502::ROL_a()
{
    uint8_t value = bus.read(operand);
    uint8_t newValue = value << 1;

    if (flags.carry)
        newValue |= 1;

    flags.carry = IS_NEGATIVE(newValue);

    value = newValue;

    flags.negative = IS_NEGATIVE(value);
    flags.zero = (value == 0);

    bus.write(operand, value);
}

// 30: BMI r - branch relative if negative flag is set - 2, 2
void CPU_6502::BMI_r()
{
    if (flags.negative)
        PC += (int8_t)operand;
}

// 31: AND (zp),y - Perform logical AND operation between a and an indirectly indexed value in memory - 5, 2
void CPU_6502::AND_zp_ind_y()
{
    // TODO: if operand is 0xFF, should high byte of address be 0x0 or 0x100?
    uint16_t address = bus.read(operand);
    address += (uint16_t)bus.read((operand + 1) & 0xFF) << 8;
    address += y;

    uint8_t value = bus.read(address);

    a &= value;

    flags.negative = IS_NEGATIVE(a);
    flags.zero = (a == 0);
}

// 35: AND zp, x - read memory from zp + x and perform bitwise AND with accumulator - 4, 2
void CPU_6502::AND_zp_x()
{    
    // handle zero-page wraparound
    uint16_t address = (operand + x) & 0xFF;

    a &= bus.read(address);

    flags.zero = (a == 0);
    flags.negative = IS_NEGATIVE(a);
}

// 36: ROL zp,x - rotate value in zero page memory offset by x one bit to the left - 6, 2
void CPU_6502::ROL_zp_x()
{
    // handle zero page wrap-around
    uint16_t address = (operand + x) & 0xFF;

    uint8_t value = bus.read(address);
    uint8_t newValue = value << 1;

    if (flags.carry)
        newValue |= 1;

    flags.carry = IS_NEGATIVE(newValue);

    value = newValue;

    flags.negative = IS_NEGATIVE(value);
    flags.zero = (value == 0);

    bus.write(address, value);
}

// 38: set carry - 2, 1
void CPU_6502::SEC()
{
    flags.carry = true;
}

// 39: AND a,x - read memory from absolute address offset by y and perform bitwise AND with accumulator - 4, 3
void CPU_6502::AND_a_y()
{
    a &= bus.read(operand + y);

    flags.zero = (a == 0);
    flags.negative = IS_NEGATIVE(a);
}

// 3D: AND a,x - read memory from absolute address offset by x and perform bitwise AND with accumulator - 4, 3
void CPU_6502::AND_a_x()
{
    a &= bus.read(operand + x);

    flags.zero = (a == 0);
    flags.negative = IS_NEGATIVE(a);
}

// 3E: ROL a,x - rotate value in absolute memory offset by x one bit to the left - 7, 3
void CPU_6502::ROL_a_x()
{
    uint16_t address = operand + x;

    uint8_t value = bus.read(address);
    uint8_t newValue = value << 1;

    if (flags.carry)
        newValue |= 1;

    flags.carry = IS_NEGATIVE(newValue);

    value = newValue;

    flags.negative = IS_NEGATIVE(value);
    flags.zero = (value == 0);

    bus.write(address, value);
}

// 40: RTI - return from interrupt - 6, 1
void CPU_6502::RTI()
{
    ++SP;
    flags.allFlags = bus.read(0x100 + SP) & 0xCF; // ignore bits 5 and 4

    ++SP;
    // pop off the low byte of the return address
    uint16_t newPC = bus.read(0x100 + SP);

    // pop off the high byte
    ++SP;
    newPC += bus.read(0x100 + SP) << 8;

    PC = newPC;
    
    //running = false;
    printf("\nRet PC - 0x%X\n\n", PC);
}

// 41: EOR (zp,x) - Perform EOR with a value from a zp indexed indirect address - 6, 2
void CPU_6502::EOR_zp_x_ind()
{
    uint16_t addr1 = (operand + x) & 0xFF;
    printf("addr1: 0x%X\n", addr1);

    uint16_t address = bus.read(addr1);
    address += (uint16_t)bus.read(addr1 + 1) << 8;

    EOR_Generic(bus.read(address));
}

// 45: EOR zp - Perform EOR between a and a value stored in zp memory, and store result in a - 3, 2
void CPU_6502::EOR_zp()
{
    EOR_Generic(bus.read(operand));
}

// 46: LSR zp - read a byte from zp, shift it one bit to the right and put it back - 5, 2
void CPU_6502::LSR_zp()
{
    uint8_t value = bus.read(operand);

    flags.carry = ((value & 1) == 1);

    value >>= 1;

    bus.write(operand, value);
}

// 48: PHA s - push a to stack - 3, 1
void CPU_6502::PHA()
{
    bus.write(0x100 + SP, a);
    --SP;
}

// 49: EOR # - Perform EOR between a and an immediate value - 2, 2
void CPU_6502::EOR_imm()
{
    EOR_Generic((uint8_t)operand);
}

// 4A: LSR - shift a one bit to the right. Set carry with old bit 0 value. - 2, 1
void CPU_6502::LSR()
{
    flags.carry = ((a & 1) == 1);

    a >>= 1;

    flags.negative = IS_NEGATIVE(a);
    flags.zero = (a == 0);
}

// 4C: JMP a (jump to absolute address) - 3, 3
void CPU_6502::JMP_a()
{
    PC = operand;
}

// 4D: EOR a - Perform an EOR between a and a value in absolute memory - 4, 3
void CPU_6502::EOR_a()
{
    EOR_Generic(bus.read(operand));
}

// 4E: LSR a - read a byte from abs memory, shift it one bit to the right and put it back - 6, 3
void CPU_6502::LSR_a()
{
    uint8_t value = bus.read(operand);

    flags.carry = ((value & 1) == 1);

    value >>= 1;

    bus.write(operand, value);
}

// 50: BVC r - branch relative if overflow flag is clear - 2, 2
void CPU_6502::BVC_r()
{
    if (!flags.overflow)
        PC += (int8_t)operand;
}

// 51: EOR (zp),y - Perform an EOR between a and an indirectly indexed value in memory - 5, 2
void CPU_6502::EOR_zp_ind_y()
{
    // TODO: if operand is 0xFF, should high byte of address be 0x0 or 0x100?
    uint16_t address = bus.read(operand);
    address += (uint16_t)bus.read((operand + 1) & 0xFF) << 8;
    address += y;

    EOR_Generic(bus.read(address));
}

// 55: EOR zp,x - Perform an EOR between a and a value in zp memory offset by x - 4, 2
void CPU_6502::EOR_zp_x()
{
    // handle zero-page wraparound
    uint16_t address = (operand + x) & 0xFF;

    EOR_Generic(bus.read(address));
}

// 56: LSR zp,x - read a byte from zp offset by x, shift it one bit to the right and put it back - 6, 2
void CPU_6502::LSR_zp_x()
{
    // handle zero-page wraparound
    uint16_t address = (operand + x) & 0xFF;

    uint8_t value = bus.read(address);

    flags.carry = ((value & 1) == 1);

    value >>= 1;

    bus.write(address, value);
}

// 58: CLI i - clear interrupt disable flag - 2, 1
void CPU_6502::CLI()
{
    flags.irqDisable = false;
}

// 59: EOR a,y - Perform an EOR between a and a value in absolute memory offset by y - 4, 3
void CPU_6502::EOR_a_y()
{
    EOR_Generic(bus.read(operand + y));
}

// 5D: EOR a,x - Perform an EOR between a and a value in absolute memory offset by x - 4, 3
void CPU_6502::EOR_a_x()
{
    EOR_Generic(bus.read(operand + x));
}

// 5E: LSR a,x - read a byte from memory offset by x, shift it one bit to the right and put it back - 7, 3
void CPU_6502::LSR_a_x()
{
    uint8_t value = bus.read(operand + x);

    flags.carry = ((value & 1) == 1);

    value >>= 1;

    bus.write(operand + x, value);
}

// 60: RTS pop an adress of the stack, add one, and jump there - 6, 1
void CPU_6502::RTS()
{
    ++SP;
    // pop off the low byte
    uint16_t newPC = bus.read(0x100 + SP);

    // pop off the high byte
    ++SP;
    newPC += bus.read(0x100 + SP) << 8;

    PC = newPC + 1;
}

// 61: ADC (zp,x) - Perform an add with carry between a and a value from a zp indexed indirect address - 6, 2
void CPU_6502::ADC_zp_x_ind()
{
    uint16_t addr1 = (operand + x) & 0xFF;
    printf("addr1: 0x%X\n", addr1);

    uint16_t address = bus.read(addr1);
    address += (uint16_t)bus.read(addr1 + 1) << 8;

    ADC_Generic(bus.read(address));
}

// 65: ADC zp (add memory in zp to accumulator)
void CPU_6502::ADC_zp()
{
    ADC_Generic(bus.read(operand));
}

// 66: ROR zp - Move value stored in zp one bit to the right - 5, 2
void CPU_6502::ROR_zp()
{
    uint8_t value = bus.read(operand);

    uint8_t oldValue = value;

    value >>= 1;

    if (flags.carry)
        value |= 0x80;

    flags.carry = (oldValue & 1);
    flags.zero = (value == 0);
    flags.negative = IS_NEGATIVE(value);

    bus.write(operand, value);
}

// 68: PLA s - pull off of stack and into a - 4, 1
void CPU_6502::PLA()
{
    ++SP;
    a = bus.read(0x100 + SP);

    flags.zero = (a == 0);
    flags.negative = IS_NEGATIVE(a);
}

// 69: ADC # - add immediate
void CPU_6502::ADC_imm()
{
    ADC_Generic((uint8_t)operand);
}

// 6A: ROR A - Rotate accumulator one bit to the right - 2, 1
void CPU_6502::ROR_A()
{
    uint8_t newA = a >> 1;

    if (flags.carry)
        newA |= 0x80;

    flags.carry = ((a && 1) == 1);

    a = newA;

    flags.zero = (a == 0);
    flags.negative = IS_NEGATIVE(a);
}

// 6C: JMP (a) (jump to the address contained at address a) - 6, 3
void CPU_6502::JMP_ind()
{
    uint16_t addr = bus.read(operand);
    
    // The original 6502 has an error we need to check for which occurs if operand is 0xXXff
    // If so, it will read the address from 0xXX00 instead of operand + 1
    if((operand & 0xFF) == 0xFF)
        addr += (uint16_t)bus.read(operand & 0xFF00) << 8;
    else
        addr += (uint16_t)bus.read(operand + 1) << 8;

    PC = addr;
}

// 6D: ADC a - add value in absolute memory to accumulator - 4, 3
void CPU_6502::ADC_a()
{
    ADC_Generic(bus.read(operand));
}

// 6E: ROR a - Move value stored in absolute memory one bit to the right - 6, 3
void CPU_6502::ROR_a()
{
    uint8_t value = bus.read(operand);

    uint8_t oldValue = value;

    value >>= 1;

    if (flags.carry)
        value |= 0x80;

    flags.carry = (oldValue & 1);
    flags.zero = (value == 0);
    flags.negative = IS_NEGATIVE(value);

    bus.write(operand, value);
}

// 70: BVS r - Branch relative if overflow flag is set - 2, 2
void CPU_6502::BVS_r()
{
    if (flags.overflow)
        PC += (int8_t)operand;
}

// 71: ADC (zp),y - Perform an add with carry between a and an indirectly indexed value in memory - 5, 2 
void CPU_6502::ADC_zp_ind_y()
{
    // TODO: if operand is 0xFF, should high byte of address be 0x0 or 0x100?
    uint16_t address = bus.read(operand);
    address += (uint16_t)bus.read((operand + 1) & 0xFF) << 8;
    address += y;

    ADC_Generic(bus.read(address));
}

// 75: ADC zp,x - Perform an add with carry between a and a memory value in zero page offset by x - 4, 2
void CPU_6502::ADC_zp_x()
{
    // Handle zero-page wrap-around
    uint16_t address = (operand + x) & 0xFF;
    
    ADC_Generic(bus.read(address));
}

// 76: ROR zp,x - Move value stored in zero page of memory offset by x one bit to the right - 5, 2
void CPU_6502::ROR_zp_x()
{
    // Handle zero-page wrap-around
    uint16_t address = (operand + x) & 0xFF;

    uint8_t value = bus.read(address);

    uint8_t oldValue = value;

    value >>= 1;

    if (flags.carry)
        value |= 0x80;

    flags.carry = (oldValue & 1);
    flags.zero = (value == 0);
    flags.negative = IS_NEGATIVE(value);

    bus.write(address, value);
}

// 78: SEI i - set interrupt disable flag - 2, 1
void CPU_6502::SEI()
{
    flags.irqDisable = true;
}

// 79: ADC a,y - Perform an add with carry between a and a value in absolute memory offset by y - 4, 3
void CPU_6502::ADC_a_y()
{
    ADC_Generic(a + y);
}

// 7D: ADC a,x - Perform an add with carry between a and a value at an absolute address offset by x - 4, 3
void CPU_6502::ADC_a_x()
{
    ADC_Generic(bus.read(operand + x));
}

// 7E: ROR a,x - Move value stored in absolute memory offset by x one bit to the right - 7, 3
void CPU_6502::ROR_a_x()
{
    uint8_t value = bus.read(operand + x);

    uint8_t oldValue = value;

    value >>= 1;

    if (flags.carry)
        value |= 0x80;

    flags.carry = (oldValue & 1);
    flags.zero = (value == 0);
    flags.negative = IS_NEGATIVE(value);

    bus.write(operand + x, value);
}

// 81: STA (zp, x) - store a into a zp indexed indirect address - 6, 2
void CPU_6502::STA_zp_x_ind()
{
    uint16_t addr1 = (operand + x) & 0xFF;
    printf("addr1: 0x%X\n", addr1);

    uint16_t address = bus.read(addr1);
    address += (uint16_t)bus.read(addr1 + 1) << 8;

    printf("address: 0x%X\n", address);

    bus.write(address, a);
}

// 84: STY zp - store y to a value in zp memory - 3, 2
void CPU_6502::STY_zp()
{
    bus.write(operand, y);
}

// 85: STA zp (store accumulator to zp memory, 0 - 0xff)
void CPU_6502::STA_zp()
{
    bus.write(operand, a);
}

// 86: STX zp - store x in a zp memory location - 3, 2
void CPU_6502::STX_zp()
{
    bus.write(operand, x);
}

// 88: DEY i = decrement y register - 2, 1
void CPU_6502::DEY()
{
    --y;

    flags.negative = IS_NEGATIVE(y);
    flags.zero = (y == 0);
}

// 98: TYA i - transfer y to a - 2, 1
void CPU_6502::TYA()
{
    a = y;

    flags.negative = IS_NEGATIVE(a);
    flags.zero = (a == 0);
}

// 8A: TXA - transfer x to a - 2, 1
void CPU_6502::TXA()
{
    a = x;

    flags.zero = (a == 0);
    flags.negative = IS_NEGATIVE(a);
}

// 8C: STY a (store y to absolute memory address) - 4, 3
void CPU_6502::STY_a()
{
    bus.write(operand, y);
}

// 8D: STA a (store a to absolute memory address)
void CPU_6502::STA_a()
{
    bus.write(operand, a);
}

// 8E: STX a (store x to absolute memory address) - 4, 3
void CPU_6502::STX_a()
{
    bus.write(operand, x);
}

// 90: BCC r - branch relative if carry flag is clear - 2, 2
void CPU_6502::BCC_r()
{
    if (!flags.carry)
        PC += (int8_t)operand;
}

// 91: STA(zp), y - store a to indirectly indexed memory - 6, 2
void CPU_6502::STA_zp_ind_y()
{
    // TODO: if operand is 0xFF, should high byte of address be 0x0 or 0x100?
    uint16_t address = bus.read(operand);
    address += (uint16_t)bus.read((operand + 1) & 0xFF) << 8;
    address += y;

    bus.write(address, a);
}

// 94: STY zp,x - store y to a value in zp memory offset by x - 4, 2
void CPU_6502::STY_zp_x()
{
    // Handle zero page wrap-around
    uint8_t address = (uint8_t)(operand + x);

    bus.write(address, y);
}

// 95: STA zp,x (store a to zp memory offset by x) - 4, 2
void CPU_6502::STA_zp_x()
{
    bus.write((operand + x) & 0xFF, a);
}

// 96: STX zp,y - store x in a zp address offset by y - 4, 2
void CPU_6502::STX_zp_y()
{
    // TODO: Should this wrap-around?
    uint8_t address = operand + y;

    bus.write(address, y);
}

// 99: STA a,y - Store a to absolute address + y offset - 5, 3
void CPU_6502::STA_a_y()
{
    bus.write(operand + y, a);
}

// 9D: STA a,x - store a to an address offset by x - 5, 3
void CPU_6502::STA_a_x()
{
    bus.write(operand + x, a);
}

// 9A: TXS - transfer x to SP register - 2, 1
void CPU_6502::TXS()
{
    SP = x;
}

// A0: LDY # (load immediate value to Y) - 2, 2
void CPU_6502::LDY_imm()
{
    y = (uint8_t)operand;

    flags.zero = (y == 0);
    flags.negative = IS_NEGATIVE(y);
}

// A1: LDA (zp,x) - add zp address to x register and
// read the memory at the resulting address to find
// the address of the data to load into a
void CPU_6502::LDA_zp_x_ind()
{
    // handle zero-page wraparound
    uint8_t addr = (uint8_t)(operand + x);
    
    uint16_t newAddr = bus.read(addr);
    ++addr;
    newAddr += (uint16_t)bus.read(addr) << 8;
    
    a = bus.read(newAddr);

    flags.zero = (a == 0);
    flags.negative = IS_NEGATIVE(a);
}

// A2: LDX # (Load immediate into X)
void CPU_6502::LDX_imm()
{
    x = (uint8_t)operand;
    flags.zero = (x == 0);
    flags.negative = IS_NEGATIVE(x);
}

// A4: LDY zp - load y with memory from zero page - 3, 2
void CPU_6502::LDY_zp()
{
    y = bus.read(operand);

    flags.negative = IS_NEGATIVE(y);
    flags.zero = (y == 0);
}

// A5: LDA zp - load a with memory from zp - 3, 2
void CPU_6502::LDA_zp()
{
    a = bus.read(operand);

    flags.zero = (a == 0);
    flags.negative = IS_NEGATIVE(a);
}

// A6: LDX zp - Load x with memory from zero page - 3, 2
void CPU_6502::LDX_zp()
{
    x = bus.read(operand);

    flags.zero = (x == 0);
    flags.negative = IS_NEGATIVE(x);
}

// A8: TAY i = transfer a to y - 2, 1
void CPU_6502::TAY()
{
    y = a;

    flags.zero = (y == 0);
    flags.negative = IS_NEGATIVE(y);
}

// A9: LDA # (imm)
void CPU_6502::LDA_imm()
{
    a = (uint8_t)operand;
    flags.negative = ((a & 0x80) == 0x80);
    flags.zero = (a == 0);
}

// AA: TAX i - transfer a to x register
void CPU_6502::TAX()
{
    x = a;
    flags.negative = IS_NEGATIVE(x);
    flags.zero = (x == 0);
}

// AC: LDY a - Load y with a value from absolute memory - 4, 3
void CPU_6502::LDY_a()
{
    y = bus.read(operand);

    flags.negative = IS_NEGATIVE(y);
    flags.zero = (y == 0);
}

// AD: LDA a - Load a with absolute memory address - LDA 4, 3
void CPU_6502::LDA_a()
{
    a = bus.read(operand);

    flags.zero = (a == 0);
    flags.negative = IS_NEGATIVE(a);
}

// AE: LDX a - load x with value from absolute memory address - 4, 3
void CPU_6502::LDX_a()
{
    x = bus.read(operand);

    flags.zero = (x == 0);
    flags.negative = IS_NEGATIVE(x);
}

// B0: BCS r - branch relative if carry set - 2, 2
void CPU_6502::BCS_r()
{
    if (flags.carry)
        PC += (int8_t)operand;
}

// B1: LDA (zp), y - (Read 2 bytes starting at a zero-page address,
// add y, and load the memory stored at the resulting address into a) - 5, 2
void CPU_6502::LDA_zp_y_ind()
{
    uint16_t newAddress = bus.read(operand);
    newAddress += bus.read(operand + 1) << 8;
    newAddress += y;

    a = bus.read(newAddress);

    flags.zero = (a == 0);
    flags.negative = IS_NEGATIVE(a);
}

// B4: load y with memory from zero page offset by x - 4, 2
void CPU_6502::LDY_zp_x()
{
    // handle wrap-around
    uint8_t address = (uint8_t)operand + x;
    
    y = bus.read(address);

    flags.negative = IS_NEGATIVE(y);
    flags.zero = (y == 0);
}

// B5: LDA zp,x - Load a with memory at zp absolute address offset by x LDA zp,x - 4, 2
void CPU_6502::LDA_zp_x()
{    
    // handle zero-page wraparound
    uint16_t address = (operand + x) & 0xFF;

    a = bus.read(address);

    flags.zero = (a == 0);
    flags.negative = IS_NEGATIVE(a);
}

// B6: LDX zp, y - Load x with memory in zero page offset by y - 4, 2
void CPU_6502::LDX_zp_y()
{
    // handle zero-page wraparound (TODO: Is this right?)
    uint16_t address = (operand + y) & 0xFF;

    x = bus.read(address);

    flags.zero = (x == 0);
    flags.negative = IS_NEGATIVE(x);
}

// B8: CLV i - Clear overflow flag - 2, 1
void CPU_6502::CLV()
{
    flags.overflow = false;
}

// B9: LDA a,y - Load a with memory at absolute address offset by y - 4, 3
void CPU_6502::LDA_a_y()
{
    a = bus.read(operand + y);

    flags.zero = (a == 0);
    flags.negative = IS_NEGATIVE(a);
}

// BA: TSX i - copy stack pointer to x - 2, 1
void CPU_6502::TSX()
{
    x = SP;

    flags.negative = IS_NEGATIVE(x);
    flags.zero = (x == 0);
}

// BC: LDY a,x - Load y with a value from absolute memory offset by x - 4, 3
void CPU_6502::LDY_a_x()
{
    y = bus.read(operand + x);

    flags.negative = IS_NEGATIVE(y);
    flags.zero = (y == 0);
}

// BD: LDA a,x - load a with memory at absolute address offset by x - 4, 3
void CPU_6502::LDA_a_x()
{
    a = bus.read(operand + x);

    flags.zero = (a == 0);
    flags.negative = IS_NEGATIVE(a);
}

// BE: LDX a,y - load x with value at absolute memory address offset by y - 4, 3
void CPU_6502::LDX_a_y()
{
    x = bus.read(operand + y);

    flags.zero = (x == 0);
    flags.negative = IS_NEGATIVE(x);
}

// C0: CPY # - compare y with immediate value - 2, 2
void CPU_6502::CPY_imm()
{
    flags.zero = (y == operand);
    flags.carry = (y >= operand);
    flags.negative = IS_NEGATIVE(y - operand);
}

// C4: CPY zp - compare y with value stored in zero page - 3, 2
void CPU_6502::CPY_zp()
{
    uint8_t value = bus.read(operand);

    flags.zero = (y == value);
    flags.carry = (y >= value);
    flags.negative = IS_NEGATIVE(y - value);
}

// C1: CMP (zp,x) - Compare a with a zp indexed indirect value - 6, 2
void CPU_6502::CMP_zp_x_ind()
{
    uint16_t addr1 = (operand + x) & 0xFF;
    printf("addr1: 0x%X\n", addr1);

    uint16_t address = bus.read(addr1);
    // TODO: Does this need to wrap around as well?
    address += (uint16_t)bus.read(addr1 + 1) << 8;
    
    uint8_t value = bus.read(address);

    flags.zero = (a == value);
    flags.carry = (a >= value);
    flags.negative = IS_NEGATIVE(a - value);
}

// C5: CMP zp - compare accumulator with memory stored in zero page - 3, 2
void CPU_6502::CMP_zp()
{
    uint8_t value = bus.read(operand);

    flags.zero = (a == value);
    flags.carry = (a >= value);
    flags.negative = IS_NEGATIVE(a - value);
}

// C6: DEC zp - decrement a value in zp memory - 5, 2
void CPU_6502::DEC_zp()
{
    uint8_t value = bus.read(operand);

    --value;

    bus.write(operand, value);

    flags.negative = IS_NEGATIVE(value);
    flags.zero = (value == 0);
}

// C8: INY i - increment y
void CPU_6502::INY()
{
    ++y;

    flags.zero = (y == 0);
    flags.negative = IS_NEGATIVE(y);
}

// C9: CMP # (compare a to immediate value) - 2, 2
void CPU_6502::CMP_imm()
{
    flags.carry = (a >= operand);
    flags.zero = (a == operand);
    flags.negative = (IS_NEGATIVE(a - operand));
}

// CA: decrement x - 2, 1
void CPU_6502::DEX()
{
    --x;
    flags.negative = IS_NEGATIVE(x);
    flags.zero = (x == 0);
}

// CC: CPY a - compare y with value stored in absolute memory - 4, 3
void CPU_6502::CPY_a()
{
    uint8_t value = bus.read(operand);

    flags.zero = (y == value);
    flags.carry = (y >= value);
    flags.negative = IS_NEGATIVE(y - value);
}

// CD: CMP a - Compare accumulator with memory at absolute address - 4, 3
void CPU_6502::CMP_a()
{
    uint8_t value = bus.read(operand);

    flags.zero = (a == value);
    flags.carry = (a >= value);
    flags.negative = IS_NEGATIVE(a - value);
}

// CE: DEC a - decrement a value in memory - 6, 3
void CPU_6502::DEC_a()
{
    uint8_t value = bus.read(operand);

    --value;

    bus.write(operand, value);

    flags.negative = IS_NEGATIVE(value);
    flags.zero = (value == 0);
}

// D0 - BRNE r (Branch relative if z flag is cleared) - 2, 2
void CPU_6502::BRNE_r()
{
    if (!flags.zero)
    {
        int8_t displacement = (int8_t)operand;
        PC += displacement;
    }
}

// D1: CMP (zp), y - Compare a with an indirectly indexed value in memory - 5, 2
void CPU_6502::CMP_zp_ind_y()
{
    // TODO: if operand is 0xFF, should high byte of address be 0x0 or 0x100?
    uint16_t address = bus.read(operand);
    address += (uint16_t)bus.read((operand + 1) & 0xFF) << 8;
    address += y;

    uint8_t value = bus.read(address);

    flags.zero = (a == value);
    flags.carry = (a >= value);
    flags.negative = IS_NEGATIVE(a - value);
}

// D5: CMP zp,x - compare accumulator with memory stored in zero page offset by x - 4, 2
void CPU_6502::CMP_zp_x()
{
    // handle zero-page wraparound
    uint16_t address = (operand + x) & 0xFF;

    uint8_t value = bus.read(address);

    flags.zero = (a == value);
    flags.carry = (a >= value);
    flags.negative = IS_NEGATIVE(a - value);
}

// D6: DEC zp, x - decrement a value in zp memory offset by x - 6, 2
void CPU_6502::DEC_zp_x()
{
    // handle zero-page wraparound
    uint16_t address = (operand + x) & 0xFF;

    uint8_t value = bus.read(address);

    --value;

    bus.write(address, value);

    flags.negative = IS_NEGATIVE(value);
    flags.zero = (value == 0);
}

// D9: CMP a, 9 - Compare accumulator with memory at absolute address offset by y - 4, 3
void CPU_6502::CMP_a_y()
{
    uint8_t value = bus.read(operand + y);

    flags.zero = (a == value);
    flags.carry = (a >= value);
    flags.negative = IS_NEGATIVE(a - value);
}

// D8 - CLD i - clear decimal flag - 2, 1
void CPU_6502::CLD()
{
    flags.decimal = false;
}

// DD: CMP a, x - Compare accumulator with memory at absolute address offset by x - 4, 3
void CPU_6502::CMP_a_x()
{
    uint8_t value = bus.read(operand + x);

    flags.zero = (a == value);
    flags.carry = (a >= value);
    flags.negative = IS_NEGATIVE(a - value);
}

// DE: DEC a, x - decrement a value in abs memory offset by x - 7, 3
void CPU_6502::DEC_a_x()
{
    uint8_t value = bus.read(operand + x);

    --value;

    bus.write(operand + x, value);

    flags.negative = IS_NEGATIVE(value);
    flags.zero = (value == 0);
}

// E0 - compare x with immediate value - 2, 2
void CPU_6502::CPX_imm()
{
    flags.carry = (x >= operand);
    flags.zero = (x == operand);
    flags.negative = IS_NEGATIVE(x - operand);
}

// E1: SBC (zp,x) - Perform a subtract with carry from an indexed indirect value - 6, 2
void CPU_6502::SBC_zp_x_ind()
{
    uint16_t addr1 = (operand + x) & 0xFF;
    printf("addr1: 0x%X\n", addr1);

    uint16_t address = bus.read(addr1);
    // TODO: wrap-around?
    address += (uint16_t)bus.read(addr1 + 1) << 8;

    SBC_Generic(bus.read(address));
}

// E4: CPX zp - compare x with memory value stored in zero page - 3, 2
void CPU_6502::CPX_zp()
{
    uint8_t value = bus.read(operand);

    flags.carry = (x >= value);
    flags.zero = (x == value);
    flags.negative = IS_NEGATIVE(x - value);
}

// E5: SBC zp - subtract from a a value stored in the zero page - 3, 2
void CPU_6502::SBC_zp()
{
    SBC_Generic(bus.read(operand));
}

// E6: INC zp - increment a value in zp memory - 5, 2
void CPU_6502::INC_zp()
{
    uint8_t value = bus.read(operand);

    value++;

    bus.write(operand, value);

    flags.zero = (value == 0);
    flags.negative = IS_NEGATIVE(value);
}

// E8: INX - Inc X - 2, 1
void CPU_6502::INX()
{
    ++x;
    flags.zero = (x == 0);
    flags.negative = ((x & 0x80) == 0x80);
}

// E9: SBC # - subtract immediate from a - 2, 2
void CPU_6502::SBC_imm()
{
    SBC_Generic((uint8_t)operand);
}

// EA: NOP - 2, 1
void CPU_6502::NOP()
{
}

// EC: CPX a - compare x with memory value - 4, 3
void CPU_6502::CPX_a()
{
    uint8_t value = bus.read(operand);

    flags.carry = (x >= value);
    flags.zero = (x == value);
    flags.negative = IS_NEGATIVE(x - value);
}

// ED: SBC a - subtract value at absolute addr from a and store result in a - 4, 3
void CPU_6502::SBC_a()
{
    SBC_Generic(bus.read(operand));
}

// EE: INC a - increment a value in abs memory - 6, 3
void CPU_6502::INC_a()
{
    uint8_t value = bus.read(operand);

    value++;

    bus.write(operand, value);

    flags.zero = (value == 0);
    flags.negative = IS_NEGATIVE(value);
}

// F0: BEQ r - branch if equal - 2, 2
void CPU_6502::BEQ_r()
{
    if (flags.zero)
    {
        int8_t displacement = (int8_t)operand;
        PC += displacement;
    }
}

// F1: SBC (zp), y - Perform a subtraction with carry between a and an indirectly indexed value in memory - 5, 2
void CPU_6502::SBC_zp_ind_y()
{
    // TODO: if operand is 0xFF, should high byte of address be 0x0 or 0x100?
    uint16_t address = bus.read(operand);
    address += (uint16_t)bus.read((operand + 1) & 0xFF) << 8;
    address += y;

    SBC_Generic(bus.read(address));
}

// F5: SBC zp, x - Subtract value at zp offset by x from a and store result in a - 4, 2
void CPU_6502::SBC_zp_x()
{
    // handle zero-page wraparound
    uint16_t address = (operand + x) & 0xFF;

    SBC_Generic(bus.read(address));
}

// F6: INC zp,x - increment a value in zp memory offset by x - 6, 2
void CPU_6502::INC_zp_x()
{
    // handle zero-page wraparound
    uint16_t address = (operand + x) & 0xFF;

    uint8_t value = bus.read(address);

    value++;

    bus.write(address, value);

    flags.zero = (value == 0);
    flags.negative = IS_NEGATIVE(value);
}

// F9: SBC a,y - subtract value in abs memory address offset by y from a and store result in a - 4, 3
void CPU_6502::SBC_a_y()
{
    SBC_Generic(bus.read(operand + y));
}

// FD: SBC a,x - subtract value in abs memory address offset by x from a and store result in a - 4, 3
void CPU_6502::SBC_a_x()
{
    SBC_Generic(bus.read(operand + x));
}

// FE: INC a,x - increment a value in memory offset by x - 7, 3
void CPU_6502::INC_a_x()
{
    uint8_t value = bus.read(operand + x);

    value++;

    bus.write(operand + x, value);

    flags.zero = (value == 0);
    flags.negative = IS_NEGATIVE(value);
}

// F8: SED i - set decimal flag - 2, 1
void CPU_6502::SED()
{
    flags.decimal = true;
}

void CPU_6502::SetupOpcodes()
{
    for (int i = 0; i < 256; ++i)
    {
        opcodes[i] = &CPU_6502::UnhandledOpcode;
        mnemonics[i] = UNHANDLED;
        opcodeBytes[i] = 1;
    }

    // 00: BRK - break - 7, 1
    SetupOpCode(0x00, &CPU_6502::BRK, MN_BRK, 1);

    // 01: ORA (zp, x) - Perform an inclusive or between a and an indexed indirect memory location - 6, 2
    SetupOpCode(0x01, &CPU_6502::ORA_zp_x_ind, MN_ORA_ZP_X_IND, 2);
    
    // 05: ORA zp - perform inclusive or between a and value stored in zero page - 3, 2
    SetupOpCode(0x05, &CPU_6502::ORA_zp, MN_ORA_ZP, 2);

    // 06: ASL zp - shift memory value in zp to the left one bit  - 5, 2
    SetupOpCode(0x06, &CPU_6502::ASL_zp, MN_ASL_ZP, 2);
    
    // 08: PHP i - Push processor status flags onto stack - 3, 1
    SetupOpCode(0x08, &CPU_6502::PHP, MN_PHP, 1);

    // 09: ORA # - inclusive OR between a nd immediate value - 2, 2
    SetupOpCode(0x09, &CPU_6502::ORA_imm, MN_ORA_IMM, 2);

    // 0D: ORA a - Inclusive OR between a and value in memory - 4, 3
    SetupOpCode(0x0D, &CPU_6502::ORA_a, MN_ORA_ABS, 3);

    // 0A: ASL - shift accumulator value one to the left - 2, 1
    SetupOpCode(0x0A, &CPU_6502::ASL, MN_ASL, 1);

    // 0E: ASL a - shift absolute memory value to the left one bit  - 6, 3
    SetupOpCode(0x0E, &CPU_6502::ASL_a, MN_ASL_ABS, 3);
    
    // 10: BPL r - branch relative if negative flag is clear - 2, 2
    SetupOpCode(0x10, &CPU_6502::BPL_r, MN_BPL, 2);

    // 11: ORA (zp),y - Inclusive OR between a and an indirectly indexed value in memory - 5, 2
    SetupOpCode(0x11, &CPU_6502::ORA_zp_ind_y, MN_ORA_ZP_IND_Y, 2);

    // 15: Inclusive OR between a and a value in ZP memory offset by x - 4, 2
    SetupOpCode(0x15, &CPU_6502::ORA_zp_x, MN_ORA_ZP_X, 2);

    // 16: ASL zp,x - shift memory value in zp offset by x to the left one bit  - 6, 2
    SetupOpCode(0x16, &CPU_6502::ASL_zp_x, MN_ASL_ZP_X, 2);

    // 18: CLC i - clear carry - 2, 1
    SetupOpCode(0x18, &CPU_6502::CLC, MN_CLC, 1);

    // 1E: ASL a,x - shift absolute memory offset by x value to the left one bit  - 7, 3
    SetupOpCode(0x1E, &CPU_6502::ASL_a_x, MN_ASL_ABS_X, 3);
   
    // 19: ORA a,y - Inclusive OR between a and a value stored in memory offset by y - 4, 3
    SetupOpCode(0x19, &CPU_6502::ORA_a_y, MN_ORA_ABS_Y, 3);

    // 1D: ORA a,x - Inclusive OR between a and a value stored in memory offset by x - 4, 3
    SetupOpCode(0x1D, &CPU_6502::ORA_a_x, MN_ORA_ABS_X, 3);

    // 21: AND (zp, x) - Perform an AND between a and an indexed indirect memory location - 6, 2
    SetupOpCode(0x21, &CPU_6502::AND_zp_x_ind, MN_AND_ZP_X_IND, 2);

    // 20: JSR a - pushes PC - 1 then jumps to absolute address - 6, 3
    SetupOpCode(0x20, &CPU_6502::JSR, MN_JSR_ABS, 3);

    // 24: BIT zp - Perform a bit test with a value in zp memory - 3, 2
    SetupOpCode(0x24, &CPU_6502::BIT_zp, MN_BIT_ZP, 2);

    // 25: AND zp - read memory from zp and perform bitwise AND with accumulator - 3, 2
    SetupOpCode(0x25, &CPU_6502::AND_zp, MN_AND_ZP, 2);

    // 26: ROL zp - rotate value in zero page memory one bit to the left - 5, 2
    SetupOpCode(0x26, &CPU_6502::ROL_zp, MN_ROL_ZP, 2);

    // 28: PLP i - pull processor status flags from stack - 4, 1
    SetupOpCode(0x28, &CPU_6502::PLP, MN_PLP, 1);

    // 29: bitwise AND with accumulator - 2, 2
    SetupOpCode(0x29, &CPU_6502::AND_imm, MN_AND_IMM, 2);

    // 2A: ROL A - rotate accumulator one bit to the left - 2, 1
    SetupOpCode(0x2A, &CPU_6502::ROL_A, MN_ROL_A, 1);

    // 2C: BIT a - Perform a bit test with a value in abs memory - 4, 3
    SetupOpCode(0x2C, &CPU_6502::BIT_a, MN_BIT_ABS, 3);

    // 2D: AND a - read memory from absolute address and perform bitwise AND with accumulator - 4, 3
    SetupOpCode(0x2D, &CPU_6502::AND_a, MN_AND_ABS, 3);

    // 2E: ROL a - rotate value in memory one bit to the left - 6, 3
    SetupOpCode(0x2E, &CPU_6502::ROL_a, MN_ROL_ABS, 3);

    // 30: BMI r - branch relative if negative flag is set - 2, 2
    SetupOpCode(0x30, &CPU_6502::BMI_r, MN_BMI, 2);

    // 31: AND (zp),y - Perform logical AND operation between a and an indirectly indexed value in memory - 5, 2
    SetupOpCode(0x31, &CPU_6502::AND_zp_ind_y, MN_AND_ZP_IND_Y, 2);

    // 35: AND zp, x - read memory from zp + x and perform bitwise AND with accumulator - 4, 2
    SetupOpCode(0x35, &CPU_6502::AND_zp_x, MN_AND_ZP_X, 2);

    // 36: ROL zp,x - rotate value in zero page memory offset by x one bit to the left - 6, 2
    SetupOpCode(0x36, &CPU_6502::ROL_zp_x, MN_ROL_ZP_X, 2);

    // 38: set carry - 2, 1
    SetupOpCode(0x38, &CPU_6502::SEC, MN_SEC, 1);

    // 39: AND a,x - read memory from absolute address offset by y and perform bitwise AND with accumulator - 4, 3
    SetupOpCode(0x39, &CPU_6502::AND_a_y, MN_AND_ABS_Y, 3);

    // 3D: AND a,x - read memory from absolute address offset by x and perform bitwise AND with accumulator - 4, 3
    SetupOpCode(0x3D, &CPU_6502::AND_a_x, MN_AND_ABS_X, 3);

    // 3E: ROL a,x - rotate value in absolute memory offset by x one bit to the left - 7, 3
    SetupOpCode(0x3E, &CPU_6502::ROL_a_x, MN_ROL_ABS_X, 3);

    // 40: RTI - return from interrupt - 6, 1
    SetupOpCode(0x40, &CPU_6502::RTI, MN_RTI, 1);

    // 41: EOR (zp,x) - Perform EOR with a value from a zp indexed indirect address - 6, 2
    SetupOpCode(0x41, &CPU_6502::EOR_zp_x_ind, MN_EOR_ZP_X_IND, 2);

    // 45: EOR zp - Perform EOR between a and a value stored in zp memory, and store result in a - 3, 2
    SetupOpCode(0x45, &CPU_6502::EOR_zp, MN_EOR_ZP, 2);

    // 46: LSR zp - read a byte from zp, shift it one bit to the right and put it back - 5, 2
    SetupOpCode(0x46, &CPU_6502::LSR_zp, MN_LSR_ZP, 2);
    
    // 48: PHA s - push a to stack - 3, 1
    SetupOpCode(0x48, &CPU_6502::PHA, MN_PHA, 1);

    // 49: EOR # - Perform EOR between a and an immediate value - 2, 2
    SetupOpCode(0x49, &CPU_6502::EOR_imm, MN_EOR_IMM, 2);

    // 4A: LSR - shift a one bit to the right. Set carry with old bit 0 value. - 2, 1
    SetupOpCode(0x4A, &CPU_6502::LSR, MN_LSR, 1);

    // 4C: JMP a (jump to absolute address) - 3, 3
    SetupOpCode(0x4C, &CPU_6502::JMP_a, MN_JMP_ABS, 3);

    // 4D: EOR a - Perform an EOR between a and a value in absolute memory - 4, 3
    SetupOpCode(0x4D, &CPU_6502::EOR_a, MN_EOR_ABS, 3);

    // 4E: LSR a - read a byte from abs memory, shift it one bit to the right and put it back - 6, 3
    SetupOpCode(0x4E, &CPU_6502::LSR_a, MN_LSR_ABS, 3);

    // 50: BVC r - branch relative if overflow flag is clear - 2, 2
    SetupOpCode(0x50, &CPU_6502::BVC_r, MN_BVC, 2);

    // 51: EOR (zp),y - Perform an EOR between a and  - 5, 2
    SetupOpCode(0x51, &CPU_6502::EOR_zp_ind_y, MN_EOR_ZP_IND_Y, 2);

    // 55: EOR zp,x - Perform an EOR between a and a value in zp memory offset by x - 4, 2
    SetupOpCode(0x55, &CPU_6502::EOR_zp_x, MN_EOR_ZP_X, 2);

    // 56: LSR zp,x - read a byte from zp offset by x, shift it one bit to the right and put it back - 6, 2
    SetupOpCode(0x56, &CPU_6502::LSR_zp_x, MN_LSR_ZP_X, 2);
    
    // 58: CLI i - clear interrupt disable flag - 2, 1
    SetupOpCode(0x58, &CPU_6502::CLI, MN_CLI, 1);

    // 59: EOR a,y - Perform an EOR between a and a value in absolute memory offset by y - 4, 3
    SetupOpCode(0x59, &CPU_6502::EOR_a_y, MN_EOR_ABS_Y, 3);

    // 5D: EOR a,x - Perform an EOR between a and a value in absolute memory offset by x - 4, 3
    SetupOpCode(0x5D, &CPU_6502::EOR_a_x, MN_EOR_ABS_X, 3);

    // 5E: LSR a,x - read a byte from memory offset by x, shift it one bit to the right and put it back - 7, 3
    SetupOpCode(0x5E, &CPU_6502::LSR_a_x, MN_LSR_ABS_X, 3);

    // 60: RTS pop an adress of the stack, add one, and jump there - 6, 1
    SetupOpCode(0x60, &CPU_6502::RTS, MN_RTS, 1);

    // 61: ADC (zp,x) - Perform an add with carry between a and a value from a zp indexed indirect address - 6, 2
    SetupOpCode(0x61, &CPU_6502::ADC_zp_x_ind, MN_ADC_ZP_X_IND, 2);

    // 65: ADC zp (add memory in zp to accumulator) - 3, 2
    SetupOpCode(0x65, &CPU_6502::ADC_zp, MN_ADC_ZP, 2);

    // 66: ROR zp - Move value stored in zp one bit to the right - 5, 2
    SetupOpCode(0x66, &CPU_6502::ROR_zp, MN_ROR_ZP, 2);

    // 68: PLA s - pull off of stack and into a - 4, 1
    SetupOpCode(0x68, &CPU_6502::PLA, MN_PLA, 1);

    // 69: ADC # - add immediate - 2, 2
    SetupOpCode(0x69, &CPU_6502::ADC_imm, MN_ADC_IMM, 2);

    // 6A: ROR A - Rotate accumulator one bit to the right - 2, 1
    SetupOpCode(0x6A, &CPU_6502::ROR_A, MN_ROR_A, 1);

    // 6C: JMP (a) (jump to the address contained at address a) - 6, 3
    SetupOpCode(0x6C, &CPU_6502::JMP_ind, MN_JMP_IND, 3);

    // 6D: ADC a - add value in absolute memory to accumulator - 4, 3
    SetupOpCode(0x6D, &CPU_6502::ADC_a, MN_ADC_ABS, 3);

    // 6E: ROR a - Move value stored in absolute memory one bit to the right - 6, 3
    SetupOpCode(0x6E, &CPU_6502::ROR_a, MN_ROR_a, 3);

    // 70: BVS r - Branch relative if overflow flag is set - 2, 2
    SetupOpCode(0x70, &CPU_6502::BVS_r, MN_BVS, 2);
    
    // 71: ADC (zp),y - Perform an add with carry between a and an indirectly indexed value in memory - 5, 2 
    SetupOpCode(0x71, &CPU_6502::ADC_zp_ind_y, MN_ADC_ZP_IND_Y, 2);

    // 75: ADC zp,x - Perform an add with carry between a and a memory value in zero page offset by x - 4, 2
    SetupOpCode(0x75, &CPU_6502::ADC_zp_x, MN_ADC_ZP_X, 2);

    // 76: ROR zp,x - Move value stored in zero page of memory offset by x one bit to the right - 5, 2
    SetupOpCode(0x76, &CPU_6502::ROR_zp_x, MN_ROR_ZP_X, 2);

    // 78: SEI i - set interrupt disable flag - 2, 1
    SetupOpCode(0x78, &CPU_6502::SEI, MN_SEI, 1);

    // 79: ADC a,y - Perform an add with carry between a and a value in absolute memory offset by y - 4, 3
    SetupOpCode(0x79, &CPU_6502::ADC_a_y, MN_ADC_ABS_Y, 3);

    // 7D: ADC a,x - Perform an add with carry between a and a value at an absolute address offset by x - 4, 3
    SetupOpCode(0x7D, &CPU_6502::ADC_a_x, MN_ADC_ABS_X, 3);

    // 7E: ROR a,x - Move value stored in absolute memory offset by x one bit to the right - 7, 3
    SetupOpCode(0x7E, &CPU_6502::ROR_a_x, MN_ROR_ABS_X, 3);

    // 81: STA (zp, x) - store a into a zp indexed indirect address - 6, 2
    SetupOpCode(0x81, &CPU_6502::STA_zp_x_ind, MN_STA_ZP_X_IND, 2);

    // 84: STY zp - store y to a value in zp memory - 3, 2
    SetupOpCode(0x84, &CPU_6502::STY_zp, MN_STY_ZP, 2);

    // 85: STA zp (store a to zero page address) - 3, 2
    SetupOpCode(0x85, &CPU_6502::STA_zp, MN_STA_ZP, 2);

    // 86: STX zp - store x in a zp memory location - 3, 2
    SetupOpCode(0x86, &CPU_6502::STX_zp, MN_STX_ZP, 2);

    // 88: DEY i = decrement y register - 2, 1
    SetupOpCode(0x88, &CPU_6502::DEY, MN_DEY, 1);

    // 8A: TXA - transfer x to a - 2, 1
    SetupOpCode(0x8A, &CPU_6502::TXA, MN_TXA, 1);

    // 8C: STY a (store y to absolute memory address) - 4, 3
    SetupOpCode(0x8C, &CPU_6502::STY_a, MN_STY_ABS, 3);

    // 8D: STA a (store a to absolute memory address)
    SetupOpCode(0x8D, &CPU_6502::STA_a, STA_ABS, 3);

    // 8E: STX a (store x to absolute memory address) - 4, 3
    SetupOpCode(0x8E, &CPU_6502::STX_a, MN_STX_ABS, 3);

    // 90: BCC r - branch relative if carry flag is clear - 2, 2
    SetupOpCode(0x90, &CPU_6502::BCC_r, MN_BCC, 2);

    // 91: STA(zp), y - store a to indirectly indexed memory - 6, 2
    SetupOpCode(0x91, &CPU_6502::STA_zp_ind_y, MN_STA_ZP_IND_Y, 2);

    // 94: STY zp,x - store y to a value in zp memory offset by x - 4, 2
    SetupOpCode(0x94, &CPU_6502::STY_zp_x, MN_STY_ZP_X, 2);

    // 95: STA zp,x (store a to zp memory offset by x) - 4, 2
    SetupOpCode(0x95, &CPU_6502::STA_zp_x, MN_STA_ZP_X, 2);

    // 96: STX zp,y - store x in a zp address offset by y - 4, 2
    SetupOpCode(0x96, &CPU_6502::STX_zp_y, MN_STX_ZP_Y, 2);

    // 98: TYA i - transfer y to a - 2, 1
    SetupOpCode(0x98, &CPU_6502::TYA, MN_TYA, 1);

    // 99: STA a,y - Store a to absolute address + y offset - 5, 3
    SetupOpCode(0x99, &CPU_6502::STA_a_y, MN_STA_ABS_Y, 3);

    // 9A: TXS - transfer x to SP register - 2, 1
    SetupOpCode(0x9A, &CPU_6502::TXS, MN_TXS, 1);

    // 9D: STA a,x - store a to an address offset by x - 5, 3
    SetupOpCode(0x9D, &CPU_6502::STA_a_x, MN_STA_ABS_X, 3);

    // A0: LDY # (load immediate value to Y) - 2, 2
    SetupOpCode(0xA0, &CPU_6502::LDY_imm, MN_LDY_IMM, 2);

    // A1: LDA (zp,x) - add zp address to x register and
    // load into a the memory at the resulting address - 6, 2
    SetupOpCode(0xA1, &CPU_6502::LDA_zp_x_ind, MN_LDA_ZP_X_IND, 2);

    // A2: LDX # (Load immediate into X) - 2, 2
    SetupOpCode(0xA2, &CPU_6502::LDX_imm, MN_LDX_IMM, 2);

    // A4: LDY zp - load y with memory from zero page - 3, 2
    SetupOpCode(0xA4, &CPU_6502::LDY_zp, MN_LDY_ZP, 2);

    // A5: LDA zp - load a with memory from zp - 3, 2
    SetupOpCode(0xA5, &CPU_6502::LDA_zp, MN_LDA_ZP, 2);

    // A9: LDA # - 2,2
    SetupOpCode(0xA9, &CPU_6502::LDA_imm, LDA_IMM, 2);

    // A6: LDX zp - Load x with memory from zero page - 3, 2
    SetupOpCode(0xA6, &CPU_6502::LDX_zp, MN_LDX_ZP, 2);

    // A8: TAY i = transfer a to y - 2, 1
    SetupOpCode(0xA8, &CPU_6502::TAY, MN_TAY, 1);

    // AA: TAX - 2, 1
    SetupOpCode(0xAA, &CPU_6502::TAX, MN_TAX, 1);
    
    // AC: LDY a - Load y with a value from absolute memory - 4, 3
    SetupOpCode(0xAC, &CPU_6502::LDY_a, MN_LDY_ABS, 3);

    // AD: LDA a - Load a with absolute memory address - LDA 4, 3
    SetupOpCode(0xAD, &CPU_6502::LDA_a, MN_LDA_ABS, 3);

    // AE: LDX a - load x with value from absolute memory address - 4, 3
    SetupOpCode(0xAE, &CPU_6502::LDX_a, MN_LDX_ABS, 3);

    // B0: BCS r - branch relative if carry set - 2, 2
    SetupOpCode(0xB0, &CPU_6502::BCS_r, MN_BCS, 2);

    // B1: LDA (zp), y - (Read 2 bytes starting at a zero-page address,
    // add y, and load the memory stored at the resulting address into a) - 5, 2
    SetupOpCode(0xB1, &CPU_6502::LDA_zp_y_ind, MN_LDA_ZP_IND_Y, 2);

    // B4: load y with memory from zero page offset by x - 4, 2
    SetupOpCode(0xB4, &CPU_6502::LDY_zp_x, MN_LDY_ZP_X, 2);

    // B5: LDA zp,x - Load a with memory at zp absolute address offset by x LDA zp,x - 4, 2
    SetupOpCode(0xB5, &CPU_6502::LDA_zp_x, MN_LDA_ZP_X, 2);

    // B6: LDX zp, y - Load x with memory in zero page offset by y - 4, 2
    SetupOpCode(0xB6, &CPU_6502::LDX_zp_y, MN_LDX_ZP_Y, 2);

    // B8: CLV i - Clear overflow flag - 2, 1
    SetupOpCode(0xB8, &CPU_6502::CLV, MN_CLV, 1);

    // B9: LDA a,y - Load a with memory at absolute address offset by y - 4, 3
    SetupOpCode(0xB9, &CPU_6502::LDA_a_y, MN_LDA_ABS_Y, 3);

    // BA: TSX i - copy stack pointer to x - 2, 1
    SetupOpCode(0xBA, &CPU_6502::TSX, MN_TSX, 1);

    // BC: LDY a,x - Load y with a value from absolute memory offset by x - 4, 3
    SetupOpCode(0xBC, &CPU_6502::LDY_a_x, MN_LDY_ABS_X, 3);

    // BD: LDA a,x - load a with memory at absolute address offset by x - 4, 3
    SetupOpCode(0xBD, &CPU_6502::LDA_a_x, MN_LDA_ABS_X, 3);
    
    // BE: LDX a,y - load x with value at absolute memory address offset by y - 4, 3
    SetupOpCode(0xBE, &CPU_6502::LDX_a_y, MN_LDX_ABS_Y, 3);

    // C0: CPY # - compare y with immediate value - 2, 2
    SetupOpCode(0xC0, &CPU_6502::CPY_imm, MN_CPY_IMM, 2);

    // C1: CMP (zp,x) - Compare a with a zp indexed indirect value - 6, 2
    SetupOpCode(0xC1, &CPU_6502::CMP_zp_x_ind, MN_CMP_ZP_X, 2);

    // C4: CPY zp - compare y with value stored in zero page - 3, 2
    SetupOpCode(0xC4, &CPU_6502::CPY_zp, MN_CPY_ZP, 2);

    // C5: CMP zp - compare accumulator with memory stored in zero page - 3, 2
    SetupOpCode(0xC5, &CPU_6502::CMP_zp, MN_CMP_ZP, 2);

    // C6: DEC zp - decrement a value in zp memory - 5, 2
    SetupOpCode(0xC6, &CPU_6502::DEC_zp, MN_DEC_ZP, 2);

    // C8: INY i - increment y - 2, 1
    SetupOpCode(0xC8, &CPU_6502::INY, MN_INY, 1);

    // C9: CMP # (compare a to immediate value) - 2, 2
    SetupOpCode(0xC9, &CPU_6502::CMP_imm, MN_CMP_IMM, 2);

    // CA: decrement x - 2, 1
    SetupOpCode(0xCA, &CPU_6502::DEX, MN_DEX, 1);

    // CC: CPY a - compare y with value stored in absolute memory - 4, 3
    SetupOpCode(0xCC, &CPU_6502::CPY_a, MN_CPY_ABS, 3);

    // CD: CMP a - Compare accumulator with memory at absolute address - 4, 3
    SetupOpCode(0xCD, &CPU_6502::CMP_a, MN_CMP_ABS, 3);
    
    // CE: DEC a - decrement a value in memory - 6, 3
    SetupOpCode(0xCE, &CPU_6502::DEC_a, MN_DEC_ABS, 3);

    // D0 - BRNE r (Branch relative if z flag is cleared) - 2, 2
    SetupOpCode(0xD0, &CPU_6502::BRNE_r, MN_BRNE_r, 2);

    // D1: CMP (zp), y - Compare a with and an indirectly indexed value in memory - 5, 2
    SetupOpCode(0xD1, &CPU_6502::CMP_zp_ind_y, MN_CMP_ZP_IND_Y, 2);

    // D5: CMP zp,x - compare accumulator with memory stored in zero page offset by x - 4, 2
    SetupOpCode(0xD5, &CPU_6502::CMP_zp_x, MN_CMP_ZP_X, 2);

    // D6: DEC zp, x - decrement a value in zp memory offset by x - 6, 2
    SetupOpCode(0xD6, &CPU_6502::DEC_zp_x, MN_DEC_ZP_X, 2);

    // D8 - CLD i - clear decimal flag - 2, 1
    SetupOpCode(0xD8, &CPU_6502::CLD, MN_CLD, 1);

    // D9: CMP a, 9 - Compare accumulator with memory at absolute address offset by y - 4, 3
    SetupOpCode(0xD9, &CPU_6502::CMP_a_y, MN_CMP_ABS_Y, 3);

    // DD: CMP a, x - Compare accumulator with memory at absolute address offset by x - 4, 3
    SetupOpCode(0xDD, &CPU_6502::CMP_a_x, MN_CMP_ABS_X, 3);

    // DE: DEC a, x - decrement a value in abs memory offset by x - 7, 3
    SetupOpCode(0xDE, &CPU_6502::DEC_a_x, MN_DEC_ABS_X, 3);

    // E0 - compare x with immediate value - 2, 2
    SetupOpCode(0xE0, &CPU_6502::CPX_imm, MN_CPX_IMM, 2);

    // E1: SBC (zp,x) - Perform a subtract with carry from an indexed indirect value - 6, 2
    SetupOpCode(0xE1, &CPU_6502::SBC_zp_x_ind, MN_SBC_ZP_X_IND, 2);

    // E4: CPX zp - compare x with memory value stored in zero page - 3, 2
    SetupOpCode(0xE4, &CPU_6502::CPX_zp, MN_CPX_ZP, 2);

    // E5: SBC zp - subtract from a a value stored in the zero page - 3, 2
    SetupOpCode(0xE5, &CPU_6502::SBC_zp, MN_SBC_ZP, 2);

    // E6: INC zp - increment a value in zp memory - 5, 2
    SetupOpCode(0xE6, &CPU_6502::INC_zp, MN_INC_ZP, 2);

    // E8: INX - Inc X - 2, 1
    SetupOpCode(0xE8, &CPU_6502::INX, MN_INX, 1);

    // E9: SBC # - subtract immediate from a - 2, 2
    SetupOpCode(0xE9, &CPU_6502::SBC_imm, MN_SBC_IMM, 2);

    // EA: NOP - 2, 1
    SetupOpCode(0xEA, &CPU_6502::NOP, MN_NOP, 1);

    // EC: CPX a - compare x with memory value - 4, 3
    SetupOpCode(0xEC, &CPU_6502::CPX_a, MN_CPX_ABS, 3);

    // ED: SBC a - subtract value at absolute addr from a and store result in a - 4, 3
    SetupOpCode(0xED, &CPU_6502::SBC_a, MN_SBC_ABS, 3);

    // EE: INC a - increment a value in abs memory - 6, 3
    SetupOpCode(0xEE, &CPU_6502::INC_a, MN_INC_ABS, 3);

    // F0: BEQ r - branch if equal - 2, 2
    SetupOpCode(0xF0, &CPU_6502::BEQ_r, MN_BEQ, 2);

    // F1: SBC (zp), y - Perform a subtraction with carry between a and an indirectly indexed value in memory - 5, 2
    SetupOpCode(0xF1, &CPU_6502::SBC_zp_ind_y, MN_SBC_ZP_IND_Y, 2);

    // F5: SBC zp, x - Subtract value at zp offset by x from a and store result in a - 4, 2
    SetupOpCode(0xF5, &CPU_6502::SBC_zp_x, MN_SBC_ZP_X, 2);

    // F6: INC zp,x - increment a value in zp memory offset by x - 6, 2
    SetupOpCode(0xF6, &CPU_6502::INC_zp_x, MN_INC_ZP_X, 2);

    // F8: SED i - set decimal flag - 2, 1
    SetupOpCode(0xF8, &CPU_6502::SED, MN_SED, 1);

    // F9: SBC a,y - subtract value in abs memory address offset by y from a and store result in a - 4, 3
    SetupOpCode(0xF9, &CPU_6502::SBC_a_y, MN_SBC_ABS_Y, 3);

    // FD: SBC a,x - subtract value in abs memory address offset by x from a and store result in a - 4, 3
    SetupOpCode(0xFD, &CPU_6502::SBC_a_x, MN_SBC_ABS_X, 3);
    
    // FE: INC a,x - increment a value in memory offset by x - 7, 3
    SetupOpCode(0xFE, &CPU_6502::INC_a_x, MN_INC_ABS_X, 3);
}

void CPU_6502::SetupOpCode(uint8_t op, opcodeFuncPtr ptr, const char *mnemonic, uint8_t bytes)
{
    opcodes[op] = ptr;
    mnemonics[op] = mnemonic;
    opcodeBytes[op] = bytes;

    ++opsHandled;
}
