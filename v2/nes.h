#ifndef NESH
#define NESH

#include "SDL.h"
#include <string.h>

#include "ansi16.h"
#include "cpu.h"

enum DsModes {

    DS_DEBUGGER = 0,    // Окно отладчика
    DS_SCREEN           // Окно прода
};

// Глобализованая палитра в целом
static const int allpal[64] = {

    /* 00 */ 0x757575, 0x271B8F, 0x0000AB, 0x47009F,
    /* 04 */ 0x8F0077, 0xAB0013, 0xA70000, 0x7F0B00,
    /* 08 */ 0x432F00, 0x004700, 0x005100, 0x003F17,
    /* 0C */ 0x1B3F5F, 0x000000, 0x000000, 0x000000,
    /* 10 */ 0xBCBCBC, 0x0073EF, 0x233BEF, 0x8300F3,
    /* 14 */ 0xBF00BF, 0xE7005B, 0xDB2B00, 0xCB4F0F,
    /* 18 */ 0x8B7300, 0x009700, 0x00AB00, 0x00933B,
    /* 1C */ 0x00838B, 0x000000, 0x000000, 0x000000,
    /* 20 */ 0xFFFFFF, 0x3FBFFF, 0x5F97FF, 0xA78BFD,
    /* 24 */ 0xF77BFF, 0xFF77B7, 0xFF7763, 0xFF9B3B,
    /* 28 */ 0xF3BF3F, 0x83D313, 0x4FDF4B, 0x58F898,
    /* 2C */ 0x00EBDB, 0x000000, 0x000000, 0x000000,
    /* 30 */ 0xFFFFFF, 0xABE7FF, 0xC7D7FF, 0xD7CBFF,
    /* 34 */ 0xFFC7FF, 0xFFC7DB, 0xFFBFB3, 0xFFDBAB,
    /* 38 */ 0xFFE7A3, 0xE3FFA3, 0xABF3BF, 0xB3FFCF,
    /* 3C */ 0x9FFFF3, 0x000000, 0x000000, 0x000000,
};

class NES : public CPU {

protected:

    int width, height;

    SDL_Event    event;
    SDL_Surface* sdl_screen;

    // Память
    int mapper, prgnum;
    unsigned char nesheader[16];

    unsigned char   sram[512*1024];  // Общая память
    unsigned char   vram[65536];     // Видеопамять

    // Дизассемблер
    int     ds_start;
    int     ds_cursor;
    int     ds_mode;            // Режим окна (0-отладчик, 1-прод)
    int     ds_scale;           // =1 минимум =2 средний =4 большой
    int     ds_rows[4][64];
    int     ds_cursor_col;
    int     ds_cursor_row;
    int     ds_brk[256];
    int     ds_brk_cnt;

    // Запросы на остановку и запуск
    int     require_stop;

    // Дисплей и джойстики
    int     ctrl0, ctrl1;
    int     ppu_status;         // Состояние PPU (регистр PPU)

    // Скроллинги
    int     cntH, cntV, cntVT, cntHT,
            cntFV, cntFH, regH, regV,
            regFV, regHT, regVT, regFH;

    int     VMirroring, HMirroring;
    int     first_write, objvar;
    int     Joy1, Joy1Latch, Joy1Strobe;

    // Курсоры
    int     VRAMTmpAddress;     // Временная память
    int     VRAMSpriteAddress;  // Адрес указателя спрайта в видеопамяти
    int     VRAMAddress;        // Текущий указатель адреса видеопамяти

    // Буферизированные параметры скроллинга
    int     vb_HT[262], vb_FH[262],
            vb_VT[262], vb_FV[262];
    char    opaque [240][256];      // Прозрачность фона
    int     nesdisp[240][256];      // Буфер дисплея
    int     OAM[256];               // Память спрайтов
    int     all_palette[64];

    // Запущен ли процессор?
    int     cpu_start;
    int     rom_bank;

public:

    NES(int, int, const char*);
    ~NES();

    // Реализация виртуальных функции для CPU
    // -----------------------------------------------------------------
    unsigned char  read_byte(int);
    unsigned short read_word(int);
    void    write_byte(int, unsigned char);
    int     vmirror(int addr);
    int     rmirror(int addr);
    // -----------------------------------------------------------------

    void loop();
    void init_sdl(int w, int h, const char* caption);
    void load_nes_file(char* file);
    int  get_key(SDL_Event event);
    void update_disassembler();
    void update_screen();
    int  breakpoint_test(int);
    void swi_brk();
    void keypress(int keycode);
    void keyup(int keycode);

    // Дисплей общий
    void pset(int, int, uint);
    void block(int x1, int y1, int x2, int y2, int color);
    void cls(int color);
    void flip();
    void print(int col, int row, unsigned char ch, uint cl);
    void print(int col, int row, const char* s, uint cl);

    // NES-овский дисплей
    void regs_from_address();
    void cnts_from_address();
    void regs_to_address();
    void cnts_to_address();

    void draw_tiles_row(int i);
    void draw_sprites();
    void redraw_display();
    void set_pixel(int x, int y, int color);
    void request_nmi();
    void do_frame();
    void process_key(int keycode);
};

#endif
