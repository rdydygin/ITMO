`timescale 1ns/1ps

module tb_arithmetic;
    reg clk = 1'b0;
    reg rst = 1'b0;
    reg start = 1'b0, root_start = 1'b0;
    reg [15:0] radicand = 16'hffff;
    wire mul_done, add_done, root_done;
    wire [47:0] product;
    wire [48:0] sum;
    wire [7:0] root;

    mul_multicycle multiplier (
        .clk(clk), .rst(rst), .start(start), .a(32'hffffffff), .b(16'hffff),
        .done(mul_done), .y(product)
    );
    add_multicycle adder (
        .clk(clk), .rst(rst), .start(start), .a(48'hffffffffffff), .b(8'hff),
        .done(add_done), .y(sum)
    );
    sqrt_multicycle square_root (
        .clk(clk), .rst(rst), .start(root_start), .x(radicand), .done(root_done), .y(root)
    );

    always #5 clk = ~clk;

    integer tick, sample;
    integer expected_root = 0;
    initial begin
        #1 rst = 1'b1;
        #2 rst = 1'b0;
        @(negedge clk);
        start = 1'b1;
        root_start = 1'b1;
        @(posedge clk);
        #1;
        if ({mul_done, add_done, root_done} !== 3'b000)
            $fatal(1, "Premature arithmetic result");
        for (tick = 1; tick <= 17; tick = tick + 1) begin
            @(negedge clk);
            start = 1'b0;
            root_start = 1'b0;
            @(posedge clk);
            #1;
            if ({mul_done, add_done, root_done} !== {(tick == 16), (tick == 3), (tick == 8)})
                $fatal(1, "Incorrect arithmetic handshake at tick %0d", tick);
        end
        if (product !== 48'hfffeffff0001 || sum !== 49'h10000000000fe || root !== 8'hff)
            $fatal(1, "Incorrect arithmetic result at maximum inputs");

        for (sample = 0; sample < 65536; sample = sample + 1) begin
            if (sample == (expected_root + 1) * (expected_root + 1))
                expected_root = expected_root + 1;
            @(negedge clk);
            radicand = sample[15:0];
            root_start = 1'b1;
            @(negedge clk);
            root_start = 1'b0;
            repeat (8) @(posedge clk);
            #1;
            if (root_done !== 1'b1 || root !== expected_root[7:0])
                $fatal(1, "sqrt(%0d) = %0d, expected %0d", sample, root, expected_root);
        end
        $display("PASS: arithmetic modules (65536 radicands, full-width product and sum)");
        $finish;
    end
endmodule
