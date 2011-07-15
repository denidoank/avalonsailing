/*
% Copyright 2011 The Avalon Project Authors. All rights reserved.
% Use of this source code is governed by the Apache License 2.0
% that can be found in the LICENSE file.
% Steffen Grundmann, June 2011

function [merged_start, merged_end] = merge_blocks(blocked_start, blocked_end)
% function [merged_start, merged_end] = merge_blocks(blocked_start, blocked_end)
% Given a set of N forbidden intervals of angles merge them into one list of
% forbidden intervals.
% The minimum of each intervall is contained in blocked_start,
% the maximum of each is contained in blocked_end.

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

*/

void NoColl::MergeBlocks(const vector<double>& blocked_start,
                         const vector<double>& blocked_end,
                         vector<double>* merged_start,
                         vector<double>* merged_end) {
  blocked_start.sort();
  blocked_end.sort();
  CHECK_EQ(blocked_start.size() == blocked_end.size());
  vector<double>::iterator s = blocked_start.begin();
  vector<double>::iterator e = blocked_end.begin();
  int blocks = 0;

  while (s != blocked_start.end() &&
         e != elocked_end.end()) {
    if (s != blocked_start.end() && *s < *e) {
      ++blocks;
      if (1 == blocks)
        merged_start.push_back(*s)
      ++s
    } else {
      --blocks;
      if (0 == blocks)
        merged_end.push_back(*e);
      ++e;
    }
  }
}





