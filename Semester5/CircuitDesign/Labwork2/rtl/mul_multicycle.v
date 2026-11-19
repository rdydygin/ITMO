`timescale 1ns/1ps

module mul_multicycle (
    input  wire        clk,
    input  wire        rst,
    input  wire        start,
    input  wire [31:0] a,
    input  wire [15:0] b,
    output reg         done,
    output reg  [47:0] y
);
    localparam IDLE = 1'b0, MULTIPLY = 1'b1;
    reg state, next_state;
    reg [15:0] steps;
    reg [47:0] shifted_a;
    reg [15:0] shifted_b;
    reg [47:0] accumulator;

    wire [47:0] addend = shifted_b[0] ? shifted_a : 48'd0;
    wire [47:0] next_product = accumulator + addend;

    always @* begin
        case (state)
            IDLE:     next_state = start ? MULTIPLY : IDLE;
            MULTIPLY: next_state = steps[0] ? IDLE : MULTIPLY;
            default:  next_state = IDLE;
        endcase
    end

    always @(posedge clk or posedge rst) begin
        if (rst) begin
            state <= IDLE;
            done <= 1'b0;
            y    <= 48'd0;
        end else begin
            state <= next_state;
            done <= 1'b0;
            if (state == IDLE && start) begin
                shifted_a   <= {16'd0, a};
                shifted_b   <= b;
                accumulator <= 48'd0;
                steps       <= 16'h8000;
            end else if (state == MULTIPLY) begin
                accumulator <= next_product;
                shifted_a   <= shifted_a << 1;
                shifted_b   <= shifted_b >> 1;
                steps       <= steps >> 1;
                if (steps[0]) begin
                    y    <= next_product;
                    done <= 1'b1;
                end
            end
        end
    end
endmodule
