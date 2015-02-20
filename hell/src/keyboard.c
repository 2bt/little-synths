#include	<stdio.h>
#include	<portmidi.h>
#include	"keyboard.h"

enum {
	KEYBOARD_DEV_ID		= 3,
	KEYBOARD_MAX_EVENTS = 64,
};

static PortMidiStream* midi_stream = NULL;

int keyboard_init() {
	Pm_Initialize();
	const PmDeviceInfo* dev_info = Pm_GetDeviceInfo(KEYBOARD_DEV_ID);
	if (dev_info) {
		Pm_OpenInput(&midi_stream, KEYBOARD_DEV_ID, NULL, KEYBOARD_MAX_EVENTS, NULL, NULL);
		return 1;
	}
	return 0;
}

void keyboard_kill() {
	if (midi_stream) {
		Pm_Close(midi_stream);
		midi_stream = NULL;
	}
	Pm_Terminate();
}


int keyboard_poll(KeyboardEvent* event) {
	if (!midi_stream) return 0;
	static PmEvent events[KEYBOARD_MAX_EVENTS];
	static int pos = 0;
	static int len = 0;
	if (pos == len) {
		pos = 0;
		len = Pm_Read(midi_stream, events, KEYBOARD_MAX_EVENTS);
		if(!len) return 0;
	}
	const unsigned char* c = (const unsigned char*) &events[pos].message;
	event->type = c[0] >> 4;
	event->nr = c[0] & 0x0f;
	event->a = c[1];
	event->b = c[2];
	pos++;
	return 1;
}
