`timescale 1ns/1ps
`default_nettype none

// boe_param followed by result_register. Both demultiplexer outputs are
// stored, so the register is 2*WIDTH bits wide: result = {y1, y0}.
module boe_registered #(
    parameter WIDTH = 8
) (
    input wire clk,
    input wire rst,
    input wire [WIDTH-1:0] d,
    input wire sel,
    output wire [2*WIDTH-1:0] result
);
    wire [WIDTH-1:0] y0;
    wire [WIDTH-1:0] y1;

    boe_param #(.WIDTH(WIDTH)) u_demux (
        .d(d), .sel(sel), .y0(y0), .y1(y1)
    );

    result_register #(.WIDTH(2*WIDTH)) u_reg (
        .clk(clk), .rst(rst), .data({y1, y0}), .result(result)
    );
endmodule

`default_nettype wire
