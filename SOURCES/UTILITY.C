/* Experimental transient UTILITY_TEXT segment.
 * BREAK is dispatched through a near wrapper in DOSCMDS.C.
 */
#include "product.h"
#include <stdio.h>
#include <dos.h>
#include <io.h>
#include "4all.h"

// BREAK [ON | OFF]
// Display or change extended Ctrl+C / Ctrl+Break checking.
// With no argument, display the current setting.
// Examples: BREAK; BREAK ON; BREAK OFF.

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
}

	return 0;
}

// CLS [[BRIght] [BLInk] foreground ON background] [BORder color]
// Clear the screen, move the cursor home, and optionally set colors.
// Examples: CLS; CLS BRIGHT WHITE ON BLUE; CLS YELLOW ON BLUE BORDER RED.

int _far UtilityCls_Cmd( char *pszCmdLine )
{
	register int nAttrib = -1;

	// check if colors were requested
	if (( pszCmdLine ) && ( *pszCmdLine )) {

		if (( nAttrib = GetColors( pszCmdLine, 1 )) == -1 )
			return ( Usage( COLOR_USAGE ));

	} else if ( gpIniptr->StdColor != 0 ) {

		// check for default border (high byte)
		if (( nAttrib = ( gpIniptr->StdColor >> 8 )) != 0xFF )
			SetBorderColor( nAttrib );

		nAttrib = ( gpIniptr->StdColor & 0xFF );
	}

	// check for presence of ANSI
	if ( _isatty(STDOUT) && QueryIsANSI() ) {

		// output ANSI color sequence
		if ( nAttrib != -1 )
			set_colors( nAttrib );

		// output ANSI clear screen sequence
		printf( "\033[2J" );

	} else {

		// default to white on black
		if ( nAttrib == -1 )
			nAttrib = 7;

		// use the BIOS to clear the screen
		Scroll( 0, 0, GetScrRows(), GetScrCols()-1, 0, nAttrib );

		// move cursor to upper left corner
		SetCurPos( 0, 0 );

	}

	return 0;
}
