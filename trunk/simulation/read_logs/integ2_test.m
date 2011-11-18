function integ2_test() 
 T = 0.1;
 samples = 200;
 t = T * (0:(samples-1))';
 orig = 0.2 * sin(pi*(0:(samples-1))'/100);
 
 dt = T * ones(samples-1, 1);
 diff_orig = 1/T * diff(orig); 
 size(orig)
 size(diff_orig)
 size(t)
 size(dt)

 ix  = integ(dt,  diff_orig, 0); 
 ix2 = integ2(dt, diff_orig, 0);
 size(ix)
 size(ix2)
 assert(max(orig - ix2) < 0.01)
 plot(t, orig, "g",  t, ix2, "r") 
 %pause
 %plot(t, orig, "g", t, ix, "b", t, ix2, "r") 
 %pause

 orig = 0.2 * sin(pi*(0:(samples-1))'/100) .^ 2;
 
 dt = T * ones(samples-1, 1);
 diff_orig = 1/T * diff(orig); 

 ix  = integ(dt,  diff_orig, 0); 
 ix2 = integ2(dt, diff_orig, 0);
 size(ix)
 size(ix2)
 plot(t, orig, "g",  t, ix2, "r") 
 %pause
 %plot(t, orig, "g", t, ix, "b", t, ix2, "r") 
 %pause
 assert(max(orig - ix2) < 0.01)

endfunction







