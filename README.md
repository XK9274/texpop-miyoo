# texpop-miyoo
A tool for popping up text on the screen at any given location, size and colour

# Issues
Aliasing around edges
Flickers due to MainUI redraw

# Example
./texpop 10 "WPS Starting" white 20 250 440 > /dev/null 2>&1 &

Syntax: ./texpop duration "text" color font_size x y