#include <GL/freeglut.h>
#include <stdlib.h>
#include <stdio.h>

#include "display.h"
#include "cpu.h"

void setPalette(int id, int r, int g, int b) {

    globalPalette[id].r = r;
    globalPalette[id].g = g;
    globalPalette[id].b = b;
}

void initGlobalPal() {

    setPalette(0x00, 117, 117, 117);
    setPalette(0x01, 39,  27,  143);
    setPalette(0x02, 0,   0,   171);
    setPalette(0x03, 71,  0,   159);
    setPalette(0x04, 143, 0,   119);
    setPalette(0x05, 171, 0,   19);
    setPalette(0x06, 167, 0,   0);
    setPalette(0x07, 127, 11,  0);
    setPalette(0x08, 67,  47,  0);
    setPalette(0x09, 0,   71,  0);
    setPalette(0x0A, 0,   81,  0);
    setPalette(0x0B, 0,   63,  23);
    setPalette(0x0C, 27,  63,  95);
    setPalette(0x0D, 0,   0,   0);
    setPalette(0x0E, 0,   0,   0);
    setPalette(0x0F, 0,   0,   0);

    setPalette(0x10, 188, 188, 188);
    setPalette(0x11, 0,   115, 239);
    setPalette(0x12, 35,  59,  239);
    setPalette(0x13, 131, 0,   243);
    setPalette(0x14, 191, 0,   191);
    setPalette(0x15, 231, 0,   91);
    setPalette(0x16, 219, 43,  0);
    setPalette(0x17, 203, 79,  15);
    setPalette(0x18, 139, 115, 0);
    setPalette(0x19, 0,   151, 0);
    setPalette(0x1A, 0,   171, 0);
    setPalette(0x1B, 0,   147, 59);
    setPalette(0x1C, 0,   131, 139);
    setPalette(0x1D, 0,   0,   0);
    setPalette(0x1E, 0,   0,   0);
    setPalette(0x1F, 0,   0,   0);

    setPalette(0x20, 255, 255, 255);
    setPalette(0x21, 63,  191, 255);
    setPalette(0x22, 95,  151, 255);
    setPalette(0x23, 167, 139, 253);
    setPalette(0x24, 247, 123, 255);
    setPalette(0x25, 255, 119, 183);
    setPalette(0x26, 255, 119, 99);
    setPalette(0x27, 255, 155, 59);
    setPalette(0x28, 243, 191, 63);
    setPalette(0x29, 131, 211, 19);
    setPalette(0x2A, 79,  223, 75);
    setPalette(0x2B, 88,  248, 152);
    setPalette(0x2C, 0,   235, 219);
    setPalette(0x2D, 0,   0,   0);
    setPalette(0x2E, 0,   0,   0);
    setPalette(0x2F, 0,   0,   0);

    setPalette(0x30, 255, 255, 255);
    setPalette(0x31, 171, 231, 255);
    setPalette(0x32, 199, 215, 255);
    setPalette(0x33, 215, 203, 255);
    setPalette(0x34, 255, 199, 255);
    setPalette(0x35, 255, 199, 219);
    setPalette(0x36, 255, 191, 179);
    setPalette(0x37, 255, 219, 171);
    setPalette(0x38, 255, 231, 163);
    setPalette(0x39, 227, 255, 163);
    setPalette(0x3A, 171, 243, 191);
    setPalette(0x3B, 179, 255, 207);
    setPalette(0x3C, 159, 255, 243);
    setPalette(0x3D, 0,   0,   0);
    setPalette(0x3E, 0,   0,   0);
    setPalette(0x3F, 0,   0,   0);
};

// Пиксель
void setPixel(int x, int y, int color, int scale) {

    int i, j;
    for (i = 0; i < scale; i++) {

        for (j = 0; j < scale; j++) {

            int TX = x + j;
            int TY = HEIGHT - 1 - y - i;

            if (TX >= 0 && TX < WIDTH && TY >= 0 && TY < HEIGHT) {
                frame[ HEIGHT - 1 - y - i ][ x + j ].b =  color & 0xff;
                frame[ HEIGHT - 1 - y - i ][ x + j ].g = (color >> 8) & 0xff;
                frame[ HEIGHT - 1 - y - i ][ x + j ].r = (color >> 16) & 0xff;
            }
        }
    }

}

// Печать строки
void printString(int x, int y, char* string, int fr, int bg) {

    int a, b, color, fo;
    int scale = 2;

    x *= scale * 8;
    y *= scale * 8;

    while (*string) {

        int ch = *string;

        for (a = 0; a < 8; a++) {

            fo = sram[ 0x14000 + ch*8 + a ];
            for (b = 0; b < 8; b++) {

                color = fo & (1 << (7 - b)) ? fr : bg;
                setPixel(x + scale*b, y + scale*a, color, scale);
            }
        }

        x += 8 * scale;

        string++;
    }

}

// -----------------------------------------------------------------
// Рисование спрайтов
// -----------------------------------------------------------------

void drawSprites() {

    int i, a, b, color, fol, foh;

    if (ctrl1 & 0x10) {

        int h;
        for (i = 0; i < 256; i += 4) {

            int sprite_y = spriteRam[i + 0]; // По вертикали
            int sprite_x = spriteRam[i + 3]; // По горизонтали
            int icon     = spriteRam[i + 1]; // Иконка из знакогенератора
            int attr_spr = spriteRam[i + 2]; // Атрибут спрайта
            int at       = attr_spr & 3;     // Атрибут цвета

            /* Не отрисовывать невидимые */
            if (sprite_y >= 232 || sprite_y < 8) {
                continue;
            }

            /* Выбор знакогенератора спрайтов */
            int chrsrc = (ctrl0 & 0x08) ? 0x1000 : 0;
            int height = (ctrl0 & 0x20 ? 2 : 1);
            
            /* Если размер спрайтов = 8x16, то в icon[0] будет bank */
            if (height == 2) {
                
                chrsrc = icon & 1 ? 0x1000 : 0x0000;
                icon  &= 0xFE;
            }

            /* 8x8 или 8x16 */
            for (h = 0; h < height; h++) {

                for (b = 0; b < 8; b++) { // Y

                    for (a = 0; a < 8; a++) { // X                    

                        /* Получение битов из знакогенератора */
                        fol = sram[ 0x10000 + chrsrc + (icon + h)*16 + b + 0 ]; // low
                        foh = sram[ 0x10000 + chrsrc + (icon + h)*16 + b + 8 ]; // high bits

                        /* Расчет X, Y, биты 6 и 7 отвечают за отражение */
                        int s = 1 << (7 - a);
                        int x = sprite_x + (attr_spr & 0x40 ? 7 - a : a);
                        int y = sprite_y + (attr_spr & 0x80 ? (height == 2 ? 15 : 7) - b : b) + h*8 + 1;

                        /* Вычислить 2 бита цвета спрайта из знакогенератора */
                        color = (fol & s ? 1 : 0) | (foh & s ? 2 : 0);

                        /* Если у спрайта задан 5-й бит, то, если фон НЕ прозрачный (=0), НЕ рисуем спрайт */
                        if ((attr_spr & 0x20) == 0x20 && opaque[y][x] == 0) {
                            continue;
                        }

                        /* Отображать пиксель если цвет не 0, и спрайт в пределах экрана */
                        if (color && x < 256 && y < 232 && y > 8) {

                            /* Формируем цвет из атрибута (2 бита) + 2 бита из знакогенератора
                             * Палитра берется из 3F10 */

                            color = (4*at) | color;
                            color = sram[ 0x13F10 + color ];
                            color = 65536*globalPalette[ color ].r +
                                      256*globalPalette[ color ].g +
                                          globalPalette[ color ].b;

                            /* Отображать пиксель:
                             * a) Все спрайты видны
                             * b) Не в крайнем левом столбце */

                            if ((ctrl1 & 0b100) || ((ctrl1 & 0b100) == 0 && x >= 8)) {
                                setPixel(2*x, 2*y, color, 2);
                            }
                        }
                    }
                }
            }

        }
    }

}

// Отрисовать одну линию тайлов
void drawTiles(int i) {

    int screen_h = (ctrl0 & 0x01) ? 1 : 0,
        screen_v = (ctrl0 & 0x02) ? 1 : 0;
    
    int screen_id  = screen_h ^ screen_v ^ cntH ^ cntV;
    int active_chr = (ctrl0 & 0x10) ? 0x1000 : 0x0;

    int xp, yp;
    int j, a, b, ch, fol, foh, at, color, bn;
    int ADDRNT, ADDRPG, ADDRAT;    

    /* 32+1 Нужно дополнительно рисовать 8 пикселей в случае превышения размера */
    for (j = 0; j < 33; j++) {
        
        int bCoarseY = vb_VT[ 8*i ],
            bCoarseX = vb_HT[ 8*i ],
            bFineX   = vb_FH[ 8*i ],
            bFineY   = vb_FV[ 8*i ];

        // -----------------------
        /* Выполнить скроллинг Y */
        int scroll_y  = (i + bCoarseY);
        int scroll_oy = VMirroring ? (scroll_y >= 0x20) : 0;
            scroll_y  = scroll_y & 0x1F;

        /* Выполнить скроллинг X */
        int scroll_x  = (j + bCoarseX);
        int scroll_ox = HMirroring ? scroll_x >= 0x20 : 0;
            scroll_x  = scroll_x & 0x1F;
        // -----------------------

        // Активная страница либо 0, либо 1, в зависимости от переполнения еще
        ADDRNT = 0x12000 + (screen_id ^ scroll_ox ^ scroll_oy ? 0x400 : 0x0);

        // Расcчитать позицию на этой странице
        ADDRPG = (0x20*scroll_y + scroll_x);
        ADDRAT = (scroll_y >> 2)*8 + (scroll_x >> 2);

        ch = sram[ ADDRNT + 0x000 + ADDRPG ];
        at = sram[ ADDRNT + 0x3C0 + ADDRAT ];

        // 0 1 Тайлы 4x4
        // 2 3 Каждый 2x2

        // Номер тайлов 2x2: 0,1,2,3
        bn = ((scroll_y & 2) << 1) + (scroll_x & 2);

        // Извлекаем атрибуты
        at = (at >> bn) & 3;

        for (a = 0; a < 8; a++) {

            fol = sram[ 0x10000 + ch*16 + a + 0 + active_chr ]; // low
            foh = sram[ 0x10000 + ch*16 + a + 8 + active_chr ]; // high bits

            for (b = 0; b < 8; b++) {

                int s = 1 << (7 - b);

                /* Получение 4-х битов фона */
                color = (fol & s ? 1 : 0) | (foh & s ? 2 : 0);
                color = (4*at) | color;

                /* Отображается ли фон? */
                color = (cpu_running == 0 || (ctrl1 & 0x08)) ? color : 0;

                xp = 8*j + b - bFineX;
                yp = 8*i + a - bFineY;

                /* Края не показывать */
                if (yp < 8 || yp > 232) color = 0;

                if (color & 3) {

                    color = sram[ 0x13F00 + color ]; // 16 цветов палитры фона
                    opaque[yp][xp] = 0;

                } else {

                    color = sram[ 0x13F00 ]; // "Прозрачный" цвет фона
                    opaque[yp][xp] = 1;
                }

                color = 65536*globalPalette[ color ].r +
                          256*globalPalette[ color ].g +
                              globalPalette[ color ].b;

                // Закраска черным цветом одной линии
                if (yp > 231) {
                    setPixel(2*xp, 2*yp, 0, 2);
                }
                // Рисовать только в видимой области
                else if (xp < 256 && yp < 240) {
                    setPixel(2*xp, 2*yp, color, 2);
                }
            }
        }
    }
}

// Обновление экрана
void swap() {

    // Очистка буфера. В том числе и Z-буфера
    glDrawPixels(WIDTH, HEIGHT, GL_RGB, GL_UNSIGNED_BYTE, (void*)& frame);
    glutSwapBuffers();
}

// -----------------------------------------------------------------
// Обновление символов фона
// -----------------------------------------------------------------

void printScreen() {

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    int i;
    for (i = 0; i < 31; i++) {
        drawTiles(i);
    }

    drawSprites();
    swap();
}

// Рисование линии (для проверки)
void drawLine(int x1, int y1, int x2, int y2, int color) {

    int deltax = x2 > x1 ? x2 - x1 : x1 - x2;
    int deltay = y2 > y1 ? y2 - y1 : y1 - y2;
    int error = 0, y = y1, x;
    int dir = y2 > y1 ? 1 : -1;

    for (x = x1; x < x2; x++) {

        setPixel(2*x, 2*y, color, 2);

        /*
        int a, b, i = y >> 3, j = x >> 3;
        for (a = 0; a < 8; a++) {
            for (b = 0; b < 8; b++) {
                setPixel(a*2 + 16*j, b*2 + 16*i, color, 2);
            }
        }
        */

        error += deltay;
        if (2 * error >= deltax) {
            y += dir;
            error -= deltax;
        }
    }

}

// Обновить окно отладчика
void updateDebugger() {
    
    int i, j;
    
    /* Полная очистка */
    for (i = 0; i < HEIGHT; i++) {
    for (j = 0; j < WIDTH; j++) {
        frame[i][j].r = 0;
        frame[i][j].g = 0;
        frame[i][j].b = 0;
    } }

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    /* Печать регистров */
    printRegisters();
    
    /* Дизассемблер */
    disassembleAll();
    
    /* Обновить экран */
    printScreen();

    swap();    
}

// Постоянное отображение информации на дисплее из буфера
void display() {

    int i, j;

    // Отладка
    if (cpu_running == 0) {

        if (redrawDump) {
            updateDebugger();
        }

        redrawDump    = 0;
        justRedrawAll = 1;
    }
    else {

        /* Сделать Disabled для области отладки */
        if (justRedrawAll) {
            
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            for (i = 0; i < HEIGHT; i++) {
                for (j = i % 2; j < WIDTH; j += 2) {
                    frame[i][j].r = 0;
                    frame[i][j].g = 0;
                    frame[i][j].b = 0;
                } }

            swap();
            
        }

        redrawDump    = 1;
        justRedrawAll = 0;

        /* Выполнить код фрейма */
        nmi_exec();
    }

    glutPostRedisplay();
}

// Печать hex-значений
void printHex(int x, int y, unsigned int value, int size, int fr, int bg) {

    char digs[5], a, b;

    if (size == 2) {

        a = (value & 0xF0) >> 4;
        b = (value & 0x0F);

        digs[0] = '0' + a + (a >= 10 ? 7 : 0);
        digs[1] = '0' + b + (b >= 10 ? 7 : 0);
        digs[2] = 0;
    }
    else {

        a = (value & 0xF000) >> 12;
        b = (value & 0x0F00) >> 8;

        digs[0] = '0' + a + (a >= 10 ? 7 : 0);
        digs[1] = '0' + b + (b >= 10 ? 7 : 0);

        a = (value & 0xF0) >> 4;
        b = (value & 0x0F);

        digs[2] = '0' + a + (a >= 10 ? 7 : 0);
        digs[3] = '0' + b + (b >= 10 ? 7 : 0);
        digs[4] = 0;

    }

    printString(x, y, digs, fr, bg);
}

// Пропечатка бинарного значения
void printBin(int x, int y, unsigned int value, int fr, int bg) {

    char s[2]; s[1] = 0;
    int i;

    for (i = 0; i < 8; i++) {

        s[0] = value & (1 << (7 - i)) ? '1' : '0';
        printString(x + i, y, s, fr, bg);
    }
}

// Печать содержания регистров
void printRegisters() {

    int  baseline = 31;
    char s[2]; s[1] = 0;
    int  i, j;
    char sym[16] = "nv_bdizcNV_BDIZC";

    // Болванка
    printString(1, baseline + 0, "\x04  HEX  BINARY", 0xffff00, 0);
    printString(1, baseline + 1, "A  00   00000000", 0x00ff00, 0);
    printString(1, baseline + 2, "X  00   00000000", 0x00ff00, 0);
    printString(1, baseline + 3, "Y  00   00000000", 0x00ff00, 0);
    printString(1, baseline + 4, "S  00   00000000", 0x00ff00, 0);
    printString(1, baseline + 5, "P  00   Nv_bdIzc", 0x00ff00, 0);
    printString(1, baseline + 6, "PC 0000", 0x00ff00, 0);
    printString(1, baseline + 7, "EA 0000", 0x00ff00, 0);

    // Значение регистров
    printHex(4, baseline + 1, reg_A, 2, 0xffffff, 0);
    printHex(4, baseline + 2, reg_X, 2, 0xffffff, 0);
    printHex(4, baseline + 3, reg_Y, 2, 0xffffff, 0);
    printHex(4, baseline + 4, reg_S, 2, 0xffffff, 0);
    printHex(4, baseline + 5, reg_P, 2, 0xffffff, 0);
    printHex(4, baseline + 6, reg_PC, 4, 0xffffff, 0);

    // Значение регистров (бинарное)
    printBin(9, baseline + 1, reg_A, 0xc0c0c0, 0);
    printBin(9, baseline + 2, reg_X, 0xc0c0c0, 0);
    printBin(9, baseline + 3, reg_Y, 0xc0c0c0, 0);
    printBin(9, baseline + 4, reg_S, 0xc0c0c0, 0);
    printBin(9, baseline + 5, reg_P, 0xc0c0c0, 0);

    for (i = 0; i < 8; i++) {

        int bit = reg_P & (1 << (7 - i)) ? 1 : 0;
        int color = bit ? 0x00ff00 : 0x008000;

        s[0] = sym[i + 8*bit];
        printString(9 + i, baseline + 6, s, color, 0);
    }

    unsigned int iaddr = getEffectiveAddress(deb_addr) & 0xffff;

    // Показать текущий эффективный адрес
    printHex(4, baseline + 7, iaddr, 4, 0xffffff, 0);

    // Значения в этой точке
    for (i = 0; i < 7; i++) {

        sprintf(sym, "%02X", sram[ iaddr + i ]);
        printString(9 + i*3, baseline + 7, sym, i == 0 ? 0x00ffff : 0x00c0c0, 0);
    }

    // Таб
    printString(1, baseline + 9, "ZP",    dump_mode == DUMP_ZP ? 0xffffff : 0x00a0a0, 0);
    printString(4, baseline + 9, "Stack", dump_mode == DUMP_STACK ? 0xffffff : 0x00a0a0, 0);
    printString(10, baseline + 9, "OAM", dump_mode == DUMP_OAM ? 0xffffff : 0x00a0a0, 0);

    // Памятка
    printString(1,  baseline + 19, "F1 Help F3 Seek F5 Run F7 Step", 0x808080, 0);
    printString(1,  baseline + 20, "F2 Bkpt F4 Save F6 Tab F8 Load", 0x808080, 0);
    printString(1,  baseline + 21, "                F9 One        ", 0x808080, 0);
    
    // Подсветка для функциональных кнопок
    printString(1,  baseline + 19, "F1", 0xC0A000, 0);
    printString(9,  baseline + 19, "F3", 0xC0A000, 0);
    printString(17, baseline + 19, "F5", 0xC0A000, 0);
    printString(24, baseline + 19, "F7", 0xC0A000, 0);
    printString(1,  baseline + 20, "F2", 0xC0A000, 0);
    printString(9,  baseline + 20, "F4", 0xC0A000, 0);
    printString(17, baseline + 20, "F6", 0xC0A000, 0);
    printString(24, baseline + 20, "F8", 0xC0A000, 0);
    printString(17, baseline + 21, "F9", 0xC0A000, 0);
    
    if (cpu_running) {        
        printString(17, baseline + 20, "F5 Run", 0x80FF80, 0);        
     }

    // PPU
    printString(33, baseline + 21, "ADDR 0000  CTL 00 00 X00 Y00", 0x808080, 0);
    printHex(38, baseline + 21, VRAMAddress, 4, 0xffffff, 0);
    printHex(48, baseline + 21, ctrl0, 2, 0xffffff, 0);
    printHex(51, baseline + 21, ctrl1, 2, 0xffffff, 0);
    printHex(55, baseline + 21, regHT, 2, 0xffff00, 0);
    printHex(59, baseline + 21, regVT, 2, 0xffff00, 0);

    int zp;
    printString(6, baseline + 11, "+0 +1 +2 +3 +4 +5 +6 +7", 0xffffff, 0);
    switch (dump_mode) {

        case DUMP_ZP:

            zp = zp_base;
            for (i = 0; i < 6; i++) {

                printHex(1, baseline + i + 12, zp + i*8, 4, 0xffffff, 0);
                for (j = 0; j < 8; j++) {

                    sprintf(sym, "%02X", readB( zp + j + i * 8 ));
                    printString(6 + j*3, baseline + 12 + i, sym, 0xc0c0c0, 0);
                }
            }

            break;

        case DUMP_STACK:

            zp = 0x0100 + reg_S + 1;
            for (i = 0; i < 6; i++) {

                printHex(1, baseline + i + 12, (((zp + i*8) & 0xff) | 0x100), 4, 0xffffff, 0);
                for (j = 0; j < 8; j++) {

                    sprintf(sym, "%02X", readB( ((zp + i * 8 + j) & 0xff) | 0x100 ));
                    printString(6 + j*3, baseline + 12 + i, sym, 0xc0c0c0, 0);
                }
            }

            break;
    }
}

// Загрузка шрифта
void fontsLoad() {

    FILE* f = fopen("8x8cp1251.fnt", "rb");

    if (f == NULL) {

        printf("Lost file: 8x8cp1251.fnt\n");
        exit(1);
    }

    // Основные шрифты
    fseek(f, 0, SEEK_SET);
    rdsize = fread(sram + 0x14000, 1, 2048, f);

    fclose(f);
}
