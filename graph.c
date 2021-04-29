#include "graph.h"
#include "file.h"
#include "jctl.h"
#include "ofp/state.h"
#include "tinydir.h"
#include "wildcard.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>


static void jctl_graph_entry_new (ofp_state *S, jctl_graph *g, char *fn, jctl_uint fnlen, jctl_uint dirlen, jctl_uint wc);

/*
 * Return the length of unsigned integer 'n'.
 * Used for linecount padding in 'jctl_graph_print'.
 */
static jctl_uint numlen (ofp_uint n)
{
    if (n < 1e1)  return 1;
    if (n < 1e2)  return 2;
	if (n < 1e3)  return 3;
	if (n < 1e4)  return 4;
	if (n < 1e5)  return 5;
	if (n < 1e6)  return 6;
	if (n < 1e7)  return 7;
	if (n < 1e8)  return 8;
	if (n < 1e9)  return 9;
	if (n < 1e10) return 10;
	return 11;
}


/*
 * Throw an error for graph 'g'.
 */
static inline int jctl_graph_throw (jctl_graph *g)
{
	longjmp(g->errbuf, 1);
}


/*
 * Compare two graph entries by name (alphabetically).
 * Used in 'jctl_graph_sort'.
 */
static inline int jctl_graph_entry_compare_name (const void *a, const void *b)
{
	jctl_graph_entry *ea = (jctl_graph_entry*) a;
	jctl_graph_entry *eb = (jctl_graph_entry*) b;
	return _jctl_strcmp(ea->fn, eb->fn);
}


/* 
 * Compare two graph entries by line count (increasing).
 * Used in 'jctl_graph_sort'
 */
static inline int jctl_graph_entry_compare_line_inc (const void *a, const void *b)
{
	jctl_graph_entry *ea = (jctl_graph_entry*) a;
	jctl_graph_entry *eb = (jctl_graph_entry*) b;
	return ea->lc - eb->lc;
}


/*
 * Compare two graph entries by line count (decreasing).
 * Used in 'jctl_graph_sort'.
 */
static inline int jctl_graph_entry_compare_line_dec (const void *a, const void *b)
{
	jctl_graph_entry *ea = (jctl_graph_entry*) a;
	jctl_graph_entry *eb = (jctl_graph_entry*) b;
	return eb->lc - ea->lc;
}


/*
 * Sort graph entries given the sort order 'so' and graph 'g'.
 * Used the standard C quick sort implementation 'qsort'.
 */
static void jctl_graph_sort (ofp_state *S, jctl_graph *g, jctl_graph_sortorder so)
{
	switch(so)
	{
	case JCTL_GRAPH_SORT_NAME:
		_jctl_graph_sort(g->entries, g->entrytop, sizeof(*g->entries), jctl_graph_entry_compare_name);
		break;
	case JCTL_GRAPH_SORT_LINE_INC:
		_jctl_graph_sort(g->entries, g->entrytop, sizeof(*g->entries), jctl_graph_entry_compare_line_inc);
		break;
	case JCTL_GRAPH_SORT_LINE_DEC:
		_jctl_graph_sort(g->entries, g->entrytop, sizeof(*g->entries), jctl_graph_entry_compare_line_dec);
		break;
	}
}


/* 
 * Print the graph 'g'.
 * Includes padding for more readibilty.
 */
static void jctl_graph_print (ofp_state *S, jctl_graph *g)
{
	jctl_uint max_padding;
	if(g->hfnlen > g->hdirlen)
		max_padding = g->hfnlen;
	else
		max_padding = g->hdirlen;

	/*
	 * Initialize the padding
	 * buffers for printing.
	 */
	char space[max_padding + 1];
	char equals[JCTL_GRAPH_BARS + 1];
	memset(space, ' ', max_padding);
	memset(equals, '=', JCTL_GRAPH_BARS);
	space[max_padding] = '\0';
	equals[JCTL_GRAPH_BARS] = '\0';

	/*
	 * No need to keep track of
	 * the highest line count length.
	 * The global line count will always
	 * be the highest, so calculate
	 * it's numeric length.
	 */
	jctl_uint hlclen = numlen(g->glc);

	/*
	 * Flag that specifies if
	 * the slash should be taken
	 * into account in entry padding.
	 */
	int incslsh = (g->hdirlen != 0);

	/*
	 * Iterate through graph entries
	 * and print their data using the 'jctl_printf'
	 * function including the padding.
	 */
	for(jctl_uint i = 0; i < g->entrytop; ++i)
	{
		jctl_graph_entry *e = g->entries + i;

		/*
		 * Flag that specifies if
		 * the file resides in
		 * executable's directory.
		 */
		int exedir = (e->dirlen == 0);

		/* "directory" padding */
		_jctl_printf("%.*s", g->hdirlen - e->dirlen + incslsh * exedir, space);

		/* filename */
		printf(e->fn);
		/*
		 * Free the filename
		 * malloc'ed while interpreting
		 * the wildcard in function 'jctl_graph_entry_wildcard'.
		 */
		if(e->wc)
			free(e->fn);

		/* "post-filename" padding */
		_jctl_printf("%.*s", g->hfnlen - e->fnlen, space);
		_jctl_printf(" | ");

		/* line count */
		_jctl_printf("%u", e->lc);
		_jctl_printf("%.*s", hlclen - numlen(e->lc), space);
		_jctl_printf(" line%s", (e->lc == 1) ? "  [" : "s [");

		/* graph */
		jctl_uint prc = e->lc * 100 / g->glc;
		jctl_uint bars = prc * JCTL_GRAPH_BARS / 100;
		_jctl_printf("%.*s", bars, equals);
		_jctl_printf("%.*s", JCTL_GRAPH_BARS - bars, space);

		/* percentage */
		_jctl_printf("] %u%%\n", prc);
	}

	/* global line count */
	_jctl_printf("%.*s", g->hfnlen + g->hdirlen + incslsh, space);
	_jctl_printf("   %u line%s\n", g->glc, (g->glc == 1) ? "" : "s");
}


/*
 * Check if the graph entry with given filename 'fn' of length 'fnlen' exists.
 * Returns 1 if it does exists,
 * otherwise returns 0.
 */
static int jctl_graph_entry_exists (ofp_state *S, jctl_graph *g, char *fn, jctl_uint fnlen)
{
	for(jctl_uint i = 0; i < g->entrytop; ++i)
	{
		jctl_graph_entry *e = g->entries + i;
		if(e->fnlen == fnlen)
			if(_jctl_strcmp(e->fn, fn) == 0)
				return 1;
	}
	return 0;
}


#if (defined _MSC_VER || defined __MINGW32__)
/*
 * Look for files matching wildcard syntax in the
 * specified directory and register them as new graph entries.
 * By default ignores directory recursion, which can be enabled using the '-r' flag.
 */
static void jctl_graph_entry_wildcard (ofp_state *S, jctl_graph *g, char *fn, jctl_uint fnlen, jctl_uint dirlen)
{
	/*
	 * Default path for tinydir,
	 * stays unchanged in case of 'fn'
	 * being wildcard not including filepath.
	 */
	char *dir_path = ".";
	jctl_uint dir_len = 0;

	/*
	 * If the wildcard includes a filepath
	 * (includes the slash '/'),
	 * separate the directory and wildcard
	 * for wildcard parser and
	 * tinydir to open the directory.
	 */
	char *lslsh = strrchr(fn, '/'); /* last slash */
	if(lslsh != NULL)
	{
		dir_path = fn;
		dir_len = lslsh - fn;
		fnlen -= dir_len + 1;
		fn = lslsh + 1;
		/*
		 * Thanks to tinydir not requiring an "final slash"
		 * at the end, we can replace the last slash (lslsh)
		 * with a NULL terminator.
		 */
		*lslsh = '\0';
	}

	/* open directory using tinydir */
	tinydir_dir dir;
	tinydir_open(&dir, dir_path);

	while(dir.has_next)
	{
		/* get file data */
		tinydir_file file;
		tinydir_readfile(&dir, &file);

		/* skip directory */
		if(file.is_dir)
		{
			tinydir_next(&dir);
			continue;
		}

		jctl_uint file_len = _jctl_strlen(file.name);

		/* check for wildcard match */
		if(wc_match(fn, file.name, file_len))
		{
			/*
			 * If graph entry already exists
			 * (has been included without using wildcard),
			 * start parsing the next file.
			 */
			if(jctl_graph_entry_exists(S, g, file.name, file_len))
			{
				tinydir_next(&dir);
				continue;
			}

			/*
			 * Concatenate the directory with filename
			 * and succesfully register a new graph entry.
			 */
			if(lslsh == NULL)
			{
				/* no path, just filename */
				char *file_name = malloc(sizeof(*file_name) * file_len + 1);
				memcpy(file_name, file.name, file_len + 1);
				jctl_graph_entry_new(S, g, file_name, file_len, 0, 1);
			}
			else
			{
				/* path including filename */
				jctl_uint filename_len = file_len + dir_len + 1;
				char *file_name = malloc(sizeof(*file_name) * filename_len + 1);
				memcpy(file_name, dir_path, dir_len);
				memcpy(file_name + dir_len + 1, file.name, file_len + 1);
				file_name[dir_len] = '/';
				jctl_graph_entry_new(S, g, file_name, file_len, dir_len, 1);
			}
		}

		tinydir_next(&dir);
	}

	tinydir_close(&dir);
}
#endif


/*
 * Register a new graph entry.
 * Wildcards get processed and "highest" values updated.
 */
static void jctl_graph_entry_new
(
	ofp_state *S,		/* OFP state */
	jctl_graph *g,		/* JCTL graph */
	char *fp,			/* filepath */
	jctl_uint fplen,	/* filepath's length */
	jctl_uint dirlen,	/* directory's lenght */
	jctl_uint wc		/* wildcard flag */
)
{
	if(g->entrytop >= JCTL_GRAPH_MAX_ENTRIES)
	{
		jctl_graph_throw(g);
	}

#if (defined _MSC_VER || defined __MINGW32__)
	/*
	 * Command line doesn't handle Wildcards.
	 * Check for wildcard syntax.
	 */
	if(wc_correct(fp))
	{
		/*
		 * 'fp' matches wildcard syntax,
		 * evaluate the entry as wildcard
		 * using the 'jctl_graph_entry_wildcard' function.
		 */
		jctl_graph_entry_wildcard(S, g, fp, fplen, dirlen);
		return;
	}
	else if(dirlen == 0)
#else
	/*
	 * Wildcard is supported by the command line.
	 * Every UIA is an filepath.
	 * Validate that an entry hasn't
	 * already been registered.
	 */
	if(jctl_graph_entry_exists(S, g, fp, fplen))
	{
		return;
	}
#endif
	{
		/*
		 * Validate that the given filepath
		 * is not an directory.
		 */
		tinydir_dir dir;
		if(tinydir_open(&dir, fp) == 0)
		{
			tinydir_close(&dir);
			return;
		}

		/*
		 * Validate that the given file exists.
		 */
		if(!jctl_file_exists(fp))
		{
			return;
		}

		/*
		 * If non-wildcard argument 
		 * is a path, not a filename,
		 * figure out the directory and filename length.
		 */
		char *lslsh = strrchr(fp, '/');
		if(lslsh != NULL)
		{
			dirlen = lslsh - fp;
			fplen -= dirlen + 1;
		}
	}

	jctl_graph_entry *e = g->entries + g->entrytop++;
	e->fn = fp;
	e->wc = wc;

	/* fnlen */
	e->fnlen = fplen;
	if(fplen > g->hfnlen)
		g->hfnlen = fplen;

	/* lc */
	jctl_uint lc = jctl_file_linecount(e->fn);
	e->lc = lc;
	g->glc += lc;

	/* dirlen */
	e->dirlen = dirlen;
	if(dirlen > g->hdirlen)
		g->hdirlen = dirlen;
}


/*
 * Run a graph for OFP state 'S';
 * Register graph entries and print them sorted by order 'so'.
 *
 * Return 0 if the routine ran successfuly,
 * otherwise return 1.
 */
jctl_uint jctl_graph_run (ofp_state *S, jctl_graph_sortorder so)
{
	jctl_graph *g = (jctl_graph*)malloc(sizeof(*g));
	if(g == NULL)
		return 1;

	g->entries = (jctl_graph_entry*)malloc(sizeof(*g->entries) * JCTL_GRAPH_MAX_ENTRIES);
	if(g->entries == NULL)
		return 1;

	if(setjmp(g->errbuf))
		return 1;

	/* initialize members */
	g->glc = 0;
	g->hfnlen = 0;
	g->hdirlen = 0;
	g->entrytop = 0;

	/*
	 * Iterate through NAL
	 * and register graph entries.
	 */
	for(ofp_uint i = 0; i < S->nalt; ++i)
	{
		char *fn = S->nal[i];
		if(fn == NULL)
			continue;
		jctl_graph_entry_new(S, g, fn, _jctl_strlen(fn), 0, 0);
	}

	jctl_graph_sort(S, g, so);
	jctl_graph_print(S, g);

	return 0;
}
