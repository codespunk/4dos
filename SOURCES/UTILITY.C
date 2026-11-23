/* Experimental transient UTILITY_TEXT segment.
 * BREAK is dispatched through a near wrapper in DOSCMDS.C.
 */
#include "product.h"
#include <stdio.h>
#include <dos.h>
#include "4all.h"

int _far UtilityBreak_Cmd( char *pszCmdLine )
{
	int argc;

	if (( pszCmdLine == NULL ) || ( *pszCmdLine == '\0' )) {

		// inquiring about break status

_asm {
		mov	ax,03300h
		int	21h
		xor	dh,dh
		mov	argc,dx
}
		printf( BREAK_IS, (( argc ) ? ON : OFF ));

	} else {		// setting new break status

		if (( argc = OffOn( pszCmdLine )) == -1 )
			return ( Usage( BREAK_USAGE ));
_asm {
		mov	ax,03301h
		mov	dx,argc
		int	21h
}
	}

	return 0;
}
