`timescale 1ns/1ps

module function_multicycle (
    input  wire        clk,
    input  wire        rst,
    input  wire        start,
    input  wire [15:0] a,
    input  wire [15:0] b,
    output wire        busy,
    output reg         done,
    output reg  [47:0] y
);
    localparam [1:0] IDLE = 0, WAIT_SQUARE = 1, WAIT_CUBE = 2, WAIT_SUM = 3;
    reg [1:0] state, next_state;
    reg [15:0] a_reg;
    reg root_ready;

    wire accept = (state == IDLE) && start;
    wire mul_done, root_done, add_done;
    wire [47:0] product;
    wire [7:0] root;
    wire [48:0] sum;
    wire start_cube = (state == WAIT_SQUARE) && mul_done;
    wire start_sum  = (state == WAIT_CUBE) && mul_done && root_ready;
    wire finish_sum = (state == WAIT_SUM) && add_done;

    assign busy = (state != IDLE);

    mul_multicycle multiplier (
        .clk(clk), .rst(rst), .start(accept || start_cube),
        .a(start_cube ? product[31:0] : {16'd0, a}), .b(start_cube ? a_reg : a),
        .done(mul_done), .y(product)
    );

    sqrt_multicycle square_root (
        .clk(clk), .rst(rst), .start(accept), .x(b), .done(root_done), .y(root)
    );

    add_multicycle adder (
        .clk(clk), .rst(rst), .start(start_sum),
        .a(product), .b(root), .done(add_done), .y(sum)
    );

    always @* begin
        next_state = state;
        case (state)
            IDLE:        if (accept)     next_state = WAIT_SQUARE;
            WAIT_SQUARE: if (start_cube) next_state = WAIT_CUBE;
            WAIT_CUBE:   if (start_sum)  next_state = WAIT_SUM;
            WAIT_SUM:    if (finish_sum) next_state = IDLE;
            default:     next_state = IDLE;
        endcase
    end

    always @(posedge clk or posedge rst) begin
        if (rst) begin
            state      <= IDLE;
            root_ready <= 1'b0;
            done       <= 1'b0;
            y          <= 48'd0;
        end else begin
            state <= next_state;
            done <= finish_sum;
            if (accept) begin
                a_reg      <= a;
                root_ready <= 1'b0;
            end else if (root_done) begin
                root_ready <= 1'b1;
            end
            if (finish_sum)
                y <= sum[47:0];
        end
    end
endmodule
