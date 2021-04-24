/**
*
* @file state.h
* @brief OFP State
*
*/


#ifndef OFP_STATE_H
#define OFP_STATE_H


/* OFP headers */
#include "ofp.h"
#include "error.h"
#include "argument.h"

/* C headers */
#include <setjmp.h>


struct ofp_state_s
{
	char p;							///< argument prefix
	ofp_uint maxuda;				///< maximum UDA count
	ofp_argument_priority prty;		///< parsing priority

	int argc;						///< UIA count
	char **argv;					///< UIA values

	ofp_uint udalt;					///< UDAL  top
	ofp_uint uuialt;				///< UUIAL top
	ofp_uint nalt;					///< NAL   top
	ofp_uint errtop;				///< error stack top

	int uuiac;						///< UUIA count
	int nac;						///< NA count

	ofp_argument *udal;				///< UDA List (UDAL)
	char **uuial;					///< Unknown UIA List (UUIAL)
	char **nal;						///< Non-Argument List (NAL)
	ofp_error *err;					///< error stack

	jmp_buf ferrbuf;				///< fatal error jmp_buf
	ofp_errorcode ferr;				///< fatal error code
};


#endif /* OFP_STATE_H */