module eval(

    input wire  [ 7:0] ctrl1,       /* Управление */
    input wire         valid,       /* Отображается? */
    input wire  [31:0] sprite,      /* Информация о спрайте */
    input wire  [ 4:0] bg,          /* Текущий цвет фона в точке */
    input wire  [ 8:0] x,           /* Текущий X */
    output reg  [ 4:0] color,       /* Цвет спрайта, color[4] - палитра */
    output reg         hit          /* Существует попадание */

);

reg opaque;

/* Для расчета номера бита */
wire [2:0]  xbitn  = x - sprite[7:0];
wire [15:0] bits   = {sprite[24], sprite[ 8],
                      sprite[25], sprite[ 9],
                      sprite[26], sprite[10],
                      sprite[27], sprite[11],
                      sprite[28], sprite[12],
                      sprite[29], sprite[13],
                      sprite[30], sprite[14],
                      sprite[31], sprite[15]};

wire [3:0]  rcolor  = {sprite[17:16],               // Два старших бита атрибута
                       bits[ {xbitn[2:0], 1'b1} ],  // Старший бит
                       bits[ {xbitn[2:0], 1'b0} ]}; // Младший бит */


always @* begin

    /* a) Спрайты не видны в крайнем левом столбце b) Спрайты не отображаются */
    if ((~ctrl1[2] && x < 8) || ~ctrl1[4]) begin

        color = bg;
        hit   = 0;

    end

    /* Проверка X */
    else if (x >= sprite[7:0] && x < (sprite[7:0] + 8) && valid) begin

        // Непрозрачный, если младшие 2 бит не равны 0
        // opaque = rcolor[1:0] != 2'b00;

        // 1. Спрайт прозрачный - использовать цвет фона
        // 2. Спрайт непрозрачный
        // 2.1 Если установлен атрибут "спрайт за фоном", то если фон 0, то вывести фон
        // 2.2 В другом случае вывод точки спрайта

        hit    = (rcolor[1:0] != 2'b00);
        opaque = hit ? (sprite[16 + 5] && bg[1:0] != 2'b00 ? 0 : 1) : 0;
        color  = opaque ? {1'b1, rcolor} : bg;

    end else begin

        hit    = 0;
        color  = bg;
        
    end

end

endmodule
