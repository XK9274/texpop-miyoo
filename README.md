# texpop-miyoo
A tool for popping up text on the screen at any given location, size and colour

Based on the say package by Shauninman - https://github.com/shauninman/MiniUI

# Issues
- ~Aliasing around edges~
- Flickers due to MainUI redraw

# Example
`./texpop 10 "Hello, World!" white 40 190 425 > /dev/null 2>&1 &`

`Syntax: ./texpop duration "text" color font_size x y`

![MainUI_025](https://github.com/XK9274/texpop-miyoo/assets/47260768/c0561d5a-ea9b-44c2-9832-188ffeda12eb)


