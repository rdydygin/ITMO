`timescale 1ns/1ps

module lab2_board (
    input  wire       CLK,
    input  wire       RST_N,
    output wire [1:0] LED
);
    wire clk, locked;
    pll_100mhz pll (.clk50(CLK), .clk100(clk), .locked(locked));

    wire reset_request = !RST_N || !locked;
    reg [1:0] reset_pipe = 2'b11;
    wire rst = reset_pipe[1];

    always @(posedge clk or posedge reset_request) begin
        if (reset_request)
            reset_pipe <= 2'b11;
        else
            reset_pipe <= {reset_pipe[0], 1'b0};
    end

    localparam [2:0] START_SMALL = 0, WAIT_SMALL = 1,
                     START_MAX = 2, WAIT_MAX = 3, PASSED = 4, FAILED = 5;
    reg [2:0] state, next_state;
    wire max_vector = (state == START_MAX) || (state == WAIT_MAX);
    wire start = !rst && ((state == START_SMALL) || (state == START_MAX));
    wire done;
    wire [47:0] result;

    function_multicycle calculator (
        .clk(clk), .rst(rst), .start(start),
        .a(max_vector ? 16'hffff : 16'd3), .b(max_vector ? 16'hffff : 16'd15),
        .busy(), .done(done), .y(result)
    );

    always @* begin
        next_state = state;
        case (state)
            START_SMALL: next_state = WAIT_SMALL;
            WAIT_SMALL:  if (done) next_state = (result == 48'd30) ? START_MAX : FAILED;
            START_MAX:   next_state = WAIT_MAX;
            WAIT_MAX:    if (done) next_state = (result == 48'hfffd000300fe) ? PASSED : FAILED;
            PASSED, FAILED: next_state = state;
            default: next_state = FAILED;
        endcase
    end

    always @(posedge clk or posedge rst) begin
        if (rst)
            state <= START_SMALL;
        else
            state <= next_state;
    end

    assign LED[0] = (state == PASSED);
    assign LED[1] = (state == FAILED);
endmodule
