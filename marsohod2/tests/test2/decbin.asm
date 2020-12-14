
        ; Input [0000] WORD Адрес числа

        ; (11) Копировать строку из представленного адреса в $00 в $200
        ldy #$00    ; 2 Y = 0
CPL     lda ($00),y ; 1 A = ($00) + Y
        sta $200,y  ; 3 mem[200 + Y] = A
        beq EOL     ; 2 end of line
        iny         ; 1 Y++
        bne CPL     ; 2 Почти безусловный переход
ERR     ; ERROR (String out of range)
EOL     ; OK

