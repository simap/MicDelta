#include "displayVisualization.h"
#include "console.h"

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
//		dv->displayBuffer[pi] = dv->displayBuffer[pi] > 0 ? dv->displayBuffer[pi] - 1 : 0;
		dv->displayBuffer[pi] >>= 1;
	}
}


static void displayVisualizerPaint(DisplayVisualizer *dv, q15_t angleX, q15_t strengthX, q15_t angleY, q15_t strengthY) {
	uint8_t x = clamp(((angleX + 1) >> 2) + 3, 0, 7);
	uint8_t y = clamp(((angleY + 1) >> 2) + 3, 0, 7);
	uint8_t value = clamp((strengthX + strengthY) >> 3, 0, 255);


	if (scanMode) {
		ConsoleIoSendString("DRAW (");
		ConsoleSendParamInt16(x);
		ConsoleIoSendString(",");
		ConsoleSendParamInt16(y);
		ConsoleIoSendString(")\t");
		ConsoleSendParamInt16(value);
		ConsoleIoSendString(STR_ENDLINE);

		ConsoleIoSendString("angle X ");
		angleFinderDumpToConsole(dv->angleFinderX);

		ConsoleIoSendString("angle Y ");
		angleFinderDumpToConsole(dv->angleFinderY);

	}

	value = 64;

	//flip odd rows for zig-zag wiring
	if (y & 1) {
		x = 7 - x;
	}
	int pixelOffset = (y * DISPLAY_BUFFER_WIDTH + x) * 3;
	dv->displayBuffer[pixelOffset]     = value;
	dv->displayBuffer[pixelOffset + 1] = value;
	dv->displayBuffer[pixelOffset + 2] = value;

	//TODO maybe something cooler, like a radial gradient using fractional pixel coordinates
	//could squish/fade more on one axis than another if it had relatively lower strength
}

void displayVisualizerProcess(DisplayVisualizer *dv) {

	//snapshot the mic data for processing
	stopSampling();
	delayUs(100); //in case a conversion is happening now
	micDataSnapshot(dv->angleFinderX->md1);
	micDataSnapshot(dv->angleFinderX->md2);
	micDataSnapshot(dv->angleFinderY->md1);
	micDataSnapshot(dv->angleFinderY->md2);
	startSampling(); //can sample while we process the data


	displayVisualizerFade(dv); //fade out old pixels a bit

	//get angles and correlation strength for X and Y

	angleFinderProcess(dv->angleFinderX);
	angleFinderProcess(dv->angleFinderY);


	q15_t angleX = dv->angleFinderX->angle;
	q15_t angleY = dv->angleFinderY->angle;;
	q15_t strengthX = dv->angleFinderX->strength;
	q15_t strengthY = dv->angleFinderY->strength;


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
	if (strengthX > 100 && strengthY > 100) {
		displayVisualizerPaint(dv, angleX, strengthX, angleY, strengthY);
	}

	ws2812DriverDraw(dv->ws2812Driver, dv->displayBuffer);

}


