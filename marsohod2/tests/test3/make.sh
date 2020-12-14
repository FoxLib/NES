# Ассемблировать
a6 -fb main.asm -o main.bin

# Для Icarus Verilog
php ../tohex.php main.bin > ../../init/rom.hex

# Скомпилировать в Icarus
cd ../../ && sh make.sh && cd tests/test3

# Запустить. Нужен 8x8cp1251.fnt в test3
d6502 main.bin
