`timescale 1ns/1ps

module sqrt_multicycle (
    input  wire        clk,
    input  wire        rst,
    input  wire        start,
    input  wire [15:0] x,
    output reg         done,
    output reg  [7:0]  y
);
    localparam IDLE = 1'b0, EXTRACT = 1'b1;
    reg state, next_state;
    reg [15:0] remainder;
    reg [15:0] root;
    reg [15:0] mask;

    wire [15:0] trial = root | mask;
    wire [16:0] difference = {1'b0, remainder} - {1'b0, trial};
    wire accept_bit = !difference[16];
    wire [15:0] next_root = (root >> 1) | (accept_bit ? mask : 16'd0);

    always @* begin
        case (state)
            IDLE:    next_state = start ? EXTRACT : IDLE;
            EXTRACT: next_state = mask[0] ? IDLE : EXTRACT;
            default: next_state = IDLE;
        endcase
    end

    always @(posedge clk or posedge rst) begin
        if (rst) begin
            state <= IDLE;
            done <= 1'b0;
            y    <= 8'd0;
        end else begin
            state <= next_state;
            done <= 1'b0;
            if (state == IDLE && start) begin
                remainder <= x;
                root      <= 16'd0;
                mask      <= 16'h4000;
            end else if (state == EXTRACT) begin
                if (accept_bit)
                    remainder <= difference[15:0];
                root <= next_root;
                mask <= mask >> 2;
                if (mask[0]) begin
                    y    <= next_root[7:0];
                    done <= 1'b1;
                end
            end
        end
    end
endmodule
