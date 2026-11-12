`timescale 1ns/1ps
`default_nettype none

// WIDTH-bit 1-to-2 demultiplexer: one boe_assign per data bit,
// all instances share the select signal.
module boe_param #(
    parameter WIDTH = 8
) (
    input wire [WIDTH-1:0] d,
    input wire sel,
    output wire [WIDTH-1:0] y0,
    output wire [WIDTH-1:0] y1
);
    genvar i;
    generate
        for (i = 0; i < WIDTH; i = i + 1) begin : g_bit
            boe_assign u_boe (
                .d(d[i]),
                .sel(sel),
                .y0(y0[i]),
                .y1(y1[i])
            );
        end
    endgenerate
endmodule

`default_nettype wire
