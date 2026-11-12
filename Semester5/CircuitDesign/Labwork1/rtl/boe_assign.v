`timescale 1ns/1ps
`default_nettype none

// 1-to-2 demultiplexer built only from continuous assignments.
// sel = 0 routes d to y0, sel = 1 routes d to y1; the other output is 0.
module boe_assign (
    input wire d,
    input wire sel,
    output wire y0,
    output wire y1
);
    assign y0 = d & ~sel;
    assign y1 = d & sel;
endmodule

`default_nettype wire
