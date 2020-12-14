/*
 * Реализация процессора 6502 для NES и не только
 * Этот процессор надо сделать универсальным!
 */
module cpu6502(

    input   wire        pin_ce,         // ChipEnable
    input   wire        pin_reset,      // Сброс
    input   wire        pin_clock,      // Тактовый генератор
    output  wire [15:0] pin_address,    // Текущий адрес
    output  wire [15:0] pin_ppuaddr,    // Указатель на шину
    input   wire  [7:0] pin_i,          // Входящие данные
    output  reg   [7:0] pin_o,          // Исходящие данные
    output  reg         pin_w,          // Разрешение на запись
    output  reg         pin_read        // Процессор читает данные (на обратном фронте)
);

// Параметры. определения и начальный старт
// =====================================================================

// bus=0 указывает на PC, иначе на address
assign pin_address = bus ? address : pc;
assign pin_ppuaddr = address;

// Значения, выставленные по умолчания
initial begin pin_w = 0; pin_read = 0; end

// Ссылки на микрокод
`define IDLE    5'h00 // Первое состояние
`define NDX     5'h01 // Indirect,X
`define NDX2    5'h02
`define NDX3    5'h03
`define NDY     5'h04 // Indirect,Y
`define NDY2    5'h05
`define NDY3    5'h06
`define ZP      5'h07 // ZeroPage
`define ZPX     5'h08 // ZeroPage,X
`define ZPY     5'h09 // ZeroPage,Y
`define ABS     5'h0A // Absolute
`define ABS2    5'h0B
`define ABX     5'h0C // Absolute,X
`define ABX2    5'h0D
`define ABY     5'h0E // Absolute,Y
`define ABY2    5'h0F
`define LAT1    5'h10 // Задержка
`define START   5'h11 // Immediate, Implied, Accumulator
`define REL     5'h17 // Relative
`define RESET   5'h1E // Сброс процессора
`define RESET2  5'h1F

// Вычисления адресов
// =====================================================================
wire [8:0]  sum_din_x   = pin_i + x;  // Данные на шине + X
wire [8:0]  sum_din_y   = pin_i + y;  // Данные на шине + Y
wire [15:0] next_address = address + 1; // Следущий `address`
// Инструкции inc/dec
wire        incdec_instr =  ({opcode[7:6], opcode[2:0]} == 5'b11_1_10) ||
                            ({opcode[7],   opcode[2:0]} == 4'b0__1_10);
// Возможность задержки исполнения на 1 такт
wire        latency     = cout | (opcode[7:5] == 3'b100) | incdec_instr;

// Состояние процессора в данный момент времени
// =====================================================================
reg         bus       = 1'b1;       // Указатель на [0=pc, 1=address]
reg [15:0]  address   = 16'hFFFC;   // Указатель в память (RST)
reg [7:0]   opcode    = 8'h00;      // Сохраненный опкод
reg [4:0]   microcode = `RESET;     // Микрокод (начало на сбросе)
reg [7:0]   temp      = 8'h00;      // Регистр временных данных
reg         cout      = 1'b0;       // Вычисления границ страниц
reg         do_read   = 1'b0;       // Процессор может читать из шины

// Регистры процессора
// =====================================================================
reg [15:0]  pc  = 16'h8000;
reg [ 7:0]  a   = 8'h00;
reg [ 7:0]  x   = 8'h00;
reg [ 7:0]  y   = 8'h00;
reg [ 7:0]  p   = 8'h00;

// Исполнение инструкции на позитивном фронте
// =====================================================================
always @(posedge pin_clock) begin

    // Сброс процессора
    if (pin_reset) begin address <= 16'hFFFC; bus <= 1'b1; pin_w <= 1'b0; microcode <= `RESET; end
    else case (microcode)

        // Прием опкода и его исполнение
        // -----------------------------
        `IDLE: begin

            casex (pin_i)

                8'bxxx_000_x1: begin microcode <= `NDX; end // Indirect, X
                8'bxxx_010_x1, // Immediate
                8'b1xx_000_x0: begin microcode <= `START; end
                8'bxxx_100_x1: begin microcode <= `NDY; end // Indirect, Y
                8'bxxx_110_x1: begin microcode <= `ABY; end // Absolute, Y
                8'bxxx_001_xx: begin microcode <= `ZP;  end // ZeroPage
                8'bxxx_011_xx, // Absolute
                8'b001_000_00: begin microcode <= `ABS; end
                8'b10x_101_1x: begin microcode <= `ZPY; end // ZeroPage, Y
                8'bxxx_101_xx: begin microcode <= `ZPX; end // ZeroPage, X
                8'b10x_111_1x: begin microcode <= `ABY; end // Absolute, Y
                8'bxxx_111_xx: begin microcode <= `ABX; end // Absolute, X
                8'bxxx_100_00: begin microcode <= `REL; end // Relative
                8'b0xx_010_10: begin microcode <= `START; end // Accumulator
                default:       begin microcode <= `START; end

            endcase

            opcode  <= pin_i;   // Сохранить опкод
            pc      <= pc + 1;  // Отметить опкод прочтённым
            pin_w   <= 1'b0;    // Сброс записи в память
            do_read <= 1'b1;    // Разрешение чтения из адреса

            // Читать все данные из шины, кроме STA
            casex (opcode) 8'b100_xxx_01, 8'b100_xx1_x0: do_read <= 1'b0; endcase

        end

        // Исполнение сброса процессора
        // -----------------------------
        `RESET:  begin microcode <= `RESET2; pc[7:0]  <= pin_i; address <= next_address; end
        `RESET2: begin microcode <= `IDLE;   pc[15:8] <= pin_i; bus <= 1'b0; end

        // Задержка исполнения на 1 такт, сброс такта чтения адреса
        `LAT1: begin microcode <= `START; pin_read <= 1'b0; end

        // Indirect, X (непрямая адресация по X)
        // Берется адрес, следующий за опкодом, складывается с X
        // По полученному адресу читается слово из ZeroPage области
        // -------------------------------------------------------------
        `NDX:  begin microcode <= `NDX2; address <= sum_din_x[7:0]; bus <= 1'b1; pc <= pc + 1; end
        `NDX2: begin microcode <= `NDX3; address <= next_address[7:0]; temp <= pin_i; end
        `NDX3: begin microcode <= `LAT1; address <= {pin_i, temp}; pin_read <= do_read; end

        // Indirect, Y: Сначала читается WORD из ZP, который указан в
        // операнде. Потом к этому адресу прибавляется +Y и считывается
        // байт из вычисленного адреса
        // -------------------------------------------------------------
        `NDY:  begin microcode <= `NDY2; address <= pin_i; bus <= 1'b1; pc <= pc + 1; end
        `NDY2: begin microcode <= `NDY3; address <= next_address[7:0]; temp <= sum_din_y[7:0]; cout <= sum_din_y[8]; end
        `NDY3: begin microcode <= latency ? `LAT1 : `START; address <= {pin_i + cout, temp}; pin_read <= do_read; end

        // ZP; ZP,X; ZP,Y
        // -------------------------------------------------------------
        `ZP:  begin microcode <= `START; address <= pin_i;          bus <= 1'b1; pin_read <= do_read; pc <= pc + 1; end
        `ZPX: begin microcode <= `LAT1;  address <= sum_din_x[7:0]; bus <= 1'b1; pin_read <= do_read; pc <= pc + 1; end
        `ZPY: begin microcode <= `LAT1;  address <= sum_din_y[7:0]; bus <= 1'b1; pin_read <= do_read; pc <= pc + 1; end

        // Абсолютная адресация
        // -------------------------------------------------------------

        // ABS
        `ABS:  begin microcode <= `ABS2; temp <= pin_i; pc <= pc + 1; end
        `ABS2: // JMP ABS
        if (opcode == 8'h4C) begin

             microcode <= `IDLE;
             pc <= {pin_i, temp};

        end else begin microcode <= `START;

            bus      <= 1'b1;
            address  <= {pin_i, temp};
            pin_read <= do_read;
            pc <= pc + 1;

        end

        // ABS,X
        `ABX:  begin microcode <= `ABY2; temp <= sum_din_x[7:0]; pc <= pc + 1; cout <= sum_din_x[8]; end
        `ABX2: begin microcode <= latency ? `LAT1 : `START;
                     bus       <= 1'b1;
                     address   <= {pin_i + cout, temp};
                     pin_read  <= do_read;
                     pc <= pc + 1;
        end

        // ABS,Y
        `ABY:  begin microcode <= `ABY2; temp <= sum_din_y[7:0]; pc <= pc + 1; cout <= sum_din_y[8]; end
        `ABY2: begin microcode <= latency ? `LAT1 : `START;
                     bus       <= 1'b1;
                     address   <= {pin_i + cout, temp};
                     pin_read  <= do_read;
                     pc <= pc + 1;
        end

        // Исполнение инструкции
        // -------------------------------------------------------------
        `START: begin

            pin_read <= 1'b0; // Сброс такта чтения адреса

        end

    endcase

end

endmodule
