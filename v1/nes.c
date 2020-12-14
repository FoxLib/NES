/*
 * Упрощенный графический отладчик 6502 NES модели процессора
 * Лицензия как обычно, никакая
 * 
 * ./debugger [filename [start_load [PC]]]
 */

#include <stdlib.h>
#include <stdio.h>
#include <GL/freeglut.h>

int rdsize;

// Заголовки
#include "fonts.h"
#include "load.h"
#include "display.h"
#include "keyboard.h"
#include "cpu.h"

// Модули
#include "keyboard.c"
#include "display.c"
#include "load.c"
#include "cpu.c"

int main(int argc, char* argv[]) {

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGB | GLUT_DEPTH | GLUT_DOUBLE);
    glutInitWindowSize(WIDTH, HEIGHT);
    glutCreateWindow("NES 6502 EMULATOR");
    glClearColor(0.0f, 0.25f, 0.5f, 1.0f);
    
    fontsLoad();
    initGlobalPal();
    initCPU();

    if (argc > 1) {
        
        // Загрузить файл NES
        if (is_nes_file(argv[1])) {
            load_nes_file(argv[1]);
        } else {
            load_own_file(argv[1]);
        }
        
        deb_top  = reg_PC;
        deb_addr = reg_PC;        
    }    
    
    // http://grafika.me/node/130
    glutKeyboardFunc(& keyboard );
    glutKeyboardUpFunc(& keyboard_up );
    
    glutSpecialFunc(& keyboard_func );
    glutSpecialUpFunc(& keyboard_func_up );

    glutDisplayFunc(& display);
    glutMainLoop();
    return 0;
}
