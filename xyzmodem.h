#ifndef PUTTY_XYZMODEM_H
#define PUTTY_XYZMODEM_H

#include "putty.h"

int  xyzmodem_handle(Backend *back, void *backhandle, Terminal *term);
void xyzmodem_download(Terminal *term);
void xyzmodem_upload(Terminal *term);
void xyzmodem_abort(Terminal *term);

void xyzmodem_update_menu(Terminal *term);
void xyzmodem_update_title(Terminal *term);

int xyzmodem_handle_receive(Terminal *term, const char *buffer, int len);

#endif

