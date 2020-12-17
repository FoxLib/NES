/*****************************************************************************
 *                                                                           *
 * Module:       Altera_UP_PS2_Data_In                                       *
 * Description:                                                              *
 *      This module accepts incoming data from a PS2 core.                   *
 *                                                                           *
 *****************************************************************************/


module Altera_UP_PS2_Data_In (
	// Inputs
	clk,
	reset,

	wait_for_incoming_data,
	start_receiving_data,

	ps2_clk_posedge,
	ps2_clk_negedge,
	ps2_data,

	// Bidirectionals

	// Outputs
	received_data,
	received_data_en			// If 1 - new data has been received
);


/*****************************************************************************
 *                           Parameter Declarations                          *
 *****************************************************************************/


/*****************************************************************************
 *                             Port Declarations                             *
 *****************************************************************************/
// Inputs
input				clk;
input				reset;

input				wait_for_incoming_data;
input				start_receiving_data;

input				ps2_clk_posedge;
input				ps2_clk_negedge;
input			 	ps2_data;

// Bidirectionals

// Outputs
output reg	[7:0]	received_data;

output reg		 	received_data_en;

/*****************************************************************************
 *                           Constant Declarations                           *
 *****************************************************************************/
// states
localparam	PS2_STATE_0_IDLE			= 3'h0,
			PS2_STATE_1_WAIT_FOR_DATA	= 3'h1,
			PS2_STATE_2_DATA_IN			= 3'h2,
			PS2_STATE_3_PARITY_IN		= 3'h3,
			PS2_STATE_4_STOP_IN			= 3'h4;

/*****************************************************************************
 *                 Internal wires and registers Declarations                 *
 *****************************************************************************/
// Internal Wires
reg			[3:0]	data_count;
reg			[7:0]	data_shift_reg;

// State Machine Registers
reg			[2:0]	ns_ps2_receiver;
reg			[2:0]	s_ps2_receiver;

/*****************************************************************************
 *                         Finite State Machine(s)                           *
 *****************************************************************************/

always @(posedge clk)
begin
	if (reset == 1'b1)
		s_ps2_receiver <= PS2_STATE_0_IDLE;
	else
		s_ps2_receiver <= ns_ps2_receiver;
end

always @(*)
begin

	// Defaults
	ns_ps2_receiver = PS2_STATE_0_IDLE;

    case (s_ps2_receiver)
    
	PS2_STATE_0_IDLE:
    
		begin
			if ((wait_for_incoming_data == 1'b1) && (received_data_en == 1'b0))
				ns_ps2_receiver = PS2_STATE_1_WAIT_FOR_DATA;
                
			else if ((start_receiving_data == 1'b1) && (received_data_en == 1'b0))
				ns_ps2_receiver = PS2_STATE_2_DATA_IN;
                
			else
				ns_ps2_receiver = PS2_STATE_0_IDLE;
		end
        
	PS2_STATE_1_WAIT_FOR_DATA:
    
		begin
			if ((ps2_data == 1'b0) && (ps2_clk_posedge == 1'b1))
				ns_ps2_receiver = PS2_STATE_2_DATA_IN;
			else if (wait_for_incoming_data == 1'b0)
				ns_ps2_receiver = PS2_STATE_0_IDLE;
			else
				ns_ps2_receiver = PS2_STATE_1_WAIT_FOR_DATA;
		end
        
	PS2_STATE_2_DATA_IN:
    
		begin
			if ((data_count == 3'h7) && (ps2_clk_posedge == 1'b1))
				ns_ps2_receiver = PS2_STATE_3_PARITY_IN;
			else
				ns_ps2_receiver = PS2_STATE_2_DATA_IN;
		end
        
	PS2_STATE_3_PARITY_IN:
    
		begin
			if (ps2_clk_posedge == 1'b1)
				ns_ps2_receiver = PS2_STATE_4_STOP_IN;
			else
				ns_ps2_receiver = PS2_STATE_3_PARITY_IN;
		end
        
	PS2_STATE_4_STOP_IN:
    
		begin
			if (ps2_clk_posedge == 1'b1)
				ns_ps2_receiver = PS2_STATE_0_IDLE;
			else
				ns_ps2_receiver = PS2_STATE_4_STOP_IN;
		end
        
	default:
    
		begin
			ns_ps2_receiver = PS2_STATE_0_IDLE;
		end
	endcase
end

/*****************************************************************************
 *                             Sequential logic                              *
 *****************************************************************************/

always @(posedge clk)
begin

	if (reset == 1'b1) 
		data_count	<= 3'h0;
        
	else if ((s_ps2_receiver == PS2_STATE_2_DATA_IN) && (ps2_clk_posedge == 1'b1))
		data_count	<= data_count + 3'h1;
        
	else if (s_ps2_receiver != PS2_STATE_2_DATA_IN)
		data_count	<= 3'h0;
end

always @(posedge clk)
begin
	if (reset == 1'b1)
		data_shift_reg <= 8'h00;
        
	else if ((s_ps2_receiver == PS2_STATE_2_DATA_IN) && (ps2_clk_posedge == 1'b1))
		data_shift_reg	<= {ps2_data, data_shift_reg[7:1]};
end

always @(posedge clk)
begin
	if (reset == 1'b1) begin
        received_data <= 8'h00;        
    end
        
	else if (s_ps2_receiver == PS2_STATE_4_STOP_IN) begin      
		received_data  <= keyb_xt[7:0];          
    end

end

always @(posedge clk)
begin
	if (reset == 1'b1) begin    
		received_data_en <= 1'b0;        
    end
	else if ((s_ps2_receiver == PS2_STATE_4_STOP_IN) && (ps2_clk_posedge == 1'b1)) begin       
        received_data_en <= 1'b1;
    end
	else begin
		received_data_en <= 1'b0;
    end
end

/*****************************************************************************
 *                            Combinational logic                            *
 *****************************************************************************/

reg [7:0] keyb_xt;

always @* begin

    case (data_shift_reg)
    
        /* A   */ 8'h1C: keyb_xt = 8'h1E;
        /* B   */ 8'h32: keyb_xt = 8'h30;
        /* C   */ 8'h21: keyb_xt = 8'h2E;
        /* D   */ 8'h23: keyb_xt = 8'h20;
        /* E   */ 8'h24: keyb_xt = 8'h12;
        /* F   */ 8'h2B: keyb_xt = 8'h21;
        /* G   */ 8'h34: keyb_xt = 8'h22;
        /* H   */ 8'h33: keyb_xt = 8'h23;
        /* I   */ 8'h43: keyb_xt = 8'h17;
        /* J   */ 8'h3B: keyb_xt = 8'h24;
        /* K   */ 8'h42: keyb_xt = 8'h25;
        /* L   */ 8'h4B: keyb_xt = 8'h26;
        /* M   */ 8'h3A: keyb_xt = 8'h32;
        /* N   */ 8'h31: keyb_xt = 8'h31;
        /* O   */ 8'h44: keyb_xt = 8'h18;
        /* P   */ 8'h4D: keyb_xt = 8'h19;
        /* Q   */ 8'h15: keyb_xt = 8'h10;
        /* R   */ 8'h2D: keyb_xt = 8'h13;
        /* S   */ 8'h1B: keyb_xt = 8'h1F;
        /* T   */ 8'h2C: keyb_xt = 8'h14;
        /* U   */ 8'h3C: keyb_xt = 8'h16;
        /* V   */ 8'h2A: keyb_xt = 8'h2F;
        /* W   */ 8'h1D: keyb_xt = 8'h11;
        /* X   */ 8'h22: keyb_xt = 8'h2D;
        /* Y   */ 8'h35: keyb_xt = 8'h15;
        /* Z   */ 8'h1A: keyb_xt = 8'h2C;
        /* 0   */ 8'h45: keyb_xt = 8'h0B;
        /* 1   */ 8'h16: keyb_xt = 8'h02;
        /* 2   */ 8'h1E: keyb_xt = 8'h03;
        /* 3   */ 8'h26: keyb_xt = 8'h04;
        /* 4   */ 8'h25: keyb_xt = 8'h05;
        /* 5   */ 8'h2E: keyb_xt = 8'h06;
        /* 6   */ 8'h36: keyb_xt = 8'h07;
        /* 7   */ 8'h3D: keyb_xt = 8'h08;
        /* 8   */ 8'h3E: keyb_xt = 8'h09;
        /* 9   */ 8'h46: keyb_xt = 8'h0A;        
        /* ~   */ 8'h0E: keyb_xt = 8'h29;
        /* -   */ 8'h4E: keyb_xt = 8'h0C;
        /* =   */ 8'h55: keyb_xt = 8'h0D;
        /* \   */ 8'h5D: keyb_xt = 8'h2B;
        /* [   */ 8'h54: keyb_xt = 8'h1A;
        /* ]   */ 8'h5B: keyb_xt = 8'h1B;
        /* ;   */ 8'h4C: keyb_xt = 8'h27;
        /* '   */ 8'h52: keyb_xt = 8'h28;
        /* ,   */ 8'h41: keyb_xt = 8'h33;
        /* .   */ 8'h49: keyb_xt = 8'h34;
        /* /   */ 8'h4A: keyb_xt = 8'h35;
        /* BS  */ 8'h66: keyb_xt = 8'h0E;
        /* SPC */ 8'h29: keyb_xt = 8'h39;
        /* TAB */ 8'h0D: keyb_xt = 8'h0F;
        
        /* Кнопки модификации */
        /* CAP */ 8'h58: keyb_xt = 8'h3A; /* CAPS LOCK */
        /* LSH */ 8'h12: keyb_xt = 8'h2A; /* LEFT SHIFT */
        /* LCT */ 8'h14: keyb_xt = 8'h1D; /* LEFT CTRL */
        /* LAT */ 8'h11: keyb_xt = 8'h38; /* LEFT ALT */
        /* LWI */ 8'h1F: keyb_xt = 8'h5B; /* LEFT WIN */
        /* RSH */ 8'h59: keyb_xt = 8'h36; /* RIGHT SHIFT */
        /* RWI */ 8'h27: keyb_xt = 8'h5C; /* RIGHT WIN */
        /* MNU */ 8'h2F: keyb_xt = 8'h5D; /* MENU */
        /* ENT */ 8'h5A: keyb_xt = 8'h1C; /* ENTER */
        
        /* Функциональная панель */        
        /* ESC */ 8'h76: keyb_xt = 8'h01;        
        /* F1  */ 8'h05: keyb_xt = 8'h3B;
        /* F2  */ 8'h06: keyb_xt = 8'h3C;
        /* F3  */ 8'h04: keyb_xt = 8'h3D;
        /* F4  */ 8'h0C: keyb_xt = 8'h3E;
        /* F5  */ 8'h03: keyb_xt = 8'h3F;
        /* F6  */ 8'h0B: keyb_xt = 8'h40;
        /* F7  */ 8'h83: keyb_xt = 8'h41;
        /* F8  */ 8'h0A: keyb_xt = 8'h42;
        /* F9  */ 8'h01: keyb_xt = 8'h43;
        /* F10 */ 8'h09: keyb_xt = 8'h44;
        /* F11 */ 8'h78: keyb_xt = 8'h57;
        /* F12 */ 8'h07: keyb_xt = 8'h58;
        /* SCL */ 8'h7E: keyb_xt = 8'h46;
        
        /* Цифровая клавиатура */
        /* NUM */ 8'h77: keyb_xt = 8'h45;
        /* *   */ 8'h7C: keyb_xt = 8'h37;
        /* -   */ 8'h7B: keyb_xt = 8'h4A;
        /* +   */ 8'h79: keyb_xt = 8'h4E;
        /* .   */ 8'h71: keyb_xt = 8'h53;                
        /* 0   */ 8'h70: keyb_xt = 8'h52;
        /* 1   */ 8'h69: keyb_xt = 8'h4F;
        /* 2   */ 8'h72: keyb_xt = 8'h50;
        /* 3   */ 8'h7A: keyb_xt = 8'h51;
        /* 4   */ 8'h6B: keyb_xt = 8'h4B;
        /* 5   */ 8'h73: keyb_xt = 8'h4C;
        /* 6   */ 8'h74: keyb_xt = 8'h4D;
        /* 7   */ 8'h6C: keyb_xt = 8'h47;
        /* 8   */ 8'h75: keyb_xt = 8'h48;
        /* 9   */ 8'h7D: keyb_xt = 8'h49;
        
        /* E0, E1, ... */
        default: keyb_xt = data_shift_reg;
        
    endcase
end
 
/*****************************************************************************
 *                              Internal Modules                             *
 *****************************************************************************/


endmodule

