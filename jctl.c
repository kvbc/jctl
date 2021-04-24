#include "ofp/ofp.h"
#include "ofp/state.h"
#include "ofp/argument.h"
#include "graph.h"
#include "file.h"
#include "jctl.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>


/*
*
* Handle OFP UDA error
*
*/
void arg_error (ofp_argument *arg, ofp_errorcode ec)
{
	_jctl_printf("jctl: error: ");
    switch(ec)
    {
    case OFP_ERR_ARG_REQ:
        _jctl_printf("required command line option '-%s'", arg->id);
        break;
    case OFP_ERR_ARG_NOVAL:
        _jctl_printf("command line option '-%s' requires a value", arg->id);
        break;
    }
    _jctl_printf("\n");
}


/*
*
* Print JCTL usage
*
*/
void print_usage (char **argv)
{
	_jctl_printf
	(
		"Usage: %s [-o[nlL]] names\n"
		"\n"
		"  names       Specifies a list of one or more files.\n"
		"              Wildcards are supported.\n"
		"\n"
		"  -o          List by files in sorted order\n"
		"  sortorder     n : By name (alphabetical)\n"
		"                l : By line count (increasing)\n"
		"                L : By line count (decreasing)\n"
		"\n",
		*argv
	);
}


void print_error (char *err)
{
	_jctl_printf("jctl: error: %s\n", err);
}


/*
*
* Print an JCTL error
* using variable arguments
*
*/
void vprintf_error (char *fmt, ...)
{
	va_list arg;
	va_start(arg, fmt);
	_jctl_printf("jctl: error: ");
	vprintf(fmt, arg);
	_jctl_printf("\n");
	va_end(arg);
}


/*
*
* Main program entry
*
*/
int main (int argc, char **argv)
{
	if(argc == 1)
	{
		print_usage(argv);
		return EXIT_SUCCESS;
	}

	/*
	*
	* OFP State Initialization
	*
	*/

	ofp_state *S = ofp_state_new(argv + 1, argc - 1, OFP_ARG_PRTY_FIRST, 1);

	if(S == NULL)
	{
		print_error("out of memory");
		return EXIT_FAILURE;
	}

	S->p = '-';
	ofp_argument *arg_sortorder;

	/*
	*
	* OFP Fatal Error
	*
	*/

	ofp_on_ferror(S)
	{
		_jctl_printf("jctl: fatal error: 0x%02X\n", S->ferr);
        goto clean_up;
	}

	/*
	*
	* OFP Argument Definition
	*
	*/

	arg_sortorder = ofp_argument_register(S, OFP_ARG_TYPE_SUIA_OPTION, OFP_ARG_PRTY_INHERIT, OFP_ARG_NOT_REQUIRED, arg_error, "o", 1, NULL);
	ofp_parser_parse(S);

	/*
	*
	* JCTL Error Checking
	*
	*/

	int exit = 0;

	if(S->uuiac > 0)
	{
		for(int i = 0; i < S->uuialt; ++i)
		{
			if(S->uuial[i] != NULL)
			{
				exit = 1;
				_jctl_printf("jctl: error: unrecognized command line option '-%s'\n", S->uuial[i]);
			}
		}
	}

	if(S->nac == 0)
	{
		_jctl_printf("jctl: fatal error: no input files");
		exit = 1;
	}

	if(exit || ofp_any_error(S))
		goto clean_up;

	/*
	*
	* JCTL Output
	*
	*/

	jctl_graph_sortorder so = ofp_option_enumval(arg_sortorder, 3,
		"n", 1, JCTL_GRAPH_SORT_NAME,
		"l", 1, JCTL_GRAPH_SORT_LINE_INC,
		"L", 1, JCTL_GRAPH_SORT_LINE_DEC
	);

	if(so == -1)
	{
		vprintf_error("undefined sortorder '%s' for argument '-%s'", arg_sortorder->v.o, arg_sortorder->id);
		_jctl_printf("jctl: error: undefined sortorder '%s' for argument '-%s'\n", arg_sortorder->v.o, arg_sortorder->id);
		goto clean_up;
	}

	if(jctl_graph_run(S, so))
	{
		_jctl_printf("jctl: error: out of memory\n");
	}

	/*
	*
	* JCTL Clean-Up
	*
	*/

clean_up:
	ofp_state_free(S);

	return EXIT_SUCCESS;
}