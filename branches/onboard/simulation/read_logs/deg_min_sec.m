function deg_min_sec_string = deg_min_sec(deg_fractional) 
  if deg_fractional < 0
    sign = -1;
    deg_fractional *= -1;
  endif
  deg = floor(deg_fractional);
  deg_fractional = (deg_fractional - deg) * 60;
  min = floor(deg_fractional);
  sec = (deg_fractional - min) * 60;
  deg_min_sec_string = sprintf("%03.0fdeg %02.0fmin %02.3fsec", deg, min, sec);
endfunction







