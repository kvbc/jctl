#include "file.h"
#include "tinydir.h"
#include <stdio.h>

#define is_newline(c) (((c) == '\n') || ((c) == '\r'))


/*
 * Return the line count of file of name 'fn'.
 * Supports the following line break types:
 * - CR   : Commodore, Apple II, Mac OS, ...
 * - LF   : Unix and Unix-like systems
 * - CRLF : Windows, DOS, ...
 */
jctl_uint jctl_file_linecount (char *fn)
{
	if(fn == NULL)
		return 0;

	FILE *fp = fopen(fn, "r");
	jctl_uint lc = 1;

	if(fp != NULL)
	{
		while(!feof(fp))
		{
			char c1 = fgetc(fp);
			if(is_newline(c1))
			{
				++lc;
				char c2 = fgetc(fp);
				if(is_newline(c2))
					lc += (c1 == c2);
			}
		}
	}

	return lc;
}


/*
 * Returns the file count in directory 'path'.
 */
jctl_uint jctl_dir_filecount (char *path)
{
	tinydir_dir dir;
	tinydir_open_sorted(&dir, ".");
	jctl_uint fc = dir.n_files;
	tinydir_close(&dir);
	return fc;
}