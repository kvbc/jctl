/**
*
* @file error.h
* @brief Error handling
*
*/


#ifndef OFP_ERROR_H
#define OFP_ERROR_H


/* OFP headers */
#include "argument.h"


enum ofp_errorcode_e OFP_ENUMTYPE
{
	OFP_ERR_NONE,		///< no error

	/* argument */
	OFP_ERR_ARG_REQ,	///< argument required
	OFP_ERR_ARG_NOVAL,	///< argument has no value

	/*
	*
	* Fatal Error
	*
	*/

	OFP_FERR_ERR_MAX,	///< error stack reached its limit
	OFP_FERR_AL_MAX,	///< argument array reached its limit
	OFP_FERR_UL_MAX,	///< unknown argument array reached its limit
	OFP_FERR_NAL_MAX,	///< non-argument array reached its limit
	OFP_FERR_MEM_OUT,	///< out of memory
};


struct ofp_error_s
{
	ofp_errorcode ec;	///< error code
	ofp_argument *arg;	///< argument
};


#endif /* OFP_ERROR_H */