@echo off

REM Копировать из NES-файла 16Кб для загрузки
dd if=16-special.nes of=rom.bin skip=16 bs=1 count=32768
dd if=16-special.nes of=rom.bin skip=32784 seek=32768 bs=1 count=8192

REM Загрузить программу в память ПЛИС
mode com8 baud=460800 data=8
copy rom.bin /b com8 

pause