#include "nes.h"

// Конструктор
NES::NES(int w, int h, const char* caption) {

    init_sdl(w, h, caption);

    // Стартовый адрес по умолчанию
    reg_PC = 0x8000; ds_start = reg_PC;

    ds_scale = 2;
    for (int i = 0x1800; i < 0x2000; i++) sram[i] = 0x17;
}

// Деструктор
NES::~NES() { }

// Загрузка файла в память
void NES::load_file(const char* filename) {

    FILE* fp = fopen(filename, "rb");
    if (fp) {

        fseek(fp, 0, SEEK_END);
        int size = ftell(fp);
        fseek(fp, 0, SEEK_SET);
        fread(sram + 0x8000, 1, size, fp);
        fclose(fp);

    } else {
        printf("File not found\n");
        exit(1);
    }
}

// Чтение байта
unsigned char NES::read_byte(int addr) {

    int tmp, olddat, mirr;

    addr &= 0xffff;
    return sram[ addr ];
}

// Запись байта
void NES::write_byte(int addr, unsigned char data) {

    addr &= 0xffff;
    sram[ addr ] = data;

    // Обновить символ или атрибут
    if (addr >= 0x1000 && addr < 0x1800)      { addr -= 0x1000; update_char(addr % 80, addr / 80); }
    else if (addr >= 0x1800 && addr < 0x2000) { addr -= 0x1800; update_char(addr % 80, addr / 80); }
}

// Запрос NMI
void NES::request_nmi() {

    if (IF_INTERRUPT()) {

        do_brk();
        reg_PC = read_word(0xFFFA);
    }
}

// Исполнение одного фрейма (341 x 262)
void NES::do_frame() {

    int cycles = 0;
    while (cycles < 89342) {

        if (!breakpoint_test(reg_PC))
             { cycles += 3 * step(); }
        else { cpu_start = 0; require_stop = 1; break; }
    }

    request_nmi();
}

// Обновить экран
void NES::update_screen() {

    switch (ds_mode) {

        case DS_DEBUGGER: update_disassembler(); break;
        default:          redraw_display(); break;
    }
}

int NES::get_color(int cl) {

    switch (cl) {

        case 0:  return 0x000000;
        case 1:  return 0x000080;
        case 2:  return 0x008000;
        case 3:  return 0x008080;
        case 4:  return 0x800000;
        case 5:  return 0x800080;
        case 6:  return 0x808000;
        case 7:  return 0xcccccc;
        case 8:  return 0x808080;
        case 9:  return 0x0000ff;
        case 10: return 0x00ff00;
        case 11: return 0x00ffff;
        case 12: return 0xff0000;
        case 13: return 0xff00ff;
        case 14: return 0xffff00;
        case 15: return 0xffffff;
    }

    return 0;
}

// Обновление 1 элемента 
void NES::update_char(int x, int y) {

    int ch   = read_byte(0x1000 + y*80 + x);
    int attr = read_byte(0x1800 + y*80 + x);
    int c_f  = get_color(attr & 0xf);
    int c_b  = get_color((attr >> 4) & 0x7);

    if (y >= 25) return;

    if (ds_scale > 1) {
        
        for (int i = 0; i < 24; i++) {

            int mask = ansi16[ch][(i*2/3)];
            for (int j = 0; j < 12; j++) {
                pset(32 + 12*x + j, 180 + 24*y + i, (mask & (1 << (7 - j*2/3))) ? c_f : c_b);
            }
        }
    }
    else {
        
        for (int i = 0; i < 16; i++) {

            int mask = ansi16[ch][i];
            for (int j = 0; j < 8; j++) {
                pset(192 + 8*x + j, 280 + 16*y + i, (mask & (1 << (7 - j))) ? c_f : c_b);
            }
        }
    }
}

// Обновить экран (прод)
void NES::redraw_display() {

    cls(0);

    for (int y = 0; y < 25; y++)
    for (int x = 0; x < 80; x++)
        update_char(x, y);
}

// Процессинг клавиши
void NES::process_key(int keycode) { }
void NES::keyup(int keycode) { }
