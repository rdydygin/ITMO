`timescale 1ns/1ps

module add_multicycle (
    input  wire        clk,
    input  wire        rst,
    input  wire        start,
    input  wire [47:0] a,
    input  wire [7:0]  b,
    output reg         done,
    output reg  [48:0] y
);
    localparam IDLE = 1'b0, ADD = 1'b1;
    reg state, next_state;
    reg [2:0] steps;
    reg [47:0] words;
    reg [7:0] carry;

    // The initial carry is b; subsequent iterations add just the carry bit.
    wire [16:0] sum = {1'b0, words[15:0]} + {9'd0, carry};
    wire [47:0] next_words = {sum[15:0], words[47:16]};

    always @* begin
        case (state)
            IDLE:    next_state = start ? ADD : IDLE;
            ADD:     next_state = steps[0] ? IDLE : ADD;
            default: next_state = IDLE;
        endcase
    end

    always @(posedge clk or posedge rst) begin
        if (rst) begin
            state <= IDLE;
            done <= 1'b0;
            y    <= 49'd0;
        end else begin
            state <= next_state;
            done <= 1'b0;
            if (state == IDLE && start) begin
                words <= a;
                carry <= b;
                steps <= 3'b100;
            end else if (state == ADD) begin
                words <= next_words;
                carry <= {7'd0, sum[16]};
                steps <= steps >> 1;
                if (steps[0]) begin
                    y    <= {sum[16], next_words};
                    done <= 1'b1;
                end
            end
        end
    end
endmodule
