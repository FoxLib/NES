# Компиляция
if (gcc debugger.c -Os -lglut -lGL -Wall -o d6502)
then
    cp d6502 /usr/local/bin

    # Исполнение
    ./d6502 battle.nes
    #./d6502 mario.nes
     
fi

