// PIE (ESP32-S3 SIMD) kernel of the Reed-Solomon remainder loop (CONFIG_RS_PIE).
// The remainder of the division by the generator polynomial is 16 bytes, which is exactly
// one 128-bit Q register: byte lane j of the register holds data_mod[j]. One step of the
// division shifts that register down by one byte lane (the leading coefficient leaves,
// the next data byte enters at lane 15) and XORs the row remTable[x] into it, so a whole
// step costs one vector load and one vector XOR instead of 16 byte loads, XORs and stores.
//
// remTable is 16-byte aligned, so every row remTable[x] (16 bytes) is aligned as well and
// can be fetched by a single EE.VLD.128.

#include "sdkconfig.h"

#ifdef CONFIG_RS_IN_RAM
    .section .iram1,"ax"  // Place this function in the IRAM1 section (for faster access)
#else
    .text
#endif
    .align  4
/////////////////////////////////////////////////////////////////////////////////////////
// void rs_poly_remainder_pie(uint8_t *state, const uint8_t *tail, uint32_t count,
//                            const uint8_t *remtab)
// The main loop of RSEncode16::poly_remainder, one iteration per data byte:
//     x = state[0];
//     for (j = 0; j < 15; j++) state[j] = state[j + 1] ^ remtab[x * 16 + j];
//     state[15] = *tail++ ^ remtab[x * 16 + 15];
// state is the 16-byte remainder after the first step (16-byte aligned, in and out),
// tail points at the data bytes still to be shifted in (&data[17]), count is size - 1.
    .type   rs_poly_remainder_pie,@function
    .align  4
    .global rs_poly_remainder_pie
rs_poly_remainder_pie:
// state  - a2  (remainder, 16-byte aligned, updated in place)
// tail   - a3  (next data byte to shift in, one per iteration)
// count  - a4  (number of steps, size - 1)
// remtab - a5  (&remTable[0][0], 16-byte aligned)
    entry   a1, 16

    movi.n  a6, 1
    wur.sar_byte    a6              // EE.SRC.Q shifts the 256-bit pair right by one byte
    ee.vld.128.ip   q0, a2, 0       // q0 = remainder, byte lane j == data_mod[j]

    loopnez a4, .rs_poly_remainder_pie_loop_end
        ee.movi.32.a    q0, a6, 0   // a6 = lanes 0..3; lane 0 is the leading coefficient
        ee.vldbc.8.ip   q2, a3, 1   // broadcast the next data byte, tail++
        extui           a6, a6, 0, 8    // x = data_mod[0]
        slli            a6, a6, 4       // x * 16 (the row is 16 bytes)
        ee.src.q        q0, q0, q2  // lanes 0..14 <- 1..15, lane 15 <- the data byte
        add.n           a6, a5, a6  // &remTable[x][0]
        ee.vld.128.ip   q1, a6, 0   // remTable[x][0..15]
        ee.xorq         q0, q0, q1
    .rs_poly_remainder_pie_loop_end:

    ee.vst.128.ip   q0, a2, 0
    retw.n
