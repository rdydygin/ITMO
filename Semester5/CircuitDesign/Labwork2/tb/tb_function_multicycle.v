`timescale 1ns/1ps

module tb_function_multicycle;
    localparam integer LATENCY = 38;
    reg clk = 1'b0;
    reg rst = 1'b0;
    reg start = 1'b0;
    reg [15:0] a = 16'd0, b = 16'd0;
    wire busy, done;
    wire [47:0] y;
    wire [7:0] a_low = a[7:0], b_low = b[7:0], y_low = y[7:0];

    function_multicycle dut (
        .clk(clk), .rst(rst), .start(start), .a(a), .b(b),
        .busy(busy), .done(done), .y(y)
    );

    always #5 clk = ~clk;

    integer checks = 0;
    integer seed = 2, i;
    reg [15:0] random_a, random_b;

    function [47:0] expected_y(input [15:0] a_in, input [15:0] b_in);
        integer lo, hi, mid;
        reg [47:0] a_wide;
        begin
            lo = 0;
            hi = 256;
            while (hi - lo > 1) begin
                mid = (lo + hi) >> 1;
                if (mid * mid <= b_in)
                    lo = mid;
                else
                    hi = mid;
            end
            a_wide = {32'd0, a_in};
            expected_y = a_wide * a_wide * a_wide + lo;
        end
    endfunction

    task check_vector(input [15:0] a_in, input [15:0] b_in);
        integer tick;
        time accepted_at;
        reg [47:0] expected, previous;
        begin
            expected = expected_y(a_in, b_in);
            previous = y;
            @(negedge clk);
            a = a_in;
            b = b_in;
            start = 1'b1;
            @(posedge clk);
            accepted_at = $time;
            #1;
            if ({busy, done, y} !== {2'b10, previous})
                $fatal(1, "Incorrect handshake at start");

            for (tick = 1; tick <= LATENCY; tick = tick + 1) begin
                @(negedge clk);
                // Change inputs and issue an extra request while busy.
                start = (tick == 3 || tick == LATENCY);
                if (tick == 3) begin
                    a = ~a_in;
                    b = ~b_in;
                end
                @(posedge clk);
                #1;
                if (tick < LATENCY && {busy, done, y} !== {2'b10, previous})
                    $fatal(1, "Output changed early at tick %0d", tick);
            end
            if ({busy, done, y} !== {2'b01, expected})
                $fatal(1, "a=%h b=%h: got %h, expected %h (busy=%b done=%b)",
                       a_in, b_in, y, expected, busy, done);
            if ($time - 1 - accepted_at != 380)
                $fatal(1, "Incorrect latency");

            checks = checks + 1;
            if (checks <= 18)
                $display("a=%h b=%h y=%h, latency=%0d ns", a_in, b_in, y, $time - 1 - accepted_at);

            @(negedge clk);
            start = 1'b0;
            @(posedge clk);
            #1;
            if ({busy, done, y} !== {2'b00, expected})
                $fatal(1, "Result was not held after done");
        end
    endtask

    initial begin
        $dumpfile("build/function.vcd");
        $dumpvars(1, tb_function_multicycle, dut.state, dut.mul_done, dut.root_done,
                  dut.add_done, dut.root);
        #1 rst = 1'b1;
        #2;
        if ({busy, done, y} !== 50'd0)
            $fatal(1, "Asynchronous reset failed");
        #1 rst = 1'b0;

        check_vector(3, 15);
        check_vector(0, 0);
        check_vector(0, 16'hffff);
        check_vector(1, 1);
        check_vector(2, 2);
        check_vector(7, 48);
        check_vector(7, 49);
        check_vector(7, 50);
        check_vector(16'h00ff, 16'd255);
        check_vector(16'h0100, 16'd256);
        check_vector(16'h0101, 16'd257);
        check_vector(16'h7fff, 16'h8000);
        check_vector(16'h8000, 16'hffff);
        check_vector(16'hfffe, 16'hfe00);
        check_vector(16'hffff, 16'hfe01);
        check_vector(16'hffff, 16'hffff);
        check_vector(16'hffff, 16'd0);
        check_vector(16'h1234, 16'h5678);

        for (i = 0; i < 512; i = i + 1) begin
            random_a = $random(seed);
            random_b = $random(seed);
            check_vector(random_a, random_b);
        end

        @(negedge clk);
        start = 1'b1;
        @(negedge clk);
        start = 1'b0;
        repeat (7) @(negedge clk);
        #2 rst = 1'b1;
        #1;
        if ({busy, done, y} !== 50'd0)
            $fatal(1, "Reset while busy failed");
        #1 rst = 1'b0;
        repeat (LATENCY) begin
            @(posedge clk);
            #1;
            if ({busy, done, y} !== 50'd0)
                $fatal(1, "Cancelled request returned a result");
        end
        check_vector(5, 80);

        $display("PASS: function_multicycle (%0d vectors, %0d cycles at 100 MHz)", checks, LATENCY);
        $finish;
    end
endmodule
