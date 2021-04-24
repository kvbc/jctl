/**
*
* @file argument.h
* @brief Argument manipulation
*
*/


#ifndef OFP_ARGUMENT_H
#define OFP_ARGUMENT_H


/* OFP headers */
#include "ofp.h"


enum ofp_argument_priority_e OFP_ENUMTYPE
{
	OFP_ARG_PRTY_INHERIT,		///< Inherit priority from the state
	OFP_ARG_PRTY_FIRST,			///< First appearance
	OFP_ARG_PRTY_LAST			///< Last appearance
};


enum ofp_argument_type_e OFP_ENUMTYPE
{
	OFP_ARG_TYPE_FLAG,			///< Flag
	OFP_ARG_TYPE_DUIA_OPTION,	///< Double-UIA option
	OFP_ARG_TYPE_SUIA_OPTION	///< Single-UIA option
};


struct ofp_argument_s
{
	int r;						///< required
	int i;						///< included
	char *id;					///< identifier
	char *desc;					///< description
	ofp_uint idlen;				///< identifier length
	ofp_Cfunction ef;			///< error function
	ofp_argument_type t;		///< type
	ofp_argument_priority prty; ///< parsing priority
	union
	{
		int f;		///< flag value
		char *o;	///< option value
	} v;			///< value
};


#define OFP_ARG_REQUIRED		(1)
#define OFP_ARG_NOT_REQUIRED	(0)


#endif /* OFP_ARGUMENT_H */