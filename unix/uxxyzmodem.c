/* TODO: not yet implemented */
#include "putty.h"

int xyzmodem_handle_receive(Terminal *term, const char *buffer, int len) { return 0; }
int xyzmodem_check(Backend *back, void *backhandle, Terminal *term, int outerr) { return 0; }
void xyzmodem_done(Terminal *term) { }

int xyzmodem_spawn(Terminal *term, const char *incommand, char *inparams) { return 0; }
const char* xyzmodem_last_error() { return ""; }
char *get_prog_name(const char *prog) { return ""; }

void xyzmodem_update_menu(Terminal *term) { }
