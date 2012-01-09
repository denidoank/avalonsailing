% function red = chunk(x, N)
% downsample x , reducing its size to 1/N.
function red = chunk(x, N)
assert(columns(x) == 1);
red = zeros(length(x)/N, 1); 
for out=1:(length(x)/N)
  red(out) = sum(x(((out-1)*N+1):(out*N), 1)) / N;
endfor
