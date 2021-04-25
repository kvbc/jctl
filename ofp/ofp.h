/**
*
* @file ofp.h
* @brief OFP Environment
*
*/


#ifndef OFP_H
#define OFP_H


/* C headers */
#include <setjmp.h>


#ifdef __cplusplus
	#define OFP_ENUMTYPE : int
#else
	#define OFP_ENUMTYPE
#endif


/*
*
* Macros
*
*/


/** @brief
 * Define behaviour for
 * handling fatal errors
 */
#define ofp_on_ferror(S) if(setjmp((S)->ferrbuf))


/** @brief
 * Return a flag that
 * indicates if there's any errors
 */
#define ofp_any_error(S) ((S)->errtop > 0)

#ifndef OFP_API
	/** @brief
	 * API's visibility and/or linkage
	 */
	#if defined(_MSC_VER)
	    #define OFP_API __declspec(dllexport)
	#elif defined(__GNUC__)
	    #define OFP_API __attribute__((visibility("default")))
	#else
	    #define OFP_API
	    #pragma warning Unknown dynamic link import/export semantics.
	#endif
#endif /* OFP_API */


#ifndef OFP_PREFIX_DEFAULT
	/** @brief
	 * Prefix used by default
	 * by the OFP state
	 */
	#define OFP_PREFIX_DEFAULT '/'
#endif /* OFP_PREFIX_DEFULT */


/*
*
* Typedefs
*
*/


typedef unsigned int ofp_uint;

#ifdef __cplusplus
	enum ofp_argument_type_e	 OFP_ENUMTYPE;
	enum ofp_argument_priority_e OFP_ENUMTYPE;
	enum ofp_errorcode_e		 OFP_ENUMTYPE;
#endif

/* enum typedefs */
typedef enum ofp_argument_type_e		ofp_argument_type;
typedef enum ofp_argument_priority_e	ofp_argument_priority;
typedef enum ofp_errorcode_e			ofp_errorcode;

/* struct typedefs */
typedef struct ofp_argument_s	ofp_argument;
typedef struct ofp_error_s		ofp_error;
typedef struct ofp_state_s		ofp_state;

/** @brief
* Pointer to C function used for argument error handling
*/
typedef void (*ofp_Cfunction) (ofp_argument *arg, ofp_errorcode ec);


/*******************************************************************************************************************************************************
*                                                                                                                                                      *
* Function declarations                                                                                                                                *
*                                                                                                                                                      *
********************************************************************************************************************************************************/


/*
*
* State
*
*/

OFP_API ofp_state*	ofp_state_new			(char **argv, int argc, ofp_argument_priority prty, ofp_uint maxuda);
OFP_API void		ofp_state_free			(ofp_state *S);

					/* error */
OFP_API void		ofp_state_error_throw	(ofp_state *S, ofp_errorcode ec);


/*
*
* Argument
*
*/

OFP_API ofp_argument*	ofp_argument_register	(ofp_state *S, ofp_argument_type t, ofp_argument_priority prty, int r, ofp_Cfunction ef, char *id, ofp_uint len, char *desc);

						/* error */
OFP_API void			ofp_argument_error_push (ofp_state *S, ofp_argument *arg, ofp_errorcode ec);


/*
*
* Option
*
*/


OFP_API int ofp_option_enumval (ofp_argument *arg, ofp_uint count, ...);


/*
*
* Parser
*
*/

OFP_API void ofp_parser_parse (ofp_state *S);


/*
*
* Memory
*
*/


OFP_API void*	ofp_memory_allocate			(ofp_state *S, ofp_uint size);	

				/* array */
OFP_API void	ofp_memory_array_admissible	(ofp_state *S, ofp_uint top, ofp_uint limit, ofp_errorcode ferr);


#endif /* OFP_H */
