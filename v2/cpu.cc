#include "cpu.h"
#include <stdio.h>
#include <stdlib.h>

// Инициализация процессора
CPU::CPU() {

    reg_A  = 0x00;
    reg_X  = 0x00;
    reg_Y  = 0x00;
    reg_S  = 0x00;
    reg_P  = 0x00;
    reg_PC = 0xc000;

    fatal = 0;
}

// Реализовать виртуальные функции
// ---------------------------------------------------------------------
unsigned char  CPU::read_byte(int) { return 0; }
unsigned short CPU::read_word(int) { return 0; }
void           CPU::write_byte(int, unsigned char) { }
// ---------------------------------------------------------------------

// Получение эффективного адреса
int CPU::get_effective(int addr) {

    int opcode, iaddr;
    int tmp, rt, pt;

    // Чтение опкода
    opcode = read_byte(addr++); addr &= 0xffff;

    // Разобрать операнд
    switch (operand_types[ opcode ]) {

        // PEEK( PEEK( (arg + X) % 256) + PEEK((arg + X + 1) % 256) * 256
        // Indirect, X (b8,X)
        case NDX: {

            tmp = read_byte( addr );
            tmp = (tmp + reg_X) & 0xff;

            return read_byte( tmp ) + ((read_byte((1 + tmp) & 0xff) << 8));
        }

        // Indirect, Y (b8),Y
        case NDY: {

            tmp = read_byte(addr);
            rt  = read_byte(0xff & tmp);
            rt |= read_byte(0xff & (tmp + 1)) << 8;
            pt  = rt;
            rt  = (rt + reg_Y) & 0xffff;

            if ((pt & 0xff00) != (rt & 0xff00))
                cycles_ext++;

            return rt;
        }

        // Zero Page
        case ZP:  return read_byte( addr );

        // Zero Page, X
        case ZPX: return (read_byte(addr) + reg_X) & 0x00ff;

        // Zero Page, Y
        case ZPY: return (read_byte(addr) + reg_Y) & 0x00ff;

        // Absolute
        case ABS: return read_word(addr);

        // Absolute, X
        case ABX: {

            pt = read_word(addr);
            rt = pt + reg_X;

            if ((pt & 0xff00) != (rt & 0xff00))
                cycles_ext++;

            return rt & 0xffff;
        }

        // Absolute, Y
        case ABY: {

            pt = read_word(addr);
            rt = pt + reg_Y;

            if ((pt & 0xff00) != (rt & 0xff00))
                cycles_ext++;

            return rt & 0xffff;
        }

        // Indirect
        case IND: {

            addr  = read_word(addr);
            iaddr = read_byte(addr) + 256*read_byte((addr & 0xFF00) + ((addr + 1) & 0x00FF));
            return iaddr;
        }

        // Relative
        case REL: {

            iaddr = read_byte(addr);
            return (iaddr + addr + 1 + (iaddr < 128 ? 0 : -256)) & 0xffff;
        }
    }

    return -1;
}

// Перехо на новый адрес
int CPU::do_branch(int& addr, int iaddr) {

    addr = iaddr;

    if ((addr & 0xff00) != (iaddr & 0xff00))
        return 2;

    return 1;
}

// Исполнить переход по BRK
void CPU::do_brk() {

    PUSH((reg_PC >> 8) & 0xff);	     /* Вставка обратного адреса в стек */
    PUSH(reg_PC & 0xff);
    SET_BREAK(1);                    /* Установить BFlag перед вставкой */
    reg_P |= 0b00100000;             /* 1 */
    PUSH(reg_P);
    SET_INTERRUPT(1);
}

// Декодирование линии, указанной по адресу
int CPU::disassemble(int addr) {

    int t;
    int regpc = addr;
    unsigned char op;
    char operand[32];

    op   = read_byte(addr);
    addr = (addr + 1) & 0xffff;

    // Получение номера опкода
    int op_name_id = opcode_names[ op ];
    int op_oper_id = operand_types[ op ];

    // Декодирование операнда
    switch (op_oper_id) {

        /* IMMEDIATE VALUE */
        case IMM: t = read_byte(addr); addr++; sprintf(operand, "#%02X", t); break;

        /* INDIRECT X */
        case NDX: t = read_byte(addr); addr++; sprintf(operand, "($%02X,X)", t); break;

        /* ZEROPAGE */
        case ZP: t = read_byte(addr); addr++; sprintf(operand, "$%02X", t); break;

        /* ABSOLUTE */
        case ABS: t = read_word(addr); addr += 2; sprintf(operand, "$%04X", t); break;

        /* INDIRECT Y */
        case NDY: t = read_byte(addr); addr++; sprintf(operand, "($%02X),Y", t); break;

        /* ZEROPAGE X */
        case ZPX: t = read_byte(addr); addr++; sprintf(operand, "$%02X,X", t); break;

        /* ABSOLUTE Y */
        case ABY: t = read_word(addr); addr += 2; sprintf(operand, "$%04X,Y", t); break;

        /* ABSOLUTE X */
        case ABX: t = read_word(addr); addr += 2; sprintf(operand, "$%04X,X", t); break;

        /* RELATIVE */
        case REL: t = read_byte(addr); addr++; sprintf(operand, "$%04X", addr + (t < 128 ? t : t - 256));  break;

        /* ACCUMULATOR */
        case ACC: sprintf(operand, "A"); break;

        /* ZEROPAGE Y */
        case ZPY: t = read_byte(addr); addr++; sprintf(operand, "$%02X,Y", t); break;

        /* INDIRECT ABS */
        case IND: t = read_word(addr); addr += 2; sprintf(operand, "($%04X)", t);  break;

        /* IMPLIED, UNDEFINED */
        default: operand[0] = 0;
    }

    addr &= 0xffff;

    // Полная строка дизассемблера
    sprintf((char*) disasmrow, "%s %s", operand_names_string[ op_name_id ], operand);

    // Частично
    strcpy(ds_mnem, operand_names_string[ op_name_id ]);
    strcpy(ds_oper, operand);

    return addr - regpc;
}

// Исполнение шага инструкции
int CPU::step() {

    int temp, optype, opname, ppurd = 1, src = 0;
    int addr = reg_PC, opcode;
    int cycles_per_instr = 2;

    // Доп. циклы разбора адреса
    cycles_ext = 0;

    // Определение эффективного адреса
    int iaddr = get_effective(addr);

    // Прочесть информацию по опкодам
    opcode = read_byte(addr) & 0xff;
    optype = operand_types[ opcode ];
    opname = opcode_names [ opcode ];

    // Эти инструкции НЕ ДОЛЖНЫ читать что-либо из памяти перед записью
    if (opname == STA || opname == STX || opname == STY) {
        ppurd = 0;
    }

    // Инкремент адреса при чтении опкода
    addr = (addr + 1) & 0xffff;

    // Базовые циклы + доп. циклы
    cycles_per_instr = cycles_basic[ opcode ] + cycles_ext;

    // --------------------------------
    // Чтение операнда из памяти
    // --------------------------------

    switch (optype) {

        case ___: printf("Opcode %02x error at %04x\n", opcode, reg_PC); exit(0);
        case NDX: // Indirect X (b8,X)
        case NDY: // Indirect, Y
        case ZP:  // Zero Page
        case ZPX: // Zero Page, X
        case ZPY: // Zero Page, Y
        case REL: // Relative

            addr = (addr + 1) & 0xffff;
            if (ppurd) src = read_byte( iaddr );
            break;

        case ABS: // Absolute
        case ABX: // Absolute, X
        case ABY: // Absolute, Y
        case IND: // Indirect

            addr = (addr + 2) & 0xffff;
            if (ppurd) src = read_byte( iaddr );
            break;

        case IMM: // Immediate

            if (ppurd) src = read_byte(addr);
            addr = (addr + 1) & 0xffff;
            break;

        case ACC: // Accumulator source

            src = reg_A;
            break;
    }

    // --------------------------------
    // Разбор инструкции и исполнение
    // --------------------------------

    switch (opname) {

        // Сложение с учетом переноса
        case ADC: {

            temp = src + reg_A + (reg_P & 1);
            SET_ZERO(temp & 0xff);
            SET_SIGN(temp);
            SET_OVERFLOW(((reg_A ^ src ^ 0x80) & 0x80) && ((reg_A ^ temp) & 0x80) );
            SET_CARRY(temp > 0xff);
            reg_A = temp & 0xff;
            break;
        }

        // Логическое умножение
        case AND: {

            src &= reg_A;
            SET_SIGN(src);
            SET_ZERO(src);
            reg_A = src;
            break;
        }

        // Логический сдвиг вправо
        case ASL: {

            SET_CARRY(src & 0x80);
            src <<= 1;
            src &= 0xff;
            SET_SIGN(src);
            SET_ZERO(src);

            if (optype == ACC) reg_A = src; else write_byte(iaddr, src);
            break;
        }

        // Условный переходы
        case BCC: if (!IF_CARRY())      cycles_per_instr += do_branch(addr, iaddr); break;
        case BCS: if ( IF_CARRY())      cycles_per_instr += do_branch(addr, iaddr); break;
        case BNE: if (!IF_ZERO())       cycles_per_instr += do_branch(addr, iaddr); break;
        case BEQ: if ( IF_ZERO())       cycles_per_instr += do_branch(addr, iaddr); break;
        case BPL: if (!IF_SIGN())       cycles_per_instr += do_branch(addr, iaddr); break;
        case BMI: if ( IF_SIGN())       cycles_per_instr += do_branch(addr, iaddr); break;
        case BVC: if (!IF_OVERFLOW())   cycles_per_instr += do_branch(addr, iaddr); break;
        case BVS: if ( IF_OVERFLOW())   cycles_per_instr += do_branch(addr, iaddr); break;

        // Копированиь бит 6 в OVERFLOW флаг
        case BIT: {

            SET_SIGN(src);
            SET_OVERFLOW(0x40 & src);
            SET_ZERO(src & reg_A);
            break;
        }

        // Программное прерывание
        case BRK: {

            reg_PC = (reg_PC + 2) & 0xffff;
            do_brk();
            addr = read_word(0xFFFE);
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
            write_byte(iaddr, src);
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
            write_byte(iaddr, src);
            break;
        }

        /* Уменьшение X на единицу */
        case INX: {

            reg_X = (reg_X + 1) & 0xff;
            SET_SIGN(reg_X);
            SET_ZERO(reg_X);
            break;
        }

        /* Увеличение Y на единицу */
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
            if (optype == ACC) reg_A = src; else write_byte(iaddr, src);
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
            if (IF_CARRY()) src |= 0x1;
            SET_CARRY(src > 0xff);
            src &= 0xff;
            SET_SIGN(src);
            SET_ZERO(src);
            if (optype == ACC) reg_A = src; else write_byte(iaddr, src);
            break;
        }

        /* Циклический сдвиг вправо */
        case ROR: {

            if (IF_CARRY()) src |= 0x100;
            SET_CARRY(src & 0x01);
            src >>= 1;
            SET_SIGN(src);
            SET_ZERO(src);
            if (optype == ACC) reg_A = src; else write_byte(iaddr, src);
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

            temp = reg_A - src - (IF_CARRY() ? 0 : 1);

            SET_SIGN(temp);
            SET_ZERO(temp & 0xff);
            SET_OVERFLOW(((reg_A ^ temp) & 0x80) && ((reg_A ^ src) & 0x80));
            SET_CARRY(temp >= 0);
            reg_A = (temp & 0xff);
            break;
        }

        /* Запись содержимого A,X,Y в память */
        case STA: write_byte(iaddr, reg_A); break;
        case STX: write_byte(iaddr, reg_X); break;
        case STY: write_byte(iaddr, reg_Y); break;

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

        // -------------------------------------------------------------
        // Недокументированные инструкции
        // -------------------------------------------------------------

        case SLO: {

            /* ASL */
            SET_CARRY(src & 0x80);
            src <<= 1;
            src &= 0xff;
            SET_SIGN(src);
            SET_ZERO(src);

            if (optype == ACC) reg_A = src;
            else write_byte(iaddr, src);

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
            if (IF_CARRY()) src |= 0x1;
            SET_CARRY(src > 0xff);
            src &= 0xff;
            SET_SIGN(src);
            SET_ZERO(src);
            if (optype == ACC) reg_A = src; else write_byte(iaddr, src);

            /* AND */
            src &= reg_A;
            SET_SIGN(src);
            SET_ZERO(src);
            reg_A = src;
            break;
        }

        case RRA: {

            /* ROR */
            if (IF_CARRY()) src |= 0x100;
            SET_CARRY(src & 0x01);
            src >>= 1;
            SET_SIGN(src);
            SET_ZERO(src);
            if (optype == ACC) reg_A = src; else write_byte(iaddr, src);

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
            if (optype == ACC) reg_A = src; else write_byte(iaddr, src);

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
            write_byte(iaddr, src);

            /* CMP */
            src = reg_A - src;
            SET_CARRY(src >= 0);
            SET_SIGN(src);
            SET_ZERO(src & 0xff);
            break;
        }

        // Увеличить на +1 и вычесть из A полученное значение
        case ISC: {

            /* INC */
            src = (src + 1) & 0xff;
            SET_SIGN(src);
            SET_ZERO(src);
            write_byte(iaddr, src);

            /* SBC */
            temp = reg_A - src - (IF_CARRY() ? 0 : 1);

            SET_SIGN(temp);
            SET_ZERO(temp & 0xff);
            SET_OVERFLOW(((reg_A ^ temp) & 0x80) && ((reg_A ^ src) & 0x80));
            SET_CARRY(temp >= 0);
            reg_A = (temp & 0xff);
            break;
        }

        // A,X = src
        case LAX: {

            reg_A = (src);
            SET_SIGN(src);
            SET_ZERO(src);
            reg_X = (src);
            break;
        }

        case AAX: write_byte(iaddr, reg_A & reg_X); break;

        // AND + Carry
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

        // Работает правильно, а тесты все равно не проходят эти 2
        case SYA: {

            temp = read_byte(reg_PC + 2);
            temp = ((temp + 1) & reg_Y);
            write_byte(iaddr, temp & 0xff);
            break;
        }

        case SXA: {

            temp = read_byte(reg_PC + 2);
            temp = ((temp + 1) & reg_X);
            write_byte(iaddr, temp & 0xff);
            break;
        }
    }

    // Установка нового адреса
    reg_PC = addr;

    return cycles_per_instr;
}
