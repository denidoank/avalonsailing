% Copyright 2011 The Avalon Project Authors. All rights reserved.
% Use of this source code is governed by the Apache License 2.0
% that can be found in the LICENSE file.
% Steffen Grundmann, June 2011

function [merged_start, merged_end] = merge_blocks(blocked_start, blocked_end)
  assert(all(imag(blocked_start) == 0));
  assert(all(imag(blocked_end) == 0));
  blocked_start = sort(blocked_start);
  blocked_end = sort(blocked_end);
  assert (length(blocked_start) == length(blocked_end));
  merged_start = [];
  merged_end = [];
  s = 1;
  e = 1;
  blocks = 0;
  while s <= length(blocked_start) || ...
        e <= length(blocked_end)
    if s <= length(blocked_start) && ...
       blocked_start(s) < blocked_end(e)
      blocks += 1;
      if blocks == 1
        merged_start = [merged_start blocked_start(s)];
      endif
      s += 1;
    else
      blocks -= 1;
      if blocks == 0
        merged_end = [merged_end blocked_end(e)];
      endif
      e += 1;
    endif
  endwhile
endfunction