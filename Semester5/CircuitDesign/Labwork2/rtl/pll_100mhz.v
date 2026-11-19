`timescale 1ns/1ps

module pll_100mhz (
    input  wire clk50,
    output wire clk100,
    output wire locked
);
    wire [4:0] clocks;
    assign clk100 = clocks[0];

    altpll #(
        .intended_device_family("Cyclone IV E"),
        .inclk0_input_frequency(20000),
        .clk0_multiply_by(2),
        .clk0_divide_by(1),
        .clk0_duty_cycle(50),
        .clk0_phase_shift("0"),
        .compensate_clock("CLK0"),
        .operation_mode("NORMAL"),
        .port_clk0("PORT_USED"),
        .port_inclk0("PORT_USED"),
        .port_locked("PORT_USED"),
        .width_clock(5)
    ) pll_ip (
        .inclk({1'b0, clk50}),
        .areset(1'b0),
        .pllena(1'b1),
        .pfdena(1'b1),
        .clkena(6'b111111),
        .clk(clocks),
        .locked(locked)
    );
endmodule
