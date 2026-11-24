/* Experimental transient UTILITY_TEXT segment.
 * Commands are dispatched directly through far command-table pointers.
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

int _far Break_Cmd( char *pszCmdLine )
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

int _far Cls_Cmd( char *pszCmdLine )
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

int _far Truename_Cmd( char *pszCmdLine )
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

int _far Ver_Cmd( char *pszCmdLine )
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

int _far Idle_Cmd( char *pszCmdLine )
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
static unsigned int _near GetNibble( void )
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
static char _near DecompressChar( void )
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

// Expand the first four bytes of pKey into three identical copies.
// Requires twelve writable bytes; modifies pKey in place, returns nothing.
// Retain the original deliberately obscure copy order.
void _far _fastcall TripleKey( char *pKey )
{
	static int nIndexes[4] = {4, 2, 1, 3};
	register int i;

	for (i = 0; i < 4; i++)
		pKey[nIndexes[i] + 7] = pKey[nIndexes[i] + 3] = pKey[nIndexes[i] - 1];
}

// BEEP [frequency duration] [frequency duration ...]
// Generate tones using configured defaults when arguments are omitted.
// Examples: BEEP; BEEP 440 2.
// pszCmdLine supplies tone pairs; returns zero or Usage() status.
// SysBeep handles timing; consume a pending key on Ctrl+C as before.
int _far Beep_Cmd( LPTSTR pszCmdLine )
{
	unsigned int uFrequency, uDuration, uLength;

	uFrequency = gpIniptr->BeepFreq;
	uDuration = gpIniptr->BeepDur;

	if ( pszCmdLine == NULL )
		pszCmdLine = NULLSTR;

	do {
		if ( *pszCmdLine ) {
			// skip non-numeric args
			sscanf( pszCmdLine, _TEXT("%*[^0123456789] %u %n"), &uFrequency, &uLength );
			pszCmdLine += uLength;
			if ( *pszCmdLine ) {
				sscanf( pszCmdLine, _TEXT("%d %n"), &uDuration, &uLength );
				pszCmdLine += uLength;
			}

			if ( uLength == 0 )
				return ( Usage( BEEP_USAGE ));
		}

		// kludge to abort on a ^C
		if ( SysBeep( uFrequency, uDuration ) == CTRLC )
			_getch();

	} while ( *pszCmdLine );

	return 0;
}

// DELAY [/B] [/M] [duration]
// Wait for the requested duration (default one second).
// /B permits a keypress to end the wait; /M selects milliseconds.
// Examples: DELAY; DELAY 5; DELAY /M 100.
// pszCmdLine supplies options and duration; returns zero after SysWait.
// Preserves existing token parsing and interrupt behavior.
int _far Delay_Cmd( LPTSTR pszCmdLine )
{
	register unsigned int i;
	LPTSTR pszToken;
	ULONG ulWait = 1L;
	long lFlags = 0, lTemp;

	if (( pszCmdLine ) && ( *pszCmdLine )) {

		for ( i = 0; (( pszToken = ntharg( pszCmdLine, i )) != NULL ); i++ ) {
			if (( lTemp = switch_arg( pszToken, _TEXT("BM") )) != 0L )
				lFlags |= lTemp;
			else if ( isdigit( *pszToken ))
				sscanf( pszToken, FMT_ULONG, &ulWait );
		}
	}

	SysWait( ulWait, (int)lFlags );

	return 0;
}
