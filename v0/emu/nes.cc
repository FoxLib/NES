#include "nes.h"

// @doc https://github.com/bfirsh/jsnes/blob/master/src/ppu.js

// Конструктор
NES::NES(int w, int h, const char* caption) {

    int i, j;
    init_sdl(w, h, caption);

    // Инициализация тестовой палитры
    for (int i = 0; i < 32; i++) vram[0x3F00 + i] = (2*i + 3) & 0x3f;

    // Дефолтные
    vram[0x3F00] = 0x0F;
    vram[0x3F01] = 0x00;
    vram[0x3F02] = 0x10;
    vram[0x3F03] = 0x30;

    // Очистить экран буфера
    for (i = 0; i < 240; i++)
    for (j = 0; j < 256; j++) nesdisp[i][j] = 0x000000;
    for (i = 0x2000; i <= 0x23FF; i++) vram[i] = (1231*i) & 255;

    Joy1 = 0;
}

// Деструктор
NES::~NES() { }

// Загрузка NES-файла
void NES::load_nes_file(char* file) {

    // ------------------------
    // $FFFA – NMI (VBlink)
    // $FFFC – RESET
    // $FFFE - IRQ/BRK
    // ------------------------

    int size, i = 0;
    FILE* f = fopen(file, "rb");

    if (f == NULL) {
        printf("File %s not found\n", file);
        exit(1);
    }

    fseek(f, 0, SEEK_END);
    size = ftell(f);

    // Заголовок
    fseek(f, 0x00, SEEK_SET);
    fread(nesheader, 1, 0x10, f);

    // Маппер
    mapper = (nesheader[6] >> 4) | (nesheader[7] & 0xf0);
    prgnum = nesheader[4];

    // 16-кб ROM
    if (size == 0x6010) {

        fseek(f, 0x10, SEEK_SET);
        fread(sram + 0xc000, 1, 0x4000, f);  // PRG-ROM(0)

        fseek(f, 0x10, SEEK_SET);
        fread(sram + 0x8000, 1, 0x4000, f);  // PRG-ROM(1)

        fseek(f, 0x4010, SEEK_SET);
        fread(vram, 1, 0x2000, f); // CHR-ROM (8 кб)

        reg_PC = read_word(0xFFFC);
    }
    // 32-кб ROM
    else if (size == 0x6010) {

        // PRG-ROM(0,1)
        fseek(f, 0x10, SEEK_SET);
        fread(sram + 0x8000, 1, 0x8000, f);

        // CHR-ROM (8 кб)
        fseek(f, 0x8010, SEEK_SET);
        fread(vram, 1, 0x2000, f);

        reg_PC = read_word(0xFFFC);
    }
    // Разный ROM
    else {

        int prgsize = 0x4000 * nesheader[4];

        fseek(f, 0x10, SEEK_SET);
        fread(sram + 0x8000, 1, prgsize, f);

        if (nesheader[5]) {
            fseek(f, 0x10 + prgsize, SEEK_SET);
            fread(vram, 1, 0x2000 * nesheader[5], f);
        }

        // Последние байты использовать
        reg_PC = read_word(0xFFFC);
    }

    ds_start  = reg_PC;
    ds_cursor = reg_PC;

    // Стартовые значения
    ctrl0 = 0x00;
    ctrl1 = 0x08;
    cpu_start = 0;
    rom_bank = 0;

    // Включение отрезкаливания (по обоим направлениям)
    HMirroring = nesheader[6] & 1 ? 1 : 0;
    VMirroring = nesheader[6] & 2 ? 1 : 0;

    fclose(f);
}

// Зеркала для видеопамяти
int NES::vmirror(int addr) {

    // Зеркала палитры
    if ((addr & 0x3F00) == 0x3F00) {
        return (addr & 0x1F) | 0x3F00;
    }

    // Зеркала nametable (4 зеркала)
    else if (addr >= 0x2800) {
        return (addr & 0x7FF) | 0x2000;
    }

    return addr;
}

// Зеркала оперативной памяти
int NES::rmirror(int addr) {

    if (addr >= 0x0800 && addr < 0x2000) {
        return (addr & 0x7FF);
    }

    return addr & 0xffff;
}

// Чтение байта
unsigned char NES::read_byte(int addr) {

    int tmp, olddat, mirr;

    addr &= 0xffff;

    // Джойстик 1
    if (addr == 0x4016) {

        tmp = (Joy1Latch & 1) | 0x40;
        Joy1Latch >>= 1;
        return tmp;
    }

    // Джойстик 2
    if (addr == 0x4017) {
        return 0;
    }

    // Разрешенные адреса
    if (addr >= 0x2000 && addr <= 0x3FFF) {

        switch (addr & 7) {

            // Читать статус
            case 2: {

                tmp         = ppu_status;              // Предыдущее значение
                ppu_status  = ppu_status & 0b00111111; // Сброс при чтении
                first_write = 1;                       // Сброс 2005, 2007 чтения/записи
                return tmp;
            }

            // Читать спрайт
            case 4: {

                tmp = OAM[ VRAMSpriteAddress++ ];
                VRAMSpriteAddress &= 0xff;
                return tmp;
            }

            // Чтение из видеопамяти (кроме STA)
            case 7: {

                mirr = vmirror(VRAMAddress);

                // Читать из регистров палитры
                if (VRAMAddress >= 0x3F00) {

                    // Чтение из любой палитры с индексом & 3 = 0 читает из BG
                    if (mirr >= 0x3F00 && mirr < 0x3F20 && (mirr & 3) == 0) {
                        mirr = 0x3F00;
                    }

                    olddat = vram[ mirr ];


                // Читать с задержкой буфера
                } else {

                    olddat = objvar;
                    objvar = vram[ mirr ];
                }

                VRAMAddress += (ctrl0 & 0x04 ? 32 : 1);
                return olddat;
            }
        }
    }

    switch (mapper) {

        /* UxROM */
        case 2:

            // Самый последний банк подключен к вершине
            if (addr >= 0xc000)
                return sram[ 0x8000 + (addr & 0x3fff) + (prgnum - 1)*16384];

            // Этот банк подключается через запрос маппера
            if (addr >= 0x8000)
                return sram[ 0x8000 + (addr & 0x3fff) + 16384*rom_bank];
    }

    return sram[ rmirror(addr) ];
}

// Запись байта
void NES::write_byte(int addr, unsigned char data) {

    int mirr, i, baseaddr, b1, b2;

    addr &= 0xffff;

    // Переключение банков
    switch (mapper) {

        case 2:

            if (addr >= 0x8000) { rom_bank = data & 0x0F;  }
            break;
    }

    // Не писать сюда
    if (addr >= 0x8000) {
        return;
    }

    // Обновление спрайтов из DMA
    if (addr == 0x4014) {
        for (i = 0; i < 256; i++) {
            OAM[i] = sram[0x100 * data + i];
        }
        return;
    }

    // Геймпад 1
    if (addr == 0x4016) {

        // Защелка: получение данных из Joy1
        if ((Joy1Strobe & 1) == 1 && ((data & 1) == 0)) {
            Joy1Latch = Joy1 | (1 << 19);
        }

        Joy1Strobe = data & 1;
        return;
    }

    // Геймпад 2: Не используется
    if (addr == 0x4017) {
        return;
    }

    // Запись в системные регистры
    if (addr >= 0x2000 && addr <= 0x3FFF) {

        switch (addr & 7) {

            // Контрольные регистры
            case 0: ctrl0 = data; break;
            case 1: ctrl1 = data; break;
            case 3: VRAMSpriteAddress = data; break;

            // Запись в память спрайтов
            case 4: OAM[ VRAMSpriteAddress++ ] = data; VRAMSpriteAddress &= 0xff; break;

            // Скроллинг
            case 5: {

                // Горизонтальный
                if (first_write) {

                    regFH =  data & 7;        // Точный скроллинг по X (Fine Horizontal)
                    regHT = (data >> 3) & 31; // Грубый скроллинг по X (Horizontal Tile)
                }
                // Вертикальный
                else {

                    regFV =  data & 7;        // Точный скроллинг по Y (Fine Vertical)
                    regVT = (data >> 3) & 31; // Грубый скроллинг по Y (Vertical Tile)
                }

                first_write = !first_write;
                break;
            }

            // Запись адреса курсора в память
            case 6: {

                if (first_write) {

                    regFV = (data >> 4) & 3;
                    regV  = (data >> 3) & 1;
                    regH  = (data >> 2) & 1;
                    regVT = (regVT & 7) | ((data & 3) << 3);

                } else {

                    regVT = (regVT & 24) | ((data >> 5) & 7);
                    regHT =  data & 31;

                    // Готовые результаты
                    cntHT = regHT;
                    cntVT = regVT;
                    cntFV = regFV;
                    cntV  = regV;
                    cntH  = regH;
                }

                first_write = !first_write;

                // FE DC BA 98765 43210
                // .. UU VH YYYYY XXXXX

                // Старшие биты
                b1  = (cntFV & 7) << 4; // Верхние 2 бита
                b1 |= ( cntV & 1) << 3; // Экран (2, 3)
                b1 |= ( cntH & 1) << 2; // Экран (0, 1)
                b1 |= (cntVT >> 3) & 3; // Y[4:3]

                // Младшие биты
                b2  = (cntVT & 7) << 5; // Y[2:0]
                b2 |=  cntHT & 31;      // X[4:0]

                VRAMAddress = ((b1 << 8) | b2) & 0x7fff;
                break;
            }

            // Запись данных в видеопамять
            case 7: {

                mirr = vmirror(VRAMAddress);

                // Запись в 3F00 равнозначна 0x3F10
                if (mirr == 0x3F00 || mirr == 0x3F10) {
                    mirr = 0x3F00;
                }

                vram[ mirr ] = data;
                VRAMAddress += (ctrl0 & 0x04 ? 32 : 1);

                // Писать новые счетчики
                // cnts_from_address();
                // regs_from_address();

                break;
            }
        }

    } else {
        sram[ rmirror(addr) ] = data;
    }
}

// Запрос NMI
void NES::request_nmi() {

    // PPU генерирует обратный кадровый импульс (VBlank)
    ppu_status |= 0b10000000;

    // Обновить счетчики
    cntVT = 0;
    cntH  = 0; regH = 0;
    cntV  = 0; regV = 0;

    // Вызвать NMI
    if ((ctrl0 & 0x80) && IF_INTERRUPT())  {

        PUSH((reg_PC >> 8) & 0xff);	     // Вставка обратного адреса в стек
        PUSH(reg_PC & 0xff);
        SET_BREAK(1);                    // Установить BFlag перед вставкой
        reg_P |= 0b00100000;             // 1
        PUSH(reg_P);
        SET_INTERRUPT(1);
        reg_PC = read_word(0xFFFA);
    }
}

// Обновление скроллинговых регистров из адреса RAM
void NES::regs_from_address() {

    int vramTmpAddress = VRAMAddress;
    int address = (vramTmpAddress >> 8) & 0xff;

    regFV = (address >> 4) & 7;
    regV  = (address >> 3) & 1;
    regH  = (address >> 2) & 1;
    regVT = (regVT & 7) | ((address & 3) << 3);

    address = vramTmpAddress & 0xff;
    regVT = (regVT & 24) | ((address >> 5) & 7);
    regHT = address & 31;
};

// Updates the scroll registers from a new VRAM address.
void NES::cnts_from_address() {

    int address = (VRAMAddress >> 8) & 0xff;

    cntFV = (address >> 4) & 3;
    cntV  = (address >> 3) & 1;
    cntH  = (address >> 2) & 1;
    cntVT = (cntVT & 7) | ((address & 3) << 3);

    address = VRAMAddress & 0xff;
    cntVT = (cntVT & 24) | ((address >> 5) & 7);
    cntHT = address & 31;
}

// Регистры во временный адрес
void NES::regs_to_address() {

    int b1 = (regFV & 7) << 4;
    b1 |= (regV & 1) << 3;
    b1 |= (regH & 1) << 2;
    b1 |= (regVT >> 3) & 3;

    int b2 = (regVT & 7) << 5;
    b2 |= regHT & 31;

    VRAMTmpAddress = ((b1 << 8) | b2) & 0x7fff;
}

void NES::cnts_to_address() {

    int b1 = (cntFV & 7) << 4;
    b1 |= (cntV & 1) << 3;
    b1 |= (cntH & 1) << 2;
    b1 |= (cntVT >> 3) & 3;

    int b2 = (cntVT & 7) << 5;
    b2 |= cntHT & 31;

    VRAMAddress = ((b1 << 8) | b2) & 0x7fff;
}

// Исполнение одного фрейма (341 x 262)
void NES::do_frame() {

    int row = 0, cycles = 0;

    // Выполнить 262 строк (1 кадр)
    for (row = 0; row < 262; row++) {

        // Сохранить буферизированные значения. Нужны для отладчика
        vb_HT[ row ] = regHT;
        vb_VT[ row ] = regVT;
        vb_FH[ row ] = regFH;
        vb_FV[ row ] = regFV;

        // Вызвать NMI на обратном синхроимпульсе
        if (row == 241) request_nmi();

        // Сбросить VBLank и Sprite0Hit, активной экранной страницы
        if (row == 261) {

            ctrl0      &= 0b11111100;
            ppu_status &= 0b00111111;
        }

        // Достигнут Sprite0Hit
        if (OAM[0] + 1 == row) {
            ppu_status |= 0b01000000;
        }

        // Выполнить 1 строку (341 такт PPU)
        while (cycles < 341) {

            // Если нет точек остановка, выполнить инструкцию
            if (!breakpoint_test(reg_PC))
                 { cycles += 3 * step(); }
            else { cpu_start = 0; require_stop = 1; break; }

            // Фатальная ошибка от процессора
            if (fatal) { fatal = 0; cpu_start = 0; require_stop = 1; break; }
        }

        // "Кольцо вычета" циклов исполнения
        cycles = cycles % 341;

        // Синхронизация счетчиков
        // regs_to_address();
        // cnts_to_address();

        // Отрисовка линии с тайлами (каждые 8 строк)
        if ((row % 8) == 0 && row < 248) draw_tiles_row(row >> 3);

        // Выход, если процессор остановлен
        if (cpu_start == 0) break;
    }

    // Рисовка спрайтов
    draw_sprites();

    // Обновить экран
    update_screen();
}

// Отрисовать одну линию тайлов
void NES::draw_tiles_row(int i) {

    int xp, yp;
    int j, a, b, ch, fol, foh, at, color, bn;
    int ADDRNT, ADDRPG, ADDRAT;

    // Номер текущего отображаемого банка
    int screen_h = (ctrl0 & 0x01) ? 1 : 0,
        screen_v = (ctrl0 & 0x02) ? 1 : 0;

    // Вычисление скроллинга на основе динамических параметров
    int screen_id  = screen_h ^ screen_v ^ cntH ^ cntV;

    // Номер банка CHR
    int active_chr = (ctrl0 & 0x10) ? 0x1000 : 0x0;

    // 32+1 Нужно дополнительно рисовать 8 пикселей в случае превышения размера
    for (j = 0; j < 33; j++) {

        // Буферизированные значения
        int bCoarseY = vb_VT[ 8*i ], // Грубый скролл X
            bCoarseX = vb_HT[ 8*i ], // Грубый скролл Y
            bFineX   = vb_FH[ 8*i ], // Точный скролл X
            bFineY   = vb_FV[ 8*i ]; // Точный скролл Y

        // -----------------------
        // Выполнить скроллинг Y
        int scroll_y  = (i + bCoarseY);
        int scroll_oy = VMirroring ? (scroll_y >= 0x20) : 0;
            scroll_y  = scroll_y & 0x1F;

        // Выполнить скроллинг X
        int scroll_x  = (j + bCoarseX);
        int scroll_ox = HMirroring ? scroll_x >= 0x20 : 0;
            scroll_x  = scroll_x & 0x1F;
        // -----------------------

        // Активная страница либо 0, либо 1, в зависимости от переполнения еще
        ADDRNT = 0x2000 + (screen_id ^ scroll_ox ^ scroll_oy ? 0x400 : 0x0);

        // Расcчитать позицию на этой странице
        ADDRPG = (0x20*scroll_y + scroll_x);
        ADDRAT = (scroll_y >> 2)*8 + (scroll_x >> 2);

        ch = vram[ ADDRNT + 0x000 + ADDRPG ];
        at = vram[ ADDRNT + 0x3C0 + ADDRAT ];

        // 0 1 Тайлы 4x4
        // 2 3 Каждый 2x2

        // Номер тайлов 2x2: 0,1,2,3
        bn = ((scroll_y & 2) << 1) + (scroll_x & 2);

        // Извлекаем атрибуты
        at = (at >> bn) & 3;

        for (a = 0; a < 8; a++) {

            // Извлечь значения битов цвета
            fol = vram[ ch*16 + a + 0 + active_chr ]; // low
            foh = vram[ ch*16 + a + 8 + active_chr ]; // high

            for (b = 0; b < 8; b++) {

                int s = 1 << (7 - b);

                // Получение 4-х битов цвета фона
                color = (at << 2) | (fol & s ? 1 : 0) | (foh & s ? 2 : 0);

                // Отображается ли фон?
                color = (ctrl1 & 0x08) ? color : 0;

                xp = 8*j + b - bFineX;
                yp = 8*i + a - bFineY;

                // Края не показывать
                if (xp >= 0 && xp < 256 && yp >= 0 && yp < 240) {

                    if (color & 3) {

                        color = vram[ 0x3F00 + color ]; // 16 цветов палитры фона
                        opaque[yp][xp] = 0;

                    } else {

                        color = vram[ 0x3F00 ];         // "Прозрачный" цвет фона
                        opaque[yp][xp] = 1;
                    }

                    // Рисуем цвет палитры в back-буфере и на экране
                    nesdisp[yp][xp] = allpal[color];
                    set_pixel(xp, yp, allpal[color]);
                }
            }
        }
    }
}

// Отрисовка всех спрайтов
void NES::draw_sprites() {

    int i, a, b, color, fol, foh, heicnt;

    if (ctrl1 & 0x10) {

        int h;
        for (i = 0; i < 256; i += 4) {

            int sprite_y = OAM[i + 0];  // По вертикали
            int sprite_x = OAM[i + 3];  // По горизонтали
            int icon     = OAM[i + 1];  // Иконка из знакогенератора
            int attr_spr = OAM[i + 2];  // Атрибут спрайта
            int at       = attr_spr&3;  // Атрибут цвета

            // Не отрисовывать невидимые
            if (sprite_y >= 232 || sprite_y < 8) {
                continue;
            }

            // Выбор знакогенератора спрайтов
            int chrsrc = (ctrl0 & 0x08) ? 0x1000 : 0;
            int heicnt = (ctrl0 & 0x20 ? 2 : 1);

            // Если размер спрайтов = 8x16, то в icon[0] будет bank
            if (heicnt == 2) {

                chrsrc = icon & 1 ? 0x1000 : 0x0000;
                icon  &= 0xFE;
            }

            // 8x8 или 8x16
            for (h = 0; h < heicnt; h++) {
                for (b = 0; b < 8; b++) { // Y
                    for (a = 0; a < 8; a++) { // X

                        // Получение битов из знакогенератора
                        fol = vram[ chrsrc + (icon + h)*16 + b + 0 ]; // low
                        foh = vram[ chrsrc + (icon + h)*16 + b + 8 ]; // high

                        // Расчет X, Y, биты 6 и 7 отвечают за отражение
                        int s = 1 << (7 - a);
                        int x = sprite_x + (attr_spr & 0x40 ? 7 - a : a);
                        int y = sprite_y + (attr_spr & 0x80 ? (heicnt == 2 ? 15 : 7) - b : b) + h*8 + 1;

                        // Вычислить 2 бита цвета спрайта из знакогенератора
                        color = (fol & s ? 1 : 0) | (foh & s ? 2 : 0);

                        // Если у спрайта задан 5-й бит, то, если фон НЕ прозрачный (=0), НЕ рисуем спрайт
                        if ((attr_spr & 0x20) == 0x20 && opaque[y][x] == 0) {
                            continue;
                        }

                        // Отображать пиксель если цвет не 0, и спрайт в пределах экрана
                        if (color && x < 256 && y < 232 && y > 8) {

                            // Формируем цвет из атрибута (2 бита) + 2 бита из знакогенератора
                            // Палитра берется из 3F10

                            color = (4*at) | color;
                            color = vram[ 0x3F10 + color ];
                            color = allpal[ color ];

                            // -------------------------------
                            // Отображать пиксель:
                            // a) Все спрайты видны
                            // b) Не в крайнем левом столбце
                            // -------------------------------

                            if ((ctrl1 & 0b100) || ((ctrl1 & 0b100) == 0 && x >= 8)) {

                                nesdisp[y][x] = color;
                                set_pixel(x, y, color);
                            }
                        }
                    }
                }
            }
        }
    }
}

// Обновить экран
void NES::update_screen() {

    switch (ds_mode) {

        case DS_DEBUGGER:

            update_disassembler();
            break;

        default:

            for (int i = 0; i < 240; i++)
            for (int j = 0; j < 256; j++)
                set_pixel(j, i, nesdisp[i][j]);

            break;
    }
}

// Обновить экран полностью на основе NES структур
void NES::redraw_display() {

    cls(allpal[vram[0x3f00]]); // Очистка в фоновый цвет

    // Рисовать все тайлы
    for (int i = 0; i < 30; i++)
        draw_tiles_row(i);

    // Рисовать спрайты
    draw_sprites();
}

// Процессинг NES-кнопок
void NES::process_key(int keycode) {

    if (cpu_start) {

             if (keycode == 53)  Joy1 |= 0b00000001; // A
        else if (keycode == 52)  Joy1 |= 0b00000010; // B
        else if (keycode == 54)  Joy1 |= 0b00000100; // SELECT
        else if (keycode == 55)  Joy1 |= 0b00001000; // START
        else if (keycode == 111) Joy1 |= 0b00010000; // UP
        else if (keycode == 116) Joy1 |= 0b00100000; // DOWN
        else if (keycode == 113) Joy1 |= 0b01000000; // LEFT
        else if (keycode == 114) Joy1 |= 0b10000000; // RIGHT
    }
}

// Отжатие клавиши
void NES::keyup(int keycode) {

    // Джойстик 1
    if (keycode == 53)       Joy1 &= ~0b00000001; // A
    else if (keycode == 52)  Joy1 &= ~0b00000010; // B
    else if (keycode == 54)  Joy1 &= ~0b00000100; // SELECT
    else if (keycode == 55)  Joy1 &= ~0b00001000; // START
    else if (keycode == 111) Joy1 &= ~0b00010000; // UP
    else if (keycode == 116) Joy1 &= ~0b00100000; // DOWN
    else if (keycode == 113) Joy1 &= ~0b01000000; // LEFT
    else if (keycode == 114) Joy1 &= ~0b10000000; // RIGHT
}
