#pragma once

typedef struct {
	unsigned char type, nr, a, b;
} KeyboardEvent;

int		keyboard_init();
void	keyboard_kill();
int		keyboard_poll(KeyboardEvent* e);
