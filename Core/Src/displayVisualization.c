#include "displayVisualization.h"
#include "console.h"
#include "consoleio.h"

#define clamp(n, low, high)  (n > high ? high : (n < low ? low : n))
extern uint8_t scanMode;

void displayVisualizerInit(DisplayVisualizer *dv, AngleFinder *angleFinderX, AngleFinder *angleFinderY, WS2812Driver * ws2812Driver) {
	dv->angleFinderX = angleFinderX;
	dv->angleFinderY = angleFinderY;
	dv->ws2812Driver = ws2812Driver;
	memset(dv->displayBuffer, 0, sizeof(dv->displayBuffer));

	//clear LEDs during init
	ws2812DriverDraw(ws2812Driver, dv->displayBuffer);
}

static void displayVisualizerFade(DisplayVisualizer *dv) {
	for (int pi = 0; pi < DISPLAY_BUFFER_SIZE; pi++) {
		dv->displayBuffer[pi] >>= 1; //fade by halving each time
	}
}

static inline uint8_t scaleTo8Bit(float f) {
	return f * 255;
}


static void hsv(float h, float s, float v, uint8_t rgb[]) {
    int i;
    float hh, p, q, t, ff;

    if (v > 1)
        v = 1;
    if (v <= 0) {
        rgb[0] = rgb[1] = rgb[2] = 0;
        return;
    }

    if (s < 0)
        s = 0;
    if (s > 1)
        s = 1;
    if (s == 0) {
        rgb[0] = rgb[1] = rgb[2] = scaleTo8Bit(v);
        return;
    }

    h = fmodf(h, 1);
    if (h < 0)
        h += 1;

    hh = h * 6;
    i = (int) hh; //floor it
    ff = hh - (float) i;

    p = v * (1 - s);
    q = v * (1 - (s * ff));
    t = v * (1 - (s * (1 - ff)));

    switch (i) {
        case 0:
            rgb[0] = scaleTo8Bit(v);
            rgb[1] = scaleTo8Bit(t);
            rgb[2] = scaleTo8Bit(p);
            break;
        case 1:
            rgb[0] = scaleTo8Bit(q);
            rgb[1] = scaleTo8Bit(v);
            rgb[2] = scaleTo8Bit(p);
            break;
        case 2:
            rgb[0] = scaleTo8Bit(p);
            rgb[1] = scaleTo8Bit(v);
            rgb[2] = scaleTo8Bit(t);
            break;

        case 3:
            rgb[0] = scaleTo8Bit(p);
            rgb[1] = scaleTo8Bit(q);
            rgb[2] = scaleTo8Bit(v);
            break;
        case 4:
            rgb[0] = scaleTo8Bit(t);
            rgb[1] = scaleTo8Bit(p);
            rgb[2] = scaleTo8Bit(v);
            break;
        case 5:
        default:
            rgb[0] = scaleTo8Bit(v);
            rgb[1] = scaleTo8Bit(p);
            rgb[2] = scaleTo8Bit(q);
            break;
    }
}

static void displayVisualizerPaint(DisplayVisualizer *dv, q15_t angleX, q15_t strengthX, q15_t angleY, q15_t strengthY) {
	uint8_t x = clamp(((angleX + 1) >> 2) + 3, 0, 7);
	uint8_t y = clamp(((angleY + 1) >> 2) + 3, 0, 7);

	if (scanMode) {
		ConsoleIoSendString("DRAW (");
		ConsoleSendParamInt16(x);
		ConsoleIoSendString(",");
		ConsoleSendParamInt16(y);
		ConsoleIoSendString(STR_ENDLINE);
	}

	//flip odd rows for zig-zag wiring
	if (y & 1) {
		x = 7 - x;
	}

	//paint low strength as red (fading to black), through green, to purple for the highest values
	float strength = (strengthX + strengthY) / 32767.0f;
	if (strength > 1)
		strength = 1;
	uint8_t rgb[3];
	hsv(strength * .7, 1, strength * 10, rgb);

	//map rgb to grb for the LEDs
	//TODO move this to the driver & pixelmap
	int pixelOffset = (y * DISPLAY_BUFFER_WIDTH + x) * 3;
	dv->displayBuffer[pixelOffset]     = rgb[1];
	dv->displayBuffer[pixelOffset + 1] = rgb[0];
	dv->displayBuffer[pixelOffset + 2] = rgb[2];

	//TODO maybe something cooler, like a radial gradient using fractional pixel coordinates
	//could squish/fade more on one axis than another if it had relatively lower strength
}

void displayVisualizerProcess(DisplayVisualizer *dv) {

	//FIXME HACK WORKAROUND: sample audio when system is idle to work around board analog issues
	//was seeing supply spikes on the scope and phantom signals on ADC when sampled while the system was busy.
	//the phantom ADC signals had a strong 0 phase correlation between channels
	//likely bad analog / digital power design on my board.
	//taking samples while the system is idle works around this issue and gives good data
	delayUs(6000); //ensure we have enough clean samples
	stopSampling();
	delayUs(20); //give some time for any ongoing conversion to complete
	//snapshot the mic data for processing
	micDataSnapshot(dv->angleFinderX->md1);
	micDataSnapshot(dv->angleFinderX->md2);
	micDataSnapshot(dv->angleFinderY->md1);
	micDataSnapshot(dv->angleFinderY->md2);
	startSampling(); //can sample while we process

	displayVisualizerFade(dv); //fade out old pixels a bit

	//get angles and correlation strength for X and Y
	angleFinderProcess(dv->angleFinderX);
	angleFinderProcess(dv->angleFinderY);


	q15_t angleX = dv->angleFinderX->angle;
	q15_t angleY = dv->angleFinderY->angle;;
	q15_t strengthX = dv->angleFinderX->strength;
	q15_t strengthY = dv->angleFinderY->strength;

	//in scan mode, print out angle and strength info for every frame
	if (scanMode) {
		ConsoleSendParamInt16(angleX);
		ConsoleIoSendString("\t");
		ConsoleSendParamInt16(strengthX);
		ConsoleIoSendString("\t");
		ConsoleSendParamInt16(angleY);
		ConsoleIoSendString("\t");
		ConsoleSendParamInt16(strengthY);
		ConsoleIoSendString(STR_ENDLINE);
	}

	//if both are over a threshold, we can paint!
	if (strengthX > VISUALIZATION_CORRELATION_THRESHOLD
			&& strengthY > VISUALIZATION_CORRELATION_THRESHOLD) {
		displayVisualizerPaint(dv, angleX, strengthX, angleY, strengthY);
	}

	ws2812DriverDraw(dv->ws2812Driver, dv->displayBuffer);

}
