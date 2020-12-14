all:
	a6 main.asm -o main.obj

	# Скомпилить для verilog
	cd .. && ./util prg test/main.obj > prg.hex && make

	# Запуск отладчика
	cd ../app && ./nes ../test/main.obj
