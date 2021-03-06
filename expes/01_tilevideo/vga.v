module vga
(
    input   wire        CLOCK,        
    output  reg  [3:0]  VGA_R,
    output  reg  [3:0]  VGA_G,
    output  reg  [3:0]  VGA_B,
    output  wire        VGA_HS,
    output  wire        VGA_VS
);
       
// ---------------------------------------------------------------------
    
// Тайминги для горизонтальной развертки
parameter hz_visible = 1366;
parameter hz_front   = 64;
parameter hz_sync    = 112;
parameter hz_back    = 250;
parameter hz_whole   = 1792;

// Тайминги для вертикальной развертки
parameter vt_visible = 768;
parameter vt_front   = 3;
parameter vt_sync    = 6;
parameter vt_back    = 18;
parameter vt_whole   = 795;

// ---------------------------------------------------------------------
assign VGA_HS = x >= (hz_back + hz_visible + hz_front); // POS
assign VGA_VS = y >= (vt_back + vt_visible + vt_front); // POS
// ---------------------------------------------------------------------

wire        xmax = (x == hz_whole - 1);
wire        ymax = (y == vt_whole - 1);
reg  [10:0] x    = 0;
reg  [10:0] y    = 0;
wire [10:0] X    = x - hz_back; // X=[0..1365]
wire [ 9:0] Y    = y - vt_back; // Y=[0..767]

always @(posedge CLOCK) begin

    // Кадровая развертка
    x <= xmax ?         0 : x + 1;
    y <= xmax ? (ymax ? 0 : y + 1) : y;

    // Вывод окна видеоадаптера
    if (x >= hz_back && x < hz_visible + hz_back &&
        y >= vt_back && y < vt_visible + vt_back)
    begin
         {VGA_R, VGA_G, VGA_B} <= X[3:0] == 0 || Y[3:0] == 0 ? 12'hFFF : {X[4]^Y[4], 3'h0, X[5]^Y[5], 3'h0, X[6]^Y[6], 3'h0};
    end
    else {VGA_R, VGA_G, VGA_B} <= 12'b0;

end

endmodule