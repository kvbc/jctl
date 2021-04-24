#ifndef JCTL_FILE_H
#define JCTL_FILE_H

#include "jctl.h"
#include "ofp/ofp.h"


jctl_uint jctl_file_linecount	(char *fn);
jctl_uint jctl_dir_filecount	(char *path);


#endif /* JCTL_FILE_H */