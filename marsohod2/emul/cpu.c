#include <stdlib.h>
#include <stdio.h>

#include "display.h"
#include "cpu.h"

#define INCRADDR            addr = (addr + 1) & 0xffff
#define SET_ZERO(x)         reg_P =  x         ? (reg_P & 0xFD) : (reg_P | 0x02)
#define SET_SIGN(x)         reg_P = (x & 0x80) ? (reg_P | 0x80) : (reg_P & 0x7F)
#define SET_OVERFLOW(x)     reg_P = x          ? (reg_P | 0x40) : (reg_P & 0xBF)
#define SET_CARRY(x)        reg_P = x          ? (reg_P | 0x01) : (reg_P & 0xFE)
#define SET_DECIMAL(x)      reg_P = x          ? (reg_P | 0x08) : (reg_P & 0xF7)
#define SET_BREAK(x)        reg_P = x          ? (reg_P | 0x10) : (reg_P & 0xEF)
#define SET_INTERRUPT(x)    reg_P = x          ? (reg_P | 0x04) : (reg_P & 0xFB)

#define IF_CARRY            (reg_P & 0x01 ? 1 : 0)
#define IF_ZERO             (reg_P & 0x02 ? 1 : 0)
#define IF_OVERFLOW         (reg_P & 0x40 ? 1 : 0)
#define IF_SIGN             (reg_P & 0x80 ? 1 : 0)

#define PUSH(x)             writeB(0x100 + reg_S, x & 0xff); reg_S = ((reg_S - 1) & 0xff)

// Типы операндов для каждого опкода
unsigned char operandTypes[256] = {

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

int operandNames[ 256 ] = {

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

const char* operandNamesString[70] = {

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

};

int cycles_basic[256] = {

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

void initCPU() {

    int i;

    reg_A  = 0x00;
    reg_X  = 0x00;
    reg_Y  = 0x00;
    reg_S  = 0x00;
    reg_P  = 0x00;
    reg_PC = 0xC000;

    deb_bottom  = -1;
    deb_top     = reg_PC;
    deb_addr    = reg_PC;
    dump_mode   = DUMP_ZP;
    zp_base     = 0;
    cpu_running = 0;
    firstWrite  = 1;
    redrawDump  = 1;

    /* Отражение экранных страниц. Зависит от картриджа */
    HMirroring  = 1; // 1
    VMirroring  = 0; // 0

    /* Джойстики */
    Joy1        = 0; Joy2        = 0;
    Joy1Strobe  = 0; Joy2Strobe  = 0;

    for (i = 0; i < 64; i++) {
        debAddr[i] = 0;
    }

    breakpointsMax = 0;
    for (i = 0; i < 256; i++) {
        breakpoints[i] = -1;
    }

    /* Подробная отладка */
    if (TRACER) {

        FILE* f = fopen("trace.log", "w+");
        fclose(f);
    }

    /* Точка прерывания */
    if (BREAKPOINT) {

        breakpointsMax = 1;
        breakpoints[0] = BREAKPOINT;
    }
}

// Чтение слова из памяти
unsigned int readW(int addr) {
    return sram[addr & 0xffff] + 256 * sram[(1 + addr) & 0xffff];
}

// Извлечение из стека
unsigned char PULL() {

    reg_S = (reg_S + 1) & 0xff;
    return readB(0x100 + reg_S);
}

/* Зеркала для видеопамяти */
int vmirror(int addr) {

    /* Зеркала палитры */
    if ((addr & 0x3F00) == 0x3F00) {
        return (addr & 0x1F) | 0x3F00;
    }

    /* Зеркала nametable */
    else if (addr > 0x2800) {
        return (addr & 0x7FF) | 0x2000;
    }

    return addr;
}

int rmirror(int addr) {

    if (addr > 0x0800 && addr < 0x2000) {
        return (addr & 0x7FF);
    }

    return addr & 0xffff;
}

// Чтение байта из памяти
unsigned char readB(int addr) {

    int tmp, olddat, mirr;

    // Джойстик 1
    if (addr == 0x4016) {

        tmp = (Joy1Latch & 1) | 0x40;
        Joy1Latch >>= 1;
        return tmp;
    }

    // Джойстик 2
    if (addr == 0x4017) {
        return 0;
    }

    if (addr >= 0x2000 && addr <= 0x3FFF) {

        switch (addr & 7) {

            case 2:

                /* Предыдущее значение */
                tmp = ppu_status;

                /* Сброс при чтении */
                ppu_status = ppu_status & 0b00111111;

                /* Сброс 2005, 2007 чтения/записи */
                firstWrite = 1;

                return tmp;

            /* Читать спрайт */
            case 4:

                tmp = spriteRam[ spraddr ];
                spraddr = (spraddr + 1) & 0xff;
                return tmp;

            /* Чтение из видеопамяти (кроме STA) */
            case 7:

                mirr = vmirror(VRAMAddress);

                // Читать из регистров палитры
                if (VRAMAddress >= 0x3F00) {

                    // Чтение из любой палитры с индексом & 3 = 0 читает из BG
                    if (mirr >= 0x3F00 && mirr < 0x3F20 && (mirr & 3) == 0) {
                        mirr = 0x3F00;
                    }

                    olddat = sram[ 0x10000 + mirr ];


                // Читать с задержкой буфера
                } else {

                    olddat = objvar;
                    objvar = sram[ 0x10000 + mirr ];
                }

                VRAMAddress += (ctrl0 & 0x04 ? 32 : 1);
                return olddat;
        }
    }

    return sram[ rmirror(addr) ];
}

// Запись байта в память
void writeB(int addr, unsigned char data) {

    int mirr;

    if (addr >= 0x8000) {
        return;
    }

    /* Обновление спрайтов */
    if (addr == 0x4014) {

        int i;
        int baseaddr = 0x100 * data;
        for (i = 0; i < 256; i++) {
            spriteRam[i] = sram[baseaddr + i];
        }
        return;
    }

    // Геймпад 1
    if (addr == 0x4016) {

        // Защелка: получение данных из Joy1
        if ((Joy1Strobe & 1) == 1 && ((data & 1) == 0)) {
            Joy1Latch = Joy1 | (1 << 19);
        }

        Joy1Strobe = data & 1;
        return;
    }

    // Геймпад 2: Пока не используется
    if (addr == 0x4017) {
        return;
    }

    if (addr >= 0x2000 && addr <= 0x3FFF) {

        switch (addr & 7) {

            case 0: ctrl0   = data; break;
            case 1: ctrl1   = data; break;
            case 3: spraddr = data; break;
            case 4: spriteRam[ spraddr ] = data; spraddr = (spraddr + 1) & 0xff; break;

            /* Скроллинг */
            case 5:

                /* Горизонтальный скроллинг */
                if (firstWrite) {

                  regFH =  data & 7;        // Точный скроллинг по X (Fine Horizontal)
                  regHT = (data >> 3) & 31; // Грубый скроллинг по X (Horizontal Tile)

                /* Вертикальный */
                } else {

                  regFV =  data & 7;        // Точный скроллинг по Y (Fine Vertical)
                  regVT = (data >> 3) & 31; // Грубый скроллинг по Y (Vertical Tile)
                }

                firstWrite = !firstWrite;
                break;

            case 6: // Запись адреса курсора в память

                if (firstWrite) {

                    regFV = (data >> 4) & 3;
                    regV  = (data >> 3) & 1;
                    regH  = (data >> 2) & 1;
                    regVT = (regVT & 7) | ((data & 3) << 3);

                } else {

                    regVT = (regVT & 24) | ((data >> 5) & 7);
                    regHT =  data & 31;

                    /* Готовые результаты */
                    cntHT = regHT;
                    cntVT = regVT;
                    cntFV = regFV;
                    cntV  = regV;
                    cntH  = regH;
                }

                firstWrite = !firstWrite;

                // FE DC BA 98765 43210
                // .. UU VH YYYYY XXXXX

                // Старшие биты
                b1  = (cntFV & 7) << 4; // Верхние 2 бита
                b1 |= ( cntV & 1) << 3; // Экран (2, 3)
                b1 |= ( cntH & 1) << 2; // Экран (0, 1)
                b1 |= (cntVT >> 3) & 3; // Y[4:3]

                // Младшие биты
                b2  = (cntVT & 7) << 5; // Y[2:0]
                b2 |=  cntHT & 31;      // X[4:0]

                VRAMAddress = ((b1 << 8) | b2) & 0x7fff;
                break;

            // Запись данных в видеопамять
            case 7:

                mirr = vmirror(VRAMAddress);

                /* Запись в 3F00 равнозначна 0x3F10 */
                if (mirr == 0x3F00 || mirr == 0x3F10) {
                    mirr = 0x3F00;
                }

                sram[ 0x10000 + mirr ] = data;
                VRAMAddress += (ctrl0 & 0x04 ? 32 : 1);
                break;
        }

    } else {
        sram[ rmirror(addr) ] = data;
    }
}

// По адресу, определить эффективный адрес (если он есть)
unsigned int getEffectiveAddress(int addr) {

    int opcode, iaddr;
    int tmp, rt, pt;

    opcode  = sram[ addr ];
    INCRADDR;

    switch (operandTypes[ opcode ]) {

        /* Indirect, X (b8,X) */
        case NDX:

        	// PEEK( PEEK( (arg + X) % 256) + PEEK((arg + X + 1) % 256) * 256
            tmp     = sram[ addr ];
            tmp     = (tmp + reg_X) & 0xff;
            return sram[ tmp ] + ((sram[(1 + tmp) & 0xff] << 8));

        /* Indirect, Y (b8),Y */
        case NDY:

            tmp = sram[ addr ];
            rt  = sram[ 0xff & tmp ];
            rt |= sram[ 0xff & (tmp + 1) ] << 8;
            pt  = rt;
            rt  = (rt + reg_Y) & 0xffff;
            if  ((pt & 0xff00) != (rt & 0xff00)) cycles_ext++;
            return rt;

        /* Zero Page */
        case ZP:  return sram[ addr ];

        /* Zero Page, X */
        case ZPX: return (sram[ addr ] + reg_X) & 0x00ff;

        /* Zero Page, Y */
        case ZPY: return (sram[ addr ] + reg_Y) & 0x00ff;

        /* Absolute */
        case ABS: return readW(addr);

        /* Absolute, X */
        case ABX:

            pt = readW(addr);
            rt = pt + reg_X;
            if  ((pt & 0xff00) != (rt & 0xff00)) cycles_ext++;
            return rt & 0xffff;

        /* Absolute, Y */
        case ABY:

            pt = readW(addr);
            rt = pt + reg_Y;
            if  ((pt & 0xff00) != (rt & 0xff00)) cycles_ext++;
            return rt & 0xffff;

        /* Indirect */
        case IND:

            addr  = readW(addr);
            iaddr = readB(addr) + 256*readB((addr & 0xFF00) + ((addr + 1) & 0x00FF));
            return iaddr;

        /* Relative */
        case REL:

            iaddr = sram[ addr ];
            return (iaddr + addr + 1 + (iaddr < 128 ? 0 : -256)) & 0xffff;
    }

    return -1;
}

// Исполнение инструкции
int exec() {

    int temp, optype, opname, ppurd = 1, src = 0;
    int addr = reg_PC, opcode;
    int cycles_per_instr = 2;

    /* Доп. циклы разбора адреса */
    cycles_ext = 0;

    // Определение эффективного адреса
    int iaddr = getEffectiveAddress(addr);

    opcode = readB(addr) & 0xff;
    optype = operandTypes[ opcode ];
    opname = operandNames[ opcode ];

    if (opname == STA || opname == STX || opname == STY) {
        ppurd = 0;
    }

    INCRADDR;

    /* Базовые циклы + доп. циклы */
    cycles_per_instr = cycles_basic[ opcode ] + cycles_ext;

    // Тип операнда
    switch (optype) {

        case ___: printf("Opcode %02x error at %04x\n", opcode, reg_PC); exit(0);
        case NDX: /* Indirect X (b8,X) */
        case NDY: /* Indirect, Y */
        case ZP:  /* Zero Page */
        case ZPX: /* Zero Page, X */
        case ZPY: /* Zero Page, Y */
        case REL: /* Relative */

            INCRADDR;
            if (ppurd) src = readB( iaddr );
            break;

        case ABS: /* Absolute */
        case ABX: /* Absolute, X */
        case ABY: /* Absolute, Y */
        case IND: /* Indirect */

            INCRADDR;
            INCRADDR;

            if (ppurd) src = readB( iaddr );
            break;

        case IMM: /* Immediate */

            if (ppurd) src = readB(addr);
            INCRADDR;
            break;

        case ACC: /* Accumulator source */

            src = reg_A;
            break;
    }

    /* Разбор инструкции и исполнение */
    switch (opname) {

        /* Сложение с учетом переноса */
        case ADC: {

            temp = src + reg_A + (reg_P & 1);
            SET_ZERO(temp & 0xff);
            SET_SIGN(temp);
            SET_OVERFLOW(((reg_A ^ src ^ 0x80) & 0x80) && ((reg_A ^ temp) & 0x80) );
            SET_CARRY(temp > 0xff);
            reg_A = temp & 0xff;
            break;
        }

        /* Логическое умножение */
        case AND: {

            src &= reg_A;
            SET_SIGN(src);
            SET_ZERO(src);
            reg_A = src;
            break;
        }

        /* Логический сдвиг вправо */
        case ASL: {

            SET_CARRY(src & 0x80);
            src <<= 1;
            src &= 0xff;
            SET_SIGN(src);
            SET_ZERO(src);

            if (optype == ACC) reg_A = src; else writeB(iaddr, src);
            break;
        }

        /* Переход если CF=0 */
        case BCC: if (!IF_CARRY) BRANCH; break;

        /* Переход если CF=1 */
        case BCS: if ( IF_CARRY) BRANCH; break;

        /* Переход если ZF=0 */
        case BNE: if (!IF_ZERO) BRANCH; break;

        /* Переход если ZF=1 */
        case BEQ: if ( IF_ZERO) BRANCH; break;

        /* Переход если NF=0 */
        case BPL: if (!IF_SIGN) BRANCH; break;

        /* Переход если NF=1 */
        case BMI: if ( IF_SIGN) BRANCH; break;

        /* Переход если NF=0 */
        case BVC: if (!IF_OVERFLOW) BRANCH; break;

        /* Переход если NF=1 */
        case BVS: if ( IF_OVERFLOW) BRANCH; break;

        /* Копированиь бит 6 в OVERFLOW флаг. */
        case BIT: {

            SET_SIGN(src);
            SET_OVERFLOW(0x40 & src);
            SET_ZERO(src & reg_A);
            break;
        }

        /* Программное прерывание */
        case BRK: {

            reg_PC = (reg_PC + 2) & 0xffff;
            PUSH((reg_PC >> 8) & 0xff);	     /* Вставка обратного адреса в стек */
            PUSH(reg_PC & 0xff);
            SET_BREAK(1);                    /* Установить BFlag перед вставкой */
            reg_P |= 0b00100000;             /* 1 */
            PUSH(reg_P);
            SET_INTERRUPT(1);
            addr = readW(0xFFFE);
            break;
        }

        /* Флаги */
        case CLC: SET_CARRY(0); break;
        case SEC: SET_CARRY(1); break;
        case CLD: SET_DECIMAL(0); break;
        case SED: SET_DECIMAL(1); break;
        case CLI: SET_INTERRUPT(0); break;
        case SEI: SET_INTERRUPT(1); break;
        case CLV: SET_OVERFLOW(0); break;

        /* Сравнение A, X, Y с операндом */
        case CMP:
        case CPX:
        case CPY: {

            src = (opname == CMP ? reg_A : (opname == CPX ? reg_X : reg_Y)) - src;
            SET_CARRY(src >= 0);
            SET_SIGN(src);
            SET_ZERO(src & 0xff);
            break;
        }

        /* Уменьшение операнда на единицу */
        case DEC: {

            src = (src - 1) & 0xff;
            SET_SIGN(src);
            SET_ZERO(src);
            writeB(iaddr, src);
            break;
        }

        /* Уменьшение X на единицу */
        case DEX: {

            reg_X = (reg_X - 1) & 0xff;
            SET_SIGN(reg_X);
            SET_ZERO(reg_X);
            break;
        }

        /* Уменьшение Y на единицу */
        case DEY: {

            reg_Y = (reg_Y - 1) & 0xff;
            SET_SIGN(reg_Y);
            SET_ZERO(reg_Y);
            break;
        }

        /* Исключающее ИЛИ */
        case EOR: {

            src ^= reg_A;
            SET_SIGN(src);
            SET_ZERO(src);
            reg_A = src;
            break;
        }

        /* Увеличение операнда на единицу */
        case INC: {

            src = (src + 1) & 0xff;
            SET_SIGN(src);
            SET_ZERO(src);
            writeB(iaddr, src);
            break;
        }

        /* Уменьшение X на единицу */
        case INX: {

            reg_X = (reg_X + 1) & 0xff;
            SET_SIGN(reg_X);
            SET_ZERO(reg_X);
            break;
        }

        /* Уменьшение Y на единицу */
        case INY: {

            reg_Y = (reg_Y + 1) & 0xff;
            SET_SIGN(reg_Y);
            SET_ZERO(reg_Y);
            break;
        }

        /* Переход по адресу */
        case JMP: addr = iaddr; break;

        /* Вызов подпрограммы */
        case JSR: {

            addr = (addr - 1) & 0xffff;
            PUSH((addr >> 8) & 0xff);	/* Вставка обратного адреса в стек (-1) */
            PUSH(addr & 0xff);
            addr = iaddr;
            break;
        }

        /* Загрузка операнда в аккумулятор */
        case LDA: {

            SET_SIGN(src);
            SET_ZERO(src);
            reg_A = (src);
            break;
        }

        /* Загрузка операнда в X */
        case LDX: {

            SET_SIGN(src);
            SET_ZERO(src);
            reg_X = (src);
            break;
        }

        /* Загрузка операнда в Y */
        case LDY: {

            SET_SIGN(src);
            SET_ZERO(src);
            reg_Y = (src);
            break;
        }

        /* Логический сдвиг вправо */
        case LSR: {

            SET_CARRY(src & 0x01);
            src >>= 1;
            SET_SIGN(src);
            SET_ZERO(src);
            if (optype == ACC) reg_A = src; else writeB(iaddr, src);
            break;
        }

        /* Логическое побитовое ИЛИ */
        case ORA: {

            src |= reg_A;
            SET_SIGN(src);
            SET_ZERO(src);
            reg_A = src;
            break;
        }

        /* Стек */
        case PHA: PUSH(reg_A); break;
        case PHP: PUSH((reg_P | 0x30)); break;
        case PLP: reg_P = PULL(); break;

        /* Извлечение из стека в A */
        case PLA: {

            src = PULL();
            SET_SIGN(src);
            SET_ZERO(src);
            reg_A = src;
            break;
        }

        /* Циклический сдвиг влево */
        case ROL: {

            src <<= 1;
            if (IF_CARRY) src |= 0x1;
            SET_CARRY(src > 0xff);
            src &= 0xff;
            SET_SIGN(src);
            SET_ZERO(src);
            if (optype == ACC) reg_A = src; else writeB(iaddr, src);
            break;
        }

        /* Циклический сдвиг вправо */
        case ROR: {

            if (IF_CARRY) src |= 0x100;
            SET_CARRY(src & 0x01);
            src >>= 1;
            SET_SIGN(src);
            SET_ZERO(src);
            if (optype == ACC) reg_A = src; else writeB(iaddr, src);
            break;
        }

        /* Возврат из прерывания */
        case RTI: {

            reg_P = PULL();
            src   = PULL();
            src  |= (PULL() << 8);
            addr  = src;
            break;
        }

        /* Возврат из подпрограммы */
        case RTS: {

            src  = PULL();
            src += ((PULL()) << 8) + 1;
            addr = (src);
            break;
        }

        /* Вычитание */
        case SBC: {

            temp = reg_A - src - (IF_CARRY ? 0 : 1);

            SET_SIGN(temp);
            SET_ZERO(temp & 0xff);
            SET_OVERFLOW(((reg_A ^ temp) & 0x80) && ((reg_A ^ src) & 0x80));
            SET_CARRY(temp >= 0);
            reg_A = (temp & 0xff);
            break;
        }

        /* Запись содержимого A,X,Y в память */
        case STA: writeB(iaddr, reg_A); break;
        case STX: writeB(iaddr, reg_X); break;
        case STY: writeB(iaddr, reg_Y); break;

        /* Пересылка содержимого аккумулятора в регистр X */
        case TAX: {

            src = reg_A;
            SET_SIGN(src);
            SET_ZERO(src);
            reg_X = (src);
            break;
        }

        /* Пересылка содержимого аккумулятора в регистр Y */
        case TAY: {

            src = reg_A;
            SET_SIGN(src);
            SET_ZERO(src);
            reg_Y = (src);
            break;
        }

        /* Пересылка содержимого S в регистр X */
        case TSX: {

            src = reg_S;
            SET_SIGN(src);
            SET_ZERO(src);
            reg_X = (src);
            break;
        }

        /* Пересылка содержимого X в регистр A */
        case TXA: {

            src = reg_X;
            SET_SIGN(src);
            SET_ZERO(src);
            reg_A = (src);
            break;
        }

        /* Пересылка содержимого X в регистр S */
        case TXS: reg_S = reg_X; break;

        /* Пересылка содержимого Y в регистр A */
        case TYA: {

            src = reg_Y;
            SET_SIGN(src);
            SET_ZERO(src);
            reg_A = (src);
            break;
        }

        /* Недокументированные инструкции */
        // -------------------------------------------------------------

        case SLO: {

            /* ASL */
            SET_CARRY(src & 0x80);
            src <<= 1;
            src &= 0xff;
            SET_SIGN(src);
            SET_ZERO(src);

            if (optype == ACC) reg_A = src;
            else writeB(iaddr, src);

            /* ORA */
            src |= reg_A;
            SET_SIGN(src);
            SET_ZERO(src);
            reg_A = src;
            break;
        }

        case RLA: {

            /* ROL */
            src <<= 1;
            if (IF_CARRY) src |= 0x1;
            SET_CARRY(src > 0xff);
            src &= 0xff;
            SET_SIGN(src);
            SET_ZERO(src);
            if (optype == ACC) reg_A = src; else writeB(iaddr, src);

            /* AND */
            src &= reg_A;
            SET_SIGN(src);
            SET_ZERO(src);
            reg_A = src;
            break;
        }

        case RRA: {

            /* ROR */
            if (IF_CARRY) src |= 0x100;
            SET_CARRY(src & 0x01);
            src >>= 1;
            SET_SIGN(src);
            SET_ZERO(src);
            if (optype == ACC) reg_A = src; else writeB(iaddr, src);

            /* ADC */
            temp = src + reg_A + (reg_P & 1);
            SET_ZERO(temp & 0xff);
            SET_SIGN(temp);
            SET_OVERFLOW(((reg_A ^ src ^ 0x80) & 0x80) && ((reg_A ^ temp) & 0x80) );
            SET_CARRY(temp > 0xff);
            reg_A = temp & 0xff;
            break;

        }

        case SRE: {

            /* LSR */
            SET_CARRY(src & 0x01);
            src >>= 1;
            SET_SIGN(src);
            SET_ZERO(src);
            if (optype == ACC) reg_A = src; else writeB(iaddr, src);

            /* EOR */
            src ^= reg_A;
            SET_SIGN(src);
            SET_ZERO(src);
            reg_A = src;

            break;
        }

        case DCP: {

            /* DEC */
            src = (src - 1) & 0xff;
            SET_SIGN(src);
            SET_ZERO(src);
            writeB(iaddr, src);

            /* CMP */
            src = reg_A - src;
            SET_CARRY(src >= 0);
            SET_SIGN(src);
            SET_ZERO(src & 0xff);
            break;
        }

        /* Увеличить на +1 и вычесть из A полученное значение */
        case ISC: {

            /* INC */
            src = (src + 1) & 0xff;
            SET_SIGN(src);
            SET_ZERO(src);
            writeB(iaddr, src);

            /* SBC */
            temp = reg_A - src - (IF_CARRY ? 0 : 1);

            SET_SIGN(temp);
            SET_ZERO(temp & 0xff);
            SET_OVERFLOW(((reg_A ^ temp) & 0x80) && ((reg_A ^ src) & 0x80));
            SET_CARRY(temp >= 0);
            reg_A = (temp & 0xff);
            break;
        }

        /* A,X = src */
        case LAX: {

            reg_A = (src);
            SET_SIGN(src);
            SET_ZERO(src);
            reg_X = (src);
            break;
        }

        case AAX: writeB(iaddr, reg_A & reg_X); break;

        /* AND + Carry */
        case AAC: {

            /* AND */
            src &= reg_A;
            SET_SIGN(src);
            SET_ZERO(src);
            reg_A = src;

            /* Carry */
            SET_CARRY(reg_A & 0x80);
            break;
        }

        case ASR: {

            /* AND */
            src &= reg_A;
            SET_SIGN(src);
            SET_ZERO(src);
            reg_A = src;

            /* LSR A */
            SET_CARRY(reg_A & 0x01);
            reg_A >>= 1;
            SET_SIGN(reg_A);
            SET_ZERO(reg_A);
            break;
        }

        case ARR: {

            /* AND */
            src &= reg_A;
            SET_SIGN(src);
            SET_ZERO(src);
            reg_A = src;

            /* P[6] = A[6] ^ A[7]: Переполнение */
            SET_OVERFLOW((reg_A ^ (reg_A >> 1)) & 0x40);

            temp = (reg_A >> 7) & 1;
            reg_A >>= 1;
            reg_A |= (reg_P & 1) << 7;

            SET_CARRY(temp);
            SET_SIGN(reg_A);
            SET_ZERO(reg_A);
            break;
        }

        case ATX: {

            reg_A |= 0xFF;

            /* AND */
            src &= reg_A;
            SET_SIGN(src);
            SET_ZERO(src);
            reg_A = src;

            reg_X = reg_A;
            break;

        }

        case AXS: {

            temp = (reg_A & reg_X) - src;
            SET_SIGN(temp);
            SET_ZERO(temp);
            SET_CARRY(((temp >> 8) & 1) ^ 1);
            reg_X = temp;
            break;
        }
    }

    // Установка нового адреса
    reg_PC      = addr;
    deb_addr    = addr;

    // ---- Отладка ---
    if (TRACER && debugOldPC != reg_PC) {

        FILE* ab = fopen("trace.log", "a+");

        decodeLine(reg_PC);
        int AE = getEffectiveAddress(reg_PC);

        fprintf(ab, "%04X | %02X %02X %02X | %04X: %02X | %s\n", reg_PC, sram[ reg_PC ], sram[ reg_PC+1 ], sram[ reg_PC+2 ], AE & 0xFFFF, sram[AE], debLine);
        fclose(ab);
        debugOldPC = reg_PC;
    }

    return cycles_per_instr;
}

// Декодирование линии, указанной по адресу
int decodeLine(int addr) {

    int t;
    int regpc = addr;
    unsigned char op;
    char operand[32];

    op   = sram[ addr ];
    addr = (addr + 1) & 0xffff;

    // Получение номера опкода
    int op_name_id = operandNames[ op ];
    int op_oper_id = operandTypes[ op ];

    // Декодирование операнда
    switch (op_oper_id) {

        /* IMMEDIATE VALUE */
        case IMM: t = sram[ addr ]; addr++; sprintf(operand, "#%02X", t); break;

        /* INDIRECT X */
        case NDX: t = sram[ addr ]; addr++; sprintf(operand, "($%02X,X)", t); break;

        /* ZEROPAGE */
        case ZP: t = sram[ addr ]; addr++; sprintf(operand, "$%02X", t); break;

        /* ABSOLUTE */
        case ABS: t = readW(addr); addr += 2; sprintf(operand, "$%04X", t); break;

        /* INDIRECT Y */
        case NDY: t = sram[ addr ]; addr++; sprintf(operand, "($%02X),Y", t); break;

        /* ZEROPAGE X */
        case ZPX: t = sram[ addr ]; addr++; sprintf(operand, "$%02X,X", t); break;

        /* ABSOLUTE Y */
        case ABY: t = readW(addr); addr += 2; sprintf(operand, "$%04X,Y", t); break;

        /* ABSOLUTE X */
        case ABX: t = readW(addr); addr += 2; sprintf(operand, "$%04X,X", t); break;

        /* RELATIVE */
        case REL: t = sram[ addr ]; addr++; sprintf(operand, "$%04X", addr + (t < 128 ? t : t - 256));  break;

        /* ACCUMULATOR */
        case ACC: sprintf(operand, "A"); break;

        /* ZEROPAGE Y */
        case ZPY: t = sram[ addr ]; addr++; sprintf(operand, "$%02X,Y", t); break;

        /* INDIRECT ABS */
        case IND: t = readW(addr); addr += 2; sprintf(operand, "($%04X)", t);  break;

        /* IMPLIED, UNDEFINED */
        default: operand[0] = 0;
    }

    addr &= 0xffff;
    sprintf((char*)debLine, "%s %s", operandNamesString[ op_name_id ], operand);
    return addr - regpc;
}

// Печать дампа
void disassembleAll() {

    int bytes, i, j, current_bg, addr, breakOn = 0;

    // Выровнять по верхней границе
    if (deb_addr < deb_top) {
        deb_top = deb_addr;
    }

    // Проверить выход за нижнюю границу
    if (deb_bottom >= 0 && deb_addr > deb_bottom) {
        deb_top = deb_addr;
    }

    // Выровнять если PC ушел
    if (reg_PC < deb_top || reg_PC > deb_bottom) {
        deb_top = reg_PC;
    }

    addr = deb_top;
    for (i = 0; i < 50; i++) {

        // Записать текущую линию в буфер линии
        debAddr[ i ] = addr;

        // Текущий фон
        current_bg = 0;

        // Декодировать линию
        bytes = decodeLine(addr);

        // Поиск точек останова
        for (j = 0; j < breakpointsMax; j++) {
            if (breakpoints[j] == addr) {
                current_bg = 0xFF0000;
                printString(32, 1 + i, "                                ", 0x00ff00, current_bg);
                breakOn = 1;
                break;
            }
        }

        // Выделение текущей линии
        if (deb_addr == addr) {
            current_bg = 0x0000FF;
            printString(33, 1 + i, "                               ", 0x00ff00, current_bg);
        }

        // Показать текущую позицию исполнения
        if (reg_PC == addr) {
            printString(32, 1 + i, breakOn ? "\x0F" : "\x10", 0xffffff, 0);
        }

        // Пропечатать адрес
        printHex(33, 1 + i, addr, 4, 0xffffff, current_bg);

        // Пропечать байты
        for (j = 0; j < bytes; j++) {
            printHex(38 + 3*j, 1 + i, sram[addr + j], 2, 0xf0f000, current_bg);
        }

        // Печатать саму строку
        printString(47, 1 + i, debLine, 0x00ff00, current_bg);

        // Почти нижняя граница?
        if (i == 44) {
            deb_bottom = addr;
        }

        addr += bytes;
    }
}

// Поиск точек останова
int breakpoint_test() {

    int bt = sram[ reg_PC ], j;

    // Программная точка останова (KIL)
    if (bt == 0x02) {

        cpu_running = 0;
        return 1;
    }
    else {

        for (j = 0; j < breakpointsMax; j++) {

            if (breakpoints[ j ] == reg_PC) {

                deb_addr    = reg_PC;
                cpu_running = 0;
                return 1;
            }
        }
    }

    return 0;
}

/* Запрос NMI */
void request_NMI() {

    /* PPU генерирует обратный кадровый импульс (VBlank) */
    ppu_status |= 0b10000000;

    /* Обновить счетчики */
    cntVT = 0;
    cntH  = 0; regH = 0;
    cntV  = 0; regV  = 0;

    /* Вызвать NMI */
    if ((ctrl0 & 0x80) && 1)  {

        PUSH((reg_PC >> 8) & 0xff);	     /* Вставка обратного адреса в стек */
        PUSH(reg_PC & 0xff);
        SET_BREAK(1);                    /* Установить BFlag перед вставкой */
        reg_P |= 0b00100000;             /* 1 */
        PUSH(reg_P);
        SET_INTERRUPT(1);
        reg_PC = readW(0xFFFA);
    }
}

// Исполнение кванта инструкции по NMI (1/60)
void nmi_exec() {

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    int row = 0, cycles = 0;

    // Выполнить 262 строк (1 кадр)
    for (row = 0; row < 262; row++) {

        /* Сохранить буферизированные значения. Нужны для отладчика */
        vb_HT[ row ] = regHT;
        vb_VT[ row ] = regVT;
        vb_FH[ row ] = regFH;
        vb_FV[ row ] = regFV;

        /* Вызвать NMI на обратном синхроимпульсе */
        if (row == 241) {
            request_NMI();
        }

        /* Сбросить VBLank и Sprite0Hit, активной экранной страницы */
        if (row == 261) {

            ctrl0 &= 0b11111100;
            ppu_status &= 0b00111111;
        }

        /* Достигнут Sprite0Hit */
        if (spriteRam[0] + 1 == row) {
            ppu_status |= 0b01000000;
        }

        /* Выполнить 1 строку (341 такт PPU) */
        while (cycles < 341) {

            /* Если нет точек остановка, выполнить инструкцию */
            if (!breakpoint_test()) {
                cycles += 3 * exec();
            } else {
                cpu_running = 0;
                break;
            }
        }

        // "Кольцо вычета" циклов исполнения
        cycles = cycles % 341;

        if (!cpu_running) {
            break;
        }

        /* Отрисовка линии с тайлами */
        if ((row % 8) == 0 && row < 248) {
            drawTiles(row >> 3);
        }
    }

    /* Нарисовать спрайты */
    drawSprites();

    /* Перерисовка дампа и прочей отладочной информации */
    swap();
}
