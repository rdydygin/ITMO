module mul (
    input clk,
    input rst,

    input [31:0] a_bi, //bi - input bus
    input [31:0] b_bi,
    input start,

    output reg busy_o,
    output reg [63:0] y_bo // output bus
);
    parameter N = 32;

    localparam IDLE = 0;
    localparam WORK = 1;
    localparam END_STATE = 2;

    reg [1:0] state;
    reg [31:0] cnt;
    reg [63:0] part_res;

    reg [31:0] a;
    reg [31:0] b;

    wire [63:0] part_sum;
    wire [63:0] shifted_part_sum;
    // b[cnt] == 1
    //32{b[cnt]} -> 1111_1111_..._1111_1111
    //a & {32{b[cnt]}} -> if b[cnt] == 1 then part_sum = a
    //a & {32{b[cnt]}} -> if b[cnt] == 0 then part_sum = 0

    assign part_sum ={32'h0000, (a & {32{b[cnt]}})};
    assign shifted_part_sum = part_sum << cnt;

    always @(posedge clk) begin
        if(rst) begin
            state <= IDLE;

            y_bo <= 0;
            cnt <= 0;
            part_res <= 0;
            busy_o <= 0;
        end
        else begin
            case (state)
                IDLE: begin
                    if(start) begin
                        state <= WORK;

                        a <= a_bi;
                        b <= b_bi;
                        cnt <= 0;
                        part_res <= 0;
                        busy_o <= 1;
                    end
                end
                WORK: begin
                    if(cnt == N) begin
                        state <= END_STATE;
                    end
                    else begin
                        part_res <= part_res + shifted_part_sum;
                        cnt <= cnt + 1;
                    end
                end
                END_STATE: begin
                    y_bo <= part_res;
                    state <= IDLE;

                    busy_o <= 0;
                end
                default: state <= IDLE;
            endcase
        end
    end
    
endmodule