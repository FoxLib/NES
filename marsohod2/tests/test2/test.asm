
        .org    $8000
  
        lda #$20
        sta $2006
        lda #$00
        sta $2006
        
        ldx 0
AF        
        lda #$00
        sta $2007
        dex
        bne AF
        
        
        lda #$04
        sta $2003

        lda #$10
        sta $2004
        lda #$00
        sta $2004
        lda #$00
        sta $2004
        lda #$30
        sta $2004

        lda #$00
        sta $2006
        lda #$00
        sta $2006
        
LP        
        jmp LP