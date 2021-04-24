#ifndef JCTL_WILDCARD_H
#define JCTL_WILDCARD_H

#include <stdlib.h>


int wc_match	(const char *wildcard, const char *target, size_t target_len);
int wc_correct	(const char *wildcard);


#endif /* JCTL_WILDCARD_H */