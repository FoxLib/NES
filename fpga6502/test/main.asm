
        .org    $8000

        cli

        ; Загрузка адреса строки
        lda     #<cox
        sta     $00
        lda     #>cox
        sta     $01
        lda     #$00
        tay
        sta     $02
        lda     #$10
        sta     $03
br1
        lda     ($00),y
        beq     br2
        sta     ($02),y
        iny
        jmp     br1
br2
        jmp     br2

cox
        .null   "nyancat"
