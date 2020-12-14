`timescale 10ns / 1ns

module main;

// ---------------------------------------------------------------------

reg clock;
reg clock_25;

always #0.5 clock    = ~clock;
always #2.0 clock_25 = ~clock_25;

initial begin clock = 1; clock_25 = 0; #10000 $finish; end
initial begin $dumpfile("main.vcd"); $dumpvars(0, main); end

initial $readmemh("zp.hex",  sram, 16'h0000); // Zero Page
initial $readmemh("prg.hex", sram, 16'h8000); // Program Area

// ---------------------------------------------------------------------
// Контроллер памяти на 64К
// ---------------------------------------------------------------------

reg [7:0] sram[65536];

always @(posedge clock) begin

    pin_i <= sram[pin_address];
    if (pin_w) sram[pin_address] <= pin_o;

end

// ---------------------------------------------------------------------
// ЦЕНТРАЛЬНЫЙ ПРОЦЕССОРНЫЙ БЛОК
// ---------------------------------------------------------------------

wire [15:0] pin_address;        // Адрес / PC
wire [15:0] pin_ppuaddr;        // Адрес
reg  [7:0]  pin_i;              // Входящие данные
wire [7:0]  pin_o;              // Исходящие данные
wire        pin_w;              // Разрешение на запись
wire        pin_read;           // Читает в данный момент
wire        pin_ce;

cpu6502 NESCESSOR(

    pin_ce,
    pin_reset,
    clock_25,
    pin_address,
    pin_ppuaddr,
    pin_i,
    pin_o,
    pin_w,
    pin_read

);

endmodule
