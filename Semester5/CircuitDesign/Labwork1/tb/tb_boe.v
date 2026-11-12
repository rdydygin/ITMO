`timescale 1ns/1ps
module tb_boe;
    localparam W = 8;
    localparam RW = 4;

    // Elementary blocks: both implementations get the same inputs.
    // No initializer: the first assignment must be an event for always @*.
    reg d;
    reg sel;
    wire y0_assign, y1_assign, y0_always, y1_always;
    boe_assign u_assign (.d(d), .sel(sel), .y0(y0_assign), .y1(y1_assign));
    boe_always u_always (.d(d), .sel(sel), .y0(y0_always), .y1(y1_always));

    // Parameterized block, minimal and wide instances.
    reg n1_d = 0;
    reg n1_sel = 0;
    wire n1_y0, n1_y1;
    boe_param #(.WIDTH(1)) u_n1 (
        .d(n1_d), .sel(n1_sel), .y0(n1_y0), .y1(n1_y1)
    );

    reg [W-1:0] nw_d = 0;
    reg nw_sel = 0;
    wire [W-1:0] nw_y0, nw_y1;
    boe_param #(.WIDTH(W)) u_nw (
        .d(nw_d), .sel(nw_sel), .y0(nw_y0), .y1(nw_y1)
    );

    // Integrated block with the result register.
    reg clk = 0;
    reg rst = 0;
    reg [RW-1:0] r_d = 0;
    reg r_sel = 0;
    wire [2*RW-1:0] r_result;
    boe_registered #(.WIDTH(RW)) u_reg (
        .clk(clk), .rst(rst), .d(r_d), .sel(r_sel), .result(r_result)
    );
    always #5 clk = ~clk;

    integer i;
    integer errors = 0;
    integer checks = 0;

    // Truth table of the 1-to-2 demultiplexer, written out explicitly.
    task check_boe;
        input exp_y0;
        input exp_y1;
        begin
            checks = checks + 1;
            if (y0_assign !== exp_y0 || y1_assign !== exp_y1) begin
                errors = errors + 1;
                $display("FAIL boe_assign d=%b sel=%b: y=%b%b, expected %b%b",
                         d, sel, y1_assign, y0_assign, exp_y1, exp_y0);
            end
            if (y0_always !== exp_y0 || y1_always !== exp_y1) begin
                errors = errors + 1;
                $display("FAIL boe_always d=%b sel=%b: y=%b%b, expected %b%b",
                         d, sel, y1_always, y0_always, exp_y1, exp_y0);
            end
        end
    endtask

    // Independent behavioural model: the selected output copies the data
    // bus, the other output stays zero.
    task check_wide;
        input [W-1:0] din;
        input s;
        reg [W-1:0] exp_y0, exp_y1;
        begin
            nw_d = din;
            nw_sel = s;
            exp_y0 = s ? {W{1'b0}} : din;
            exp_y1 = s ? din : {W{1'b0}};
            #1;
            checks = checks + 1;
            if (nw_y0 !== exp_y0 || nw_y1 !== exp_y1) begin
                errors = errors + 1;
                $display("FAIL boe_param d=%h sel=%b: y0=%h y1=%h",
                         din, s, nw_y0, nw_y1);
                $display("     expected y0=%h y1=%h", exp_y0, exp_y1);
            end
        end
    endtask

    task check_n1;
        input din;
        input s;
        begin
            n1_d = din;
            n1_sel = s;
            #1;
            checks = checks + 1;
            if (n1_y0 !== (s ? 1'b0 : din) || n1_y1 !== (s ? din : 1'b0)) begin
                errors = errors + 1;
                $display("FAIL boe_param W=1 d=%b sel=%b: y0=%b y1=%b",
                         din, s, n1_y0, n1_y1);
            end
        end
    endtask

    task check_reg;
        input [2*RW-1:0] expected;
        begin
            checks = checks + 1;
            if (r_result !== expected) begin
                errors = errors + 1;
                $display("FAIL boe_registered at %0t: result=%h, expected %h",
                         $time, r_result, expected);
            end
        end
    endtask

    initial begin
        $dumpfile("build/boe.vcd");
        $dumpvars(0, tb_boe);

        // 1. Elementary block, full truth table: 0..40 ns.
        d = 0; sel = 0; #10 check_boe(0, 0);
        d = 1; sel = 0; #10 check_boe(1, 0);
        d = 0; sel = 1; #10 check_boe(0, 0);
        d = 1; sel = 1; #10 check_boe(0, 1);

        // 2. WIDTH=1 and boundary vectors of the wide block: 40..~120 ns.
        for (i = 0; i < 4; i = i + 1)
            check_n1(i[0], i[1]);
        check_wide(8'h00, 0); #9;
        check_wide(8'hff, 0); #9;
        check_wide(8'hff, 1); #9;
        check_wide(8'ha5, 0); #9;
        check_wide(8'ha5, 1); #9;
        check_wide(8'h5a, 1); #9;
        check_wide(8'h01, 1); #9;
        check_wide(8'h80, 0); #9;

        // 3. Exhaustive comparison of the wide block with the model.
        for (i = 0; i < 2 ** (W + 1); i = i + 1)
            check_wide(i[W-1:0], i[W]);

        // 4. Registered block: reset, capture, hold, reset priority.
        @(negedge clk);
        rst = 1; r_d = 4'hd; r_sel = 0;
        #1 check_reg(8'h00);                 // asynchronous reset
        @(posedge clk); #1 check_reg(8'h00); // reset overrides capture
        @(negedge clk) rst = 0;
        @(posedge clk); #1 check_reg(8'h0d); // sel=0: d goes to y0
        @(negedge clk) r_sel = 1;
        #1 check_reg(8'h0d);                 // holds until the next edge
        @(posedge clk); #1 check_reg(8'hd0); // sel=1: d goes to y1
        @(negedge clk) r_d = 4'h6;
        #1 check_reg(8'hd0);
        @(posedge clk); #1 check_reg(8'h60);
        @(negedge clk) r_sel = 0; r_d = 4'hf;
        @(posedge clk); #1 check_reg(8'h0f);
        #2 rst = 1;
        #1 check_reg(8'h00);                 // reset between clock edges
        @(negedge clk) rst = 0; r_d = 4'h9; r_sel = 1;
        @(posedge clk); #1 check_reg(8'h90);
        #10;

        if (errors != 0)
            $fatal(1, "FAIL: %0d of %0d checks failed", errors, checks);
        $display("PASS: boe (%0d checks)", checks);
        $finish;
    end
endmodule
