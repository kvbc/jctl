#ifndef JCTL_GRAPH_H
#define JCTL_GRAPH_H

#include "jctl.h"
#include "ofp/ofp.h"
#include <setjmp.h>


/*
 * Amount of graph "bars" to show the
 * percentage of line count compared to
 * line count of all inputted files
 */
#define JCTL_GRAPH_BARS			(25)

/*
 * Amount of the maximum graph entries.
 */
#define JCTL_GRAPH_MAX_ENTRIES	(1024)


/*
*
* Graph Sort Order
*
*/
typedef enum jctl_graph_sortorder_e
{
	JCTL_GRAPH_SORT_NAME,
	JCTL_GRAPH_SORT_LINE_INC,
	JCTL_GRAPH_SORT_LINE_DEC
} jctl_graph_sortorder;


/*
*
* Graph Entry
*
*/
typedef struct jctl_graph_entry_s
{
	char *fn;			/* filename */
	jctl_uint wc;		/* uses wildcard */
	jctl_uint lc;		/* line count */
	jctl_uint fnlen;	/* filename length */
	jctl_uint dirlen;	/* directory length */
} jctl_graph_entry;


/*
*
* Graph Context
*
* "Highest" values exist for printing the
* padding in the 'jctl_graph_print' function.
*
*/
typedef struct jctl_graph_s
{
	jmp_buf errbuf;				/* error buffer */
	jctl_uint glc;				/* global line count */
	jctl_uint hfnlen;			/* highest filename length */
	jctl_uint hdirlen;			/* highest directory length */
	jctl_uint entrytop;			/* top entry index */
	jctl_graph_entry *entries;	/* entry stack */
} jctl_graph;


jctl_uint jctl_graph_run (ofp_state *S, jctl_graph_sortorder so);


#endif /* JCTL_GRAPH_H */
