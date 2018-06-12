/*
 * Stubs of functions in terminal.c, for use in programs that don't
 * have a terminal.
 */

#include "putty.h"
#include "terminal.h"

void term_nopaste(Terminal *term) { }
void set_title(void *frontend, char *t) { }
void set_icon(void *frontend, char *t) { }
char *get_window_title(void *frontend, int icon) { return "moo"; }

void xyzmodem_update_menu(Terminal *term) { }
