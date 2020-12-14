`timescale 10ns / 1ns

module main;

// ---------------------------------------------------------------------

reg         clk;
always #0.5 clk = ~clk;

initial begin clk = 1; #2000 $finish; end
initial begin $dumpfile("nes.vcd"); $dumpvars(0, main); end

wire [4:0]  red;
wire [5:0]  green;
wire [4:0]  blue;
wire        hs;
wire        vs;
wire        rd;
wire [15:0] address;
wire [15:0] ea;

wire [7:0]  dout;
reg  [7:0]  i_data;
reg  [7:0]  spin;

wire        wreq;
wire        ppuclk;
wire        cpuclk;
wire        NMI;
wire [7:0]  DEBUG;
wire [7:0]  DEBUGPPU;
wire [1:0]  KEYS;
wire        reset = 1'b0;
wire        oamw;
wire [7:0]  sar;
reg  [7:0]  srd;
reg         DVRAM;

// Внутрисхемная память
// ---------------------------------------------------------------------
reg [ 7:0] sram[65536]; 
reg [ 7:0] vram[2048]; /* Видеопамять */
reg [ 7:0] crom[8192]; /* CHR-ROM */
reg [ 7:0]  oam[256]; 


wire VWREN = {DVRAM, vwreq} == 2'b01;

always @(posedge clk) begin

    /* SRAM */
    if (wreq)  sram[ ea ] <= dout;
    i_data  <= sram[ curaddr ];
    
    /* VRAM */
    if (VWREN) vram[ waddr ] <= wdata;
    vin     <= vram[ waddr ];
    fin     <= crom[ waddr ];
    
    if (oamw)  oam[ waddr[7:0] ] <= wdata;
    spin    <= oam[ waddr[7:0] ];
    srd     <= oam[ sar ];

end

/* Роутинг из памяти при DMA */
wire [15:0] curaddr = dma ? waddr : address;

// Роутинг памяти (из PPU к процессору). Важно указывать именно address
wire [7:0] din = (curaddr[15:13] == 3'b001) ? ppu_dout : i_data;

initial begin $readmemh("init/ram.hex", sram, 16'h0000); end
initial begin $readmemh("init/rom.hex", sram, 16'h8000); end // 32K
initial begin $readmemh("init/oam.hex", oam,  16'h0000); end // 32K

initial begin 

    // NMI
    // sram[ 16'hFFFA ] = 8'h05;
    // sram[ 16'hFFFB ] = 8'h80;
    
    // RESET
    // sram[ 16'hFFFC ] = 8'h00;
    // sram[ 16'hFFFD ] = 8'h80; 
    
end

always @(posedge clk) DVRAM <= vwreq;

// Центральный процессор
// ---------------------------------------------------------------------

cpu CPU( reset, cpuclk, !dma, address, din, dout, ea, wreq, rd, NMI, DEBUG, KEYS); 

// Графический процессор
// ---------------------------------------------------------------------

wire [ 7:0] ppu_dout;
wire [10:0] vaddr; wire [7:0] vdata;
wire [12:0] faddr; wire [7:0] fdata;

reg  [ 7:0] vin;
reg  [ 7:0] fin;
wire [ 7:0] wdata;
wire [15:0] waddr;
wire        vwreq;
reg         dvram = 1'b0;
wire [ 7:0] Joy1;
wire [ 7:0] Joy2;

always @(posedge clk) dvram <= vwreq;

ppu PPU(

    /* 100 Mhz */
    clk,    
    reset,
    
    /* VGA */
    red, green, blue, hs, vs, 
    
    /* VRAM */
    vaddr, vdata,               /* Чтение видеоадаптером */
    vin, vwreq, waddr, wdata,   /* Чтение и запись */  
    
    /* CHR  */
    faddr, fdata, fin,          /* Чтение из CHR-ROM */
    
    ppuclk, /* 5 Mhz */
    cpuclk, /* 1.6 Mhz */
    
    /* Данные на запись/чтение */
    /* I */ ea, 
    /* I */ dout, 
    /* I */ rd, 
    /* I */ wreq, 
    /* O */ ppu_dout, 
    /* O */ dma,
    /* O */ oamw,
    /* I */ din,
    /* I */ spin,
    /* O */ sar,
    /* I */ srd,
    
    /* NonMasking */
    NMI, DEBUGPPU,
    
    Joy1, Joy2
);

endmodule
