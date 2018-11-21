#include "putty.h"
#include "terminal.h"
#include "xyzmodem.h"

#include <assert.h>

int xyzmodem_handle(Backend *back, void *backhandle, Terminal *term)
{
	if (conf_get_int(term->conf, CONF_xyzmodem_remote_download_command_enable) && back->protocol != PROT_RAW) {
		if (term->xyzmodem && !term->xyzmodem_remote_command_sent) {
			char *rz_remote = conf_get_str(term->conf, CONF_xyzmodem_remote_download_command);

			back->send(backhandle, rz_remote, strlen(rz_remote));

			switch (back->protocol) {
				case PROT_SERIAL: back->send(backhandle, "\r", 1);   break;
				case PROT_SSH: 	  back->send(backhandle, "\n", 1);   break;
				case PROT_TELNET:
				case PROT_RLOGIN: back->send(backhandle, "\r\n", 2); break;
				default: assert(!"Wrong protocol");
			}
			term->xyzmodem_remote_command_sent = TRUE;
		}
	}

	return xyzmodem_check(back, backhandle, term, 0) + xyzmodem_check(back, backhandle, term, 1);
}

void xyzmodem_download(Terminal *term)
{
	const char *prog = get_prog_name(filename_to_str(conf_get_filename(term->conf,
					CONF_xyzmodem_download_command)));
	char *params = conf_get_str(term->conf, CONF_xyzmodem_download_options);
	if (!xyzmodem_spawn(term, prog, params)) {
		nonfatal("Unable to start receiving '%s' with parameters '%s': %s"
			, prog, params, xyzmodem_last_error());
	} else {
		term->xyzmodem_xfer = 1;
	}
}

void xyzmodem_upload(Terminal *term, char* filelist)
{
	char *params;
	const char *prog;

	if(!filelist || *filelist == 0)
		return;

	params = dupcat(conf_get_str(term->conf, CONF_xyzmodem_upload_options), " ", filelist, NULL);

	prog = get_prog_name(filename_to_str(conf_get_filename(term->conf, CONF_xyzmodem_upload_command)));

	if (xyzmodem_spawn(term, prog, params))
		term->xyzmodem_xfer = 1;
	else
		nonfatal("Unable to start sending '%s' with parameters '%s': %s"
				, prog, params, xyzmodem_last_error());

	term->xyzmodem_remote_command_sent = FALSE;

	sfree(params);
}

void xyzmodem_abort(Terminal *term)
{
	xyzmodem_done(term);
}

void xyzmodem_update_ui(Terminal *term)
{
	xyzmodem_update_menu(term);
	xyzmodem_update_title(term);
}

void xyzmodem_update_title(Terminal *term) {
    char *new_title;

    if (term->xyzmodem_xfer) {
	new_title = dupcat(get_window_title(NULL, TRUE), " (X/Y/ZModem transfer)", NULL);
	if (new_title) {
		set_icon(NULL, new_title);
		set_title(NULL, new_title);
		sfree(new_title);
	}
	return;
    }

    new_title = dupstr(get_window_title(NULL, TRUE));
    if (new_title) {
	char *p = strrchr(new_title, '(');
	if (p) {
		*(p - 1) = 0;
		set_icon(NULL, new_title);
		set_title(NULL, new_title);
	}
	sfree(new_title);
    }
}

int xyzmodem_download_autodetect(Terminal *term, char ch)
{
	static char* matchstr = "**\030B00000000000000\r";
	static char* p = NULL;

	if (term->xyzmodem_xfer)
		return 0;

	if (!ch)
		return 0;

	if (!p)
		p = matchstr;

	if (ch != *p) {
		p = matchstr;
		return 0;
	}

	p++;
	if (*p == '\0') {
		p = matchstr;
		return 1;
	}

	return 0;
}
