function ix = integ2(dt, x, ix0) 
% function ix = integ2(dt, x, ix0)
% trapez integration with variable time steps and initial value ix0.
  assert(size(ix0) == [1, 1])
  assert(columns(dt) == 1)
  assert(columns(x)  == 1)
  assert(rows(x)  == rows(dt))
  r = rows(x)
  x = [x; x(r)];

  % input   0  1    1    1    1.8 (1.8 guessed)
  % t       1  1    1    2    1
  % to add  0
  %            (0+1)*1/2
  %                 (1+1)*1/2
  %                      (1+1)*1/2
  %                           (1+1.8)*2/2
  %                                (1.8+1.8)*1/2
  % integral 0 0.5 1.5
  c = 0.5 * dt .* (x(1:r, 1) + x(2:(r+1), 1));
  ix = ix0 + [0; cumsum(c)];
endfunction







