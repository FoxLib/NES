all: util
	iverilog -g2005-sv -DICARUS=1 cpu6502.v main.v -o main.qqq
	vvp main.qqq >> /dev/null

util: util.cc
	g++ util.cc -o util

wave:
	gtkwave main.vcd

gtk:
	gtkwave wave.gtkw

clean:
	rm -f main.qqq
	rm -rf db incremental_db simulation timing
	rm -f *.jdi *.pof *.sld *.rpt *.summary *.sof *.done *.pin *.qws *.bak *.smsg *.qws *.vcd *.qqq *.jic *.map *.qqq
	rm -f .qsys_edit greybox_tmp undo_redo.txt PLLJ_PLLSPE_INFO.txt c5_pin_model_dump.txt

