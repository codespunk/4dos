/* Experimental transient UTILITY_TEXT segment.
 * BREAK is dispatched through a near wrapper in DOSCMDS.C.
 */
#include "product.h"
#include <stdio.h>
#include <dos.h>
#include <io.h>
#include <stdlib.h>
#include <ctype.h>
#include <conio.h>
#include <fcntl.h>
#include <malloc.h>
#include <share.h>
#include <string.h>
#include <direct.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "4all.h"
#include "batcomp.h"

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

// TRUENAME filename
// Display the true DOS pathname, resolving substituted drives where supported.
// Examples: TRUENAME C:\AUTOEXEC.BAT; TRUENAME C:\DOS\..\CONFIG.SYS.

int _far UtilityTruename_Cmd( char *pszCmdLine )
{
	char name[MAXFILENAME];

	if (( pszCmdLine == NULL ) || ( *pszCmdLine == '\0' ))
		return ( Usage( TRUENAME_USAGE ));

	if ( true_name( pszCmdLine, name ) == NULL )
		return ERROR_EXIT;

	printf( FMT_STR_CRLF, name );

	return 0;
}

extern void _far UtilityShowOS( void );

// VER [/R]
// Display the DOS version; /R adds the 4DOS build, DOS revision,
// and DOS kernel location information where available.
// Examples: VER; VER /R.

int _far UtilityVer_Cmd( char *pszCmdLine )
{
	extern int fWin98;
	extern int fWinME;
	long lVerFlags;

	if (( lVerFlags = switch_arg( pszCmdLine, "R" )) < 0 )
		return ( Usage( VER_USAGE ));

	UtilityShowOS();

	// display version info (Rev level, & whether in HMA or ROM)
	if ( lVerFlags == 1 ) {

		printf( FOURDOS_REV, VER_BUILD, __DATE__ );

		// DR-DOS doesn't have an internal revision number
		if (( gnOSFlags & DOS_IS_DR ) == 0 ) {

			if ( gnOSFlags & DOS_IS_OS2 )
				printf( OS2_REVISION, ( gnOSFlags & 0x7FF ));
			else
				printf( DOS_REVISION, ( gnOSFlags & 0x7 ) + 'A' );

			printf( DOS_LOCATION, (( gnOSFlags & DOS_IN_HMA ) ? DOS_HMA : (( gnOSFlags & DOS_IN_ROM ) ? DOS_ROM : DOS_LOW )));
		}

		crlf();

	}

	return 0;
}

// IDLE [ON | OFF]
// Display or change DR DOS dynamic idle detection (BatteryMAX).
// With no argument, display the current setting.
// Examples: IDLE; IDLE ON; IDLE OFF.
//
// Match the 16-bit DR DOS idle-state layout without the PORTAB.H dependency.
// The original IDLE.H uses WORD fields and a VOID FAR * pointer.
#define UTILITY_IDLE_ON 0x4000
#define UTILITY_IDLE_ENABLE 0x8000
struct utility_idle_state {
    unsigned short count;
    unsigned short max;
    unsigned short flags;
    void _far *vec;
    unsigned short delay;
    unsigned short reload;
    unsigned short indos;
};

int _far UtilityIdle_Cmd( char *pszCmdLine )
{
    struct utility_idle_state _far *idle = GetIdlePtr();

    // DR DOS boolean flags are reversed.
    if ( idle->flags & UTILITY_IDLE_ENABLE )
        return error( ERROR_4DOS_IDLE_DISABLED, NULL );
    if ( pszCmdLine == NULL || *pszCmdLine == '\0' )
        printf( IDLE_IS, idle->flags & UTILITY_IDLE_ON ? OFF : ON );
    else switch ( OffOn( pszCmdLine )) {
        case 1:
            idle->flags &= ~UTILITY_IDLE_ON;
            break;
        case 0:
            idle->flags |= UTILITY_IDLE_ON;
            break;
        default:
            return Usage( IDLE_USAGE );
    }
    return 0;
}

static int fNibble;			// 0 = first nibble, 1 = second
static LPBYTE pInputText;	// pointer to current character in input
static LPBYTE pTransTab;	// pointer to translation table


// Read the next four-bit code (0..15) from the compressed stream.
// No parameters. Advances pInputText on high-nibble reads and toggles
// fNibble; retains the current byte for the following low-nibble read.
static unsigned int GetNibble( void )
{
	static unsigned char cThis = 0;
	unsigned int uResult;

	uResult = ( fNibble ? ( cThis & 0xF ) : (( cThis = *pInputText++ ) >> 4 ));
	fNibble ^= 1;
	return uResult;
}


// Decode one character using the current translation table.
// No parameters. Returns the decoded byte and consumes one, two or
// three nibbles through GetNibble(). Requires initialized decoder state.
static char DecompressChar( void )
{
	register unsigned int uNibble;

	//Get next nibble
	uNibble = GetNibble();

	// Determine decompression type
	switch(uNibble) {
	case 0:
		// Return an uncompressed byte
		uNibble = GetNibble();
		return (char)(uNibble | ((unsigned char)GetNibble() << 4));
	case 1:
		// Return a compressed byte from the second half of the table
		uNibble = GetNibble();
		return (pTransTab[uNibble + 14]);
	default:
		// Return a compressed byte from the first part of the table
		return (pTransTab[uNibble - 2]);
	}
}


// Decode uCount bytes from lpHeader into lpszOutputBuf.
// Caller supplies valid old/new-format data and sufficient output space.
// Initializes private decoder state; skips the key in encrypted new-format
// data. Input must already be decrypted. No return value or EOF appended.
void _far BatchDecompile( CompHead _far *lpHeader, LPBYTE lpszOutputBuf, unsigned int uCount )
{
	// Initialize globals
	pTransTab = &(lpHeader->cCommon[0]);
	pInputText = pTransTab + BC_COM_SIZE;
	fNibble = 0;

	// Check for new format
	if ( lpHeader->usSignature == 0xEBBE ) {

		// is it encrypted?
		if ((( lpHeader->fEncrypted & 0xF ) ^ ( lpHeader->fEncrypted >> 4 )) == 0 ) {
			// skip password
			pInputText += 4;
		}

	} else {
		// old format doesn't have encryption word
		pInputText -= 2;
		pTransTab -= 2;
	}

	// Decompress all characters
	while ( uCount-- )
		*lpszOutputBuf++ = DecompressChar();
}
