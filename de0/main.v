`timescale 10ns / 1ns

module main;

// ---------------------------------------------------------------------

reg clk; reg clk25;

always #0.5 clk   = ~clk;
always #2.0 clk25 = ~clk25;

initial begin clk = 1; clk25 = 0; #7000 $finish; end
initial begin $dumpfile("main.vcd"); $dumpvars(0, main); end

initial $readmemh("icarus_rom.hex", sram, 16'h0000);
initial $readmemh("icarus_oam.hex", oam,  16'h0000);

// ---------------------------------------------------------------------
// Контроллер памяти
// ---------------------------------------------------------------------

reg [7:0] sram[65536];
reg [7:0] oam[256];

always @(posedge clk) begin

    // ROM
    pin_i <= sram[pin_address];
    if (pin_w) sram[pin_address] <= pin_o;

    // OAM
    ppu_data_oam <= oam[ ppu_cursor_oam ];

end

// ---------------------------------------------------------------------
// Центральный процессор
// ---------------------------------------------------------------------

wire [15:0] pin_address;        // Адрес памяти на I/O
wire [15:0] pin_address_ppu;    // Адрес памяти на VRAM
reg  [7:0]  pin_i;              // Входящие данные
wire [7:0]  pin_o;              // Исходящие данные
wire        pin_w;              // Разрешение на запись
wire        pin_read;           // Чтение из PPU
wire        pin_clock = clk25;  // 25 МГц
wire        pin_ce    = cpu_clock; // Разрешение исполнения тактов
wire        pin_nmi;            // Импульс NMI от PPU
wire        pin_rd;             // Защелка для PPU
wire        pin_reset = 1'b0;

// Исполнительное устройство
cpu CentralUnitProcessor
(
    pin_reset,
    pin_clock,
    pin_ce,
    pin_address,
    pin_address_ppu,
    pin_i,
    pin_o,
    pin_w,
    pin_read,
    pin_nmi,
    pin_rd
);

// ---------------------------------------------------------------------
// Видеопроцессор
// ---------------------------------------------------------------------

wire [3:0]  vga_r;
wire [3:0]  vga_g;
wire [3:0]  vga_b;
wire        vga_hs;
wire        vga_vs;
wire        ppu_clock;
wire        cpu_clock;
wire [10:0] ppu_cursor;
wire [12:0] ppu_cursor_chr;
wire [ 7:0] ppu_cursor_oam;
reg  [ 7:0] ppu_data;
reg  [ 7:0] ppu_data_chr;
reg  [ 7:0] ppu_data_oam;

ppu PixelProcessingUnit
(
    // 25 мегагерц
    pin_clock,
    ppu_clock,
    cpu_clock,

    // Обмен данными
    ppu_cursor,
    ppu_cursor_chr,
    ppu_cursor_oam,
    ppu_data,
    ppu_data_chr,
    ppu_data_oam,

    // Выходные данные
    vga_r,
    vga_g,
    vga_b,
    vga_hs,
    vga_vs
);

endmodule
