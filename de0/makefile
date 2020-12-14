all: 
	iverilog -g2005-sv -DICARUS=1 alu.v cpu.v eval.v ppu.v main.v -o main.qqq
	vvp main.qqq >> /dev/null

wave:
	gtkwave main.vcd

gtk:
	gtkwave wave.gtkw


# Использовать 16КБ ROM
rom16k:
	dd if=rom/unchained.nes of=chr_rom.bin bs=1 skip=16400 count=8192
	php makemif.php chr_rom.bin=0 > mem_chr.mif
	php makemif.php videoram.bin=0 > mem_vram.mif
	php makemif.php oam.bin=0 > mem_oam.mif
	rm chr_rom.bin

# Создать тестовые файлы mif для теста видеоадаптера
rom32k:
	dd if=rom/mario.nes of=chr_rom.bin bs=1 skip=32784 count=8192
	php makemif.php chr_rom.bin=0 > mem_chr.mif
	php makemif.php videoram.bin=0 > mem_vram.mif
	php makemif.php oam.bin=0 > mem_oam.mif
	rm chr_rom.bin

clean:
	rm -f main.qqq
	rm -rf db incremental_db simulation timing
	rm -f *.jdi *.pof *.sld *.rpt *.summary *.sof *.done *.pin *.qws *.bak *.smsg *.qws *.vcd *.qqq *.jic *.map *.qqq
	rm -f .qsys_edit greybox_tmp undo_redo.txt PLLJ_PLLSPE_INFO.txt c5_pin_model_dump.txt

