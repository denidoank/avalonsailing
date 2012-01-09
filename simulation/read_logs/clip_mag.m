function [x_filt, y_filt] = clip_mag(x, y, clip_at)
    r = max([(x .^ 2 + y .^ 2)'; ones(1, length(x)) * clip_at ^ 2]);
    f = clip_at ./ sqrt(r');
    x_filt = f .* x;
    y_filt = f .* y;
endfunction
