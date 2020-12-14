#ifndef KEYBH
#define KEYBH
/*  Описание специальных клавиш
 *
    GLUT_KEY_F1
    GLUT_KEY_F2
    GLUT_KEY_F3
    GLUT_KEY_F4
    GLUT_KEY_F5
    GLUT_KEY_F6
    GLUT_KEY_F7
    GLUT_KEY_F8
    GLUT_KEY_F9
    GLUT_KEY_F1
    GLUT_KEY_F11
    GLUT_KEY_F12
    GLUT_KEY_LEFT   - клавиша стрелка влево
    GLUT_KEY_RIGHT  - клавиша стрелка вправо
    GLUT_KEY_UP     - клавиша стрелка вверх
    GLUT_KEY_DOWN   - клавиша стрелка вниз
    GLUT_KEY_PAGE_UP    - Page Up  клавиша
    GLUT_KEY_PAGE_DOWN  - Page Down  клавиша
    GLUT_KEY_HOME   - клавиша на главнуб( домой )
    GLUT_KEY_END    - клавиша End
    GLUT_KEY_INSERT - клавиша Insert
*/

void keyboard(unsigned char, int, int);
void keyboard_up(unsigned char, int, int);

void keyboard_func(int, int, int);
void keyboard_func_up(int key, int x, int y);

#endif
