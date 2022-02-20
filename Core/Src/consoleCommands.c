// ConsoleCommands.c
// This is where you add commands:
//		1. Add a protoype
//			static eCommandResult_T ConsoleCommandVer(const char buffer[]);
//		2. Add the command to mConsoleCommandTable
//		    {"ver", &ConsoleCommandVer, HELP("Get the version string")},
//		3. Implement the function, using ConsoleReceiveParam<Type> to get the parameters from the buffer.

#include <string.h>
#include "consoleCommands.h"
#include "console.h"
#include "consoleIo.h"
#include "version.h"
#include "micData.h"
#include "angleFinder.h"
#include "displayVisualization.h"

extern uint8_t doEchoConsoleInput;
extern uint8_t scanMode;
extern MicData micData1;
extern MicData micData2;
extern MicData micData3;
extern MicData micData4;
extern AngleFinder angleFinderX;
extern AngleFinder angleFinderY;
extern DisplayVisualizer displayVisualizer;

void startSampling();
void stopSampling();
uint32_t micros();

#define IGNORE_UNUSED_VARIABLE(x)     if ( &x == &x ) {}


static eCommandResult_T ConsoleCommandComment(const char buffer[]);
static eCommandResult_T ConsoleCommandVer(const char buffer[]);
static eCommandResult_T ConsoleCommandHelp(const char buffer[]);
static eCommandResult_T ConsoleCommandParamExampleInt16(const char buffer[]);
static eCommandResult_T ConsoleCommandParamExampleHexUint16(const char buffer[]);

static eCommandResult_T ConsoleCommandEcho(const char buffer[]);
static eCommandResult_T ConsoleCommandMic(const char buffer[]);
static eCommandResult_T ConsoleCommandAngle(const char buffer[]);
static eCommandResult_T ConsoleCommandScan(const char buffer[]);


static const sConsoleCommandTable_T mConsoleCommandTable[] = {
		{ ";", &ConsoleCommandComment, HELP("Comment! You do need a space after the semicolon. ") },
		{"help", &ConsoleCommandHelp, HELP("Lists the commands available") },
		{"ver", &ConsoleCommandVer, HELP("Get the version string") }, { "int", &ConsoleCommandParamExampleInt16, HELP("How to get a signed int16 from params list: int -321") },
		{"u16h", &ConsoleCommandParamExampleHexUint16, HELP("How to get a hex u16 from the params list: u16h aB12") },

		{ "echo", &ConsoleCommandEcho, HELP("Change console echo setting: on / off") },
		{ "mic", &ConsoleCommandMic, HELP("Show output for a mic: 1-4") },
		{ "angle", &ConsoleCommandAngle, HELP("Show output for an angle: 1-2") },
		{ "scan", &ConsoleCommandScan, HELP("Toggle continuous scanning") },

		CONSOLE_COMMAND_TABLE_END // must be LAST
};

static eCommandResult_T ConsoleCommandComment(const char buffer[]) {
	// do nothing
	IGNORE_UNUSED_VARIABLE(buffer);
	return COMMAND_SUCCESS;
}

static eCommandResult_T ConsoleCommandEcho(const char buffer[]) {
	uint32_t startIndex = 0;
	eCommandResult_T result;
	result = ConsoleParamFindN(buffer, 1, &startIndex);
	if (COMMAND_SUCCESS == result) {
		if (strncmp("on", buffer + startIndex, 2) == 0) {
			doEchoConsoleInput = 1;
			ConsoleIoSendString("Console echo on");
			ConsoleIoSendString(STR_ENDLINE);
		} else if (strncmp("off", buffer + startIndex, 3) == 0) {
			doEchoConsoleInput = 0;
			ConsoleIoSendString("Console echo off");
			ConsoleIoSendString(STR_ENDLINE);
		} else {
			result = COMMAND_ERROR;
		}
	}
	return result;
}

static eCommandResult_T ConsoleCommandMic(const char buffer[]) {
	int16_t micNumber;
	eCommandResult_T result;
	result = ConsoleReceiveParamInt16(buffer, 1, &micNumber);
	if (COMMAND_SUCCESS == result) {
		MicData *md = NULL;
		switch (micNumber) {
		case 1:
			md = &micData1;
			break;
		case 2:
			md = &micData1;
			break;
		case 3:
			md = &micData1;
			break;
		case 4:
			md = &micData1;
			break;
		default:
			result = COMMAND_ERROR;
		}
		if (COMMAND_SUCCESS == result) {
			ConsoleIoSendString("Mic dc offset: ");
			ConsoleSendParamInt16(md->dcOffset);
			ConsoleIoSendString(STR_ENDLINE);

			stopSampling();
			HAL_Delay(2); //in case a conversion is happening now
			micDataSnapshot(md);
			startSampling(); //can sample while we process the data

			micDataProcess(md);

			ConsoleIoSendString("Mic output: ");
			ConsoleIoSendString(STR_ENDLINE);
			for (int i = 0; i < ADC_BUF_SIZE; i++) {
				ConsoleSendParamInt16(md->workingBuffer[i]);
				ConsoleIoSendString(STR_ENDLINE);
			}
		}
	}
	return result;
}

static eCommandResult_T ConsoleCommandAngle(const char buffer[]) {
	q15_t detectedAngle;
	q15_t strength;
	int16_t angleNumber;
	eCommandResult_T result;
	result = ConsoleReceiveParamInt16(buffer, 1, &angleNumber);
	if (COMMAND_SUCCESS == result) {
		AngleFinder *af = NULL;
		switch (angleNumber) {
		case 1:
			af = &angleFinderX;
			break;
		case 2:
			af = &angleFinderY;
			break;
		default:
			result = COMMAND_ERROR;
		}
		if (COMMAND_SUCCESS == result) {
			stopSampling();
			HAL_Delay(2); //in case a conversion is happening now

			uint32_t tstart = micros();
			micDataSnapshot(af->md1);
			micDataSnapshot(af->md2);
			startSampling(); //can sample while we process the data

			angleFinderProcess(af, &detectedAngle, &strength);
			uint32_t tend = micros();

			ConsoleIoSendString("correlation data: ");
			ConsoleIoSendString(STR_ENDLINE);
			ConsoleIoSendString("mic1\tmic2\tcorr");
						ConsoleIoSendString(STR_ENDLINE);
			for (int i = 0; i < ADC_BUF_SIZE; i++) {
				ConsoleSendParamInt16(af->md1->workingBuffer[i]);
				ConsoleIoSendString("\t");
				ConsoleSendParamInt16(af->md2->workingBuffer[i]);
				ConsoleIoSendString("\t");
				// only show the middle bit of the correlation
				ConsoleSendParamInt16(af->buffer[i + ADC_BUF_SIZE/2]);
				ConsoleIoSendString(STR_ENDLINE);
			}

			ConsoleIoSendString("Angle found: ");
			ConsoleSendParamInt16(detectedAngle);
			ConsoleIoSendString(" in ");
			ConsoleSendParamInt32(tend - tstart);
			ConsoleIoSendString(" us.");
			ConsoleIoSendString(STR_ENDLINE);

		}
	}
	return result;
}

static eCommandResult_T ConsoleCommandScan(const char buffer[]) {
	scanMode = !scanMode;
	return COMMAND_SUCCESS;
}

static eCommandResult_T ConsoleCommandHelp(const char buffer[]) {
	uint32_t i;
	uint32_t tableLength;
	eCommandResult_T result = COMMAND_SUCCESS;

	IGNORE_UNUSED_VARIABLE(buffer);

	tableLength = sizeof(mConsoleCommandTable)
			/ sizeof(mConsoleCommandTable[0]);
	for (i = 0u; i < tableLength - 1u; i++) {
		ConsoleIoSendString(mConsoleCommandTable[i].name);
#if CONSOLE_COMMAND_MAX_HELP_LENGTH > 0
		ConsoleIoSendString(" : ");
		ConsoleIoSendString(mConsoleCommandTable[i].help);
#endif // CONSOLE_COMMAND_MAX_HELP_LENGTH > 0
		ConsoleIoSendString(STR_ENDLINE);
	}
	return result;
}

static eCommandResult_T ConsoleCommandParamExampleInt16(const char buffer[]) {
	int16_t parameterInt;
	eCommandResult_T result;
	result = ConsoleReceiveParamInt16(buffer, 1, &parameterInt);
	if (COMMAND_SUCCESS == result) {
		ConsoleIoSendString("Parameter is ");
		ConsoleSendParamInt16(parameterInt);
		ConsoleIoSendString(" (0x");
		ConsoleSendParamHexUint16((uint16_t) parameterInt);
		ConsoleIoSendString(")");
		ConsoleIoSendString(STR_ENDLINE);
	}
	return result;
}
static eCommandResult_T ConsoleCommandParamExampleHexUint16(const char buffer[]) {
	uint16_t parameterUint16;
	eCommandResult_T result;
	result = ConsoleReceiveParamHexUint16(buffer, 1, &parameterUint16);
	if (COMMAND_SUCCESS == result) {
		ConsoleIoSendString("Parameter is 0x");
		ConsoleSendParamHexUint16(parameterUint16);
		ConsoleIoSendString(STR_ENDLINE);
	}
	return result;
}

static eCommandResult_T ConsoleCommandVer(const char buffer[]) {
	eCommandResult_T result = COMMAND_SUCCESS;

	IGNORE_UNUSED_VARIABLE(buffer);

	ConsoleIoSendString(VERSION_STRING);
	ConsoleIoSendString(STR_ENDLINE);
	return result;
}

const sConsoleCommandTable_T* ConsoleCommandsGetTable(void) {
	return (mConsoleCommandTable);
}

