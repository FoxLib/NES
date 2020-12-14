#include <stdlib.h>
#include <stdio.h>
#include <GL/glut.h>

#include "keyboard.h"
#include "display.h"
#include "cpu.h"

void keyboard(unsigned char key, int x, int y) {

    if (key == 27) {
        exit(0);
    }

    if (key == 'x' || key == 'x') Joy1 |= 0b00000001; // A
    if (key == 'z' || key == 'Z') Joy1 |= 0b00000010; // B
    if (key == 'c' || key == 'C') Joy1 |= 0b00000100; // SELECT
    if (key == 'v' || key == 'V') Joy1 |= 0b00001000; // START

}

void keyboard_up(unsigned char key, int x, int y) {

    if (key == 'x' || key == 'X') Joy1 &= 0b11111110; // A
    if (key == 'z' || key == 'Z') Joy1 &= 0b11111101; // B
    if (key == 'c' || key == 'C') Joy1 &= 0b11111011; // SELECT
    if (key == 'v' || key == 'V') Joy1 &= 0b11110111; // START
}

void keyboard_func(int key, int x, int y) {

    int i, current_id = 0, debugOn = 0;

    redrawDump = 1;

    // Посмотреть, в какой позиции сейчас стоит deb_addr
    for (i = 0; i < 64; i++) {
        if (debAddr[i] == deb_addr) {
            current_id = i;
        }
    }
    
    if (key == GLUT_KEY_F4) {        
        save();        
    }
    
    if (key == GLUT_KEY_F8) {
        loadsav();
    }
    
    /* Выполненение 1 кадра */
    if (key == GLUT_KEY_F9) {    
        
        cpu_running = 1;
        nmi_exec();
        cpu_running = 0;
    }

    // Выполнить 1 шаг
    if (key == GLUT_KEY_F7) {
        
        exec();
        swap();
    }

    // Переключение режима отладчика
    if (key == GLUT_KEY_F2) {

        // Поиск точек останова, чтобы удалить ее
        for (i = 0; i < breakpointsMax; i++) {
            if (breakpoints[i] == deb_addr) {
                breakpoints[i] = -1;
                debugOn = 1;
                break;
            }
        }

        // Точка останова не была найдена - установить новую
        if (debugOn == 0) {
            for (i = 0; i < 256; i++) {
                if (breakpoints[i] == -1) {
                    breakpoints[i] = deb_addr;
                    break;
                }
            }
        }

        // Может быть 0
        breakpointsMax = 0;

        // Пересчитать новую позицию
        for (i = 255; i >= 0; i--) {
            if (breakpoints[i] != -1) {
                breakpointsMax = i + 1;
                break;
            }
        }
    }

    // Переключение режима отладчика
    if (key == GLUT_KEY_F5) {

        cpu_running = 1 - cpu_running;
        
        updateDebugger();
        swap();        
    }

    if (key == GLUT_KEY_F6) {

        switch (dump_mode) {

            case DUMP_ZP: dump_mode = DUMP_STACK; break;
            case DUMP_STACK: dump_mode = DUMP_ZP; break;
        }

        display();
    }

    // Просмотр ZP
    if (key == GLUT_KEY_F3) {

        zp_base = getEffectiveAddress(deb_addr) & 0xffff;
        display();
    }

    // Сохранение состояния видеопамяти и видеорегистров для отладки
    if (key == GLUT_KEY_F12) {

        FILE* fp = fopen("videoram.bin", "wb");
        
        if (fp) {
            
            fwrite(sram + 0x12000, 2048, 1, fp);
            fclose(fp);
        }
        
        fp = fopen("oam.bin", "wb");
        if (fp) {
            fwrite(spriteRam, 256, 1, fp);
            fclose(fp);            
        }
        
        printf("videoram.bin saved\nBG: ");
        for (i = 0; i < 16; i++) { printf("%02X ", sram[ 0x13F00 + i ]); }
        printf("\nSP: ");
        for (i = 0; i < 16; i++) { printf("%02X ", sram[ 0x13F10 + i ]); }
        printf("\n");        
    }

    // -----------------------------------------------------------------

    if (cpu_running) {

        if (key == 101) Joy1 |= 0b00010000; // UP
        if (key == 103) Joy1 |= 0b00100000; // DOWN
        if (key == 100) Joy1 |= 0b01000000; // LEFT
        if (key == 102) Joy1 |= 0b10000000; // RIGHT

        return;
    }

    // -----------------------------------------------------------------

    // Кнопка "вверх"
    if (key == GLUT_KEY_UP) {

        if (current_id > 0) {
            deb_addr = debAddr[ current_id - 1 ];
        } else {
            deb_addr = debAddr[ 0 ] - 1;
        }
    }

    // Кнопка "вниз"
    if (key == GLUT_KEY_DOWN) {

        if (current_id < 45) {
            deb_addr = debAddr[ current_id + 1 ];
        }
    }

    // -----------------------------------------------------------------

    // Постраничник вниз
    if (key == GLUT_KEY_PAGE_DOWN) {
        deb_addr = debAddr[45];
    }

    // Постраничник вверх
    if (key == GLUT_KEY_PAGE_UP) {

        deb_addr -= (debAddr[44] - debAddr[0]);
        deb_addr = deb_addr > 0 ? deb_addr : 0;
    }

}

void keyboard_func_up(int key, int x, int y) {

    if (cpu_running) {

        if (key == 101) Joy1 &= ~0b00010000; // UP
        if (key == 103) Joy1 &= ~0b00100000; // DOWN
        if (key == 100) Joy1 &= ~0b01000000; // LEFT
        if (key == 102) Joy1 &= ~0b10000000; // RIGHT
        return;
    }

}
