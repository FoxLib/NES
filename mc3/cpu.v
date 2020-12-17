module cpu(

    input  wire        RESET,   // Сброс процессора
    input  wire        CLK,     // 1.71 МГц
    input  wire        CE,      // Готовность
    output wire [15:0] ADDR,    // Адрес программы или данных
    input  wire [7:0]  DIN,     // Входящие данные
    output reg  [7:0]  DOUT,    // Исходящие данные
    output wire [15:0] EAWR,    // Эффективный адрес для записи
    output reg         WREQ,    // =1 Запись в память по адресу EA
    output reg         RD,      // На нисходящем CLK при RD=1, защелка PPU
    input  wire        NMI,     // Синхроимпульс NMI
    output wire [7:0]  DEBUG,   // Отладочный
    input  wire [1:0]  DKEY     // Отладочные кнопки

);

assign ADDR  = AS ? {8'h01, S} : (AM ? EA : PC);
assign EAWR  = AS ? {8'h01, S} : EA;

// Отладочный
assign  DEBUG = DKEY == 2'b11 ? A[7:0] :  // Обе отжаты
                DKEY == 2'b10 ? X[7:0] :  // Нажата 0-я
                DKEY == 2'b01 ? Y[7:0] :  // Нажата 1-я
                                P[7:0];   // Нажаты две

`define DEBUGPC 0 // 16'hDD63 // hDD63

// ---------------------------------------------------------------------

/* Ссылки на микрокод */
`define NDX     5'h01
`define NDY     5'h04
`define ZP      5'h07
`define ZPX     5'h08
`define ZPY     5'h09
`define ABS     5'h0A
`define ABX     5'h0C
`define ABY     5'h0E

// Везде переход к исполнению
`define LAT1    5'h10
`define IMM     5'h11
`define IMP     5'h11
`define ACC     5'h11

// Исполнение инструкции
`define EXEC    5'h11
`define EXEC2   5'h12
`define EXEC3   5'h13
`define EXEC4   5'h14
`define EXEC5   5'h15
`define EXEC6   5'h16

// Переход
`define REL     5'h17
`define REL1    5'h18
`define REL2    5'h19
`define REL3    5'h1A

// 5h1B, 5h1C - Unused yet

// Сброс
`define RST     5'h1D
`define RST2    5'h1E
`define RST3    5'h1F

initial begin EA = 16'h0000; WREQ = 1'b0; DOUT = 8'h00; RD = 1'b0; end

/* Регистры */
reg  [7:0]  A   = 8'h00;
reg  [7:0]  X   = 8'h00;
reg  [7:0]  Y   = 8'h00;
reg  [7:0]  S   = 8'h00;
reg  [7:0]  P   = 8'b00000000;
reg [15:0]  PC  = 16'hC000;

/* Состояние процессора */
reg  [4:0]  MS     = `RST;      /* Исполняемый цикл */
reg         AS     = 1'b0;      /* 0=PC/AM, 1=S Указатель стека (приоритет) */
reg         AM     = 1'b0;      /* 0=PC, 1=EA */
reg  [1:0]  IRQ    = 2'b11;     /* BRK, $FFFE по умолчанию */
reg         ISBRK  = 1'b0;      /* Является BRK */
reg         Cout   = 1'b0;      /* Переносы при вычислении адреса */
reg         HOP    = 1'b0;      /* =1 Операнду требуется PC++ */
reg  [7:0]  PCL    = 8'h00;     /* Для JSR */
reg  [7:0]  TR     = 3'h0;      /* Temporary Register */
reg  [15:0] EA     = 16'h0000;  /* Эффективный адрес */
reg         NMI_trigger = 1'b0; /* Для отслеживания изменения в синхроимпульсе NMI */

/* Микрокод */
reg         WR     = 1'b0;      /* Записать в регистр RA из АЛУ */
reg         FW     = 1'b0;      /* Записать в регистр P из АЛУ */
reg         SW     = 1'b0;      /* Записать в S из АЛУ */
reg         ACC    = 1'b0;      /* Указать A вместо DIN */
reg         BR     = 1'b0;      /* Если =1, условие выполняется */
reg         SEI    = 1'b0;      /* Установить I=1 */
reg         ENARD  = 1'b0;      /* Если =1, то разрешить RD */
reg  [7:0]  opcode = 8'h0;      /* Текущий опкод */

/* Некоторые часто употребляемые выражения */
wire [15:0] PCINC   = PC + 1'b1;         /* Инкремент PC */
wire [15:0] PCRel   = PCINC + {{8{DIN[7]}}, DIN[7:0]}; /* Для переходов */
wire [4:0]  MSINC   = MS + 1'b1;         /* Инкремент MS */
wire [15:0] EAINC   = EA + 1'b1;         /* Инкремент EA */
wire [8:0]  XDin    = X + DIN;           /* Для преиндексной адресации */
wire [8:0]  YDin    = Y + DIN;           /* Для постиндексной адресации */
wire [7:0]  HIDin   = DIN + Cout;        /* Перенос */
wire [15:0] EADIN   = {DIN,   TR};
wire [15:0] EADIH   = {HIDin, TR};
wire        Latency = Cout | (opcode[7:5] == 3'b100) | INCDEC; /* STA, Cout, Сдвиговые */
wire [4:0]  LATAD   = Latency ? `LAT1 : `EXEC; /* Код адреса при Latency */

/* Определить безусловный Latency для Inc, Dec и сдвиговых */
wire        INCDEC  = ({opcode[7:6], opcode[2:0]} == 5'b11_1_10) ||
                      ({opcode[7],   opcode[2:0]} == 4'b0__1_10);

/* Текущий статус NMI */
wire        NMI_status = NMI_trigger ^ NMI;

/* Для Icarus Verilog: чтобы легче было отлаживать */
wire debug_clk = CLK && MS == 1'b0;
reg LN = 1'b1;

/* Исполнение микрокода */
always @(posedge CLK) begin

    /* Сброс процессора */
    if (RESET || !DKEY[0]) begin

        MS   <= `RST;
        WREQ <= 1'b0;
        {AS, AM} <= 2'b00;

    end

    /* Нормальное исполнение: если CE=1 */
    else if (CE) case (MS)

        /* ИНИЦИАЛИЗАЦИЯ */
        4'h0: begin

            // Останов процессора для отладки
            if (DIN == 8'h02 /* KIL */) begin

                AM <= 1'b0;
                AS <= 1'b0;

            end else begin

                /* Получено изменение NMI. Переброска статуса. */
                if (NMI_status) begin
                    NMI_trigger <= NMI_status ^ NMI_trigger;
                end

                /* Восходящий фронт NMI. Срабатывает при изменений статуса NMI. */
                if (NMI && NMI_status) begin

                    opcode <= 8'h00; // BRK / NMI
                    IRQ    <= 2'b01; // $FFFA
                    ISBRK  <= 1'b0;
                    MS     <= `IMP;
                    HOP    <= 1'b0;
                    LN     <= 0;

                /* Обычное исполнение */
                end else begin

                    opcode <= DIN;   /* Принять новый опкод */
                    IRQ    <= 2'b11; /* Для BRK -> $FFFE */
                    PC     <= PCINC; /* PC++ */
                    ISBRK  <= DIN == 8'h00;
                    HOP    <= 1'b1;

                    casex (DIN)

                        8'bxxx_000_x1: begin MS <= `NDX; end // Indirect, X
                        8'bxxx_010_x1, // Immediate
                        8'b1xx_000_x0: begin MS <= `IMM; end
                        8'bxxx_100_x1: begin MS <= `NDY; end // Indirect, Y
                        8'bxxx_110_x1: begin MS <= `ABY; end // Absolute, Y
                        8'bxxx_001_xx: begin MS <= `ZP;  end // ZeroPage
                        8'bxxx_011_xx, // Absolute
                        8'b001_000_00: begin MS <= `ABS; end
                        8'b10x_101_1x: begin MS <= `ZPY; end // ZeroPage, Y
                        8'bxxx_101_xx: begin MS <= `ZPX; end // ZeroPage, X
                        8'b10x_111_1x: begin MS <= `ABY; end // Absolute, Y
                        8'bxxx_111_xx: begin MS <= `ABX; end // Absolute, X
                        8'bxxx_100_00: begin MS <= `REL; end // Relative
                        8'b0xx_010_10: begin MS <= `ACC; HOP <= 1'b0; end // Accumulator
                        default:       begin MS <= `IMP; HOP <= 1'b0; end

                    endcase

                end

                /* Нормализовать указатели */
                AS      <= 1'b0;  /* Указатель стека */
                RD      <= 1'b0;  /* Для PPU */
                WREQ    <= 1'b0;  /* Отключение записи в память EA */

            end

        end

        /* АДРЕСАЦИЯ */

        /* Indirect, X */
        // -------------------------------------------------------------
        4'h1: begin MS <= MSINC; EA <= XDin[7:0];  AM <= 1'b1; end
        4'h2: begin MS <= MSINC; EA <= EAINC[7:0]; TR <= DIN;  end
        4'h3: begin MS <= `LAT1; EA <= EADIN;      RD <= ENARD; end

        /* Indirect, Y */
        // -------------------------------------------------------------
        4'h4: begin MS <= MSINC; EA <= DIN;        AM <= 1'b1; end
        4'h5: begin MS <= MSINC; EA <= EAINC[7:0]; TR <= YDin[7:0]; Cout <= YDin[8]; end
        4'h6: begin MS <= LATAD; EA <= EADIH;      RD <= ENARD; end

        /* ZP */
        // -------------------------------------------------------------
        4'h7: begin MS <= `EXEC; EA <= DIN;       {AM, RD} <= {1'b1, ENARD}; end

        /* ZP,X */
        // -------------------------------------------------------------
        4'h8: begin MS <= `LAT1; EA <= XDin[7:0]; {AM, RD} <= {1'b1, ENARD}; end

        /* ZP,Y */
        // -------------------------------------------------------------
        4'h9: begin MS <= `LAT1; EA <= YDin[7:0]; {AM, RD} <= {1'b1, ENARD}; end

        /* Absolute */
        // -------------------------------------------------------------
        4'hA: begin MS <= MSINC; TR <= DIN; PC <= PCINC;  end
        4'hB: begin

            /* JMP ABS */
            if (opcode == 8'h4C)
                 begin MS <= 1'b0;  PC <= EADIN; end
            else begin MS <= `EXEC; EA <= EADIN; {AM, RD} <= {1'b1, ENARD}; end

        end

        /* Absolute,X */
        // -------------------------------------------------------------
        4'hC: begin MS <= MSINC; TR <= XDin[7:0]; PC <= PCINC; Cout <= XDin[8]; end
        4'hD: begin MS <= LATAD; EA <= EADIH;     {AM, RD} <= {1'b1, ENARD}; end

        /* Absolute,Y */
        // -------------------------------------------------------------
        4'hE: begin MS <= MSINC; TR <= YDin[7:0]; PC <= PCINC; Cout <= YDin[8]; end
        4'hF: begin MS <= LATAD; EA <= EADIH;     {AM, RD} <= {1'b1, ENARD}; end

        /* Отложенный такт (для адресации) */
        `LAT1: MS <= `EXEC;

        /* Исполнение инструкции */
        // -------------------------------------------------------------

        /* Исполнение инструкции */
        `EXEC: begin

            RD  <= 1'b0;    /* Сброс такта для PPU */
            PCL <= PC[7:0]; /* Для JSR */

            /* Инкремент PC по завершении разбора адреса */
            if (HOP | ISBRK) PC <= PCINC;

            casex (opcode)

                /* STA/STY/STX для АЛУ */
                8'b100_xxx_01,
                8'b100_xx1_x0: {AM, MS, WREQ, DOUT} <= {1'b0, 5'h0,  1'b1, AR};

                /* JMP (IND) */
                8'b011_011_00: {MS, TR, EA} <= {MSINC, DIN, EA[15:8], EAINC[7:0]};

                /* ROL/ROR/ASR/LSR/DEC/INC <mem> */
                8'b0xx_xx1_10,
                8'b11x_xx1_10: {AM, MS, WREQ, DOUT} <= {1'b0, MSINC, 1'b1, AR};

                /* JSR: Записываем в стек */
                8'b001_000_00: {AS, MS, WREQ, DOUT} <= {1'b1, MSINC, 1'b1, PC[15:8]};

                /* BRK или NMI */
                8'b000_000_00: {AS, MS, WREQ, DOUT} <= {1'b1, MSINC, 1'b1, ISBRK ? PCINC[15:8] : PC[15:8]};

                /* RTS, RTI */
                8'b01x_000_00: {AS, MS} <= {1'b1, MSINC};

                /* PHP */
                8'b000_010_00: {AS, MS, WREQ, DOUT} <= {1'b1, MSINC, 1'b1, P[7:6], 2'b11, P[3:0]};

                /* PHA */
                8'b010_010_00: {AS, MS, WREQ, DOUT} <= {1'b1, MSINC, 1'b1, A};

                /* PLP, PLA */
                8'b0x1_010_00: {AS, MS} <= {1'b1, MSINC};

                // По умолчанию, завершение инструкции
                default: {AM, MS} <= 6'b0_00000;

            endcase

        end

        /* Для особых инструкции */
        // -------------------------------------------------------------

        `EXEC2: casex (opcode)

            /* JSR */ 8'b001_000_00: begin MS <= MSINC; DOUT <= PCL; end
            /* BRK */ 8'b000_000_00: begin MS <= MSINC; DOUT <= PC[7:0]; end
            /* RTS */ 8'b011_000_00: begin MS <= MSINC; PC[7:0] <= DIN; end
            /* RTI */ 8'b010_000_00: begin MS <= MSINC; end
            /* JMP */ 8'b011_011_00: begin MS <= 1'b0;  PC <= EADIN; AM <= 1'b0; end  /* Indirect */
            /* PHx */ 8'b0x0_010_00: begin MS <= 1'b0;  {AM, AS, WREQ} <= 3'b000; end /* PHP, PHA */
                            default: begin MS <= MSINC; {AM, AS, WREQ} <= 3'b000; end

        endcase

        `EXEC3: casex (opcode)

            /* BRK */ 8'b000_000_00: begin MS <= MSINC; DOUT <= {P[7:6], 2'b11, P[3:0]}; end
            /* JSR */ 8'b001_000_00: begin MS <= 1'b0;  PC <= EA; {AS, WREQ, AM} <= 3'b000; end
            /* RTS */ 8'b011_000_00: begin MS <= MSINC; PC[15:8] <= DIN; end
            /* RTI */ 8'b010_000_00: begin MS <= MSINC; PC[7:0]  <= DIN;  end
                            default: begin MS <= 1'b0; end

        endcase

        `EXEC4: casex (opcode)

            /* BRK */ 8'b000_000_00: begin MS <= MSINC; {AS, AM, WREQ} <= 3'b010; EA <= {12'hFFF, 1'b1, IRQ, 1'b0}; end
            /* RTS */ 8'b011_000_00: begin MS <= `REL2; /* +1T */ {AS, AM} <= 2'b00; PC <= PCINC; end
            /* RTI */ 8'b010_000_00: begin MS <= MSINC; AS <= 1'b0; PC[15:8] <= DIN; end
            /* PLx */ 8'b0x1_010_00: begin MS <= MSINC; AS <= 1'b0; end /* PLA, PLP */

        endcase

        `EXEC5: casex (opcode)

            /* BRK */ 8'b000_000_00: begin MS <= MSINC; TR <= DIN; EA <= EAINC; end
            /* RTI */ 8'b010_000_00: begin {MS, LN} <= {5'h0, 1'b1}; end

        endcase

        /* BRK/RTI */
        `EXEC6: begin MS <= 1'b0; AM <= 1'b0; PC <= EADIN;  end

        /* Исполнение инструкции B<cc> */
        // -------------------------------------------------------------

        `REL: begin

            if (BR) begin PC <= PCRel; MS <= PCRel[15:8] == PC[15:8] ? `REL2 : `REL1; end
            else    begin PC <= PCINC; MS <= 1'b0; end

        end

        `REL1: begin MS <= MSINC; end /* +2T если превышение границ */
        `REL2: begin MS <= 1'b0; end  /* +1T если переход */

        /* Сброс */
        `RST:  begin MS <= MSINC; PC <= 16'hFFFC;  end
        `RST2: begin MS <= MSINC; PC <= PCINC; TR <= DIN; end
        `RST3: begin PC <= EADIN; MS <= 1'b0; end

    endcase

    /* Отключение записи для DMA */
    else begin

        WREQ <= 1'b0;

    end
end

/* Подготовка данных на этапах исполнения */
// ---------------------------------------------------------------------

always @* begin

    alu = {1'b0, opcode[7:5]}; /* По умолчанию */
    RA  = 2'b00; /* A */
    RB  = 2'b00; /* DIN */
    WR  = 1'b0;
    FW  = 1'b0;
    SW  = 1'b0;  /* Stack Write */
    ACC = 1'b0;
    BR  = 1'b0; /* Условие выполнения Branch */
    SEI = 1'b0; /* Set Interrupt Flag */
    ENARD = 1'b1;

    /* Все методы адресации разрешить читать из PPU, кроме STA */
    casex (opcode)

        8'b100_xxx_01,
        8'b100_xx1_x0: ENARD = 1'b0;

    endcase

    case (opcode[7:6])

        /* S */ 2'b00: BR = (P[7] == opcode[5]);
        /* V */ 2'b01: BR = (P[6] == opcode[5]);
        /* C */ 2'b10: BR = (P[0] == opcode[5]);
        /* Z */ 2'b11: BR = (P[1] == opcode[5]);

    endcase

    case (MS)
    `EXEC: casex (opcode)

        /* STA */
        8'b100_xxx_01: RA <= {2'b00};

        /* STX */
        8'b100_xx1_10: RA <= {2'b01};

        /* STY */
        8'b100_xx1_00: RA <= {2'b10};

        /* CMP */
        8'b110_xxx_01: FW = opcode[6];

        /* ORA, AND, EOR, ADC, STA, LDA, CMP, SBC */
        8'bxxx_xxx_01: {WR, FW} = 2'b11;

        /* Transfer */
        8'b100_010_00: {alu, RA, RB, WR, FW} = 10'b1110_10_10_11; /* DEY */
        8'b110_010_10: {alu, RA, RB, WR, FW} = 10'b1110_01_01_11; /* DEX */
        8'b110_010_00: {alu, RA, RB, WR, FW} = 10'b1111_10_10_11; /* INY */
        8'b111_010_00: {alu, RA, RB, WR, FW} = 10'b1111_01_01_11; /* INX */

        /* TAX, TAY */
        8'b101_010_10: {alu, RB, RA, FW, WR, ACC} = 11'b0101_00_01_1_1_1; /* TAX */
        8'b101_010_00: {alu, RB, RA, FW, WR, ACC} = 11'b0101_00_10_1_1_1; /* TAY */
        8'b100_110_10: {alu, RB, SW}              = 8'b0101_01_1;         /* TXS */
        8'b100_110_00: {alu, RB, FW, WR}          = 8'b0101_10_1_1;       /* TYA */
        8'b100_010_10: {alu, RB, FW, WR}          = 8'b0101_01_1_1;       /* TXA */
        8'b101_110_10: {alu, RB, RA, FW, WR}      = 10'b0101_11_01_11;    /* TSX */

        /* ROL, ROR, ASL, LSR ACC */
        8'b0xx_010_10: {alu, ACC, RB, RA, FW, WR} = {2'b10, opcode[6:5], 1'b1, 6'b00_00_11};

        /* ASL,ROL,LSR,ROR <mem> */
        8'b0xx_xx1_10: {alu, FW} = {2'b10, opcode[6:5], 1'b1};

        /* DEC/INC */
        8'b11x_xx1_10: {alu, FW} = {3'b111, opcode[5], 1'b1};

        /* LDY */
        8'b101_xx1_00,
        8'b101_000_00: {WR, RA, FW} = 4'b1_10_1;

        /* LDX */
        8'b101_xx1_10,
        8'b101_000_10: {WR, RA, FW} = 4'b1_01_1;

        /* CPY */
        8'b110_xx1_00,
        8'b110_000_00: {RA, FW} = 3'b10_1;

        /* CPX */
        8'b111_xx1_00,
        8'b111_000_00: {alu, RA, FW} = 7'b0110_01_1;

        /* CLC, SEC, CLI, SEI, CLV, CLD, SED */
        8'bxxx_110_00: {alu, FW} = 5'b1100_1;

        /* BIT */
        8'b001_0x1_00: {alu, FW} = 5'b1101_1;

        /* RTS, RTI, PLP, PLA */
        8'b01x_000_00,
        8'b0x1_010_00: {alu, RB, SW} = 7'b1111_11_1; /* S = S+1 */

    endcase

    /* Специальные инструкции управления */
    `EXEC2: casex (opcode)

        8'b010_000_00: {alu, RB, SW, RA, WR} = 10'b1111_11_1_11_1; /* RTI */
        8'b011_000_00: {alu, RB, SW}         =  7'b1111_11_1;      /* RTS */
        8'b001_000_00,
        8'b000_000_00,
        8'b0x0_010_00: {alu, RB, SW}     = 7'b1110_11_1;  /* JSR, BRK, PHP, PHA */
        8'b001_010_00: {RA, WR}          = 3'b11_1;       /* PLP */
        8'b011_010_00: {RA, WR, FW, alu} = 8'b00_11_0101; /* PLA */

    endcase

    `EXEC3: casex (opcode)

        /* BRK */ 8'b000_000_00,
        /* JSR */ 8'b001_000_00: {alu, RB, SW} = 7'b1110_11_1;
        /* RTI */ 8'b010_000_00: {alu, RB, SW} = 7'b1111_11_1;

    endcase

    /* BRK/RTI */
    `EXEC4: casex (opcode)

        /* BRK */ 8'b000_000_00: {alu, RB, SW, SEI} = 8'b1110_11_11;

    endcase

    endcase

end

/* Запись результата */
// ---------------------------------------------------------------------

always @(posedge CLK) begin

    if (RESET) begin

        A <= 8'h00;
        X <= 8'h00;
        Y <= 8'h00;
        S <= 8'h00;
        P <= 8'h00;

    end
    else begin

        /* Писать результат АЛУ */
        if (WR) case (RA)
            2'b00: A <= AR;
            2'b01: X <= AR;
            2'b10: Y <= AR;
        endcase

        /* Флаги */
        if (SEI) /* BRK I=1, B=1 */ P <= {P[7:6], 2'b11, P[3], 1'b1, P[1:0]};
        else if (WR && RA == 2'b11) P <= DIN; /* PLP, RTI */
        else if (FW) /* Другие */   P <= AF;

        /* Записать в регистр S результат */
        if (SW) S <= AR;

    end

end

/* Вычислительное устройство */
// ---------------------------------------------------------------------

reg [3:0] alu = 4'h0;
reg [1:0] RA = 2'b00;
reg [1:0] RB = 2'b00;
reg [7:0] Ax; reg [7:0] Bx;
reg [7:0] AR; reg [7:0] AF;

always @* begin

    /* Операнд A */
    casex (RA)
        2'b00: Ax = A;
        2'b01: Ax = X;
        2'b1x: Ax = Y; /* b11 Неиспользуемый */
    endcase

    /* Операнд B */
    case (RB)
        2'b00: Bx = (ACC ? A : DIN);
        2'b01: Bx = X;
        2'b10: Bx = Y;
        2'b11: Bx = S;
    endcase

end

alu ALU(

    /* Вход */  Ax, Bx, alu, P, opcode[7:5],
    /* Выход */ AR, AF

);

endmodule
