/* Имя файла для сохранения */
char savefile[256];

/* Протестировать, что это - nes-файл */
int is_nes_file(char* file) {

    int i, l = 0; while (file[l]) l++;
    for (i = 0; i < l - 3; i++) {
        if (file[i] == '.' && file[i+1] == 'n' && file[i+2] == 'e' && file[i+3] == 's' && file[i+4] == 0) {
            return 1;
        }
    }

    return 0;
}

/* Сохранение дампа */
void save() {
    
    unsigned char regsave[16];
    
    regsave[0] = reg_A;
    regsave[1] = reg_X;
    regsave[2] = reg_Y;
    regsave[3] = reg_P;
    regsave[4] = reg_S;
    regsave[5] = reg_PC >> 8;
    regsave[6] = reg_PC & 0xff;
    
    regsave[7] = ppu_status;
    regsave[8] = ctrl0;
    regsave[9] = ctrl1;
    
    regsave[10] = spraddr;
    regsave[11] = firstWrite;
    regsave[12] = VRAMAddress >> 8;
    regsave[13] = VRAMAddress & 0xff;
    
            
    FILE* f = fopen(savefile, "w+");
    fwrite(sram, 1, 128*1024, f);
    fwrite(regsave, 1, 16, f);
    fwrite(sprite, 1, 256, f);
    fclose(f);    
}

void loadsav() {
    
    unsigned char regsave[16];
               
    FILE* f = fopen(savefile, "r");
    rdsize = fread(sram, 1, 128*1024, f);
    rdsize = fread(regsave, 1, 16, f);
    rdsize = fread(sprite, 1, 256, f);
    fclose(f);  
    
    // ----     
    reg_A = regsave[0];
    reg_X = regsave[1];
    reg_Y = regsave[2];
    reg_P = regsave[3] ;
    reg_S = regsave[4] ;
    reg_PC = regsave[5]*256 + regsave[6];
    
    ppu_status = regsave[7];
    ctrl0 = regsave[8];
    ctrl1 = regsave[9];
    
    spraddr = regsave[10];
    firstWrite = regsave[11];
    VRAMAddress = 256*regsave[12] + regsave[13];
}

// Загрузка NES-файла
// $FFFA – NMI (VBlink)
// $FFFC – RESET
// $FFFE - IRQ/BRK

void load_nes_file(char* file) {

    int size, i = 0;
    
    FILE* f = fopen(file, "rb");    

    if (f == NULL) {
        printf("File %s not found\n", file);
        exit(1);
    }
    
    /* Копирование строки */
    char* tmp = file; while (tmp[i]) { savefile[i] = tmp[i]; i++; };    
    savefile[ i++ ] = '.'; 
    savefile[ i++ ] = 's'; 
    savefile[ i++ ] = 'a';
    savefile[ i++ ] = 'v'; 
    savefile[ i   ] = 0;

    fseek(f, 0, SEEK_END);
    size = ftell(f);

    // 16-кб ROM
    if (size == 0x6010) {

        fseek(f, 0x10, SEEK_SET);
        rdsize = fread(sram + 0xc000, 1, 0x4000, f);  // PRG-ROM(0)
        fseek(f, 0x10, SEEK_SET);
        rdsize = fread(sram + 0x8000, 1, 0x4000, f);  // зеркало

        fseek(f, 0x4010, SEEK_SET);
        rdsize = fread(sram + 0x10000, 1, 0x2000, f); // CHR-ROM (8 кб)

    } else {

        fseek(f, 0x10, SEEK_SET);
        rdsize = fread(sram + 0x8000, 1, 0x8000, f);  // PRG-ROM(0,1)
        fseek(f, 0x8010, SEEK_SET);
        rdsize = fread(sram + 0x10000, 1, 0x2000, f); // CHR-ROM (8 кб)
    }

    // #RESET
    mapper = MAPPER_NES;
    reg_PC = sram[0xFFFC] + (sram[0xFFFD]<<8);

    fclose(f);
}

// Загрузка собственного файла
void load_own_file(char* file) {

    FILE* f = fopen(file, "rb");

    if (f == NULL) {
        printf("File %s not found\n", file);
        exit(1);
    }

    // 16K PRG-ROM(1)/(0)
    rdsize = fread(sram + 0x8000, 1, 32*1024, f);
    fclose(f);

    // Первичное заполнение
    int i, j;
    for (i = 0; i < 1024; i++) {
        sram[0x12000 + i] = 0x00;
    }

    // Копировать системные шрифты
    for (i = 0; i < 256; i++) {

        for (j = 0; j < 8; j++) {
            sram[0x10000 + i*16 + j + 0x00] = sram[0x14000 + i*8 + j];
            sram[0x10000 + i*16 + j + 0x08] = sram[0x14000 + i*8 + j];
        }

    }

    // Инициализация палитры
    for (i = 0; i < 8; i++) {
        
        sram[0x13F00 + i*4] = 0x3F; // Черный цвет
        sram[0x13F01 + i*4] = 0x00; // Серый
        sram[0x13F02 + i*4] = 0x20; // Белый
        sram[0x13F03 + i*4] = 0x10; // Светло-серый
    }
    

    mapper = MAPPER_OWN;
    reg_PC = 0x8000;
}
