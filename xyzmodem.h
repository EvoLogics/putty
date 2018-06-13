#ifndef PUTTY_XYZMODEM_H
#define PUTTY_XYZMODEM_H

#include "putty.h"

int  xyzmodem_handle(Backend *back, void *backhandle, Terminal *term);
void xyzmodem_download(Terminal *term);
void xyzmodem_upload(Terminal *term, char* filelist);
void xyzmodem_abort(Terminal *term);

void xyzmodem_update_ui(Terminal *term);
void xyzmodem_update_menu(Terminal *term);
void xyzmodem_update_title(Terminal *term);

char* xyzmodem_upload_files_request();

int xyzmodem_handle_receive(Terminal *term, const char *buffer, int len);

int xyzmodem_spawn(Terminal *term, const char *incommand, char *inparams);
const char* xyzmodem_last_error();
int xyzmodem_check(Backend *back, void *backhandle, Terminal *term, int outerr);
void xyzmodem_done(Terminal *term);

int xyzmodem_download_autodetect(Terminal *term, char ch);

char *get_program_path(void);
char *get_prog_name(const char *prog);

#endif

