module de0(

      /* Reset */
      input              RESET_N,

      /* Clocks */
      input              CLOCK_50,
      input              CLOCK2_50,
      input              CLOCK3_50,
      inout              CLOCK4_50,

      /* DRAM */
      output             DRAM_CKE,
      output             DRAM_CLK,
      output      [1:0]  DRAM_BA,
      output      [12:0] DRAM_ADDR,
      inout       [15:0] DRAM_DQ,
      output             DRAM_CAS_N,
      output             DRAM_RAS_N,
      output             DRAM_WE_N,
      output             DRAM_CS_N,
      output             DRAM_LDQM,
      output             DRAM_UDQM,

      /* GPIO */
      inout       [35:0] GPIO_0,
      inout       [35:0] GPIO_1,

      /* 7-Segment LED */
      output      [6:0]  HEX0,
      output      [6:0]  HEX1,
      output      [6:0]  HEX2,
      output      [6:0]  HEX3,
      output      [6:0]  HEX4,
      output      [6:0]  HEX5,

      /* Keys */
      input       [3:0]  KEY,

      /* LED */
      output      [9:0]  LEDR,

      /* PS/2 */
      inout              PS2_CLK,
      inout              PS2_DAT,
      inout              PS2_CLK2,
      inout              PS2_DAT2,

      /* SD-Card */
      output             SD_CLK,
      inout              SD_CMD,
      inout       [3:0]  SD_DATA,

      /* Switch */
      input       [9:0]  SW,

      /* VGA */
      output      [3:0]  VGA_R,
      output      [3:0]  VGA_G,
      output      [3:0]  VGA_B,
      output             VGA_HS,
      output             VGA_VS
);

// Z-state
assign DRAM_DQ = 16'hzzzz;
assign GPIO_0  = 36'hzzzzzzzz;
assign GPIO_1  = 36'hzzzzzzzz;

// LED OFF
assign HEX0 = 7'b1111111;
assign HEX1 = 7'b1111111;
assign HEX2 = 7'b1111111;
assign HEX3 = 7'b1111111;
assign HEX4 = 7'b1111111;
assign HEX5 = 7'b1111111;

// -----------------------------------------------------------------------
// Генератор тактовой частоты
// -----------------------------------------------------------------------

wire _clk;   wire clk   = _clk   & locked;
wire _clk25; wire clk25 = _clk25 & locked;

pll u0
(
    .clkin  (CLOCK_50),
    .m25    (_clk25),
    .m100   (_clk),
    .locked (locked)
);

// -----------------------------------------------------------------------
// Pixel Processing Unit (Видеоадаптер)
// -----------------------------------------------------------------------

wire    clock_ppu;

ppu PixelProcessingUnit
(
    // Тактовые частоты
    .clock      (clk25),
    .clock_ppu  (clock_ppu),

    // Подключение к видеопамяти
    .cursor     (ppu_cursor),
    .cursor_chr (ppu_cursor_chr),
    .cursor_oam (ppu_cursor_oam),
    .data       (ppu_data),
    .data_chr   (ppu_data_chr),
    .data_oam   (ppu_data_oam),
    
    // Физический интерфейс
    .r  (VGA_R),
    .g  (VGA_G),
    .b  (VGA_B),
    .hs (VGA_HS),
    .vs (VGA_VS)
);

// -----------------------------------------------------------------------
// Память для видеоконтроллера
// -----------------------------------------------------------------------

// Обмен данными
wire [10:0] vram_address;
wire [ 7:0] vram_o_data;
wire [ 7:0] vram_i_data;
wire        vram_writena;

// Генератор картинки VGA
wire [10:0] ppu_cursor;
wire [12:0] ppu_cursor_chr;
wire [ 7:0] ppu_cursor_oam;

wire [ 7:0] ppu_data;
wire [ 7:0] ppu_data_chr;
wire [ 7:0] ppu_data_oam;

// Видеопамять
mem_vram VMemory2K
(
    .clock      (_clk),

    // Обмен данными (чтение-запись)
    .address_a  (vram_address),     // Данные для чтения из видеопамяти
    .q_a        (vram_o_data),      // Чтение данных из памяти
    .data_a     (vram_i_data),      // Данные для записи в память
    .wren_a     (vram_writena),     // Запись в видео память

    // Второй порт для чтения и выдачи на VGA
    .address_b  (ppu_cursor),       // Адрес
    .q_b        (ppu_data),         // Данные
);

// Хранение символов (CHR-ROM)
mem_chr VChrROM8K
(
    .clock      (_clk),

    // Второй порт для чтения и выдачи на VGA
    .address_b  (ppu_cursor_chr),
    .q_b        (ppu_data_chr)
);


// Хранение спрайтов (OAM)
mem_oam OAMBank
(
    .clock      (_clk),

    // Сканер спрайтов из памяти
    .address_b  (ppu_cursor_oam),
    .q_b        (ppu_data_oam)
);


// -----------------------------------------------------------------------
// Контроллер клавиатуры
// -----------------------------------------------------------------------

reg         kbd_reset = 1'b0;
reg  [7:0]  ps2_command = 1'b0;
reg         ps2_command_send = 1'b0;
wire        ps2_command_was_sent;
wire        ps2_error_communication_timed_out;
wire [7:0]  ps2_data;
wire        ps2_data_clk;

ps2_keyboard ukdb
(
    /* Вход */
    .CLOCK_50       (CLOCK_50),
    .reset          (kbd_reset),
    .the_command    (ps2_command),
    .send_command   (ps2_command_send),

    /* Ввод-вывод */
    .PS2_CLK        (PS2_CLK),
    .PS2_DAT        (PS2_DAT),

    /* Статус команды */
    .command_was_sent (ps2_command_was_sent),
    .error_communication_timed_out (ps2_error_communication_timed_out),

    /* Выход полученных */
    .received_data    (ps2_data),
    .received_data_en (ps2_data_clk)
);

// ---------------------------------------------------------------------
// Данные с джойстиков (с клавиатуры)
// https://ru.wikipedia.org/wiki/%D0%A1%D0%BA%D0%B0%D0%BD-%D0%BA%D0%BE%D0%B4
// ---------------------------------------------------------------------

reg [7:0] Joy1 = 8'h00;     // Джойстик 1
reg [7:0] Joy2 = 8'h00;     // Джойстик 2
reg       kbd_press = 1'b1;

/* Используются AT-коды клавиатуры */
always @(posedge CLOCK_50) begin

    if (ps2_data_clk) begin

        /* Код отпущенной клавиши */
        if (ps2_data == 8'hF0) kbd_press <= 1'b0;
        else begin

            case (ps2_data[6:0])

                /* Z (A)   */ 8'h1A: Joy1[0] <= kbd_press;
                /* X (B)   */ 8'h22: Joy1[1] <= kbd_press;
                /* C (SEL) */ 8'h21: Joy1[2] <= kbd_press;
                /* V (STA) */ 8'h2A: Joy1[3] <= kbd_press;
                /* UP      */ 8'h75: Joy1[4] <= kbd_press;
                /* DOWN    */ 8'h72: Joy1[5] <= kbd_press;
                /* LEFT    */ 8'h6B: Joy1[6] <= kbd_press;
                /* RIGHT   */ 8'h74: Joy1[7] <= kbd_press;

            endcase

            kbd_press <= 1'b1;

        end

    end

end

endmodule
