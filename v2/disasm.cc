#include "nes.h"

// Обработчик кадра
uint DisplayTimer(uint interval, void *param) {

    SDL_Event     event;
    SDL_UserEvent userevent;

    // Создать новый Event
    userevent.type  = SDL_USEREVENT;
    userevent.code  = 0;
    userevent.data1 = NULL;
    userevent.data2 = NULL;

    event.type = SDL_USEREVENT;
    event.user = userevent;

    SDL_PushEvent(&event);
    return (interval);
}

// Инициализация SDL
void NES::init_sdl(int w, int h, const char* caption) {

    width  = w;
    height = h;

    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER);
    SDL_EnableUNICODE(1);

    sdl_screen = SDL_SetVideoMode(w, h, 32, SDL_HWSURFACE | SDL_DOUBLEBUF);
    SDL_WM_SetCaption(caption, 0);

    // 50 кадров в секунду (20)
    SDL_AddTimer(20, DisplayTimer, NULL);

    // Режим окна
    ds_mode         = DS_DEBUGGER; // DS_SCREEN
    ds_scale        = 4;
    ds_brk_cnt      = 0;
    require_stop    = 0;
    cpu_start       = 0;    
}

// Бесконечный цикл
void NES::loop() {

    int keycode;

    while (1) {

        while (SDL_PollEvent(& event)) {

            switch (event.type) {

                // Если нажата на крестик, то приложение будет закрыто
                case SDL_QUIT: return;

                // Нажата мышь
                case SDL_MOUSEBUTTONDOWN: break;

                // Кнопка мыши отжата
                case SDL_MOUSEBUTTONUP: break;

                // Мышь перемещена
                case SDL_MOUSEMOTION: break;

                // Отжатие клавиши
                case SDL_KEYUP: keyup(get_key(event)); break;            

                // Нажата какая-то клавиша
                case SDL_KEYDOWN: keypress(get_key(event)); break;

                // Вызывается по таймеру
                case SDL_USEREVENT:

                    // Остановка процессора по запросу
                    if (require_stop) {

                        require_stop = 0;
                        ds_mode      = DS_DEBUGGER;
                        ds_cursor    = reg_PC;
                        ds_start     = ds_cursor;
                        cpu_start    = 0;

                        update_screen();
                    }

                    // Процессор запущен, можно включать
                    if (cpu_start) do_frame();

                    flip();
                    break;
            }
        }

        SDL_Delay(1);
    }
}

// Чтение байта
unsigned short NES::read_word(int addr) {

    int l = read_byte(addr);
    int h = read_byte(addr+1);
    return h*256 + l;
}

// Нарисовать точку
void NES::pset(int x, int y, uint color) {

    if (x >= 0 && y >= 0 && x < width && y < height) {
        ( (Uint32*)sdl_screen->pixels )[ x + width*y ] = color;
    }
}

void NES::cls(int color) {

    for (int i = 0; i < height; i++)
    for (int j = 0; j < width; j++)
        ( (Uint32*)sdl_screen->pixels )[ j + width*i ] = color;
}

void NES::block(int x1, int y1, int x2, int y2, int color) {

    for (int i = y1; i <= y2; i++)
    for (int j = x1; j <= x2; j++)
        pset(j, i, color);
}

// Печать на экране Char
void NES::print(int col, int row, unsigned char ch, uint cl) {

    col *= 8;
    row *= 16;

    for (int i = 0; i < 16; i++) {

        unsigned char hl = ansi16[ch][i];
        for (int j = 0; j < 8; j++) {
            if (hl & (1<<(7-j)))
                pset(j + col, i + row, cl);
        }
    }
}

// Печать строки
void NES::print(int col, int row, const char* s, uint cl) {

    int i = 0;
    while (s[i]) { print(col, row, s[i], cl); col++; i++; }
}

// Обменять буфер
void NES::flip() {
    SDL_Flip(sdl_screen);
}

// Получение ссылки на структуру с данными о нажатой клавише
int NES::get_key(SDL_Event event) {

    SDL_KeyboardEvent * eventkey = & event.key;
    return eventkey->keysym.scancode;
}

// Установить или удалить brkpoint
void NES::swi_brk() {

    int dsdel = 0; // Маркер удаления из
    for (int k = 0; k < ds_brk_cnt; k++) {

        // Удалить, if has
        if (ds_brk[k] == ds_cursor) {

            ds_brk_cnt--;
            for (int i = k; i < ds_brk_cnt; i++) {
                ds_brk[i] = ds_brk[i+1];
            }
            dsdel = 1;
            break;
        }
    }

    // Добавить точку останова, если нет ее
    if (dsdel == 0) ds_brk[ds_brk_cnt++] = ds_cursor;
}

// Тест на точку остановки в этом PC
int NES::breakpoint_test(int addr) {

    for (int k = 0; k < ds_brk_cnt; k++)
        if (addr == ds_brk[k])
            return 1;

    return 0;
}

// Обработка нажатия на клавишу
void NES::keypress(int keycode) {

    //printf("%d ", keycode);

    // F5 Переключение режима экрана
    if (keycode == 71 && cpu_start == 0) {

        // Из режима отладчика перейти в режим экрана
        if (ds_mode == DS_DEBUGGER) {

            ds_mode = DS_SCREEN;
            redraw_display();
        }
        else {

            ds_mode = DS_DEBUGGER;
            update_disassembler();
        }
    }

    // F2 Точка остановки
    else if (keycode == 68 && cpu_start == 0) {

        swi_brk();
        update_screen();
    }

    // F9 Запуск или остановка
    else if (keycode == 75) {

        // Запущен, в режиме прода
        if (cpu_start && ds_mode == DS_SCREEN) {
            require_stop = 1;
        }

        // Остановлен, в режиме дебаггера
        else if (cpu_start == 0 && ds_mode == DS_DEBUGGER) {

            cls(0);
            ds_mode = DS_SCREEN;
            cpu_start = 1;
            update_screen();
        }
    }

    // F7 Одиночная инструкция
    else if (keycode == 73 && cpu_start == 0) {

        step();
        ds_cursor = reg_PC;
        update_screen();
    }

    // Down
    else if (keycode == 116 && cpu_start == 0) {

        if (ds_cursor_row < 58) {
            ds_cursor = ds_rows[ds_cursor_col][ds_cursor_row + 1];
        } else if (ds_cursor_col < 4) {
            ds_cursor = ds_rows[ds_cursor_col+1][1];
        }
        update_screen();
    }

    // Up
    else if (keycode == 111 && cpu_start == 0) {

        if (ds_cursor_row > 1) {
            ds_cursor = ds_rows[ds_cursor_col][ds_cursor_row - 1];
        } else if (ds_cursor_col > 0) {
            ds_cursor = ds_rows[ds_cursor_col - 1][58];
        } else {
            ds_start = (ds_start - 1) & 0xffff;
            ds_cursor = ds_start;
        }
        update_screen();
    }

    // Right
    else if (keycode == 114 && cpu_start == 0) {

        if (ds_cursor_col < 3) {
            ds_cursor = ds_rows[ds_cursor_col+1][ds_cursor_row];
        }
        update_screen();
    }

    // Left
    else if (keycode == 113 && cpu_start == 0) {

        if (ds_cursor_col > 0) {
            ds_cursor = ds_rows[ds_cursor_col-1][ds_cursor_row];
        }
        update_screen();
    }

    // "N" Установка новой позиции PC
    else if (keycode == 57 && cpu_start == 0 && ds_mode == DS_DEBUGGER) {

        reg_PC = ds_cursor;
        update_screen();
    }

    // Масштабирование (x1, x2, x4) - очистить и обновить экран
    else if (keycode == 10) { ds_scale = 1; if (ds_mode == DS_SCREEN) cls(0); update_screen(); }
    else if (keycode == 11) { ds_scale = 2; if (ds_mode == DS_SCREEN) cls(0); update_screen(); }
    else if (keycode == 12) { ds_scale = 4; if (ds_mode == DS_SCREEN) cls(0); update_screen(); }

    // Нажатие на кнопку джойстика
    else process_key(keycode);
}

// Рисование пикселя (x1, x2, x4)
void NES::set_pixel(int x, int y, int color) {

    if (ds_mode == DS_SCREEN) {

        switch (ds_scale) {

            case 1: pset(384 + x, 360 + y, color); break;
            case 2: for (int k = 0; k < 4; k++)  pset(256 + 2*x + (k%2), 240 + 2*y + (k>>1), color); break;
            case 4: for (int k = 0; k < 16; k++) pset(4*x + (k%4), 4*y + (k>>2), color); break;
        }
    }
}

// Обновить строки дизассемблера
void NES::update_disassembler() {

    cls(0);

    char tmp[255];
    int color;
    int row, col, catched = 0;

    // Стартовый адрес
    int addr = ds_start;

    // Вычисление эффективного адреса в PC
    int efad = get_effective(ds_cursor);

    for (col = 0; col < 4; col++)
    for (row = 1; row < 59; row++) {

        // Сохранить адрес
        ds_rows[col][row] = addr;

        sprintf(tmp, "%04X", addr);
        int bcnt = disassemble(addr);

        // Рисовать подложку
        if (addr == ds_cursor) {

            block(8*(col*26 + 0), 16*row, 8*(col*26 + 25), 16*row + 15, 0x0040a0);

            ds_cursor_col = col;
            ds_cursor_row = row;
            catched = 1;
        }

        // Адрес
        print(col*26 + 1,  row, tmp, 0x80ff80);

        // Мнемоника и операнд
        print(col*26 + 13, row, ds_mnem, 0xffffff);
        if (strlen(ds_oper)) {

            if      (ds_oper[0] == '#') color = 0x00f0f0;
            else if (ds_oper[0] == '$') color = 0xf0f0a0;
            else color = 0xffffff;
            
            print(col*26 + 17, row, ds_oper, color);
        }

        // Текущая позиция
        if (addr == reg_PC)         print(col*26 + 5, row, "\x10", 0xffffff);
        if (breakpoint_test(addr))  print(col*26 + 0, row, "\x08", 0xff0000);

        // Считывание инструкции
        if (bcnt == 1) { sprintf(tmp, "%02X", read_byte(addr)); }
        if (bcnt == 2) { sprintf(tmp, "%02X%02X", read_byte(addr), read_byte(addr+1)); }
        if (bcnt == 3) { sprintf(tmp, "%02X%02X%02X", read_byte(addr), read_byte(addr+1), read_byte(addr+2)); }

        print(col*26 + 6,  row, tmp, 0xa0a0a0);
        print(col*26 + 25, row, "\xB3", 0x808080);

        addr += bcnt;
    }

    // Если нет курсора, то найти его
    if (catched == 0) { ds_start = reg_PC; ds_cursor = ds_start; update_disassembler(); }

    // Регистры
    sprintf(tmp, "  %02X;   %02X;   %02X;   %02X", reg_A, reg_X, reg_Y, reg_S); print(105, 1, tmp, 0xc0c0c0);
    print(105, 1, "A     X     Y     S", 0x909090);
    print(105, 2, "P", 0x909090);  sprintf(tmp, "  %02X", reg_P); print(105, 2, tmp, 0xc0c0c0);
    print(120, 2, "PC", 0x909090); sprintf(tmp, "  %04X", reg_PC); print(121, 2, tmp, 0xc0c0c0);

    // Эффективный адрес
    print(120, 3, "EA", 0x909090);

    if (efad < 0) {
        print(123, 3, "----", 0xc0c0c0);
    } else {
        sprintf(tmp, "%04X", efad);       print(123, 3, tmp, 0xc0c0c0);
        sprintf(tmp, "%02X", sram[efad]); print(125, 4, tmp, 0xff8080);
    }

    // Флаги
    sprintf(tmp, "%c%c%c%c%c%c%c%c",
        reg_P & 0x80 ? 'N' : ' ', reg_P & 0x40 ? 'V' : ' ',
        reg_P & 0x20 ? '1' : ' ', reg_P & 0x10 ? 'B' : ' ',
        reg_P & 0x08 ? 'D' : ' ', reg_P & 0x04 ? 'I' : ' ',
        reg_P & 0x02 ? 'Z' : ' ', reg_P & 0x01 ? 'C' : ' ');
        print(110, 2, tmp, 0x00ff00);

    // Неактивные флаги
    sprintf(tmp, "%c%c%c%c%c%c%c%c",
        reg_P & 0x80 ? ' ' : 'n', reg_P & 0x40 ? ' ' : 'v',
        reg_P & 0x20 ? ' ' : '0', reg_P & 0x10 ? ' ' : 'b',
        reg_P & 0x08 ? ' ' : 'd', reg_P & 0x04 ? ' ' : 'i',
        reg_P & 0x02 ? ' ' : 'z', reg_P & 0x01 ? ' ' : 'c');
        print(110, 2, tmp, 0x008000);
}
