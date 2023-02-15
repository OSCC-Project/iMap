module LUT1 #( parameter INIT = 2'h0 ) (input i0, output out );
 assign out = INIT[i0];
endmodule  
 
module LUT2 #( parameter INIT = 4'h0 ) ( input i0, i1, output out );
 assign out = INIT[{i1, i0}];
endmodule  
 
module LUT3 #( parameter INIT = 8'h0 ) ( input i0, i1, i2, output out );
 assign out = INIT[{i2, i1, i0}];
endmodule  
 
module LUT4 #( parameter INIT = 16'h0 ) ( input i0, i1, i2, i3, output out );
 assign out = INIT[{i3, i2, i1, i0}];
endmodule

module LUT5 #( parameter INIT = 32'h0 ) ( input i0, i1, i2, i3, i4, output out );
 assign out = INIT[{i4, i3, i2, i1, i0}];
endmodule  
 
module LUT6 #( parameter INIT = 64'h0 ) ( input i0, i1, i2, i3, i4, i5, output out );
 assign out = INIT[{i5, i4, i3, i2, i1, i0}];
endmodule

module LUT7 #( parameter INIT = 128'h0 ) ( input i0, i1, i2, i3, i4, i5, i6, output out );
 assign out = INIT[{i6, i5, i4, i3, i2, i1, i0}];
endmodule