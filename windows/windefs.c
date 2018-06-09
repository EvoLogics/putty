/*
 * windefs.c: default settings that are specific to Windows.
 */

#include "putty.h"

#include <windows.h>
#include <shlobj.h>
#include <commctrl.h>

FontSpec *platform_default_fontspec(const char *name)
{
    if (!strcmp(name, "Font"))
        return fontspec_new("Courier New", 0, 10, ANSI_CHARSET);
    else
        return fontspec_new("", 0, 0, 0);
}

Filename *platform_default_filename(const char *name)
{
    if (!strcmp(name, "LogFileName"))
	return filename_from_str("putty.log");
    else if (!strcmp(name, "rzCommand"))
	return filename_from_str("rz.exe");
    else if (!strcmp(name, "szCommand"))
	return filename_from_str("sz.exe");
    else if (!strcmp(name, "zDownloadDir")) {
	char path[MAX_PATH+1];
	char *result = "";

	if (SHGetSpecialFolderPathA(HWND_DESKTOP, path, CSIDL_DESKTOP, FALSE))
		result = path;

	return filename_from_str(result);
    } else
	return filename_from_str("");
}

char *platform_default_s(const char *name)
{
    if (!strcmp(name, "SerialLine"))
	return dupstr("COM1");
    return NULL;
}

int platform_default_i(const char *name, int def)
{
    return def;
}
