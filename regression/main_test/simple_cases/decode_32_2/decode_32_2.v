
module decode_32_2 (input [5-1:0] sel, input [32-1:0] din, output dout);
  wire [32-1:0] p = din << sel; assign dout = p[32-1];
endmodule

