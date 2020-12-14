        
        ; Тестовый файл
        
        .org $8000
        
        lda #$00
        sta $2000
        lda #$18
        sta $2001
        
        lda #$20
        sta $2006
        lda #$20
        sta $2006
        
        ldx #$00
        ldy #$00
LP1
        sty $2007
        iny
        dex
        bne LP1        
LP2        
        ; Сброс скроллинга
        stx $2006
        stx $2006        
        beq LP2
