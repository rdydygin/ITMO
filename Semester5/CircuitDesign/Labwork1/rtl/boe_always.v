`timescale 1ns/1ps
`default_nettype none

// 1-to-2 demultiplexer described with always @*.
// Both outputs get a default value first, so no latch is inferred.
module boe_always (
    input wire d,
    input wire sel,
    output reg y0,
    output reg y1
);
    always @* begin
        y0 = 1'b0;
        y1 = 1'b0;
        if (sel)
            y1 = d;
        else
            y0 = d;
    end
endmodule

`default_nettype wire
