#include <ultra64.h>
#include "crash_screen.h"

u16* pFramebuffer = 0;

void create_debug_thread(void) {}
void start_debug_thread(void) {}
void crash_screen_set_framebuffer(u16* framebuffer) { pFramebuffer = framebuffer; }
