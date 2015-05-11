Code for LED display

Operates by holding multiple arrays of bits to be displayed.

Display array pointer points to either frame_buffer1 or frame_buffer2 depending on
which array is not currently being edited. LED display pushes bits from the
display array to the hardware.

shape_drawer.cpp contains functions to draw shapes to it's working array, as well
as functions to convert the working array format to a display array format. Due to
the angular displacement of the LED arms, bits in the working array must be shifted
by various angular displacements before being copied to the display array. Despite
the extra processing overhead, this makes it easier to create drawing functions without
worrying about shifting individual bits.