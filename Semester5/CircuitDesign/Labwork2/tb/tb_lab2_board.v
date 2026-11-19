`timescale 1ns/1ps

module tb_lab2_board;
    reg clk50 = 1'b0;
    reg rst_n = 1'b0;
    wire [1:0] led;
    integer started = 0;

    lab2_board dut (.CLK(clk50), .RST_N(rst_n), .LED(led));
    always #10 clk50 = ~clk50;

    always @(posedge dut.clk or negedge rst_n) begin
        if (!rst_n)
            started <= 0;
        else if (dut.start)
            started <= started + 1;
    end

    task expect_led(input [1:0] expected_led, input integer count);
        begin
            wait (led != 2'b00);
            @(negedge dut.clk);
            if (led !== expected_led || started != count)
                $fatal(1, "LED=%b, started=%0d (expected %b, %0d)",
                       led, started, expected_led, count);
        end
    endtask

    initial begin
        #20 rst_n = 1'b1;
        #30;
        if (started != 0 || led !== 2'b00)
            $fatal(1, "Self-test started before PLL lock");
        expect_led(2'b01, 2);
        repeat (5) @(negedge dut.clk);
        if (led !== 2'b01 || started != 2)
            $fatal(1, "Self-test did not keep the PASS indication");

        rst_n = 1'b0;
        #1;
        if (led !== 2'b00)
            $fatal(1, "Reset did not clear the LEDs");
        force dut.result = 48'd0;
        #20 rst_n = 1'b1;
        expect_led(2'b10, 1);

        release dut.result;
        rst_n = 1'b0;
        #20 rst_n = 1'b1;
        expect_led(2'b01, 2);
        $display("PASS: lab2_board (two vectors, mismatch indication and restart)");
        $finish;
    end

    initial begin
        #10000;
        $fatal(1, "Board simulation timeout");
    end
endmodule

// Behavioral clock source for testing the board logic with Icarus Verilog.
module pll_100mhz (
    input  wire clk50,
    output reg  clk100 = 1'b0,
    output reg  locked = 1'b0
);
    always #5 clk100 = ~clk100;
    initial begin
        repeat (4) @(posedge clk50);
        locked = 1'b1;
    end
endmodule
