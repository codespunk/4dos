// FILEUTIL.C - File command implementations in FILE_TEXT.
// Compiled with /NTFILE_TEXT; dispatch entry points remain near.

#include "product.h"
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <direct.h>
#include <dos.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <io.h>
#include <malloc.h>
#include <share.h>
#include <string.h>
#include "4all.h"
