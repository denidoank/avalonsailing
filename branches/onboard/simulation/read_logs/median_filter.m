% function filt = median_filter(x, N)
% Realizable median filter of x, weak at the start of vector x
function filt = median_filter(x, N)
  L = length(x);
  filt = zeros(L, 1);
  for i=1:L
    begin = floor( max(1, i - N + 1) + 0.1);
    filt(i) = median(x(begin:i));
  endfor
endfunction
