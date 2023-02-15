
module set_64_1 (input [6-1:0] sel, input [64-1:0] din, output reg [64-1:0] dout);
  always @* begin dout = din; dout = dout |  (1 << sel); end
endmodule

