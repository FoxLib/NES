
# Копировать из NES-файла 16Кб для загрузки
dd if=loderunner.nes of=rom.bin skip=16 bs=1 count=16384
dd if=loderunner.nes of=rom.bin skip=16 bs=1 seek=16384 count=16384 conv=notrunc
dd if=loderunner.nes of=rom.bin skip=16400 bs=1 seek=32768 count=8192 conv=notrunc

# Загрузить программу в память ПЛИС
#mode com8 baud=460800 data=8
#copy rom.bin /b com8 

# Состояние устройства COM8:
# http://bsvi.ru/signaly-kvitirovaniya-rts-cts-itp-i-rs232-voobshhe/
# ---------------------------
#    Скорость:              460800
#    Четность:              Even
#    Биты данных:           8
#    Стоповые биты:         1
#    Таймаут:               OFF
#    XON/XOFF:              OFF
#    Синхронизация CTS:     OFF
#    Синхронизация DSR:     OFF
#    Чувствительность DSR:  OFF
#    Цепь DTR:              ON
#    Цепь RTS:              ON

stty -F /dev/ttyUSB1 460800 cs8 raw -clocal -cstopb -parenb 
cat rom.bin > /dev/ttyUSB1
