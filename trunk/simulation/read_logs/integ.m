function ix = integ(dt, x, ix0) 
% function ix = integ2(dt, x, ix0)
% Euler integration with variable time steps and initial value ix0. 
  assert(size(ix0) == [1, 1])
  assert(columns(dt) == 1)
  assert(columns(x)  == 1)
  assert(rows(x)  == rows(dt))
  ix = ix0 + [0; cumsum(dt .* x)]
endfunction







