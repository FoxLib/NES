#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/*
 * Утилита для обделывания разных няшных дел
 * Под "ГНУ Генерала Паблоса Лицокнига".
 */

int printhex(FILE* fp) {

    fseek(fp, 0, SEEK_END);
    int filesize = ftell(fp);
    unsigned char data;
    for (int cursor = 0; cursor < filesize; cursor++) {
        fseek(fp, cursor, SEEK_SET);
        fread(&data, 1, 1, fp);
        printf("%02X\n", data);
    }

    return filesize;
}

int main(int argc, char** argv) {

    if (argc < 2) {

        printf("How to use:\n");
        printf("* prg <filename> - дамп в hex + $8000 в конце\n");
        printf("* hex <filename> - просто вывести дамп\n");
        return 1;
    }

    int is_prg = strcmp(argv[1], "prg") == 0;
    int is_hex = strcmp(argv[1], "hex") == 0;

    // Вывести HEX-дамп с простановкой RST
    if (is_prg || is_hex) {

        if (argc < 3) { printf("Ты забыл имя файла!\n"); return 2; }

        FILE* fp = fopen(argv[2], "rb");
        if (fp) {

            int filesize = printhex(fp);

            // Дописать точку запуска
            if (is_prg) {

                for (int remainder = 0; remainder < 32768 - filesize - 3; remainder++)
                    printf("00\n");

                printf("80\n00\n00\n");
            }
            fclose(fp);

        } else {

            printf("Файл не был найден\n");
            return 1;
        }
    }

    return 0;
}
