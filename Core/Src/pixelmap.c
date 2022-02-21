
#include "pixelmap.h"


/*
 * The display is a simple 8x8 WS2812 matrix with 8 pixels per row,
 * and 8 columns.
 * The rows are wired in alternating direction, or zigzaging right
 * for even rows and left for odd rows.
 *
 * For display buffers, we stick to a conventional format.
 */

uint32_t getDisplayPixelOffset(uint16_t x, uint16_t y) {
	return (y * DISPLAY_BUFFER_WIDTH + x) * 3;
}

uint32_t getOutputPixelOffset(uint16_t x, uint16_t y) {
	//flip odd rows for zig-zag wiring
	if (y & 1) {
		x = DISPLAY_BUFFER_WIDTH - 1 - x;
	}
	return getDisplayPixelOffset(x, y);
}
