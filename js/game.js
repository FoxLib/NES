// Операнды
const
    ___ = 0,
    NDX = 1,  // (b8,X)
    ZP  = 2,  // b8
    IMM = 3,  // #b8
    ABS = 4,  // b16
    NDY = 5,  // (b8),Y
    ZPX = 6,  // b8,X
    ABY = 7,  // b16,Y
    ABX = 8,  // b16,X
    REL = 9,  // b8 (адрес)
    ACC = 10, // A
    IMP = 11, // -- нет --
    ZPY = 12, // b8,Y
    IND = 13; // (b16)

// Инструкции
const         BRK = 1,  ORA = 2,  AND = 3,  EOR = 4,  ADC = 5,  STA = 6,  LDA = 7,
    CMP = 8,  SBC = 9,  BPL = 10, BMI = 11, BVC = 12, BVS = 13, BCC = 14, BCS = 15,
    BNE = 16, BEQ = 17, JSR = 18, RTI = 19, RTS = 20, LDY = 21, CPY = 22, CPX = 23,
    ASL = 24, PHP = 25, CLC = 26, BIT = 27, ROL = 28, PLP = 29, SEC = 30, LSR = 31,
    PHA = 32, PLA = 33, JMP = 34, CLI = 35, ROR = 36, SEI = 37, STY = 38, STX = 39,
    DEY = 40, TXA = 41, TYA = 42, TXS = 43, LDX = 44, TAY = 45, TAX = 46, CLV = 47,
    TSX = 48, DEC = 49, INY = 50, DEX = 51, CLD = 52, INC = 53, INX = 54, NOP = 55,
    SED = 56, AAC = 57, SLO = 58, RLA = 59, RRA = 60, SRE = 61, DCP = 62, ISC = 63,
    LAX = 64, AAX = 65, ASR = 66, ARR = 67, ATX = 68, AXS = 69, XAA = 70, AXA = 71,
    SYA = 72, SXA = 73, DOP = 74;

// Класс для рисования на canvas
class game {

    constructor() {

        this.el     = document.getElementById('viewport');
        this.ctx    = this.el.getContext('2d');
        this.width  = this.el.width;
        this.height = this.el.height;
        this.img    = this.ctx.getImageData(0, 0, this.el.width, this.el.height);

        this.init();
        //this.load("rom/unchained.nes");
        this.load("rom/mario.nes");
    }

    // Инициализация
    init() {

        // Регистры
        this.reg = { a: 0x00, x: 0x00, y: 0x00, s: 0x00, p: 0x00 };
        this.pc  = 0xc000;

        // Инициализация блоков памяти
        this.vram = new Uint8Array(64*1024);  // 64k VIDEO+CHR
        this.ram  = new Uint8Array(512*1024); // 512k RAM+ROM
        this.oam  = new Uint8Array(256);

        this.started  = 0;
        this.rom_bank = 0;
        this.prgnum   = 0;
        this.mapper   = 0;

        this.VRAMAddress    = 0;
        this.VRAMTmpAddress = 0;
        this.VRAMSpriteAddress = 0;

        this.ctrl0 = 0x00;
        this.ctrl1 = 0x00;
        this.first_write = 1;
        this.ppu_status  = 0;
        this.objvar = 0;

        this.regFV = 0; this.cntFV = 0;
        this.regVT = 0; this.cntVT = 0;
        this.regHT = 0; this.cntHT = 0;
        this.regV  = 0; this.cntV  = 0;
        this.regH  = 0; this.cntH  = 0;

        // Буфер
        this.vb_HT = new Uint16Array(512);
        this.vb_VT = new Uint16Array(512);
        this.vb_FH = new Uint16Array(512);
        this.vb_FV = new Uint16Array(512);

        this.Joy1 = 0;
        this.Joy1Latch = 0;
        this.skip_frame = 0;

        // Инициализация тестовой палитры
        for (let i = 0; i < 32; i++) this.vram[0x3F00 + i] = (2*i + 3) & 0x3f;
        for (let i = 0x2000; i <= 0x23FF; i++) this.vram[i] = (1231*i) & 255;

        // Создание маски прозрачности
        this.opaque = [];
        for (let i = 0; i < 256; i++) {
            let m = []; for (let j = 0; j < 256; j++) m.push(0);
            this.opaque.push(m);
        }

        // Регистрация событий
        window.onkeydown = function(e) { this.key_down(e); }.bind(this);
        window.onkeyup   = function(e) { this.key_up(e); }.bind(this);

        // Временные значения
        this.cycles_ext = 0;

        // Палитра
        this.palette  = [
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
        ];

        // Имена инструкции
        this.opcode_names = [

            /*        00  01   02   03   04   05   06   07   08   09   0A   0B   0C   0D   0E   0F */
            /* 00 */ BRK, ORA, ___, SLO, DOP, ORA, ASL, SLO, PHP, ORA, ASL, AAC, DOP, ORA, ASL, SLO,
            /* 10 */ BPL, ORA, ___, SLO, DOP, ORA, ASL, SLO, CLC, ORA, NOP, SLO, DOP, ORA, ASL, SLO,
            /* 20 */ JSR, AND, ___, RLA, BIT, AND, ROL, RLA, PLP, AND, ROL, AAC, BIT, AND, ROL, RLA,
            /* 30 */ BMI, AND, ___, RLA, DOP, AND, ROL, RLA, SEC, AND, NOP, RLA, DOP, AND, ROL, RLA,
            /* 40 */ RTI, EOR, ___, SRE, DOP, EOR, LSR, SRE, PHA, EOR, LSR, ASR, JMP, EOR, LSR, SRE,
            /* 50 */ BVC, EOR, ___, SRE, DOP, EOR, LSR, SRE, CLI, EOR, NOP, SRE, DOP, EOR, LSR, SRE,
            /* 60 */ RTS, ADC, ___, RRA, DOP, ADC, ROR, RRA, PLA, ADC, ROR, ARR, JMP, ADC, ROR, RRA,
            /* 70 */ BVS, ADC, ___, RRA, DOP, ADC, ROR, RRA, SEI, ADC, NOP, RRA, DOP, ADC, ROR, RRA,
            /* 80 */ DOP, STA, DOP, AAX, STY, STA, STX, AAX, DEY, DOP, TXA, XAA, STY, STA, STX, AAX,
            /* 90 */ BCC, STA, ___, AXA, STY, STA, STX, AAX, TYA, STA, TXS, AAX, SYA, STA, SXA, AAX,
            /* A0 */ LDY, LDA, LDX, LAX, LDY, LDA, LDX, LAX, TAY, LDA, TAX, ATX, LDY, LDA, LDX, LAX,
            /* B0 */ BCS, LDA, ___, LAX, LDY, LDA, LDX, LAX, CLV, LDA, TSX, LAX, LDY, LDA, LDX, LAX,
            /* C0 */ CPY, CMP, DOP, DCP, CPY, CMP, DEC, DCP, INY, CMP, DEX, AXS, CPY, CMP, DEC, DCP,
            /* D0 */ BNE, CMP, ___, DCP, DOP, CMP, DEC, DCP, CLD, CMP, NOP, DCP, DOP, CMP, DEC, DCP,
            /* E0 */ CPX, SBC, DOP, ISC, CPX, SBC, INC, ISC, INX, SBC, NOP, SBC, CPX, SBC, INC, ISC,
            /* F0 */ BEQ, SBC, ___, ISC, DOP, SBC, INC, ISC, SED, SBC, NOP, ISC, DOP, SBC, INC, ISC
        ];

        // Типы операндов для каждого опкода
        this.operand_types = [

            /*       00   01   02   03   04   05   06   07   08   09   0A   0B   0C   0D   0E   0F */
            /* 00 */ IMP, NDX, ___, NDX, ZP , ZP , ZP , ZP , IMP, IMM, ACC, IMM, ABS, ABS, ABS, ABS,
            /* 10 */ REL, NDY, ___, NDY, ZPX, ZPX, ZPX, ZPX, IMP, ABY, IMP, ABY, ABX, ABX, ABX, ABX,
            /* 20 */ ABS, NDX, ___, NDX, ZP , ZP , ZP , ZP , IMP, IMM, ACC, IMM, ABS, ABS, ABS, ABS,
            /* 30 */ REL, NDY, ___, NDY, ZPX, ZPX, ZPX, ZPX, IMP, ABY, IMP, ABY, ABX, ABX, ABX, ABX,
            /* 40 */ IMP, NDX, ___, NDX, ZP , ZP , ZP , ZP , IMP, IMM, ACC, IMM, ABS, ABS, ABS, ABS,
            /* 50 */ REL, NDY, ___, NDY, ZPX, ZPX, ZPX, ZPX, IMP, ABY, IMP, ABY, ABX, ABX, ABX, ABX,
            /* 60 */ IMP, NDX, ___, NDX, ZP , ZP , ZP , ZP , IMP, IMM, ACC, IMM, IND, ABS, ABS, ABS,
            /* 70 */ REL, NDY, ___, NDY, ZPX, ZPX, ZPX, ZPX, IMP, ABY, IMP, ABY, ABX, ABX, ABX, ABX,
            /* 80 */ IMM, NDX, IMM, NDX, ZP , ZP , ZP , ZP , IMP, IMM, IMP, IMM, ABS, ABS, ABS, ABS,
            /* 90 */ REL, NDY, ___, NDY, ZPX, ZPX, ZPY, ZPY, IMP, ABY, IMP, ABY, ABX, ABX, ABY, ABX,
            /* A0 */ IMM, NDX, IMM, NDX, ZP , ZP , ZP , ZP , IMP, IMM, IMP, IMM, ABS, ABS, ABS, ABS,
            /* B0 */ REL, NDY, ___, NDY, ZPX, ZPX, ZPY, ZPY, IMP, ABY, IMP, ABY, ABX, ABX, ABY, ABY,
            /* C0 */ IMM, NDX, IMM, NDX, ZP , ZP , ZP , ZP , IMP, IMM, IMP, IMM, ABS, ABS, ABS, ABS,
            /* D0 */ REL, NDY, ___, NDY, ZPX, ZPX, ZPX, ZPX, IMP, ABY, IMP, ABY, ABX, ABX, ABX, ABX,
            /* E0 */ IMM, NDX, IMM, NDX, ZP , ZP , ZP , ZP , IMP, IMM, IMP, IMM, ABS, ABS, ABS, ABS,
            /* F0 */ REL, NDY, ___, NDY, ZPX, ZPX, ZPX, ZPX, IMP, ABY, IMP, ABY, ABX, ABX, ABX, ABX
        ];

        // Количество циклов на опкод
        this.cycles_basic= [

              7, 6, 2, 8, 3, 3, 5, 5, 3, 2, 2, 2, 4, 4, 6, 6,
              2, 5, 2, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7,
              6, 6, 2, 8, 3, 3, 5, 5, 4, 2, 2, 2, 4, 4, 6, 6,
              2, 5, 2, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7,
              6, 6, 2, 8, 3, 3, 5, 5, 3, 2, 2, 2, 3, 4, 6, 6,
              2, 5, 2, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7,
              6, 6, 2, 8, 3, 3, 5, 5, 4, 2, 2, 2, 5, 4, 6, 6,
              2, 5, 2, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7,
              2, 6, 2, 6, 3, 3, 3, 3, 2, 2, 2, 2, 4, 4, 4, 4,
              2, 6, 2, 6, 4, 4, 4, 4, 2, 5, 2, 5, 5, 5, 5, 5,
              2, 6, 2, 6, 3, 3, 3, 3, 2, 2, 2, 2, 4, 4, 4, 4,
              2, 5, 2, 5, 4, 4, 4, 4, 2, 4, 2, 4, 4, 4, 4, 4,
              2, 6, 2, 8, 3, 3, 5, 5, 2, 2, 2, 2, 4, 4, 6, 6,
              2, 5, 2, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7,
              2, 6, 3, 8, 3, 3, 5, 5, 2, 2, 2, 2, 4, 4, 6, 6,
              2, 5, 2, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7
        ];
    }

    // -----------------------------------------------------------------

    // Нажатие на клавишу
    key_down(e) {

        let kk = e.key, kc = parseInt(e.keyCode);

        if (kc === 88 /* X */)        this.Joy1 |= 0b00000001; // A
        else if (kc === 90 /* Z */)   this.Joy1 |= 0b00000010; // B
        else if (kc === 67 /* C */)   this.Joy1 |= 0b00000100; // SELECT
        else if (kc === 86 /* V */)   this.Joy1 |= 0b00001000; // START
        else if (kk === 'ArrowUp')    this.Joy1 |= 0b00010000; // UP
        else if (kk === 'ArrowDown')  this.Joy1 |= 0b00100000; // DOWN
        else if (kk === 'ArrowLeft')  this.Joy1 |= 0b01000000; // LEFT
        else if (kk === 'ArrowRight') this.Joy1 |= 0b10000000; // RIGHT
    }

    // Клавиша отпущена
    key_up(e) {

        let kk = e.key, kc = parseInt(e.keyCode);

        if (kc === 88 /* X */)        this.Joy1 &= ~0b00000001; // A
        else if (kc === 90 /* Z */)   this.Joy1 &= ~0b00000010; // B
        else if (kc === 67 /* C */)   this.Joy1 &= ~0b00000100; // SELECT
        else if (kc === 86 /* V */)   this.Joy1 &= ~0b00001000; // START
        else if (kk === 'ArrowUp')    this.Joy1 &= ~0b00010000; // UP
        else if (kk === 'ArrowDown')  this.Joy1 &= ~0b00100000; // DOWN
        else if (kk === 'ArrowLeft')  this.Joy1 &= ~0b01000000; // LEFT
        else if (kk === 'ArrowRight') this.Joy1 &= ~0b10000000; // RIGHT
    }

    // Загрузка NES-файла
    load(url) {

        let xhr = new XMLHttpRequest();

        xhr.open("GET", url, {async: false});
        xhr.responseType = "arraybuffer";
        xhr.send();
        xhr.onload = function() {

            if (xhr.status !== 200) {

                alert(`Ошибка ${xhr.status}: ${xhr.statusText}`);

            } else {

                let hd   = new Uint8Array(xhr.response);
                let size = parseInt(hd.length);

                // Маппер
                this.mapper = (hd[6] >> 4) | (hd[7] & 0xf0);
                this.prgnum = hd[4];

                // 16-кб ROM
                // -----------------------------------------------------
                if (size === 0x6010) {

                    // PRG-ROM(0|1)
                    for (let i = 0; i < 0x4000; i++) {
                        this.ram[0x8000 + i] = hd[0x10 + i];
                        this.ram[0xc000 + i] = hd[0x10 + i];
                    }

                    // CHR-ROM (8 кб)
                    for (let i = 0; i < 0x2000; i++)
                        this.vram[i] = hd[0x4010 + i];
                }
                // 32-кб ROM
                // -----------------------------------------------------
                else if (size === 0xA010) {

                    // PRG-ROM(0,1)
                    for (let i = 0; i < 0x8000; i++)
                        this.ram[0x8000 + i] = hd[0x10 + i];

                    // CHR-ROM (8 кб)
                    for (let i = 0; i < 0x2000; i++)
                        this.vram[i] = hd[0x8010 + i];
                }
                // -----------------------------------------------------
                else {

                    let prgsize = 0x4000 * hd[4];

                    for (let i = 0; i < prgsize; i++)
                        this.ram[0x8000 + i] = hd[0x10 + i];

                    if (hd[5]) {

                        for (let i = 0; i < 0x2000 * hd[5]; i++)
                            this.vram[i] = hd[0x10 + prgsize + i];
                    }
                }

                this.pc = this.readw(0xFFFC);

                // Стартовые значения
                this.ctrl0 = 0x00;
                this.ctrl1 = 0x08;
                this.rom_bank = 0;

                // Включение отрезкаливания (по обоим направлениям)
                this.HMirroring = hd[6] & 1 ? 1 : 0;
                this.VMirroring = hd[6] & 2 ? 1 : 0;
            }

            if (this.started === 0) {
                this.started = 1;
                this.frame();
            }

        }.bind(this);
    }

    // Чтение байта
    read(addr) {

        let tmp, olddat, mirr;

        addr &= 0xffff;

        // Джойстик 1
        if (addr == 0x4016) {

            tmp = (this.Joy1Latch & 1) | 0x40;
            this.Joy1Latch >>= 1;
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

                    tmp = this.ppu_status;  // Предыдущее значение
                    this.ppu_status  = this.ppu_status & 0b00111111; // Сброс при чтении
                    this.first_write = 1; // Сброс 2005, 2007 чтения/записи
                    return tmp;
                }

                // Читать спрайт
                case 4: {

                    tmp = this.oam[ this.VRAMSpriteAddress++ ];
                    this.VRAMSpriteAddress &= 0xff;
                    return tmp;
                }

                // Чтение из видеопамяти (кроме STA)
                case 7: {

                    mirr = this.vmirror(this.VRAMAddress);

                    // Читать из регистров палитры
                    if (this.VRAMAddress >= 0x3F00) {

                        // Чтение из любой палитры с индексом & 3 = 0 читает из BG
                        if (mirr >= 0x3F00 && mirr < 0x3F20 && (mirr & 3) == 0) {
                            mirr = 0x3F00;
                        }

                        olddat = this.vram[ mirr ];

                    // Читать с задержкой буфера
                    } else {

                        olddat      = this.objvar;
                        this.objvar = this.vram[ mirr ];
                    }

                    // Инкремент в зависимости от выбранной стратегий
                    this.VRAMAddress += (this.ctrl0 & 0x04 ? 32 : 1);
                    return olddat;
                }
            }
        }

        switch (this.mapper) {

            /* UxROM */
            case 2:

                // Самый последний банк подключен к вершине
                if (addr >= 0xc000)
                    return this.ram[ 0x8000 + (addr & 0x3fff) + (this.prgnum - 1)*16384];

                if (addr >= 0x8000) // Этот банк подключается через запрос маппера
                    return this.ram[ 0x8000 + (addr & 0x3fff) + 16384*this.rom_bank];
        }

        return this.ram[ this.rmirror(addr) ];
    }

    // Чтение слова
    readw(addr) {

        let l = this.read(addr);
        let h = this.read(addr+1);
        return 256*h + l;
    }

    // Запись байта
    write(addr, data) {

        let mirr, i, baseaddr, b1, b2;

        addr &= 0xffff;

        // Переключение банков
        switch (this.mapper) {

            case 2:

                if (addr >= 0x8000) { this.rom_bank = data & 0x0F;  }
                break;
        }

        // Не писать сюда
        if (addr >= 0x8000) {
            return;
        }

        // Обновление спрайтов из DMA
        if (addr == 0x4014) {
            for (i = 0; i < 256; i++) {
                this.oam[i] = this.ram[0x100 * data + i];
            }
            return;
        }

        // Геймпад 1
        if (addr == 0x4016) {

            // Защелка: получение данных из Joy1
            if ((this.Joy1Strobe & 1) == 1 && ((data & 1) == 0)) {
                this.Joy1Latch = this.Joy1 | (1 << 19);
            }

            this.Joy1Strobe = data & 1;
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
                case 0: this.ctrl0 = data; break;
                case 1: this.ctrl1 = data; break;

                // Адрес спрайтов
                case 3: this.VRAMSpriteAddress = data; break;

                // Запись в память спрайтов
                case 4:

                    this.oam[ this.VRAMSpriteAddress++ ] = data;
                    this.VRAMSpriteAddress &= 0xff;
                    break;

                // Скроллинг
                case 5: {

                    // Горизонтальный
                    if (this.first_write) {

                        this.regFH =  data & 7;        // Точный скроллинг по X (Fine Horizontal)
                        this.regHT = (data >> 3) & 31; // Грубый скроллинг по X (Horizontal Tile)
                    }
                    // Вертикальный
                    else {

                        this.regFV =  data & 7;        // Точный скроллинг по Y (Fine Vertical)
                        this.regVT = (data >> 3) & 31; // Грубый скроллинг по Y (Vertical Tile)
                    }

                    this.first_write = !this.first_write;
                    break;
                }

                // Запись адреса курсора в память
                case 6: {

                    if (this.first_write) {

                        this.regFV = (data >> 4) & 3;
                        this.regV  = (data >> 3) & 1;
                        this.regH  = (data >> 2) & 1;
                        this.regVT = (this.regVT & 7) | ((data & 3) << 3);

                    } else {

                        this.regVT = (this.regVT & 24) | ((data >> 5) & 7);
                        this.regHT =  data & 31;

                        // Готовые результаты
                        this.cntHT = this.regHT;
                        this.cntVT = this.regVT;
                        this.cntFV = this.regFV;
                        this.cntV  = this.regV;
                        this.cntH  = this.regH;
                    }

                    this.first_write = !this.first_write;

                    // FE DC BA 98765 43210
                    // .. UU VH YYYYY XXXXX

                    // Старшие биты
                    b1  = (this.cntFV & 7) << 4; // Верхние 2 бита
                    b1 |= ( this.cntV & 1) << 3; // Экран (2, 3)
                    b1 |= ( this.cntH & 1) << 2; // Экран (0, 1)
                    b1 |= (this.cntVT >> 3) & 3; // Y[4:3]

                    // Младшие биты
                    b2  = (this.cntVT & 7) << 5; // Y[2:0]
                    b2 |=  this.cntHT & 31;      // X[4:0]

                    this.VRAMAddress = ((b1 << 8) | b2) & 0x7fff;
                    break;
                }

                // Запись данных в видеопамять
                case 7: {

                    mirr = this.vmirror(this.VRAMAddress);

                    // Запись в 3F00 равнозначна 0x3F10
                    if (mirr == 0x3F00 || mirr == 0x3F10) {
                        mirr = 0x3F00;
                    }

                    this.vram[ mirr ] = data;
                    this.VRAMAddress += (this.ctrl0 & 0x04 ? 32 : 1);

                    // Писать новые счетчики
                    // this.cnts_from_address();
                    // this.regs_from_address();

                    break;
                }
            }

        } else {
            this.ram[ this.rmirror(addr) ] = data;
        }
    }

    // Установка флагов
    set_zero(x)      { this.reg.p = x ? (this.reg.p & 0xFD) : (this.reg.p | 0x02); }
    set_overflow(x)  { this.reg.p = x ? (this.reg.p | 0x40) : (this.reg.p & 0xBF); }
    set_carry(x)     { this.reg.p = x ? (this.reg.p | 0x01) : (this.reg.p & 0xFE); }
    set_decimal(x)   { this.reg.p = x ? (this.reg.p | 0x08) : (this.reg.p & 0xF7); }
    set_break(x)     { this.reg.p = x ? (this.reg.p | 0x10) : (this.reg.p & 0xEF); }
    set_interrupt(x) { this.reg.p = x ? (this.reg.p | 0x04) : (this.reg.p & 0xFB); }
    set_sign(x)      { this.reg.p = !!(x & 0x80) ? (this.reg.p | 0x80) : (this.reg.p & 0x7F); };

    // Получение значений флагов
    if_carry()      { return !!(this.reg.p & 0x01); }
    if_zero()       { return !!(this.reg.p & 0x02); }
    if_interrupt()  { return !!(this.reg.p & 0x04); }
    if_overflow()   { return !!(this.reg.p & 0x40); }
    if_sign()       { return !!(this.reg.p & 0x80); }

    // Работа со стеком
    push(x) { this.write(0x100 + this.reg.s, x & 0xff); this.reg.s = ((this.reg.s - 1) & 0xff); }
    pull()  { this.reg.s = (this.reg.s + 1) & 0xff; return this.read(0x100 + this.reg.s); }

    // Получение эффективного адреса
    effective(addr) {

        let opcode, iaddr;
        let tmp, rt, pt;

        // Чтение опкода
        opcode = this.read(addr++);

        // Чтобы адрес не вышел за пределы
        addr &= 0xffff;

        // Разобрать операнд
        switch (this.operand_types[ opcode ]) {

            // PEEK( PEEK( (arg + X) % 256) + PEEK((arg + X + 1) % 256) * 256
            // Indirect, X (b8,X)
            case NDX: {

                tmp = this.read( addr );
                tmp = (tmp + this.reg.x) & 0xff;
                return this.read( tmp ) + ((this.read((1 + tmp) & 0xff) << 8));
            }

            // Indirect, Y (b8),Y
            case NDY: {

                tmp = this.read(addr);
                rt  = this.read(0xff & tmp);
                rt |= this.read(0xff & (tmp + 1)) << 8;
                pt  = rt;
                rt  = (rt + this.reg.y) & 0xffff;

                if ((pt & 0xff00) != (rt & 0xff00))
                    this.cycles_ext++;

                return rt;
            }

            // Zero Page
            case ZP:  return this.read( addr );

            // Zero Page, X
            case ZPX: return (this.read(addr) + this.reg.x) & 0x00ff;

            // Zero Page, Y
            case ZPY: return (this.read(addr) + this.reg.y) & 0x00ff;

            // Absolute
            case ABS: return this.readw(addr);

            // Absolute, X
            case ABX: {

                pt = this.readw(addr);
                rt = pt + this.reg.x;

                if ((pt & 0xff00) != (rt & 0xff00))
                    this.cycles_ext++;

                return rt & 0xffff;
            }

            // Absolute, Y
            case ABY: {

                pt = this.readw(addr);
                rt = pt + this.reg.y;

                if ((pt & 0xff00) != (rt & 0xff00))
                    this.cycles_ext++;

                return rt & 0xffff;
            }

            // Indirect
            case IND: {

                addr  = this.readw(addr);
                iaddr = this.read(addr) + 256*this.read((addr & 0xFF00) + ((addr + 1) & 0x00FF));
                return iaddr;
            }

            // Relative
            case REL: {

                iaddr = this.read(addr);
                return (iaddr + addr + 1 + (iaddr < 128 ? 0 : -256)) & 0xffff;
            }
        }

        return -1;
    }

    // Вычисление количества cycles для branch
    branch(addr, iaddr) {

        if ((addr & 0xff00) != (iaddr & 0xff00))
            return 2;

        return 1;
    }

    // Запрос NMI
    request_nmi() {

        // PPU генерирует обратный кадровый импульс (VBlank)
        this.ppu_status |= 0b10000000;

        // Обновить счетчики
        this.cntVT = 0;
        this.cntH  = 0; this.regH = 0;
        this.cntV  = 0; this.regV = 0;

        // Вызвать NMI: Флаг IF не играет роли, только CTRL0
        if (this.ctrl0 & 0x80)  {

            this.push((this.pc >> 8) & 0xff);      // Вставка обратного адреса в стек
            this.push(this.pc & 0xff);
            this.set_break(1);                     // Установить BFlag перед вставкой
            this.reg.p |= 0b00100000;              // 1
            this.push(this.reg.p);
            this.set_interrupt(1);
            this.pc = this.readw(0xFFFA);
        }
    }

    // Зеркала для видеопамяти
    vmirror(addr) {

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
    rmirror(addr) {

        if (addr >= 0x0800 && addr < 0x2000) {
            return (addr & 0x7FF);
        }

        return addr & 0xffff;
    }

    // Вызов прерывания
    brk() {

        this.push((this.pc >> 8) & 0xff);     /* Вставка обратного адреса в стек */
        this.push(this.pc & 0xff);
        this.set_break(1);                    /* Установить BFlag перед вставкой */
        this.reg.p |= 0b00100000;             /* 1 */
        this.push(this.reg.p);
        this.set_interrupt(1);
    }

    // Рисование пикселя на экране
    pset(x, y, k) {

        let p = 4*(x + y * this.width);

        if (x < this.width && y < this.height) {
            this.img.data[p    ] =  (k >> 16) & 0xff;
            this.img.data[p + 1] =  (k >>  8) & 0xff;
            this.img.data[p + 2] =  (k      ) & 0xff;
            this.img.data[p + 3] = ((k >> 24) & 0xff) ^ 0xff;
        }
    }

    // Обновление экрана
    flush() { this.ctx.putImageData(this.img, 0, 0); }

    // Нарисовать один фрейм
    frame() {

        let row = 0, cycles = 0;
        let time = (new Date()).getTime();

        // Выполнить 262 строк (1 кадр)
        for (row = 0; row < 262; row++) {

            // Сохранить буферизированные значения. Нужны для отладчика
            this.vb_HT[ row ] = this.regHT;
            this.vb_VT[ row ] = this.regVT;
            this.vb_FH[ row ] = this.regFH;
            this.vb_FV[ row ] = this.regFV;

            // Вызвать NMI на обратном синхроимпульсе
            if (row == 241) this.request_nmi();

            // Сбросить VBLank и Sprite0Hit, активной экранной страницы
            if (row == 261) {

                this.ctrl0      &= 0b11111100;
                this.ppu_status &= 0b00111111;
            }

            // Достигнут Sprite0Hit
            if (this.oam[0] + 1 == row) {
                this.ppu_status |= 0b01000000;
            }

            // Выполнить 1 строку (341 такт PPU)
            while (cycles < 341) cycles += 3 * this.step();

            // "Кольцо вычета" циклов исполнения
            cycles = cycles % 341;

            // Отрисовка линии с тайлами (каждые 8 строк)
            if ((row % 8) == 0 && row < 248) this.tiles_row(row >> 3);
        }

        // Рисовка спрайтов
        this.draw_sprites();
        this.flush();
        this.next_frame((new Date()).getTime() - time);
    }

    // Вычислить, когда отрисовать следующий фрейм
    next_frame(time) {

        let tout = 0;
        const duty = 20;

        // Наверстать упущенные фреймы
        if (this.skip_frame) {
            this.skip_frame--;
            tout = 1;
        }
        // Фрейм выполнился быстро
        else if (time < duty) { // 20
            tout = duty - time;
        }
        // Этот фрейм работал долго
        else {
            tout = 1;
            this.skip_frame++;
        }

        // Отослать запрос
        setTimeout(function() { this.frame() }.bind(this), tout);
    }

    // Отрисовать одну линию тайлов
    tiles_row(i) {

        let xp, yp;
        let j, a, b, ch, fol, foh, at, color, bn;
        let ADDRNT, ADDRPG, ADDRAT;

        // Номер текущего отображаемого банка
        let screen_h = (this.ctrl0 & 0x01) ? 1 : 0,
            screen_v = (this.ctrl0 & 0x02) ? 1 : 0;

        // Вычисление скроллинга на основе динамических параметров
        let screen_id  = screen_h ^ screen_v ^ this.cntH ^ this.cntV;

        // Номер банка CHR
        let active_chr = (this.ctrl0 & 0x10) ? 0x1000 : 0x0;

        // 32+1 Нужно дополнительно рисовать 8 пикселей в случае превышения размера
        for (j = 0; j < 33; j++) {

            // Буферизированные значения
            let bCoarseY = this.vb_VT[ 8*i ], // Грубый скролл X
                bCoarseX = this.vb_HT[ 8*i ], // Грубый скролл Y
                bFineX   = this.vb_FH[ 8*i ], // Точный скролл X
                bFineY   = this.vb_FV[ 8*i ]; // Точный скролл Y

            // -----------------------
            // Выполнить скроллинг Y
            let scroll_y  = (i + bCoarseY);
            let scroll_oy = this.VMirroring ? (scroll_y >= 0x20) : 0;
                scroll_y  = scroll_y & 0x1F;

            // Выполнить скроллинг X
            let scroll_x  = (j + bCoarseX);
            let scroll_ox = this.HMirroring ? scroll_x >= 0x20 : 0;
                scroll_x  = scroll_x & 0x1F;
            // -----------------------

            // Активная страница либо 0, либо 1, в зависимости от переполнения еще
            ADDRNT = 0x2000 + (screen_id ^ scroll_ox ^ scroll_oy ? 0x400 : 0x0);

            // Расcчитать позицию на этой странице
            ADDRPG = (0x20*scroll_y + scroll_x);
            ADDRAT = (scroll_y >> 2)*8 + (scroll_x >> 2);

            ch = this.vram[ ADDRNT + 0x000 + ADDRPG ];
            at = this.vram[ ADDRNT + 0x3C0 + ADDRAT ];

            // 0 1 Тайлы 4x4
            // 2 3 Каждый 2x2

            // Номер тайлов 2x2: 0,1,2,3
            bn = ((scroll_y & 2) << 1) + (scroll_x & 2);

            // Извлекаем атрибуты
            at = (at >> bn) & 3;

            for (a = 0; a < 8; a++) {

                // Извлечь значения битов цвета
                fol = this.vram[ ch*16 + a + 0 + active_chr ]; // low
                foh = this.vram[ ch*16 + a + 8 + active_chr ]; // high

                for (b = 0; b < 8; b++) {

                    let s = 1 << (7 - b);

                    // Получение 4-х битов цвета фона
                    color = (at << 2) | (fol & s ? 1 : 0) | (foh & s ? 2 : 0);

                    // Отображается ли фон?
                    color = (this.ctrl1 & 0x08) ? color : 0;

                    xp = 8*j + b - bFineX;
                    yp = 8*i + a - bFineY;

                    // Края не показывать
                    if (xp >= 0 && xp < 256 && yp >= 0 && yp < 240) {

                        if (color & 3) {

                            color = this.vram[ 0x3F00 + (color & 0x0F) ]; // 16 цветов палитры фона
                            this.opaque[yp][xp] = 0;

                        } else {

                            color = this.vram[ 0x3F00 ];         // "Прозрачный" цвет фона
                            this.opaque[yp][xp] = 1;
                        }

                        // Рисуем цвет палитры в back-буфере и на экране
                        this.pset(xp, yp, this.palette[color]);
                    }
                }
            }
        }
    }

    // Отрисовка всех спрайтов
    draw_sprites() {

        let i, a, b, color, fol, foh;

        if (this.ctrl1 & 0x10) {

            let h;
            for (i = 0; i < 256; i += 4) {

                let sprite_y = this.oam[i + 0];  // По вертикали
                let sprite_x = this.oam[i + 3];  // По горизонтали
                let icon     = this.oam[i + 1];  // Иконка из знакогенератора
                let attr_spr = this.oam[i + 2];  // Атрибут спрайта
                let at       = attr_spr & 3;     // Атрибут цвета

                // Не отрисовывать невидимые
                if (sprite_y >= 232 || sprite_y < 8)
                    continue;

                // Выбор знакогенератора спрайтов
                let chrsrc = (this.ctrl0 & 0x08) ? 0x1000 : 0;
                let height = (this.ctrl0 & 0x20) ? 2 : 1;

                // Если размер спрайтов = 8x16, то в icon[0] будет bank
                if (height == 2) {

                    chrsrc = icon & 1 ? 0x1000 : 0x0000;
                    icon  &= 0xFE;
                }

                // 8x8 или 8x16
                for (h = 0; h < height; h++)
                for (b = 0; b < 8; b++)   // Y
                for (a = 0; a < 8; a++) { // X

                    // Получение битов из знакогенератора
                    fol = this.vram[ chrsrc + (icon + h)*16 + b + 0 ]; // low
                    foh = this.vram[ chrsrc + (icon + h)*16 + b + 8 ]; // high

                    // Расчет X, Y, биты 6 и 7 отвечают за отражение
                    let s = 1 << (7 - a);
                    let x = sprite_x + (attr_spr & 0x40 ? 7 - a : a);
                    let y = sprite_y + (attr_spr & 0x80 ? (height == 2 ? 15 : 7) - b : b) + h*8 + 1;

                    // Вычислить 2 бита цвета спрайта из знакогенератора
                    color = (fol & s ? 1 : 0) | (foh & s ? 2 : 0);

                    // Если у спрайта задан 5-й бит, то, если фон НЕ прозрачный (=0), НЕ рисуем спрайт
                    if ((attr_spr & 0x20) == 0x20 && this.opaque[y][x] == 0) {
                        continue;
                    }

                    // Отображать пиксель если цвет не 0, и спрайт в пределах экрана
                    if (color && x < 256 && y < 232 && y > 8) {

                        // Формируем цвет из атрибута (2 бита) + 2 бита из знакогенератора
                        // Палитра берется из 3F10

                        color = (4*at) | color;
                        color = this.vram[ 0x3F10 + (color & 0x0F) ];
                        color = this.palette[ color ];

                        // -------------------------------
                        // Отображать пиксель:
                        // a) Все спрайты видны
                        // b) Не в крайнем левом столбце
                        // -------------------------------

                        if ((this.ctrl1 & 0b100) || ((this.ctrl1 & 0b100) == 0 && x >= 8)) {
                            this.pset(x, y, color);
                        }
                    }
                }
            }
        }
    }

    // Исполнение шага инструкции
    step() {

        let temp, optype, opname, ppurd = 1, src = 0;
        let addr = this.pc, opcode;
        let cycles_per_instr = 2;

        // Доп. циклы разбора адреса
        this.cycles_ext = 0;

        // Определение эффективного адреса
        let iaddr = this.effective(addr);

        // Прочесть информацию по опкодам
        opcode = this.read(addr);
        optype = this.operand_types[ opcode ];
        opname = this.opcode_names [ opcode ];

        // Эти инструкции НЕ ДОЛЖНЫ читать что-либо из памяти перед записью
        if (opname == STA || opname == STX || opname == STY) {
            ppurd = 0;
        }

        // Инкремент адреса при чтении опкода
        addr = (addr + 1) & 0xffff;

        // Базовые циклы + доп. циклы
        cycles_per_instr = this.cycles_basic[ opcode ] + this.cycles_ext;

        // --------------------------------
        // Чтение операнда из памяти
        // --------------------------------

        switch (optype) {

            case ___: printf("Opcode %02x error at %04x\n", opcode, this.pc); return;
            case NDX: // Indirect X (b8,X)
            case NDY: // Indirect, Y
            case ZP:  // Zero Page
            case ZPX: // Zero Page, X
            case ZPY: // Zero Page, Y
            case REL: // Relative

                addr = (addr + 1) & 0xffff;
                if (ppurd) src = this.read( iaddr );
                break;

            case ABS: // Absolute
            case ABX: // Absolute, X
            case ABY: // Absolute, Y
            case IND: // Indirect

                addr = (addr + 2) & 0xffff;
                if (ppurd) src = this.read( iaddr );
                break;

            case IMM: // Immediate

                if (ppurd) src = this.read( addr );
                addr = (addr + 1) & 0xffff;
                break;

            case ACC: // Accumulator source

                src = this.reg.a;
                break;
        }

        // --------------------------------
        // Разбор инструкции и исполнение
        // --------------------------------

        switch (opname) {

            // Сложение с учетом переноса
            case ADC: {

                temp = src + this.reg.a + (this.reg.p & 1);
                this.set_zero(temp & 0xff);
                this.set_sign(temp);
                this.set_overflow(((this.reg.a ^ src ^ 0x80) & 0x80) && ((this.reg.a ^ temp) & 0x80) );
                this.set_carry(temp > 0xff);
                this.reg.a = temp & 0xff;
                break;
            }

            // Логическое умножение
            case AND: {

                src &= this.reg.a;
                this.set_sign(src);
                this.set_zero(src);
                this.reg.a = src;
                break;
            }

            // Логический сдвиг вправо
            case ASL: {

                this.set_carry(src & 0x80);

                src <<= 1;
                src &= 0xff;
                this.set_sign(src);
                this.set_zero(src);

                if (optype == ACC) this.reg.a = src; else this.write(iaddr, src);
                break;
            }

            // Условные переходы
            case BCC: if (!this.if_carry())    { cycles_per_instr += this.branch(addr, iaddr); addr = iaddr; } break;
            case BCS: if ( this.if_carry())    { cycles_per_instr += this.branch(addr, iaddr); addr = iaddr; } break;
            case BNE: if (!this.if_zero())     { cycles_per_instr += this.branch(addr, iaddr); addr = iaddr; } break;
            case BEQ: if ( this.if_zero())     { cycles_per_instr += this.branch(addr, iaddr); addr = iaddr; } break;
            case BPL: if (!this.if_sign())     { cycles_per_instr += this.branch(addr, iaddr); addr = iaddr; } break;
            case BMI: if ( this.if_sign())     { cycles_per_instr += this.branch(addr, iaddr); addr = iaddr; } break;
            case BVC: if (!this.if_overflow()) { cycles_per_instr += this.branch(addr, iaddr); addr = iaddr; } break;
            case BVS: if ( this.if_overflow()) { cycles_per_instr += this.branch(addr, iaddr); addr = iaddr; } break;

            // Копировать бит 6 в OVERFLOW флаг
            case BIT: {

                this.set_sign(src);
                this.set_overflow(0x40 & src);
                this.set_zero(src & this.reg.a);
                break;
            }

            // Программное прерывание
            case BRK: {

                this.pc = (this.pc + 2) & 0xffff;
                this.brk();
                addr = this.readw(0xFFFE);
                break;
            }

            /* Флаги */
            case CLC: this.set_carry(0);     break;
            case SEC: this.set_carry(1);     break;
            case CLD: this.set_decimal(0);   break;
            case SED: this.set_decimal(1);   break;
            case CLI: this.set_interrupt(0); break;
            case SEI: this.set_interrupt(1); break;
            case CLV: this.set_overflow(0);  break;

            /* Сравнение A, X, Y с операндом */
            case CMP:
            case CPX:
            case CPY: {

                src = (opname == CMP ? this.reg.a : (opname == CPX ? this.reg.x : this.reg.y)) - src;
                this.set_carry(src >= 0);
                this.set_sign(src);
                this.set_zero(src & 0xff);
                break;
            }

            /* Уменьшение операнда на единицу */
            case DEC: {

                src = (src - 1) & 0xff;
                this.set_sign(src);
                this.set_zero(src);
                this.write(iaddr, src);
                break;
            }

            /* Уменьшение X на единицу */
            case DEX: {

                this.reg.x = (this.reg.x - 1) & 0xff;
                this.set_sign(this.reg.x);
                this.set_zero(this.reg.x);
                break;
            }

            /* Уменьшение Y на единицу */
            case DEY: {

                this.reg.y = (this.reg.y - 1) & 0xff;
                this.set_sign(this.reg.y);
                this.set_zero(this.reg.y);
                break;
            }

            /* Исключающее ИЛИ */
            case EOR: {

                src ^= this.reg.a;
                this.set_sign(src);
                this.set_zero(src);
                this.reg.a = src;
                break;
            }

            /* Увеличение операнда на единицу */
            case INC: {

                src = (src + 1) & 0xff;
                this.set_sign(src);
                this.set_zero(src);
                this.write(iaddr, src);
                break;
            }

            /* Уменьшение X на единицу */
            case INX: {

                this.reg.x = (this.reg.x + 1) & 0xff;
                this.set_sign(this.reg.x);
                this.set_zero(this.reg.x);
                break;
            }

            /* Увеличение Y на единицу */
            case INY: {

                this.reg.y = (this.reg.y + 1) & 0xff;
                this.set_sign(this.reg.y);
                this.set_zero(this.reg.y);
                break;
            }

            /* Переход по адресу */
            case JMP: addr = iaddr; break;

            /* Вызов подпрограммы */
            case JSR: {

                addr = (addr - 1) & 0xffff;
                this.push((addr >> 8) & 0xff);   /* Вставка обратного адреса в стек (-1) */
                this.push(addr & 0xff);
                addr = iaddr;
                break;
            }

            /* Загрузка операнда в аккумулятор */
            case LDA: {

                this.set_sign(src);
                this.set_zero(src);
                this.reg.a = (src);
                break;
            }

            /* Загрузка операнда в X */
            case LDX: {

                this.set_sign(src);
                this.set_zero(src);
                this.reg.x = (src);
                break;
            }

            /* Загрузка операнда в Y */
            case LDY: {

                this.set_sign(src);
                this.set_zero(src);
                this.reg.y = (src);
                break;
            }

            /* Логический сдвиг вправо */
            case LSR: {

                this.set_carry(src & 0x01);
                src >>= 1;
                this.set_sign(src);
                this.set_zero(src);
                if (optype == ACC) this.reg.a = src; else this.write(iaddr, src);
                break;
            }

            /* Логическое побитовое ИЛИ */
            case ORA: {

                src |= this.reg.a;
                this.set_sign(src);
                this.set_zero(src);
                this.reg.a = src;
                break;
            }

            /* Стек */
            case PHA: this.push(this.reg.a); break;
            case PHP: this.push((this.reg.p | 0x30)); break;
            case PLP: this.reg.p = this.pull(); break;

            /* Извлечение из стека в A */
            case PLA: {

                src = this.pull();
                this.set_sign(src);
                this.set_zero(src);
                this.reg.a = src;
                break;
            }

            /* Циклический сдвиг влево */
            case ROL: {

                src <<= 1;
                if (this.if_carry()) src |= 0x1;
                this.set_carry(src > 0xff);

                src &= 0xff;
                this.set_sign(src);
                this.set_zero(src);

                if (optype == ACC) this.reg.a = src; else this.write(iaddr, src);
                break;
            }

            /* Циклический сдвиг вправо */
            case ROR: {

                if (this.if_carry()) src |= 0x100;
                this.set_carry(src & 0x01);

                src >>= 1;
                this.set_sign(src);
                this.set_zero(src);

                if (optype == ACC) this.reg.a = src; else this.write(iaddr, src);
                break;
            }

            /* Возврат из прерывания */
            case RTI: {

                this.reg.p = this.pull();
                src   = this.pull();
                src  |= (this.pull() << 8);
                addr  = src;
                break;
            }

            /* Возврат из подпрограммы */
            case RTS: {

                src  = this.pull();
                src += ((this.pull()) << 8) + 1;
                addr = (src);
                break;
            }

            /* Вычитание */
            case SBC: {

                temp = this.reg.a - src - (this.if_carry() ? 0 : 1);

                this.set_sign(temp);
                this.set_zero(temp & 0xff);
                this.set_overflow(((this.reg.a ^ temp) & 0x80) && ((this.reg.a ^ src) & 0x80));
                this.set_carry(temp >= 0);
                this.reg.a = (temp & 0xff);
                break;
            }

            /* Запись содержимого A,X,Y в память */
            case STA: this.write(iaddr, this.reg.a); break;
            case STX: this.write(iaddr, this.reg.x); break;
            case STY: this.write(iaddr, this.reg.y); break;

            /* Пересылка содержимого аккумулятора в регистр X */
            case TAX: {

                src = this.reg.a;
                this.set_sign(src);
                this.set_zero(src);
                this.reg.x = (src);
                break;
            }

            /* Пересылка содержимого аккумулятора в регистр Y */
            case TAY: {

                src = this.reg.a;
                this.set_sign(src);
                this.set_zero(src);
                this.reg.y = (src);
                break;
            }

            /* Пересылка содержимого S в регистр X */
            case TSX: {

                src = this.reg.s;
                this.set_sign(src);
                this.set_zero(src);
                this.reg.x = (src);
                break;
            }

            /* Пересылка содержимого X в регистр A */
            case TXA: {

                src = this.reg.x;
                this.set_sign(src);
                this.set_zero(src);
                this.reg.a = (src);
                break;
            }

            /* Пересылка содержимого X в регистр S */
            case TXS: this.reg.s = this.reg.x; break;

            /* Пересылка содержимого Y в регистр A */
            case TYA: {

                src = this.reg.y;
                this.set_sign(src);
                this.set_zero(src);
                this.reg.a = (src);
                break;
            }

            // -------------------------------------------------------------
            // Недокументированные инструкции
            // -------------------------------------------------------------

            case SLO: {

                /* ASL */
                this.set_carry(src & 0x80);
                src <<= 1;
                src &= 0xff;
                this.set_sign(src);
                this.set_zero(src);

                if (optype == ACC) this.reg.a = src;
                else this.write(iaddr, src);

                /* ORA */
                src |= this.reg.a;
                this.set_sign(src);
                this.set_zero(src);
                this.reg.a = src;
                break;
            }

            case RLA: {

                /* ROL */
                src <<= 1;
                if (this.if_carry()) src |= 0x1;
                this.set_carry(src > 0xff);
                src &= 0xff;
                this.set_sign(src);
                this.set_zero(src);
                if (optype == ACC) this.reg.a = src; else this.write(iaddr, src);

                /* AND */
                src &= this.reg.a;
                this.set_sign(src);
                this.set_zero(src);
                this.reg.a = src;
                break;
            }

            case RRA: {

                /* ROR */
                if (this.if_carry()) src |= 0x100;
                this.set_carry(src & 0x01);
                src >>= 1;
                this.set_sign(src);
                this.set_zero(src);
                if (optype == ACC) this.reg.a = src; else this.write(iaddr, src);

                /* ADC */
                temp = src + this.reg.a + (this.reg.p & 1);
                this.set_zero(temp & 0xff);
                this.set_sign(temp);
                this.set_overflow(((this.reg.a ^ src ^ 0x80) & 0x80) && ((this.reg.a ^ temp) & 0x80) );
                this.set_carry(temp > 0xff);
                this.reg.a = temp & 0xff;
                break;

            }

            case SRE: {

                /* LSR */
                this.set_carry(src & 0x01);
                src >>= 1;
                this.set_sign(src);
                this.set_zero(src);
                if (optype == ACC) this.reg.a = src; else this.write(iaddr, src);

                /* EOR */
                src ^= this.reg.a;
                this.set_sign(src);
                this.set_zero(src);
                this.reg.a = src;

                break;
            }

            case DCP: {

                /* DEC */
                src = (src - 1) & 0xff;
                this.set_sign(src);
                this.set_zero(src);
                this.write(iaddr, src);

                /* CMP */
                src = this.reg.a - src;
                this.set_carry(src >= 0);
                this.set_sign(src);
                this.set_zero(src & 0xff);
                break;
            }

            // Увеличить на +1 и вычесть из A полученное значение
            case ISC: {

                /* INC */
                src = (src + 1) & 0xff;
                this.set_sign(src);
                this.set_zero(src);
                this.write(iaddr, src);

                /* SBC */
                temp = this.reg.a - src - (this.if_carry() ? 0 : 1);

                this.set_sign(temp);
                this.set_zero(temp & 0xff);
                this.set_overflow(((this.reg.a ^ temp) & 0x80) && ((this.reg.a ^ src) & 0x80));
                this.set_carry(temp >= 0);
                this.reg.a = (temp & 0xff);
                break;
            }

            // A,X = src
            case LAX: {

                this.reg.a = (src);
                this.set_sign(src);
                this.set_zero(src);
                this.reg.x = (src);
                break;
            }

            case AAX: this.write(iaddr, this.reg.a & this.reg.x); break;

            // AND + Carry
            case AAC: {

                /* AND */
                src &= this.reg.a;
                this.set_sign(src);
                this.set_zero(src);
                this.reg.a = src;

                /* Carry */
                this.set_carry(this.reg.a & 0x80);
                break;
            }

            case ASR: {

                /* AND */
                src &= this.reg.a;
                this.set_sign(src);
                this.set_zero(src);
                this.reg.a = src;

                /* LSR A */
                this.set_carry(this.reg.a & 0x01);
                this.reg.a >>= 1;
                this.set_sign(this.reg.a);
                this.set_zero(this.reg.a);
                break;
            }

            case ARR: {

                /* AND */
                src &= this.reg.a;
                this.set_sign(src);
                this.set_zero(src);
                this.reg.a = src;

                /* P[6] = A[6] ^ A[7]: Переполнение */
                this.set_overflow((this.reg.a ^ (this.reg.a >> 1)) & 0x40);

                temp = (this.reg.a >> 7) & 1;
                this.reg.a >>= 1;
                this.reg.a |= (this.reg.p & 1) << 7;

                this.set_carry(temp);
                this.set_sign(this.reg.a);
                this.set_zero(this.reg.a);
                break;
            }

            case ATX: {

                this.reg.a |= 0xFF;

                /* AND */
                src &= this.reg.a;
                this.set_sign(src);
                this.set_zero(src);
                this.reg.a = src;
                this.reg.x = this.reg.a;
                break;

            }

            case AXS: {

                temp = (this.reg.a & this.reg.x) - src;
                this.set_sign(temp);
                this.set_zero(temp);
                this.set_carry(((temp >> 8) & 1) ^ 1);
                this.reg.x = temp;
                break;
            }

            // Работает правильно, а тесты все равно не проходят эти 2
            case SYA: {

                temp = this.read(this.pc + 2);
                temp = ((temp + 1) & this.reg.y);
                this.write(iaddr, temp & 0xff);
                break;
            }

            case SXA: {

                temp = this.read(this.pc + 2);
                temp = ((temp + 1) & this.reg.x);
                this.write(iaddr, temp & 0xff);
                break;
            }
        }

        // Установка нового адреса
        this.pc = addr;

        return cycles_per_instr;
    }
}
