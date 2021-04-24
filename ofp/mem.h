/**
*
* @file mem.h
* @brief Memory helpers
*
*/


#ifndef OFP_MEM_H
#define OFP_MEM_H


/**
 * Push an element "e" to the array "a" of top index "t".
 *
 * @param a - array
 * @param t - top index
 * @param e - element
 */
#define ofp_memory_array_push(a,t,e) ((a)[(t)++] = (e))


#endif /* OFP_MEM_H */