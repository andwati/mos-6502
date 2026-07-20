; Original Snake game for this project's Easy6502-compatible machine.
; WASD moves. The playfield wraps at its edges.

.segment "CODE"

dirx=$00
diry=$01
head=$02
tail=$03
applex=$04
appley=$05
newx=$06
newy=$07
ptrlo=$08
ptrhi=$09
colour=$0a
score=$00fd
game_state=$00fb
snakex=$0900
snakey=$0a00

reset:
        ldx #$00
        lda #$00
clear:  sta $0200,x
        sta $0300,x
        sta $0400,x
        sta $0500,x
        inx
        bne clear
        lda #$01
        sta dirx
        lda #$00
        sta diry
        sta score
        sta game_state
        sta tail
        lda #$02
        sta head
        ldx #$00
        lda #$08
        sta snakex,x
        lda #$10
        sta snakey,x
        inx
        lda #$09
        sta snakex,x
        lda #$10
        sta snakey,x
        inx
        lda #$0a
        sta snakex,x
        lda #$10
        sta snakey,x
        jsr new_apple
        jsr redraw

game_loop:
        jsr delay
        jsr input
        ldx head
        lda snakex,x
        clc
        adc dirx
        and #$1f
        sta newx
        lda snakey,x
        clc
        adc diry
        and #$1f
        sta newy
        ldx tail
collision:
        lda snakex,x
        cmp newx
        bne next_part
        lda snakey,x
        cmp newy
        beq died
next_part:
        cpx head
        beq add_head
        inx
        jmp collision
add_head:
        inc head
        ldx head
        lda newx
        sta snakex,x
        lda newy
        sta snakey,x
        lda #$01
        sta colour
        jsr plot
        lda newx
        cmp applex
        bne move_tail
        lda newy
        cmp appley
        bne move_tail
        lda score
        cmp #$ff
        beq score_full
        inc score
score_full:
        jsr new_apple
        jmp game_loop
move_tail:
        ldx tail
        lda snakex,x
        sta newx
        lda snakey,x
        sta newy
        lda #$00
        sta colour
        jsr plot
        inc tail
        jmp game_loop

died:  lda #$01
        sta game_state
        lda #$02
        sta colour
        ldx tail
death_draw:
        lda snakex,x
        sta newx
        lda snakey,x
        sta newy
        txa
        pha
        jsr plot
        pla
        tax
        cpx head
        beq death_wait
        inx
        jmp death_draw
death_wait:
        lda $00ff
        cmp #'r'
        bne death_wait
        jmp reset

input: lda $00ff
        cmp #'w'
        beq up
        cmp #'s'
        beq down
        cmp #'a'
        beq left
        cmp #'d'
        beq right
        rts
up:    lda diry
        cmp #$01
        beq input_done
        lda #$00
        sta dirx
        lda #$ff
        sta diry
        rts
down:  lda diry
        cmp #$ff
        beq input_done
        lda #$00
        sta dirx
        lda #$01
        sta diry
        rts
left:  lda dirx
        cmp #$01
        beq input_done
        lda #$ff
        sta dirx
        lda #$00
        sta diry
        rts
right: lda dirx
        cmp #$ff
        beq input_done
        lda #$01
        sta dirx
        lda #$00
        sta diry
input_done:
        rts

new_apple:
        lda $00fe
        and #$1f
        sta applex
        lda $00fe
        and #$1f
        sta appley
        lda applex
        sta newx
        lda appley
        sta newy
        lda #$07
        sta colour
        jmp plot

redraw:
        ldx tail
draw_loop:
        lda snakex,x
        sta newx
        lda snakey,x
        sta newy
        txa
        pha
        lda #$01
        sta colour
        jsr plot
        pla
        tax
        cpx head
        beq draw_done
        inx
        jmp draw_loop
draw_done:
        rts

; Plot colour at (newx,newy), using a 16-bit zero-page pointer.
plot:  lda #$00
        sta ptrhi
        lda newy
        ldx #$05
plot_shift:
        asl
        rol ptrhi
        dex
        bne plot_shift
        clc
        adc newx
        sta ptrlo
        lda ptrhi
        adc #$02
        sta ptrhi
        ldy #$00
        lda colour
        sta (ptrlo),y
        rts

; About 100,000 cycles at a 1 MHz CPU, for roughly 10 moves/second.
delay: ldx #$50
delay_outer:
        ldy #$ff
delay_inner:
        dey
        bne delay_inner
        dex
        bne delay_outer
        rts
