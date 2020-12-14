#ifndef CPU6502H
#define CPU6502H

#include <string.h>

// Операнды
enum EnumOperands { ___ = 0,
    NDX, // (b8,X)
    ZP,  // b8
    IMM, // #b8
    ABS, // b16
    NDY, // (b8),Y
    ZPX, // b8,X
    ABY, // b16,Y
    ABX, // b16,X
    REL, // b8 (адрес)
    ACC, // A
    IMP, // -- нет --
    ZPY, // b8,Y
    IND  // (b16)
};

// Инструкциия
enum EnumInstruction {

    // Базовые
    BRK = 1,  ORA, AND, EOR, ADC, STA, LDA,
    CMP, SBC, BPL, BMI, BVC, BVS, BCC, BCS,
    BNE, BEQ, JSR, RTI, RTS, LDY, CPY, CPX,
    ASL, PHP, CLC, BIT, ROL, PLP, SEC, LSR,
    PHA, PLA, JMP, CLI, ROR, SEI, STY, STX,
    DEY, TXA, TYA, TXS, LDX, TAY, TAX, CLV,
    TSX, DEC, INY, DEX, CLD, INC, INX, NOP,

    // Расширенные
    SED, AAC, SLO, RLA, RRA, SRE, DCP,
    ISC, LAX, AAX, ASR, ARR, ATX, AXS, XAA,
    AXA, SYA, SXA, DOP
};

// Имена инструкции
static int opcode_names[ 256 ] = {

    /*        00  01   02   03   04   05   06   07   08   09   0A   0B   0C   0D   0E   0F */
    /* 00 */ BRK, ORA, ___, SLO, DOP, ORA, ASL, SLO, PHP, ORA, ASL, AAC, DOP, ORA, ASL, SLO,
    /* 10 */ BPL, ORA, ___, SLO, DOP, ORA, ASL, SLO, CLC, ORA, NOP, SLO, DOP, ORA, ASL, SLO,
    /* 20 */ JSR, AND, ___, RLA, BIT, AND, ROL, RLA, PLP, AND, ROL, AAC, BIT, AND, ROL, RLA,
    /* 30 */ BMI, AND, ___, RLA, DOP, AND, ROL, RLA, SEC, AND, NOP, RLA, DOP, AND, ROL, RLA,
    /* 40 */ RTI, EOR, ___, SRE, DOP, EOR, LSR, SRE, PHA, EOR, LSR, ASR, JMP, EOR, LSR, SRE,
    /* 50 */ BVC, EOR, ___, SRE, DOP, EOR, LSR, SRE, CLI, EOR, NOP, SRE, DOP, EOR, LSR, SRE,
    /* 60 */ RTS, ADC, ___, RRA, DOP, ADC, ROR, RRA, PLA, ADC, ROR, ARR, JMP, ADC, ROR, RRA,
    /* 70 */ BVS, ADC, ___, RRA, DOP, ADC, ROR, RRA, SEI, ADC, NOP, RRA, DOP, ADC, ROR, RRA,
    /* 80 */ DOP, STA, DOP, AAX, STY, STA, STX, AAX, DEY, DOP, TXA, XAA, STY, STA, STX, AAX,
    /* 90 */ BCC, STA, ___, AXA, STY, STA, STX, AAX, TYA, STA, TXS, AAX, SYA, STA, SXA, AAX,
    /* A0 */ LDY, LDA, LDX, LAX, LDY, LDA, LDX, LAX, TAY, LDA, TAX, ATX, LDY, LDA, LDX, LAX,
    /* B0 */ BCS, LDA, ___, LAX, LDY, LDA, LDX, LAX, CLV, LDA, TSX, LAX, LDY, LDA, LDX, LAX,
    /* C0 */ CPY, CMP, DOP, DCP, CPY, CMP, DEC, DCP, INY, CMP, DEX, AXS, CPY, CMP, DEC, DCP,
    /* D0 */ BNE, CMP, ___, DCP, DOP, CMP, DEC, DCP, CLD, CMP, NOP, DCP, DOP, CMP, DEC, DCP,
    /* E0 */ CPX, SBC, DOP, ISC, CPX, SBC, INC, ISC, INX, SBC, NOP, SBC, CPX, SBC, INC, ISC,
    /* F0 */ BEQ, SBC, ___, ISC, DOP, SBC, INC, ISC, SED, SBC, NOP, ISC, DOP, SBC, INC, ISC
};

// Типы операндов для каждого опкода
static unsigned char operand_types[256] = {

    /*       00   01   02   03   04   05   06   07   08   09   0A   0B   0C   0D   0E   0F */
    /* 00 */ IMP, NDX, ___, NDX, ZP , ZP , ZP , ZP , IMP, IMM, ACC, IMM, ABS, ABS, ABS, ABS,
    /* 10 */ REL, NDY, ___, NDY, ZPX, ZPX, ZPX, ZPX, IMP, ABY, IMP, ABY, ABX, ABX, ABX, ABX,
    /* 20 */ ABS, NDX, ___, NDX, ZP , ZP , ZP , ZP , IMP, IMM, ACC, IMM, ABS, ABS, ABS, ABS,
    /* 30 */ REL, NDY, ___, NDY, ZPX, ZPX, ZPX, ZPX, IMP, ABY, IMP, ABY, ABX, ABX, ABX, ABX,
    /* 40 */ IMP, NDX, ___, NDX, ZP , ZP , ZP , ZP , IMP, IMM, ACC, IMM, ABS, ABS, ABS, ABS,
    /* 50 */ REL, NDY, ___, NDY, ZPX, ZPX, ZPX, ZPX, IMP, ABY, IMP, ABY, ABX, ABX, ABX, ABX,
    /* 60 */ IMP, NDX, ___, NDX, ZP , ZP , ZP , ZP , IMP, IMM, ACC, IMM, IND, ABS, ABS, ABS,
    /* 70 */ REL, NDY, ___, NDY, ZPX, ZPX, ZPX, ZPX, IMP, ABY, IMP, ABY, ABX, ABX, ABX, ABX,
    /* 80 */ IMM, NDX, IMM, NDX, ZP , ZP , ZP , ZP , IMP, IMM, IMP, IMM, ABS, ABS, ABS, ABS,
    /* 90 */ REL, NDY, ___, NDY, ZPX, ZPX, ZPY, ZPY, IMP, ABY, IMP, ABY, ABX, ABX, ABY, ABX,
    /* A0 */ IMM, NDX, IMM, NDX, ZP , ZP , ZP , ZP , IMP, IMM, IMP, IMM, ABS, ABS, ABS, ABS,
    /* B0 */ REL, NDY, ___, NDY, ZPX, ZPX, ZPY, ZPY, IMP, ABY, IMP, ABY, ABX, ABX, ABY, ABY,
    /* C0 */ IMM, NDX, IMM, NDX, ZP , ZP , ZP , ZP , IMP, IMM, IMP, IMM, ABS, ABS, ABS, ABS,
    /* D0 */ REL, NDY, ___, NDY, ZPX, ZPX, ZPX, ZPX, IMP, ABY, IMP, ABY, ABX, ABX, ABX, ABX,
    /* E0 */ IMM, NDX, IMM, NDX, ZP , ZP , ZP , ZP , IMP, IMM, IMP, IMM, ABS, ABS, ABS, ABS,
    /* F0 */ REL, NDY, ___, NDY, ZPX, ZPX, ZPX, ZPX, IMP, ABY, IMP, ABY, ABX, ABX, ABX, ABX
};

// Количество циклов на опкод
static int cycles_basic[256] = {

      7, 6, 2, 8, 3, 3, 5, 5, 3, 2, 2, 2, 4, 4, 6, 6,
      2, 5, 2, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7,
      6, 6, 2, 8, 3, 3, 5, 5, 4, 2, 2, 2, 4, 4, 6, 6,
      2, 5, 2, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7,
      6, 6, 2, 8, 3, 3, 5, 5, 3, 2, 2, 2, 3, 4, 6, 6,
      2, 5, 2, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7,
      6, 6, 2, 8, 3, 3, 5, 5, 4, 2, 2, 2, 5, 4, 6, 6,
      2, 5, 2, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7,
      2, 6, 2, 6, 3, 3, 3, 3, 2, 2, 2, 2, 4, 4, 4, 4,
      2, 6, 2, 6, 4, 4, 4, 4, 2, 5, 2, 5, 5, 5, 5, 5,
      2, 6, 2, 6, 3, 3, 3, 3, 2, 2, 2, 2, 4, 4, 4, 4,
      2, 5, 2, 5, 4, 4, 4, 4, 2, 4, 2, 4, 4, 4, 4, 4,
      2, 6, 2, 8, 3, 3, 5, 5, 2, 2, 2, 2, 4, 4, 6, 6,
      2, 5, 2, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7,
      2, 6, 3, 8, 3, 3, 5, 5, 2, 2, 2, 2, 4, 4, 6, 6,
      2, 5, 2, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7
};

static const char* operand_names_string[75] = {

    // Документированные
    "KIL",   //  0 Ошибочные инструкции
    "BRK",   //  1
    "ORA",   //  2
    "AND",   //  3
    "EOR",   //  4
    "ADC",   //  5
    "STA",   //  6
    "LDA",   //  7
    "CMP",   //  8
    "SBC",   //  9
    "BPL",   // 10
    "BMI",   // 11
    "BVC",   // 12
    "BVS",   // 13
    "BCC",   // 14
    "BCS",   // 15
    "BNE",   // 16
    "BEQ",   // 17
    "JSR",   // 18
    "RTI",   // 19
    "RTS",   // 20
    "LDY",   // 21
    "CPY",   // 22
    "CPX",   // 23
    "ASL",   // 24
    "PHP",   // 25
    "CLC",   // 26
    "BIT",   // 27
    "ROL",   // 28
    "PLP",   // 29
    "SEC",   // 30
    "LSR",   // 31
    "PHA",   // 32
    "PLA",   // 33
    "JMP",   // 34
    "CLI",   // 35
    "ROR",   // 36
    "SEI",   // 37
    "STY",   // 38
    "STX",   // 39
    "DEY",   // 40
    "TXA",   // 41
    "TYA",   // 42
    "TXS",   // 43
    "LDX",   // 44
    "TAY",   // 45
    "TAX",   // 46
    "CLV",   // 47
    "TSX",   // 48
    "DEC",   // 49
    "INY",   // 50
    "DEX",   // 51
    "CLD",   // 52
    "INC",   // 53
    "INX",   // 54
    "NOP",   // 55
    "SED",   // 56
    // Недокументированные
    "AAC",   // 57
    "SLO",   // 58
    "RLA",   // 59
    "RRA",   // 60
    "SRE",   // 61
    "DCP",   // 62
    "ISC",   // 63
    "LAX",   // 64
    "AAX",   // 65
    "ASR",   // 66
    "ARR",   // 67
    "ATX",   // 68
    "AXS",   // 69
    "XAA",   // 70
    "AXA",   // 71
    "SYA",   // 72
    "SXA",   // 73
    "DOP",   // 74
};

// ---------------------------------------------------------------------
class CPU  { // : public MEM
// ---------------------------------------------------------------------

protected:

    // Регистры процессора
    unsigned char  reg_A, reg_X, reg_Y, reg_S, reg_P;
    unsigned short reg_PC;

    char disasmrow[64];
    char ds_mnem[8];
    char ds_oper[32];
    int  cycles_ext;
    int  fatal;

public:

    CPU();

    virtual unsigned char  read_byte(int);
    virtual unsigned short read_word(int);
    virtual void           write_byte(int, unsigned char);

    int  get_effective(int);
    int  do_branch(int&, int);
    void do_brk();
    int  disassemble(int);
    int  step();

    // Макросы
    void SET_ZERO(int x)        { reg_P = x ? (reg_P & 0xFD) : (reg_P | 0x02); }
    void SET_OVERFLOW(int x)    { reg_P = x ? (reg_P | 0x40) : (reg_P & 0xBF); }
    void SET_CARRY(int x)       { reg_P = x ? (reg_P | 0x01) : (reg_P & 0xFE); }
    void SET_DECIMAL(int x)     { reg_P = x ? (reg_P | 0x08) : (reg_P & 0xF7); }
    void SET_BREAK(int x)       { reg_P = x ? (reg_P | 0x10) : (reg_P & 0xEF); }
    void SET_INTERRUPT(int x)   { reg_P = x ? (reg_P | 0x04) : (reg_P & 0xFB); }
    void SET_SIGN(int x)        { reg_P = (x & 0x80) ? (reg_P | 0x80) : (reg_P & 0x7F); };

    int  IF_CARRY()      { return (reg_P & 0x01 ? 1 : 0); }
    int  IF_ZERO()       { return (reg_P & 0x02 ? 1 : 0); }
    int  IF_INTERRUPT()  { return (reg_P & 0x04 ? 1 : 0); }
    int  IF_OVERFLOW()   { return (reg_P & 0x40 ? 1 : 0); }
    int  IF_SIGN()       { return (reg_P & 0x80 ? 1 : 0); }

    void PUSH(int x)     { write_byte(0x100 + reg_S, x & 0xff); reg_S = ((reg_S - 1) & 0xff); }
    unsigned char PULL() { reg_S = (reg_S + 1) & 0xff; return read_byte(0x100 + reg_S); }
};

#endif
