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

    // Запущен ли процессор?
    int     cpu_start;

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
    int  get_key(SDL_Event event);
    void update_disassembler();
    void update_screen();
    int  breakpoint_test(int);
    void swi_brk();
    void keypress(int keycode);
    void keyup(int keycode);
    void init_sdl(int w, int h, const char* caption);

    void load_file(const char* filename);

    // Дисплей общий
    void pset(int, int, uint);
    void block(int x1, int y1, int x2, int y2, int color);
    void cls(int color);
    void flip();
    void print(int col, int row, unsigned char ch, uint cl);
    void print(int col, int row, const char* s, uint cl);
    int  get_color(int cl);

    // NES-овский дисплей
    void update_char(int x, int y);
    void redraw_display();
    void set_pixel(int x, int y, int color);
    void request_nmi();
    void do_frame();
    void process_key(int keycode);
};

#endif
