#include "asm.h"

// Логотип для понта
void NESASM::logo() {

    printf("----------------------------------------------------------------\n");
    printf("= 6502 ASSEMBLER C++ IMPLEMENTATION FOXBENTO CORP. (C)(TM)(R)  =\n");
    printf("= GNU General Public License Workflow Rhino Masters. 2019-20xx =\n");
    printf("----------------------------------------------------------------\n");
}

// Вырезать пробелы справа
string NESASM::rtrim(string s) {

    for (int i = s.length() - 1; i >= 0; i--) {
        if (isspace(s[i])) s = s.substr(0, i);
        else break;
    }

    return s;
}

// Вырезать пробелы слева
string NESASM::ltrim(string s) {

    for (int i = 0; i < (int) s.length(); i--) {
        if (isspace(s[0])) s = s.substr(1);
        else break;
    }

    return s;
}

// Ассемблирование одной строки
int NESASM::assemble(string s) {

    int i;

    // По умолчанию, ничего не ассемблировалось
    codesize = 0;

    // Удаление комментариев и пробелов перед ним
    for (i = s.length() - 1; i >= 0; i--) { if (s[i] == ';') s = s.substr(0, i); } s = rtrim(s);

    // Пустая строка не должна быть ассемблирована
    if (s.length() == 0) return ASM_EMPTYLINE;
    if (s.length() < 3) return ASM_SHORT;

    // По умолчанию, деление таким образом
    string mnemonic(s.substr(0, 3)), operand("");

    // Найденный ID в таблице мнемоник
    int mnemonic_id = KIL;

    // Разделение мнемоники и опкода
    for (i = 0; i < (int)s.length(); i++) {

        if (s[i] == ' ') {
            mnemonic = s.substr(0, i);
            operand  = s.substr(i);
            break;
        }
    }

    // Перевести мнемонику в верхний регистр для поиска
    for (i = 0; i < (int) mnemonic.length(); i++) mnemonic[i] = toupper(mnemonic[i]);

    // Поиск мнемоники
    for (i = 0; i < 75; i++) {

        if (operand_names_string[i] == mnemonic) {
            mnemonic_id = i;
            break;
        }
    }

    if (mnemonic_id == KIL) return ASM_MNEMONIC_INVALID;

    // cout << mnemonic_id << "=" << mnemonic << "+" << ltrim(operand) << endl;


    return ASM_OK;
}
