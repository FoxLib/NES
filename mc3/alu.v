module alu(

    /* Входящие данные */
    input  wire [7:0] A,        /* Значение A */
    input  wire [7:0] B,        /* Значение B */
    input  wire [3:0] ALU,      /* Режим АЛУ */
    input  wire [7:0] P,        /* Флаги на вход */
    input  wire [2:0] OP,       /* opcode[7:5] */
    
    /* Результат */
    output wire [7:0] AR,       /* Результат */
    output reg  [7:0] AF        /* Флаги */    
        
);

assign AR = R[7:0];

/* Результат исполнения */
reg  [8:0] R; 

/* Статусы ALU */
wire Zero  = ~|R[7:0];    /* Тест на Zero */
wire Sign  =  R[7];        /* Флаг нуля */
wire oADC  = (A[7] ^ B[7] ^ 1'b1) & (A[7] ^ R[7]); /* Переполнение ADC */
wire oSBC  = (A[7] ^ B[7] ^ 1'b0) & (A[7] ^ R[7]); /* Переполнение SBC */
wire Cin   =  P[0];
wire Carry =  R[8];

always @* begin

    /* Расчет результата */
    case (ALU)

        /* ORA */ 4'b0000: R = A | B;
        /* AND */ 4'b0001: R = A & B;
        /* EOR */ 4'b0010: R = A ^ B;
        /* ADC */ 4'b0011: R = A + B + Cin;
        /* STA */ 4'b0100: R = A;
        /* LDA */ 4'b0101: R = B;
        /* CMP */ 4'b0110: R = A - B;
        /* SBC */ 4'b0111: R = A - B - !Cin;
        /* ASL */ 4'b1000: R = {B[6:0], 1'b0};
        /* ROL */ 4'b1001: R = {B[6:0], P[0]};
        /* LSR */ 4'b1010: R = {1'b0, B[7:1]};
        /* ROR */ 4'b1011: R = {P[0], B[7:1]};
        /* BIT */ 4'b1101: R = A & B;
        /* DEC */ 4'b1110: R = B - 1;
        /* INC */ 4'b1111: R = B + 1;

    endcase
    
    /* Расчет флагов */
    casex (ALU) 
        
        /* ORA, AND, EOR, LDA, STA */
        4'b000x, /* ORA, AND */
        4'b0010, /* EOR */
        4'b010x, /* STA, LDA */
        4'b111x: /* DEC, INC */
                 AF = {Sign,       P[6:2], Zero, P[0]};
        
        /* ADC */
        4'b0011: AF = {Sign, oADC, P[5:2], Zero, Carry}; 
        
        /* CMP */
        4'b0110: AF = {Sign,       P[6:2], Zero, ~Carry};
        
        /* SBC */
        4'b0111: AF = {Sign, oSBC, P[5:2], Zero, ~Carry};        
        
        /* Сдвиговые */
        /* ASL, ROL */
        4'b100x: AF = {Sign, P[6:2], Zero, B[7]};
        
        /* LSR, ROR */
        4'b101x: AF = {Sign, P[6:2], Zero, B[0]};
        
        /* Флаговые */
        4'b1100: casex (OP)
        
            /* CLC */ 3'b00x: AF = {P[7:1], OP[0]};
            /* CLI */ 3'b01x: AF = {P[7:3], OP[0], P[1:0]};
            /* CLV */ 3'b101: AF = {P[7],   1'b0,  P[5:0]};
            /* CLD */ 3'b11x: AF = {P[7:4], OP[0], P[2:0]};
            
        endcase
        
        /* BIT */
        4'b1101: AF = {B[7:6], P[5:2], Zero, P[0]};
         
    endcase
    
end

endmodule
