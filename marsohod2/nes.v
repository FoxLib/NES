module nes(

    /* ----------------
     * Archectural Marsohod2
     * ---------------- */

    // CLOCK    100 Mhz
    input   wire        clk,

    // LED      4
    output  reg [3:0]   led,

    // KEYS     2
    input   wire [1:0]  keys,

    // ADC      8 bit
    output  wire        adc_clock_20mhz,
    input   wire [7:0]  adc_input,

    // SDRAM
    output  wire        sdram_clock,
    output  wire [11:0] sdram_addr,
    output  wire [1:0]  sdram_bank,
    inout   wire [15:0] sdram_dq,
    output  wire        sdram_ldqm,
    output  wire        sdram_udqm,
    output  wire        sdram_ras,
    output  wire        sdram_cas,
    output  wire        sdram_we,

    // VGA
    output  wire [4:0]  vga_red,
    output  wire [5:0]  vga_green,
    output  wire [4:0]  vga_blue,
    output  wire        vga_hs,
    output  wire        vga_vs,

    // FTDI (PORT-B)
    input   wire        ftdi_rx,
    output  wire        ftdi_tx,

    /* ----------------
     * Extension Shield
     * ---------------- */

    // USB-A    2 pins
    inout   wire [1:0]  usb,

    // SOUND    2 channel
    output  wire        sound_left,
    output  wire        sound_right,

    // PS/2     keyb / mouse
    inout   wire [1:0]  ps2_keyb,
    inout   wire [1:0]  ps2_mouse
);

// --------------------------------------------------------------------------
wire [7:0] DEBUGCPU;
wire [7:0] DEBUGPPU;
reg [31:0] Timer;

always @(posedge clk) led <= Joy1[7:4];

// begin Timer <= (Timer == 50000000) ? 0 : Timer + 1; led <= Timer > 25000000 ? DEBUGCPU[3:0] : DEBUGCPU[7:4]; end

// --------------------------------------------------------------------------

wire [15:0] address;        /* Чтение */    
wire [15:0] eawr;           /* Запись в память (EA) */
wire [ 7:0] dout;           /* Выход данных из процессора */
wire        mreq;           /* Запрос на запись из процессора */
wire        NMI;
wire        DMA;
wire        OAMW;           /* Запрос на запись в OAM из PPU */

/* Роутинг записи памяти */
wire [7:0]  Dram;
wire [7:0]  Drom;
wire [7:0]  Dppu;
reg         DVRAM = 1'b0;     /* Отложенная запись в VRAM */
reg         DSRAM = 1'b0;     /* ... в SRAM */
reg         prg_led;

/* В зависимости от того, включен ли DMA. 
   Если да - то данные перенаправляются в PPU */
   
wire [15:0] curaddr = DMA ? WADDR : address;

wire        sram_write = eawr     < 16'h2000;
wire        sram_route = curaddr  < 16'h2000;
wire        ppu_route  = curaddr >= 16'h2000 && curaddr <= 16'h4017;
wire        srom_route = curaddr >= 16'h8000;

wire [7:0]  din = sram_route ? Dram :               /* 0000-07FF SRAM */
                  ppu_route  ? Dppu :               /* 2000-3FFF PPU */
                  srom_route ? Drom : 8'h00;        /* 8000-FFFF ROM */

always @(posedge clk) DSRAM <= CLKCPU;
always @(posedge clk) DVRAM <= WVREQ;
                  
// --------------------------------------------------------------------------

cpu C6502(
    
    .RESET  ( prg_enable ),     // Сброс процессора
    .CLK    ( CLKCPU ),         // 1.71 МГц
    .CE     ( !DMA ),           // Временное блокирование исполнения (DMA Request)
    .ADDR   ( address ),        // Адрес программы или данных
    .DIN    ( din ),            // Входящие данные
    .DOUT   ( dout ),           // Исходящие данные
    .EAWR   ( eawr ),           // Эффективный адрес
    .WREQ   ( wreq ),           // =1 Запись в память по адресу EA
    .RD     ( RD ),             // =1 Чтение из PPU
    .NMI    ( NMI ),
    .DEBUG  ( DEBUGCPU ),       // Отладочный
    .DKEY   ( keys[1:0] )
);

// --------------------------------------------------------------------------

reg  [1:0]  div = 2'b00; always @(posedge clk) div <= div + 1'b1;

wire [10:0] addr_vrd; // 2048
wire [12:0] addr_frd; // 8192
wire [ 7:0] data_vrd;
wire [ 7:0] data_frd;
wire [ 7:0] FIN;
wire [ 7:0] VIN;
wire [ 7:0] WDATA;
wire [15:0] WADDR;
wire        WVREQ;
wire        RD;
wire [7:0]  SAR;
wire [7:0]  SRD;

ppu PPU(
    
    .CLK25  (div[1]),
    .RESET  (prg_enable),
    
    /* Видеовыход VGA */
    .red    (vga_red),
    .green  (vga_green),
    .blue   (vga_blue),
    .hs     (vga_hs),
    .vs     (vga_vs),
    
    /* Исходящие тактовые частоты */
    .CLKPPU (CLKPPU),
    .CLKCPU (CLKCPU),
    
    /* Видеопамять (2Кб) */
    .vaddr  (addr_vrd), /* Чтение из памяти VRAM */
    .vdata  (data_vrd), /* Данные из VRAM */
    
    /* Обмен данными с видеопамятью */
    .ea     (eawr),     /* Адрес EA */
    .din    (dout),     /* Вход из процессора */
    .RD     (RD),       /* Запрос на чтение */
    .WREQ   (wreq),     /* Запрос на запись */
    .DOUT   (Dppu),     /* Выход из PPU */
    .VIN    (VIN),      /* Вход из VRAM */
    .WVREQ  (WVREQ),    /* Запрос на запись в VRAM */
    .WADDR  (WADDR),    /* Адрес к VRAM */
    .WDATA  (WDATA),    /* Данные для записи в VRAM */
    .NMI    (NMI),
    
    /* Спрайты и DMA */
    .DMA    (DMA),
    .OAMW   (OAMW),
    .DATAIN (din),      /* Из памяти CPU */
    .SPIN   (spin),     /* Из памяти OAM */
    .SAR    (SAR),      /* Адрес памяти OAM 2-й порт */
    .SRD    (SRD),      /* Данные */
    
    /* Знакогенератор */
    .faddr  (addr_frd), /* Адрес CHR-ROM */
    .fdata  (data_frd), /* Данные CHR-ROM */
    .FIN    (FIN),      /* Данные из знакогенератора на чтение */    
    .DEBUG  (DEBUGPPU),
    
    /* Джойстики */
    .JOY1   (Joy1),
    .JOY2   (Joy2)
    
);

// Программирование ROM 
// --------------------------------------------------------------------------

wire [7:0]  rx_byte;
wire        locked;
wire        clock_25;  // 25.00
wire        clock_12;  // 12.00
wire        clock_6;   //  6.00
wire        rx_ready;

pll PLL(

    .clk        (clk),          // Входящие 100 Мгц
    .locked     (locked),       // 0 - устройство генератора ещё не сконфигурировано, 1 - готово и стабильно
    .c0         (clock_25),     // 25,0 Mhz
    .c1         (clock_12),     // 12,0 Mhz
    .c2         (clock_6)       //  6,0 Mhz

);

uart UART(
    .clk12      (clock_12),
    .rx         (ftdi_rx),
    .rx_byte    (rx_byte),
    .rx_ready   (rx_ready),
);

parameter  prg_len      = 32768 + 8192; // 32K + 8K
reg [7:0]  prg_idata    = 1'b0; /* Данные для записи */
reg [15:0] prg_addr     = 1'b0; /* Адрес (16Kb) */
reg        prg_wren     = 1'b0; /* Производится запись в память */
wire       prg_wrenrom  = prg_enable && prg_addr <  32768; /* в память CHR */
wire       prg_wrenchr  = prg_enable && prg_addr >= 32768; /* в память CHR */
reg        prg_enable   = 1'b0; /* Программирование включено */
reg [1:0]  prg_negedge  = 2'b00;

/* Регистрация negedge rx_ready */
always @(posedge clk) prg_negedge <= {prg_negedge[0], rx_ready};

// Включение программатора 32 КБ ROM памяти
always @(posedge rx_ready) begin

    prg_idata <= rx_byte;

    if (prg_enable == 1'b0) begin

        prg_enable <= 1'b1;
        prg_wren   <= 1'b1;
        prg_addr   <= 16'h0000;
        prg_led    <= 1'b1;

    end
    else begin

        if (prg_addr == (prg_len - 2)) begin
            prg_enable <= 1'b0;
            prg_wren   <= 1'b0;
            prg_led    <= 1'b0;
        end

        prg_addr <= prg_addr + 1'b1;

    end

end

// Контроллер клавиатуры
// --------------------------------------------------------------------------

reg         kbd_reset = 1'b0;
wire        ps2_command_was_sent;
wire        ps2_error_communication_timed_out;
wire [7:0]  ps2_data;
wire        ps2_data_clk;

PS2_Controller Keyboard(

	/* Вход */
    .CLOCK_50       (div[0]),
	.reset          (1'b0),
	.the_command    (1'b0),
	.send_command   (1'b0),

	/* Ввод-вывод */
	.PS2_CLK(ps2_keyb[1]),
 	.PS2_DAT(ps2_keyb[0]),

	/* Статус команды */
	.command_was_sent  (ps2_command_was_sent),
	.error_communication_timed_out (ps2_error_communication_timed_out),

    /* Выход полученных */
	.received_data      (ps2_data),
	.received_data_en   (ps2_data_clk)

);

// Данные с джойстиков
reg [7:0] Joy1 = 8'h00;
reg [7:0] Joy2 = 8'h00;

reg key_press = 1'b1;

always @(posedge div[0]) begin

    if (ps2_data_clk) begin
    
        /* Код отжимаеой клавиши */
        if (ps2_data == 8'hF0) begin               
            key_press <= 1'b0; 
            
        end else begin
            
            case (ps2_data[6:0])
            
                /* Z (A)   */ 8'h2D: Joy1[0] <= key_press;
                /* X (B)   */ 8'h2C: Joy1[1] <= key_press; 
                /* X (SEL) */ 8'h2E: Joy1[2] <= key_press;
                /* V (ST)  */ 8'h2F: Joy1[3] <= key_press;                
                /* UP */      8'h48: Joy1[4] <= key_press;
                /* DOWN */    8'h50: Joy1[5] <= key_press;
                /* LEFT */    8'h4B: Joy1[6] <= key_press;
                /* RIGHT */   8'h4D: Joy1[7] <= key_press;
                
            endcase
        
            key_press <= 1'b1;
        end
    
    end

end

// --------------------------------------------------------------------------

/* Знакогенератор 8Кб */
romchr CHRROM(

    /* Для чтения из PPU */
    .clock   (clk),
    .addr_rd (addr_frd),
    .q       (data_frd),
    
    /* Для записи из программатора */
    .addr_wr (prg_enable ? prg_addr[12:0] : WADDR[12:0]),
    .data_wr (prg_idata),
    .wren    (prg_wrenchr && prg_negedge == 2'b10),
    .qw      (FIN)

);

/* Видеопамять 2Кб */
vram VRAM(

    /* Для чтения из PPU */
    .clock   (clk),
    .addr_rd (addr_vrd),
    .q       (data_vrd),
    
    /* Для записи из PPU */
    .addr_wr (WADDR[10:0]),
    .data_wr (WDATA),
    .wren    ({DVRAM, WVREQ} == 2'b11),
    .qw      (VIN),

);

/* Память программ 16 Кб (Базовая) */
rom ROM(

    .clock    (clk),
    .addr_rd  (curaddr[14:0]), // 32K
    .q        (Drom),
        
    /* Для записи из PPU */
    .addr_wr  (prg_addr[14:0]),
    .data_wr  (prg_idata),
    .wren     (prg_wrenrom && prg_negedge == 2'b10),

);

/* Оперативная память 2 Кб */
sram SRAM(

    /* Чтение */
    .clock    (clk),
    .addr_rd  (curaddr[10:0]), // 2K
    .q        (Dram),
    
    /* Запись на обратном фронте CPU в память */
    .addr_wr  (eawr[10:0]),
    .data_wr  (dout),
    .wren     ({DSRAM, CLKCPU} == 2'b10 && wreq && sram_write),

);

/* Память для спрайтов */
oam OAM(

    /* Чтение */
    .clock   (clk),
    .addr_rd (SAR),
    .q       (SRD),
    
    /* Запись */
    .addr_wr (WADDR[7:0]),
    .data_wr (WDATA),
    .wren    (OAMW),
    .qw      (spin)

);

endmodule
