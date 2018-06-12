#include "putty.h"
#include "terminal.h"
#include "xyzmodem.h"

#include <windows.h>
#include <shlwapi.h>

#include <time.h>

#define PIPE_SIZE (64*1024)

struct xyzmodem_t {
	PROCESS_INFORMATION pi;
	HANDLE read_stdout;
	HANDLE read_stderr;
	HANDLE write_stdin;
};

char *get_program_path(void)
{
        static char putty_path[MAX_PATH] = {0};
        char *p, *q = putty_path;

	if (putty_path[0] != 0)
		return putty_path;

	GetModuleFileName(NULL, putty_path, sizeof(putty_path) - 1);

        if ((p = strrchr(q, '\\')) != NULL)
		q = p + 1;
        if ((p = strrchr(q, ':'))  != NULL)
		q = p + 1;
	*q = 0;

	return putty_path;
}

char *get_prog_name(const char *prog)
{
	static char cmd[MAX_PATH];

	strncpy(cmd, prog, sizeof(cmd));
	if (PathIsRelative(prog)) {
		FILE *fp;
		snprintf(cmd, sizeof(cmd), "%s\\%s", get_program_path(), prog);
		if ((fp = fopen(cmd, "r")) != NULL) {
			fclose(fp);
		}
	}

	return cmd;
}

void xyzmodem_done(Terminal *term)
{
	if (term->xyzmodem_xfer == 0)
		return;

	term->xyzmodem_xfer = 0;
	xyzmodem_update_ui(term);

	if (term->xyzmodem) {
		DWORD exitcode = 0;
		CloseHandle(term->xyzmodem->write_stdin);
		Sleep(500);
		CloseHandle(term->xyzmodem->read_stdout);
		CloseHandle(term->xyzmodem->read_stderr);

		GetExitCodeProcess(term->xyzmodem->pi.hProcess,&exitcode);
		if (exitcode == STILL_ACTIVE) {
			TerminateProcess(term->xyzmodem->pi.hProcess, 0);
		}
		sfree(term->xyzmodem);
		term->xyzmodem = NULL;
	}
}

int xyzmodem_check(Backend *back, void *backhandle, Terminal *term, int outerr)
{
	DWORD exitcode = 0;
	DWORD bread, avail;
	char buf[1024];
	HANDLE h;

	if (!term->xyzmodem_xfer)
		return 0;

	if (outerr)
		h = term->xyzmodem->read_stdout;
	else
		h = term->xyzmodem->read_stderr;

	bread = 0;
	PeekNamedPipe(h, buf, 1, &bread, &avail, NULL);

	/* check to see if there is any data to read from stdout */
	if (bread != 0) {

		for(;;) {
			bread = 0;
		
			PeekNamedPipe(h,buf, 1, &bread, &avail, NULL);
			if (bread == 0)
				return 0;

			/* read the stdout pipe */
			if (ReadFile(h, buf, sizeof(buf), &bread, NULL))  {
				if (bread) {
					if (outerr)
						back->send(backhandle, buf, bread);
					else
						from_backend(term, 1, buf, bread);
					continue;
				}
			}

			/* EOF or error */
			xyzmodem_done(term);
			return 1;
		}
		return 1;
	}
	
	GetExitCodeProcess(term->xyzmodem->pi.hProcess, &exitcode);
	if (exitcode != STILL_ACTIVE) {
		xyzmodem_done(term);
		return 1;
	}

	return 0;
}

int xyzmodem_spawn(Terminal *term, const char *incommand, char *inparams)
{
	STARTUPINFO si;
	SECURITY_ATTRIBUTES sa;
	SECURITY_DESCRIPTOR sd; /* security information for pipes */
	HANDLE read_stdout, read_stderr, write_stdin, newstdin, newstdout, newstderr;
	
	term->xyzmodem = (struct xyzmodem_t *)smalloc(sizeof(struct xyzmodem_t));
	memset(term->xyzmodem, 0, sizeof(struct xyzmodem_t));
	term->xyzmodem_remote_command_sent = TRUE;

        /* Initialize security descriptor (Windows NT) */
	if (osPlatformId == VER_PLATFORM_WIN32_NT)
	{
		InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION);
		SetSecurityDescriptorDacl(&sd, TRUE, NULL, FALSE);
		sa.lpSecurityDescriptor = &sd;
	}
	else sa.lpSecurityDescriptor = NULL;
	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.bInheritHandle = TRUE; /* allow inheritable handles */

	if (!CreatePipe(&newstdin, &write_stdin, &sa, PIPE_SIZE))
	{
		/* FIXME: free xyzmodem */
		return 0;
	}
	if (!CreatePipe(&read_stdout, &newstdout, &sa, PIPE_SIZE))
	{
		CloseHandle(newstdin);
		CloseHandle(write_stdin);
		return 0;
	}
	if (!CreatePipe(&read_stderr, &newstderr, &sa, PIPE_SIZE))
	{
		CloseHandle(newstdin);
		CloseHandle(write_stdin);
		CloseHandle(newstdout);
		CloseHandle(read_stdout);
		return 0;
	}

	/* set startupinfo for the spawned process */
	GetStartupInfo(&si);

	/*
	 * The dwFlags member tells CreateProcess how to make the process.
	 * STARTF_USESTDHANDLES validates the hStd* members. STARTF_USESHOWWINDOW
	 * validates the wShowWindow member.
	 */
	si.dwFlags = STARTF_USESTDHANDLES|STARTF_USESHOWWINDOW;
	si.wShowWindow = SW_HIDE;
	si.hStdOutput = newstdout;
	si.hStdError = newstderr;     /* set the new handles for the child process */
	si.hStdInput = newstdin;

	if (!DuplicateHandle(GetCurrentProcess(), read_stdout, GetCurrentProcess(),
				&term->xyzmodem->read_stdout, 0, FALSE, DUPLICATE_SAME_ACCESS))
	{
		CloseHandle(newstdin);
		CloseHandle(write_stdin);
		CloseHandle(newstdout);
		CloseHandle(read_stdout);
		CloseHandle(newstderr);
		CloseHandle(read_stderr);
		return 0;
	}

	CloseHandle(read_stdout);

	if (!DuplicateHandle(GetCurrentProcess(), read_stderr, GetCurrentProcess(),
				&term->xyzmodem->read_stderr, 0, FALSE, DUPLICATE_SAME_ACCESS))
	{
		CloseHandle(newstdin);
		CloseHandle(newstdout);
		CloseHandle(read_stdout);
		CloseHandle(write_stdin);
		CloseHandle(newstderr);
		CloseHandle(read_stderr);
		return 0;
	}

	CloseHandle(read_stderr);

	if (!DuplicateHandle(GetCurrentProcess(), write_stdin, GetCurrentProcess(),
				&term->xyzmodem->write_stdin, 0, FALSE, DUPLICATE_SAME_ACCESS))
	{
		CloseHandle(newstdin);
		CloseHandle(write_stdin);
		CloseHandle(newstdout);
		CloseHandle(term->xyzmodem->read_stdout);
		CloseHandle(newstderr);
		CloseHandle(term->xyzmodem->read_stderr);
		return 0;
	}

	CloseHandle(write_stdin);

	/* Spawn the child process */
	if (!CreateProcess(incommand, inparams, NULL, NULL, TRUE, CREATE_NEW_CONSOLE, NULL,
				filename_to_str(conf_get_filename(term->conf, CONF_xyzmodem_downloaddir)),
				&si, &term->xyzmodem->pi))
	{
		CloseHandle(newstdin);
		CloseHandle(term->xyzmodem->write_stdin);
		CloseHandle(newstdout);
		CloseHandle(term->xyzmodem->read_stdout);
		CloseHandle(newstderr);
		CloseHandle(term->xyzmodem->read_stderr);
		return 0;
	}

	CloseHandle(newstdin);
	CloseHandle(newstdout);
	CloseHandle(newstderr);

	return 1;
}

int xyzmodem_handle_receive(Terminal *term, const char *buffer, int len)
{
	DWORD written;

	/* FIXME: handle errors */
	WriteFile(term->xyzmodem->write_stdin, buffer, len, &written, NULL);

	return 0 ;
}

const char* xyzmodem_last_error()
{
	return win_strerror(GetLastError());
}
