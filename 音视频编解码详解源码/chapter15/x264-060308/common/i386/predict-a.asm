;*****************************************************************************
;* predict-a.asm: h264 encoder library
;*****************************************************************************
;* Copyright (C) 2005 x264 project
;*
;* Authors: Loren Merritt <lorenm@u.washington.edu>
;*
;* This program is free software; you can redistribute it and/or modify
;* it under the terms of the GNU General Public License as published by
;* the Free Software Foundation; either version 2 of the License, or
;* (at your option) any later version.
;*
;* This program is distributed in the hope that it will be useful,
;* but WITHOUT ANY WARRANTY; without even the implied warranty of
;* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;* GNU General Public License for more details.
;*
;* You should have received a copy of the GNU General Public License
;* along with this program; if not, write to the Free Software
;* Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111, USA.
;*****************************************************************************

BITS 32

;=============================================================================
; Macros and other preprocessor constants
;=============================================================================

%include "i386inc.asm"

; this is faster than a constant [edx + Y*FDEC_STRIDE]
%macro STORE8x8 2
    movq        [edx +   ecx], %1       ; 0
    movq        [edx + 2*ecx], %1       ; 1
    movq        [edx + 4*ecx], %1       ; 3
    movq        [edx + 8*ecx], %2       ; 7
    add         edx, eax
    movq        [edx        ], %1       ; 2
    movq        [edx + 2*ecx], %2       ; 4
    movq        [edx +   eax], %2       ; 5
    movq        [edx + 4*ecx], %2       ; 6
%endmacro

%macro SAVE_0_1 1
    movq        [%1]         , mm0
    movq        [%1 + 8]     , mm1
%endmacro

%macro SAVE_0_0 1
    movq        [%1]         , mm0
    movq        [%1 + 8]     , mm0
%endmacro


SECTION_RODATA

ALIGN 8
pw_2: times 4 dw 2
pw_8: times 4 dw 8
pb_1: times 8 db 1
pw_3210:
    dw 0
    dw 1
    dw 2
    dw 3

;=============================================================================
; Code
;=============================================================================

SECTION .text

cglobal predict_8x8_v_mmxext
cglobal predict_8x8_dc_core_mmxext
cglobal predict_8x8c_v_mmx
cglobal predict_8x8c_dc_core_mmxext
cglobal predict_8x8c_p_core_mmxext
cglobal predict_16x16_p_core_mmxext
cglobal predict_16x16_v_mmx
cglobal predict_16x16_dc_core_mmxext
cglobal predict_16x16_dc_top_mmxext

%macro PRED8x8_LOWPASS 2
    movq        mm3, mm1
    pavgb       mm1, mm2
    pxor        mm2, mm3
    movq        %1 , %2
    pand        mm2, [pb_1 GOT_ebx]
    psubusb     mm1, mm2
    pavgb       %1 , mm1     ; %1 = (t[n-1] + t[n]*2 + t[n+1] + 2) >> 2
%endmacro

%macro PRED8x8_LOAD_TOP 0
    mov         edx, [picesp + 4]
    mov         ecx, FDEC_STRIDE
    mov         eax, [picesp + 8]
    sub         edx, ecx

    and         eax, 12
    movq        mm1, [edx-1]
    movq        mm2, [edx+1]

    cmp         eax, byte 8
    jge         .have_topleft
    mov         al,  [edx]
    mov         ah,  al
    pinsrw      mm1, eax, 0
    mov         eax, [picesp + 8]
.have_topleft:

    and         eax, byte 4
    jne         .have_topright
    mov         al,  [edx+7]
    mov         ah,  al
    pinsrw      mm2, eax, 3
.have_topright:

    PRED8x8_LOWPASS mm0, [edx]
%endmacro

;-----------------------------------------------------------------------------
;
; void predict_8x8_v_mmxext( uint8_t *src, int i_neighbors )
;
;-----------------------------------------------------------------------------

ALIGN 16
predict_8x8_v_mmxext:
    picpush     ebx
    picgetgot   ebx

    PRED8x8_LOAD_TOP
    lea         eax, [ecx + 2*ecx]
    STORE8x8    mm0, mm0

    picpop      ebx
    ret

;-----------------------------------------------------------------------------
;
; void predict_8x8_dc_core_mmxext( uint8_t *src, int i_neighbors, uint8_t *pix_left );
;
;-----------------------------------------------------------------------------

ALIGN 16
predict_8x8_dc_core_mmxext:
    picpush     ebx
    picgetgot   ebx

    mov         eax, [picesp + 12]
    movq        mm1, [eax-1]
    movq        mm2, [eax+1]
    PRED8x8_LOWPASS mm4, [eax]

    PRED8x8_LOAD_TOP

    pxor        mm1, mm1
    psadbw      mm0, mm1
    psadbw      mm4, mm1
    paddw       mm0, [pw_8 GOT_ebx]
    paddw       mm0, mm4
    psrlw       mm0, 4
    pshufw      mm0, mm0, 0
    packuswb    mm0, mm0

    lea         eax, [ecx + 2*ecx]
    STORE8x8    mm0, mm0

    picpop      ebx
    ret

;-----------------------------------------------------------------------------
;
; void predict_8x8c_v_mmx( uint8_t *src )
;
;-----------------------------------------------------------------------------

ALIGN 16
predict_8x8c_v_mmx :
    mov         edx, [esp + 4]
    mov         ecx, FDEC_STRIDE
    sub         edx, ecx
    movq        mm0, [edx]
    lea         eax, [ecx + 2*ecx]
    STORE8x8    mm0, mm0
    ret

;-----------------------------------------------------------------------------
;
; void predict_8x8c_dc_core_mmxext( uint8_t *src, int s2, int s3 )
;
;-----------------------------------------------------------------------------

ALIGN 16
predict_8x8c_dc_core_mmxext:
    picpush     ebx
    picgetgot   ebx

    mov         edx, [picesp + 4]
    mov         ecx, FDEC_STRIDE
    sub         edx, ecx
    lea         eax, [ecx + 2*ecx]

    movq        mm0, [edx]
    pxor        mm1, mm1
    pxor        mm2, mm2
    punpckhbw   mm1, mm0
    punpcklbw   mm0, mm2
    psadbw      mm1, mm2        ; s1
    psadbw      mm0, mm2        ; s0

    paddw       mm0, [picesp +  8]
    pshufw      mm2, [picesp + 12], 0
    psrlw       mm0, 3
    paddw       mm1, [pw_2 GOT_ebx]
    movq        mm3, mm2
    pshufw      mm1, mm1, 0
    pshufw      mm0, mm0, 0     ; dc0 (w)
    paddw       mm3, mm1
    psrlw       mm3, 3          ; dc3 (w)
    psrlw       mm2, 2          ; dc2 (w)
    psrlw       mm1, 2          ; dc1 (w)

    packuswb    mm0, mm1        ; dc0,dc1 (b)
    packuswb    mm2, mm3        ; dc2,dc3 (b)

    STORE8x8    mm0, mm2

    picpop      ebx
    ret

;-----------------------------------------------------------------------------
;
; void predict_8x8c_p_core_mmxext( uint8_t *src, int i00, int b, int c )
;
;-----------------------------------------------------------------------------

ALIGN 16
predict_8x8c_p_core_mmxext:
    picpush     ebx
    picgetgot   ebx

    mov         edx, [picesp + 4]
    mov         ecx, FDEC_STRIDE
    pshufw      mm0, [picesp + 8], 0
    pshufw      mm2, [picesp +12], 0
    pshufw      mm4, [picesp +16], 0
    movq        mm1, mm2
    pmullw      mm2, [pw_3210 GOT_ebx]
    psllw       mm1, 2
    paddsw      mm0, mm2        ; mm0 = {i+0*b, i+1*b, i+2*b, i+3*b}
    paddsw      mm1, mm0        ; mm1 = {i+4*b, i+5*b, i+6*b, i+7*b}

    mov         eax, 8
ALIGN 4
.loop:
    movq        mm5, mm0
    movq        mm6, mm1
    psraw       mm5, 5
    psraw       mm6, 5
    packuswb    mm5, mm6
    movq        [edx], mm5

    paddsw      mm0, mm4
    paddsw      mm1, mm4
    add         edx, ecx
    dec         eax
    jg          .loop

    nop
    picpop      ebx
    ret

;-----------------------------------------------------------------------------
;
; void predict_16x16_p_core_mmxext( uint8_t *src, int i00, int b, int c )
;
;-----------------------------------------------------------------------------

ALIGN 16
predict_16x16_p_core_mmxext:

    picpush     ebx
    picgetgot   ebx

    mov         edx, [picesp + 4]
    mov         ecx, FDEC_STRIDE
    pshufw      mm0, [picesp + 8], 0
    pshufw      mm2, [picesp +12], 0
    pshufw      mm4, [picesp +16], 0
    movq        mm5, mm2
    movq        mm1, mm2
    pmullw      mm5, [pw_3210 GOT_ebx]
    psllw       mm2, 3
    psllw       mm1, 2
    movq        mm3, mm2
    paddsw      mm0, mm5        ; mm0 = {i+ 0*b, i+ 1*b, i+ 2*b, i+ 3*b}
    paddsw      mm1, mm0        ; mm1 = {i+ 4*b, i+ 5*b, i+ 6*b, i+ 7*b}
    paddsw      mm2, mm0        ; mm2 = {i+ 8*b, i+ 9*b, i+10*b, i+11*b}
    paddsw      mm3, mm1        ; mm3 = {i+12*b, i+13*b, i+14*b, i+15*b}

    mov         eax, 16
ALIGN 4
.loop:
    movq        mm5, mm0
    movq        mm6, mm1
    psraw       mm5, 5
    psraw       mm6, 5
    packuswb    mm5, mm6
    movq        [edx], mm5

    movq        mm5, mm2
    movq        mm6, mm3
    psraw       mm5, 5
    psraw       mm6, 5
    packuswb    mm5, mm6
    movq        [edx+8], mm5

    paddsw      mm0, mm4
    paddsw      mm1, mm4
    paddsw      mm2, mm4
    paddsw      mm3, mm4
    add         edx, ecx
    dec         eax
    jg          .loop

    nop
    picpop      ebx
    ret

;-----------------------------------------------------------------------------
;
; void predict_16x16_v_mmx( uint8_t *src )
;
;-----------------------------------------------------------------------------

ALIGN 16
predict_16x16_v_mmx :

    mov         edx, [esp + 4]
    mov         ecx, FDEC_STRIDE
    sub         edx, ecx                ; edx <-- line -1

    movq        mm0, [edx]
    movq        mm1, [edx + 8]
    lea         eax, [ecx + 2*ecx]      ; eax <-- 3* stride

    SAVE_0_1    (edx + ecx)             ; 0
    SAVE_0_1    (edx + 2 * ecx)         ; 1
    SAVE_0_1    (edx + eax)             ; 2
    SAVE_0_1    (edx + 4 * ecx)         ; 3
    SAVE_0_1    (edx + 2 * eax)         ; 5
    SAVE_0_1    (edx + 8 * ecx)         ; 7
    SAVE_0_1    (edx + 4 * eax)         ; 11
    add         edx, ecx                ; edx <-- line 0
    SAVE_0_1    (edx + 4 * ecx)         ; 4
    SAVE_0_1    (edx + 2 * eax)         ; 6
    SAVE_0_1    (edx + 8 * ecx)         ; 8
    SAVE_0_1    (edx + 4 * eax)         ; 12
    lea         edx, [edx + 8 * ecx]    ; edx <-- line 8
    SAVE_0_1    (edx + ecx)             ; 9
    SAVE_0_1    (edx + 2 * ecx)         ; 10
    lea         edx, [edx + 4 * ecx]    ; edx <-- line 12
    SAVE_0_1    (edx + ecx)             ; 13
    SAVE_0_1    (edx + 2 * ecx)         ; 14
    SAVE_0_1    (edx + eax)             ; 15

    ret

;-----------------------------------------------------------------------------
;
; void predict_16x16_dc_core_mmxext( uint8_t *src, int i_dc_left )
;
;-----------------------------------------------------------------------------

%macro PRED16x16_DC 3
    mov         edx, [%3 + 4]
    mov         ecx, FDEC_STRIDE
    sub         edx, ecx                ; edx <-- line -1

    pxor        mm0, mm0
    pxor        mm1, mm1
    psadbw      mm0, [edx]
    psadbw      mm1, [edx + 8]
    paddusw     mm0, mm1
    paddusw     mm0, %1                 ; FIXME is stack alignment guaranteed?
    psrlw       mm0, %2                 ; dc
    push        edi
    pshufw      mm0, mm0, 0
    lea         eax, [ecx + 2*ecx]      ; eax <-- 3* stride
    packuswb    mm0, mm0                ; dc in bytes

    mov         edi, 4
ALIGN 4
.loop:
    SAVE_0_0    (edx + ecx)             ; 0
    SAVE_0_0    (edx + 2 * ecx)         ; 1
    SAVE_0_0    (edx + eax)             ; 2
    SAVE_0_0    (edx + 4 * ecx)         ; 3
    dec         edi
    lea         edx, [edx + 4 * ecx]
    jg          .loop

    pop         edi
%endmacro

ALIGN 16
predict_16x16_dc_core_mmxext:
    PRED16x16_DC [esp+8], 5, esp
    ret

ALIGN 16
predict_16x16_dc_top_mmxext:
    picpush ebx
    picgetgot ebx
    PRED16x16_DC [pw_8 GOT_ebx], 4, picesp
    picpop ebx
    ret

