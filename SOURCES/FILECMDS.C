

/*
  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  (1) The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  (2) The Software, or any portion of it, may not be compiled for use on any
  operating system OTHER than FreeDOS without written permission from Rex Conn
  <rconn@jpsoft.com>

  (3) The Software, or any portion of it, may not be used in any commercial
  product without written permission from Rex Conn <rconn@jpsoft.com>

  (4) THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/


// FILECMDS.C - File handling commands (COPY, MOVE, DEL, etc.) for 4xxx / TCMD
//   Copyright (c) 1988 - 2003 Rex C. Conn   All rights reserved

#include "product.h"

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <direct.h>
#include <dos.h>

#ifdef __WATCOMC__
extern unsigned *__io_mode;
#else
#include <msdos.h>
#endif

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/locking.h>
#include <io.h>
#include <malloc.h>
#include <share.h>
#include <string.h>

#include "4all.h"


extern void UpdateFuzzyTree( LPTSTR , LPTSTR , int );

static void wildname( LPTSTR , LPTSTR , LPTSTR );

static int FileCopy( LPTSTR , LPTSTR , unsigned long, long *, FILESEARCH * );


static int _near delete_fcb( unsigned int );
static int _near parse_fcb( unsigned int, unsigned int );


static int _fastcall FileOutput( LPTSTR , int );

static int fDiskFull;
RANGES aRanges;				// date/time/size ranges


// display the number of files copied/moved/renamed/deleted, with special
//   kludge for "1 file" vs. "2 files"
void _fastcall FilesProcessed( LPTSTR szFormat, long lFiles )
{
	printf( szFormat, lFiles, (( lFiles == 1L ) ? ONE_FILE : MANY_FILES ));
}


// COPY_OPTIONS
#define COPY_BY_ATTRIBUTES 1
#define COPY_ASCII 2
#define COPY_BINARY 4
#define COPY_CHANGED 8
#define COPY_NOERRORS 0x10
#define COPY_PERCENT 0x20
#define COPY_HIDDEN 0x40
#define COPY_FTP_ASCII 0x80
#define COPY_MODIFIED 0x100
#define COPY_NOTHING 0x200
#define COPY_KEEP 0x400
#define COPY_QUERY 0x800
#define COPY_QUIET 0x1000
#define COPY_REPLACE 0x2000
#define COPY_SUBDIRS 0x4000
#define COPY_TOTALS 0x8000
#define COPY_UPDATED 0x10000L
#define COPY_VERIFY 0x20000L
#define COPY_CLEAR_ARCHIVE 0x40000L
#define COPY_DOS62_Y 0x80000L
#define COPY_KEEP_ATTRIBUTES 0x100000L
#define COPY_RESTART 0x200000L
#define COPY_OVERWRITE_RDONLY 0x400000L
#define COPY_EXISTS 0x800000L


typedef struct 
{
	TCHAR szSource[MAXFILENAME];
	TCHAR szSourceName[MAXFILENAME];
	TCHAR szTarget[MAXFILENAME];
	TCHAR szTargetName[MAXFILENAME];
	LPTSTR szFilename;
	LPTSTR pFirstCopyArg;		// pointer to first source arg in COPY
	unsigned long fOptions;
	unsigned long fFlags;
	long lFilesCopied;

	long lAppendOffset;

	FILESEARCH t_dir;
} COPY_STRUCT;

static int _copy( COPY_STRUCT *);
static int fClip;


// copy or append file(s)
// Near COPY dispatcher; forward to the FILE_TEXT implementation.
int _near Copy_Cmd( LPTSTR pszCmdLine )
{
	register LPTSTR pszArg;
	TCHAR szClip[MAXFILENAME], *pszTarget = NULL;
	int nArgc, nTlen, nReturn = 0, nLength, fOldVerify = 0;
	long lCopy;
	COPY_STRUCT Copy;

	Copy.lFilesCopied = 0L;
	Copy.lAppendOffset = 0;
	Copy.fFlags = COPY_CREATE;
	Copy.fOptions = 0;

	fDiskFull = 0;

	// remove the white space around appends (+)
	collapse_whitespace( pszCmdLine, _TEXT("+") );

	// get date/time/size ranges
	if ( GetRange( pszCmdLine, &aRanges, 0 ) != 0 )
		return ERROR_EXIT;

	gnExclusiveMode = gnInclusiveMode = gnOrMode = 0;

	// scan command line, parsing for & removing switches
	for ( nArgc = 0; (( pszArg = ntharg( pszCmdLine, nArgc )) != NULL ); ) {

		// check for MS-DOS 6.2 /-Y option & convert it to a COPY /R
		if ( _stricmp( pszArg, _TEXT("/-Y") ) == 0 )
			lCopy = COPY_REPLACE;

		else if (( lCopy = switch_arg( pszArg, "*ABCEGH MNOPQRSTUVXYK Z" )) == -1 )

			return ( Usage( COPY_USAGE ));	// invalid switch

		Copy.fOptions |= lCopy;

		// leading ASCII (/A) or Binary (/B) switch?
		if ( nArgc == 0 ) {
			if ( lCopy & COPY_ASCII )
				Copy.fFlags |= ASCII_SOURCE;
			else if ( lCopy & COPY_BINARY )
				Copy.fFlags |= BINARY_SOURCE;
		}

		// remove leading /A & /B, & collapse spaces before
		//   trailing /A & /B
		if (( lCopy & ( COPY_ASCII | COPY_BINARY )) && ( nArgc > 0 )) {

			// strip leading whitespace
			while (( iswhite( *(--gpNthptr) )) && ( gpNthptr >= pszCmdLine ))
				strcpy( gpNthptr, gpNthptr+1 );

		} else if ( *pszArg == gpIniptr->SwChr ) {

			// remove the switch
			strcpy( gpNthptr, skipspace( gpNthptr + strlen( pszArg )));
			continue;

		} else if ( nArgc > 0 ) {
			// save pointer to potential target
			pszTarget = gpNthptr;
		}

		nArgc++;
	}

	// check for COMMAND.COM / CMD.EXE prompt if file exists & not in batch
	if (( gpIniptr->CopyPrompt ) && ( cv.bn < 0 )) 
		Copy.fOptions |= COPY_REPLACE;

	// check for flag to preserve all attributes (won't work w/Netware!)
	if ( Copy.fOptions & COPY_KEEP_ATTRIBUTES )
		Copy.fFlags |= MOVING_FILES;

	// if query, turn off quiet flag
	if ( Copy.fOptions & COPY_QUERY )
		Copy.fOptions &= ~( COPY_QUIET | COPY_TOTALS );

	// display percentage copied?
	if (( Copy.fOptions & COPY_PERCENT ) && (( Copy.fOptions & COPY_QUIET ) == 0 ))
		Copy.fFlags |= COPY_PROGRESS;

	// abort if no file arguments
	if ( first_arg( pszCmdLine ) == NULL )
		return ( Usage( COPY_USAGE ));

	// get the target name, or "*.*" for default name
	if ( pszTarget != NULL ) {

		copy_filename( Copy.szTarget, first_arg( pszTarget ));
 
		// check for ASCII (/A) or binary (/B) switches at end of line
		switch ( switch_arg( ntharg( pszTarget, 1 ), _TEXT("AB") )) {
		case 1:
			Copy.fFlags |= ASCII_TARGET;
			break;
		case 2:
			Copy.fFlags |= BINARY_TARGET;
		}

		*pszTarget = _TEXT('\0');		// remove target from command line

		// check target for device name
		if ( QueryIsDevice( Copy.szTarget )) {
			// set "target is device" flag
			Copy.fFlags |= DEVICE_TARGET;
		} else if ( QueryIsPipeName( Copy.szTarget )) {
			// set "target is device" flag
			Copy.fFlags |= PIPE_TARGET;
		}

		if ( Copy.fFlags & ( DEVICE_TARGET | PIPE_TARGET )) {
			// can't "replace" or "update" device or pipe!
			Copy.fOptions &= ~( COPY_REPLACE | COPY_UPDATED);
		}

	} else {
		if (( pszArg = (( Copy.fOptions & COPY_SUBDIRS ) ? gcdir( NULL, 0 ) : WILD_FILE )) == NULL )
			return ERROR_EXIT;
		strcpy( Copy.szTarget, pszArg );
	}

	if ( Copy.fOptions & COPY_VERIFY ) {

		// save original VERIFY status
		fOldVerify = QueryVerifyWrite();

		// set VERIFY ON
		SetVerifyWrite( 1 );

	}

	if ( setjmp( cv.env ) == -1 ) {
		nReturn = CTRLC;
		goto copy_abort;
	}

	// if target not a char device or pipe, build full filename
	if (( Copy.fFlags & ( DEVICE_TARGET | PIPE_TARGET )) == 0 ) {

		if ( mkfname( Copy.szTarget, 0 ) == NULL )
			goto copy_abort;

		// don't build target name yet if copying subdirs
		if (( Copy.fOptions & COPY_SUBDIRS ) == 0 ) {

			if (( AddWildcardIfDirectory( Copy.szTarget ) == 0 ) &&  ( Copy.szTarget[ strlen( Copy.szTarget ) - 1] == _TEXT('\\') )) {
				nReturn = error( ERROR_PATH_NOT_FOUND, Copy.szTarget );
				goto copy_abort;
			}
		}
	}

	// check for appending
	pszArg = pszCmdLine;
	do {
		pszArg = scan( pszArg, _TEXT("+"), QUOTES + 1 );

		if ( *pszArg == _TEXT('+') ) {

			// remove the '+' and set append flags
			*pszArg = _TEXT(' ');

			// default for appending files is ASCII
			if (( Copy.fFlags & BINARY_SOURCE ) == 0 )
				Copy.fFlags |= ASCII_SOURCE;

			// can't append to a device or pipe!
			if (( Copy.fFlags & DEVICE_TARGET ) == 0 )
				Copy.fFlags |= APPEND_FLAG;
		}

	} while ( *pszArg );

	Copy.pFirstCopyArg = pszCmdLine;

	nTlen = strlen( Copy.szTarget );

	for ( nReturn = nArgc = 0; (( pszArg = ntharg( pszCmdLine, nArgc )) != NULL ); nArgc++ ) {

		// clear "source is device/pipe" flags
		Copy.fFlags &= ~( DEVICE_SOURCE | PIPE_SOURCE );

		if ( QueryIsDevice( pszArg ))
			Copy.fFlags |= DEVICE_SOURCE;
		else if ( QueryIsPipeName( pszArg ))
			Copy.fFlags |= PIPE_SOURCE;
		else {
			// if source is not a device, make a full filename
			if ( mkfname( pszArg, 0 ) == NULL )
				break;
			
			AddWildcardIfDirectory( pszArg );
		}

		copy_filename( Copy.szSource, pszArg );

		// check for trailing ASCII (/A) or Binary (/B) switches
		if (( lCopy = switch_arg( ntharg( pszCmdLine, nArgc+1 ), _TEXT("AB") )) > 0 ) {

			// remove the source ASCII or binary flags
			Copy.fFlags &= ~( ASCII_SOURCE | BINARY_SOURCE );
			Copy.fFlags |= (( lCopy == 1 ) ? ASCII_SOURCE : BINARY_SOURCE );
			nArgc++;
		}

		// check for dumdums doing an infinitely recursive COPY
		//   (i.e., "COPY /S . subdir" )
		nLength = ((( pszArg = path_part( Copy.szSource )) != NULL ) ? strlen( pszArg ) : 0 );
		if ( Copy.fOptions & COPY_SUBDIRS ) {
			if ( _strnicmp( pszArg, Copy.szTarget, nLength ) == 0 )
				return ( error( ERROR_4DOS_INFINITE_COPY, Copy.szSource ));
		}

		// save the source filename part ( for recursive calls and
		//   include lists)
		Copy.szFilename = Copy.szSource + nLength;
		pszArg = (LPTSTR)_alloca( ( strlen( Copy.szFilename ) + 1 ) * sizeof(TCHAR) );
		Copy.szFilename = strcpy( pszArg, Copy.szFilename );

		szClip[0] = _TEXT('\0');
		fClip = 0;
		if ( stricmp( Copy.szSource, CLIP ) == 0 ) {
			RedirToClip( szClip, 99 );
			if ( CopyFromClipboard( szClip ) != 0 )
				continue;
			strcpy( Copy.szSource, szClip );
			fClip = 1;
			Copy.fFlags &= ~DEVICE_SOURCE;
		} else if ( stricmp( Copy.szTarget, CLIP ) == 0 ) {
			RedirToClip( szClip, 99 );
			if ( Copy.fFlags & APPEND_FLAG ) {
				if ( CopyFromClipboard( szClip ) != 0 )
					continue;
			}
			strcpy( Copy.szTarget, szClip );
			fClip = 2;
			Copy.fFlags &= ~DEVICE_TARGET;
		}

		nReturn = _copy( &Copy );

		HoldSignals();
		if ( nReturn != CTRLC ) {

			if ( fClip & 1 )
				strcpy( Copy.szSource, CLIP );
			else if ( fClip & 2 ) {
				// copying to clipboard - get a handle to the temp
				//   file and use it to copy the file to the clip
				if (( fClip = _sopen( Copy.szTarget, _O_RDONLY | _O_BINARY, _SH_DENYNO )) >= 0 ) {
					CopyToClipboard( fClip );
					_close( fClip );
				}
				strcpy( Copy.szTarget, CLIP );
			}
		}

		if ( szClip[0] )
			remove( szClip );
		EnableSignals();

		// restore original "target"
		if (( Copy.fFlags & APPEND_FLAG ) == 0 )
			Copy.szTarget[ nTlen ] = _TEXT('\0');

		if (( setjmp( cv.env ) == -1 ) || ( cv.fException ) || ( nReturn == CTRLC )) {
			nReturn = (( fDiskFull == 0 ) ? CTRLC : ERROR_EXIT );
			break;
		}


	}

copy_abort:

	// reset VERIFY flag to original state
	if ( Copy.fOptions & COPY_VERIFY )
		SetVerifyWrite( fOldVerify );

	if (( Copy.fOptions & COPY_QUIET ) == 0 ) {

		FilesProcessed( FILES_COPIED, Copy.lFilesCopied );
	}


	return (( Copy.lFilesCopied != 0L ) ? nReturn : ERROR_EXIT );
}



#pragma alloc_text( MISC_TEXT, _copy )


static int _copy( register COPY_STRUCT *Copy )
{
	register int nFind;
	int i, nReturn = 0;
	LPTSTR pszSourceArg, pszTargetArg;
	FILESEARCH dir;
	long lMode = 0;


	EnableSignals();

	// copy date/time/size range info
	memmove( &(dir.aRanges), &aRanges, sizeof(RANGES) );

	// get hidden & system files too
	if (( Copy->fOptions & COPY_BY_ATTRIBUTES ) || ( Copy->fOptions & COPY_HIDDEN))
		lMode |= 0x07;

	// if copying subdirectories, create the target (must be a directory!)
	if ( Copy->fOptions & COPY_SUBDIRS ) {

		// remove trailing '/' or '\'
		strip_trailing( Copy->szTarget+3, SLASHES );

		if (( Copy->fOptions & COPY_NOTHING ) == 0 ) {

			// if we created the directory, then copy any
			//    existing description
			if (( Copy->fFlags & ( DEVICE_TARGET | PIPE_TARGET )) == 0 ) {

				i = is_dir( Copy->szTarget );

				MakeDirectory( Copy->szTarget, 1 );

				if ( is_dir( Copy->szTarget ) == 0 )
					return ( error( ERROR_PATH_NOT_FOUND, Copy->szTarget ));

				// copy description to newly-created target
				if ( i == 0 ) {
					copy_filename( Copy->szSourceName, path_part( Copy->szSource ));
					strip_trailing( Copy->szSourceName+3, SLASHES );
					process_descriptions( Copy->szSourceName, Copy->szTarget, DESCRIPTION_COPY );
				}
			}
		}

		if (( Copy->fFlags & ( DEVICE_TARGET | PIPE_TARGET )) == 0 )
			mkdirname( Copy->szTarget, WILD_FILE );
	}

	// suppress error message from find_file
	if ( Copy->fOptions & COPY_NOERRORS ) {
		lMode |= FIND_NO_ERRORS;
		Copy->fFlags |= NO_ERRORS;
	}

	for ( nFind = FIND_FIRST; ; nFind = FIND_NEXT ) {

		// if it's a device, only look for it once
		if ( Copy->fFlags & ( DEVICE_SOURCE | PIPE_SOURCE )) {

			if ( nFind == FIND_NEXT )
				break;

			copy_filename( Copy->szSourceName, Copy->szSource );

		} else {

			if ( find_file( nFind, Copy->szSource, ( lMode | FIND_BYATTS | FIND_RANGE | FIND_EXCLUDE ), &dir, Copy->szSourceName ) == NULL )
				break;

			if (( setjmp( cv.env ) == -1 ) || ( cv.fException )) {
				FindClose( dir.hdir );
				return CTRLC;
			}

			CheckForBreak();

		}

		// clear overwrite query flag
		Copy->fOptions &= ~COPY_EXISTS;

		// don't copy unmodified files
		if (( Copy->fOptions & COPY_MODIFIED ) && (( dir.attrib & _A_ARCH ) == 0 ))
			continue;

		// build the target name
		wildname( Copy->szTargetName, Copy->szTarget, Copy->szSourceName );

		// if changed (/C), only copy source if target is older
		// if update (/U), only copy the source if the target is
		//   older or doesn't exist
		// if replace (/R) prompt before overwriting existing file

		if (( Copy->fOptions & ( COPY_KEEP | COPY_UPDATED | COPY_CHANGED | COPY_REPLACE )) && ( find_file( FIND_FIRST, Copy->szTargetName, 0x07L | FIND_CLOSE | FIND_EXACTLY | FIND_NO_ERRORS | FIND_NO_FILELISTS, &(Copy->t_dir), NULL ) != NULL )) {

			if ( Copy->fOptions & COPY_KEEP )
				continue;		// don't overwrite existing file (/O)

			if ( Copy->fOptions & ( COPY_UPDATED | COPY_CHANGED )) {

				if (( Copy->t_dir.fd.wr_date > dir.fd.wr_date ) || (( Copy->t_dir.fd.wr_date == dir.fd.wr_date ) && ( Copy->t_dir.ft.wr_time >= dir.ft.wr_time )))

					continue;
			}

			// if querying for replacement, don't ask if appending!
			if (( Copy->fOptions & COPY_REPLACE ) && ( Copy->fFlags & COPY_CREATE ))
				Copy->fOptions |= COPY_EXISTS;

		} else if ( Copy->fOptions & COPY_CHANGED )
			continue;	// target doesn't exist

		// check for special conditions w/append
		//   (the source is the same as the target)
		if ( _stricmp( Copy->szSourceName, Copy->szTargetName ) == 0 ) {

			// if not the first arg, check to make sure the arg
			//   wasn't the target & hasn't been overwritten
			if (( Copy->fFlags & COPY_CREATE ) == 0 ) {
				// check for "copy all.lst + *.lst"
				if ( _stricmp( first_arg( Copy->pFirstCopyArg ), Copy->szSourceName ) != 0 )
					nReturn = error( ERROR_4DOS_CONTENTS_LOST, Copy->szSourceName );
			}

			// if target file is first source file, ignore it
			if ( Copy->fFlags & APPEND_FLAG )
				goto next_copy;
		}

		if ((( Copy->fOptions & ( COPY_QUIET | COPY_TOTALS )) == 0 ) || (( Copy->fOptions & COPY_REPLACE ) && ( Copy->fOptions & COPY_EXISTS ))) {

			printf( "%s =>%s %s", (( fClip == 1 ) ? CLIP : Copy->szSourceName ), (( Copy->fFlags & COPY_CREATE ) ? NULLSTR : ">" ), (( fClip == 2 ) ? CLIP : Copy->szTargetName ));


			// query if /Q, or /R and target exists
			if ( Copy->fOptions & ( COPY_QUERY | COPY_EXISTS )) {

				if ((( i = QueryInputChar((( Copy->fOptions & COPY_EXISTS ) ? REPLACE : NULLSTR ), YES_NO_ALL )) == ALL_CHAR ) || ( i == REST_CHAR ))
					Copy->fOptions &= ~( COPY_QUERY | COPY_REPLACE );
				else if ( i == NO_CHAR )
					continue;
				else if ( i == ESCAPE ) {
					FindClose( dir.hdir );
					break;
				}
			} else
				crlf();
		}

		if (( Copy->fOptions & COPY_NOTHING ) == 0 ) {

			// if /Z, overwrite RDONLY files
			if ( Copy->fOptions & COPY_OVERWRITE_RDONLY )
				SetFileMode( Copy->szTargetName, 0 );

			if ((( nReturn = FileCopy( Copy->szTargetName, Copy->szSourceName, Copy->fFlags, &( Copy->lAppendOffset ), &dir )) == CTRLC ) || ( cv.fException ) || ( setjmp( cv.env ) == -1 )) {
				// reset the directory search handles
				FindClose( dir.hdir );
				return CTRLC;
			}

			if ( nReturn == 0 ) {

				Copy->lFilesCopied++;

				if ( Copy->fOptions & COPY_CLEAR_ARCHIVE ) {
					// clear the archive bit
					SetFileMode( Copy->szSourceName, dir.attrib & 0xDF );
				}

				// copy an existing description to the new name
				if (( Copy->fFlags & ( APPEND_FLAG | DEVICE_SOURCE | DEVICE_TARGET | PIPE_SOURCE | PIPE_TARGET)) == 0 )
					process_descriptions( Copy->szSourceName, Copy->szTargetName, DESCRIPTION_COPY );
			}
		}

next_copy:
		EnableSignals();

		// if target is unambiguous file, append next file(s)
		//   if appending to wildcard, use first target name
		if ( strpbrk( Copy->szTarget, WILD_CHARS ) == NULL ) {
			if (( Copy->fFlags & ( DEVICE_TARGET | PIPE_TARGET )) == 0 )
				Copy->fFlags |= APPEND_FLAG;
		} else if ( Copy->fFlags & APPEND_FLAG )
			copy_filename( Copy->szTarget, Copy->szTargetName );

		// if appending, clear the CREATE flag
		if ( Copy->fFlags & APPEND_FLAG )
			Copy->fFlags &= ~COPY_CREATE;

	}

	// copy subdirectories too?
	if ( Copy->fOptions & COPY_SUBDIRS ) {

		CheckFreeStack( 1024 );

		sprintf( Copy->szSource, FMT_DOUBLE_STR, path_part( Copy->szSource), WILD_FILE );

		// save the current source filename start
		pszSourceArg = strchr( Copy->szSource, _TEXT('*') );

		// save the current target filename start
		if (( Copy->fFlags & ( DEVICE_TARGET | PIPE_TARGET )) == 0 )
			pszTargetArg = Copy->szTarget + strlen( path_part( Copy->szTarget ));

		// search for all subdirectories in this (sub)directory
		//   tell find_file() not to display errors, and to return
		//   only subdirectory names

		for ( nFind = FIND_FIRST; ( find_file( nFind, Copy->szSource, ( lMode | 0x10 | FIND_NO_ERRORS | FIND_ONLY_DIRS | FIND_NO_FILELISTS | FIND_EXCLUDE ), &dir, NULL ) != NULL ); nFind = FIND_NEXT ) {

			// make the new "source" and "target"

			sprintf( pszSourceArg, FMT_PATH_STR, dir.szFileName, Copy->szFilename );

			// strip the filename part from the target
			if (( Copy->fFlags & ( DEVICE_TARGET | PIPE_TARGET )) == 0 )
				copy_filename( pszTargetArg, dir.szFileName );

			// process directory tree recursively
			nReturn = _copy( Copy );
			if (( setjmp( cv.env ) == -1 ) || ( cv.fException )) {
				FindClose( dir.hdir );
				return CTRLC;
			}

			// restore the original name
			strcpy( pszSourceArg, WILD_FILE );

		}
	}

	return nReturn;
}


// MOVE remains a near dispatcher; implementation resides in FILE_TEXT.
extern int _far FileMove_Cmd( LPTSTR );
// Near MOVE dispatcher; forward to the FILE_TEXT implementation.
int _near Mv_Cmd( LPTSTR pszCmdLine )
{
	return FileMove_Cmd( pszCmdLine );
}

// Reset the shared disk-full and clipboard state before MOVE.
void _far FileMoveResetState( void )
{
	fDiskFull = 0;
	fClip = 0;
}

// Return the disk-full flag maintained by the shared copy implementation.
int _far FileMoveDiskFull( void )
{
	return fDiskFull;
}

// Far bridge to the shared FileCopy routine in MISC_TEXT.
// Forward target, source, copy flags, append offset, and search metadata.
int _far FileMoveCopyBridge( LPTSTR target, LPTSTR source, unsigned long flags, long *offset, FILESEARCH *dir )
{
	return FileCopy( target, source, flags, offset, dir );
}

#pragma alloc_text( MISC_TEXT, FileCopy )



// copy from the source to the target files
//   szOutputName = target file name
//   szInputName = source file name
//   fFlags = create/append, device & ASCII/binary flags
//   lAppendOffset = current end offset in append file
static int FileCopy( LPTSTR szOutputName, LPTSTR szInputName, unsigned long fFlags, long *lAppendOffset, FILESEARCH *dir )

{

	register int nReturn = 0;
	unsigned int fEoF, uBufferSize;
	int nInputFile = -1, nOutputFile = -1;
	BYTE _far *lpbSegment, _far *lpbPtr;
	unsigned int uAttribute = 0, nBytesRead, nBytesWritten;

	unsigned long ulTransferred;
	struct {
		char _far *pgealist;
		char _far *pfealist;
		unsigned long oError;
	} eaop;


	// check for same name
	if ( _stricmp( szInputName, szOutputName ) == 0 ) {
		if ((( fFlags & NO_ERRORS ) == 0 ) || ( *lAppendOffset != 0L ))
			nReturn = error( ERROR_4DOS_DUP_COPY, szInputName );
		goto FileCopyExit;
	}


	
	// if source has type but target doesn't, set target to source type
	// if target has type but source doesn't, set source to target type
	if (( nReturn = (unsigned int)(fFlags & ( ASCII_SOURCE | BINARY_SOURCE | ASCII_TARGET | BINARY_TARGET))) == ASCII_SOURCE )
		fFlags |= ASCII_TARGET;
	else if ( nReturn == BINARY_SOURCE )
		fFlags |= BINARY_TARGET;
	else if ( nReturn == ASCII_TARGET )
		fFlags |= ASCII_SOURCE;
	else if ( nReturn == BINARY_TARGET )
		fFlags |= BINARY_SOURCE;

	// devices & pipes default to ASCII copy
	if (( fFlags & ( DEVICE_SOURCE | PIPE_SOURCE )) && (( fFlags & BINARY_SOURCE ) == 0 ))
		fFlags |= ASCII_SOURCE;


	// request memory block
	// NT bombs on console reads with big buffer!
	uBufferSize = (( fFlags & DEVICE_SOURCE ) ? 0x400 : 0xFFF0 );

	if (( lpbSegment = (LPBYTE)AllocMem( &uBufferSize )) == 0L ) {
		nReturn = error( ERROR_NOT_ENOUGH_MEMORY, NULL );
		goto FileCopyExit;
	}

	if ( setjmp( cv.env ) == -1 ) {		// ^C trapping
		nReturn = CTRLC;
		goto filebye;
	}

	// open input file for read


	if (( nInputFile = _sopen( szInputName, (_O_RDONLY | _O_BINARY), _SH_DENYNO )) < 0 ) {
		nReturn = error( _doserrno, szInputName );
		goto filebye;
	}

	_doserrno = 0;	// can be 768 here (bug in MSVCRT library?)

	// lock the file (and check for somebody else already locking it!)
	if (( fFlags & ( DEVICE_SOURCE | PIPE_SOURCE )) == 0 ) {
		if ( InstalledCheck( 0x1000 ) < 0 /* SHARE installed */ && _locking( nInputFile, _LK_NBLCK, 0xFFFFFFFFL )) {
			if ( _doserrno != ERROR_INVALID_FUNCTION ) {
				nReturn = error( _doserrno, szInputName );
				goto filebye;
			}
		}
	}

	nReturn = 0;

	// if we are appending, open an existing file, otherwise create or
	//   truncate target file
	if (( fFlags & COPY_CREATE ) && (( fFlags & ( DEVICE_TARGET | PIPE_TARGET)) == 0 )) {

		// if source not a device, copy source attributes to target
		if (( fFlags & ( DEVICE_SOURCE | PIPE_SOURCE )) == 0 ) {

			QueryFileMode( szInputName, &uAttribute );

			// strip wacky Netware bits & RDONLY bit
			//   (unless moving files)
			uAttribute &= (( fFlags & MOVING_FILES ) ? 0x27 : 0x26 );
		}

		eaop.pgealist = 0L;
		eaop.pfealist = lpbSegment;
		eaop.oError = 0L;

		if (( _osmajor < 10 ) || ( fFlags & ( DEVICE_SOURCE | PIPE_SOURCE )) || ( gpIniptr->CopyEA == 0 )) {
no_ea_copy:
			nReturn = 0;

			// if not OS/2 or no EAs, use the old approach
			if (( nOutputFile = _sopen( szOutputName, (_O_WRONLY | _O_BINARY | _O_CREAT | _O_TRUNC), _SH_DENYRW, ( _S_IREAD | _S_IWRITE ) )) < 0 ) {
				nReturn = error( _doserrno, szOutputName );
				goto filebye;
			}

		} else {

			// copy EAs if OS/2 compatibility box
			unsigned long _far *flptr;

			// kill target (& its EA's) before copy
			// if "access denied", don't try the create (there's
			//   a bug in OS/2 2.0 GA that will crash the DOS box)
			if (( remove( szOutputName ) == -1 ) && ( _doserrno == ERROR_ACCESS_DENIED )) {
				nReturn = error( _doserrno, szOutputName );
				goto filebye;
			}

			// set the buffer size
			flptr = (unsigned long _far *)lpbSegment;
			*flptr = (long)(uBufferSize - sizeof(unsigned long));

			if (( nReturn = _dos_createEA( szInputName, nInputFile, szOutputName, &nOutputFile, uAttribute, (unsigned int)&eaop )) != 0 )
				goto no_ea_copy;
		}

	// we're appending, or target is a device or pipe!
	} else if (( nOutputFile = _sopen( szOutputName, (_O_RDWR | _O_BINARY), _SH_DENYWR )) < 0 )
		nReturn = _doserrno;

	if ( nReturn ) {

		// DOS returns an odd error if it couldn't create the file!
		if (( fFlags & COPY_CREATE ) && ( nReturn == 2 ))
			nReturn = ERROR_4DOS_CANT_CREATE;

		nReturn = error( nReturn, szOutputName );
		goto filebye;
	}


	if ( fFlags & ( DEVICE_TARGET | PIPE_TARGET )) {

		// can't append to device or FTP file
		fFlags |= COPY_CREATE;

		// char devices & pipes default to ASCII copy
		if (( fFlags & ( BINARY_SOURCE | BINARY_TARGET )) == 0 )
			fFlags |= ( ASCII_TARGET | ASCII_SOURCE );
		else {
			// set raw mode for binary copy
			SetIOMode( nOutputFile, 0 );
		}

	}

	if (( fFlags & COPY_CREATE ) == 0 ) {

		// append at EOF of the target file (we have to kludge a bit
		//   here, because some files end in ^Z and some don't, & we
		//   may be in binary mode)

		// if continuing an append, move to saved EOF in target
		if ( *lAppendOffset != 0L )
			_lseek( nOutputFile, *lAppendOffset, SEEK_SET );
		else if ( fFlags & BINARY_SOURCE )
			*lAppendOffset = QuerySeekSize( nOutputFile );
		else {

		    // We should only get here if the source is ASCII, and 
		    //   the target is the first source.  Read the file until
		    //   EOF or we find a ^Z.
		    do {

			if (( nReturn = FileRead( nOutputFile, lpbSegment, uBufferSize, &nBytesRead )) != 0 ) {
				nReturn = error( nReturn, szOutputName );
				goto filebye;
			}

			// search for a ^Z
			if (( lpbPtr = (LPBYTE)_fmemchr( lpbSegment, 0x1A, nBytesRead )) != 0L ) {

				// backup to ^Z location
				*lAppendOffset += ((ULONG_PTR)lpbPtr - (ULONG_PTR)lpbSegment);
				_lseek( nOutputFile, *lAppendOffset, SEEK_SET );
				break;
			}

			*lAppendOffset += nBytesRead;

		    } while ( nBytesRead == uBufferSize );
		}
	}

	for ( ulTransferred = 0, fEoF = 0; (( fEoF == 0 ) && ( cv.fException == 0 )); ) {

		if (( fFlags & COPY_PROGRESS ) && ( QueryIsConsole( STDOUT )) && (( fFlags & ( DEVICE_SOURCE | PIPE_SOURCE )) == 0 )) {
			// display percent copied

			// have to use EVAL to do it in DOS
			// get string equivalents (for > 4Gb drives)
			char szBuffer[64];

			if ( dir->ulSize == 0L )
				strcpy( szBuffer, "100" );
			else {
				sprintf( szBuffer, "(%lu*100)/%lu=0.0", ulTransferred, dir->ulSize );
				evaluate( szBuffer );
			}
			printf( "%s%%\r", szBuffer );

		}

		if (( nReturn = FileRead( nInputFile, lpbSegment, uBufferSize, &nBytesRead )) != 0 ) {
			nReturn = error( nReturn, szInputName );
			goto filebye;
		}

		if ( fFlags & ASCII_SOURCE ) {

			// terminate the file copy at a ^Z
			if (( lpbPtr = (LPBYTE)_fmemchr( lpbSegment, 0x1A, nBytesRead )) != 0L ) {
				nBytesRead = (unsigned int)((ULONG_PTR)lpbPtr - (ULONG_PTR)lpbSegment);
				fEoF++;
			}
		}

		if ( nBytesRead == 0 )
			fEoF++;

		*lAppendOffset += nBytesRead;

		nBytesWritten = 0;


		if (( nReturn = FileWrite( nOutputFile, lpbSegment, nBytesRead, &nBytesWritten )) != 0 ) {
			nReturn = error( nReturn, szOutputName );
			goto filebye;
		}

		// if target is ASCII, add a ^Z at EOF and set "fEoF" to
		//   force a break after the write
		if (( fFlags & ASCII_TARGET ) && (( fEoF ) || (( nBytesRead < uBufferSize ) && (( fFlags & DEVICE_SOURCE ) == 0 )))) {
			
			if (( fFlags & DEVICE_TARGET ) == 0 ) {

				TCHAR chCtrlZ[2];

				chCtrlZ[0] = 26;
				wwrite( nOutputFile, chCtrlZ, 1 );
			}
			fEoF++;
		}

		if ( nBytesRead == 0 )
			break;

		if ( nBytesRead != nBytesWritten ) {	// error in writing

			// if writing to a device, don't expect full blocks
			if (( fFlags & DEVICE_TARGET ) == 0 ) {

				error( ERROR_4DOS_DISK_FULL, szOutputName );

				fDiskFull = 1;
				nReturn = CTRLC;
			}

			goto filebye;
		}

		ulTransferred += nBytesWritten;

		CheckForBreak();

	}

	// set the file date & time to the same as the source
	if (( fFlags & COPY_CREATE ) && (( fFlags & ( APPEND_FLAG | DEVICE_SOURCE | DEVICE_TARGET | PIPE_SOURCE | PIPE_TARGET )) == 0 )) {

		_dos_getftime( nInputFile, &nBytesRead, &nBytesWritten );
		_dos_setftime( nOutputFile, nBytesRead, nBytesWritten );

	}

filebye:
	FreeMem( lpbSegment );

	if ( nInputFile > 0 ) {

		if (( fFlags & ( DEVICE_SOURCE | PIPE_SOURCE )) == 0 && InstalledCheck( 0x1000 ) < 0 /* SHARE installed */ )
			_locking( nInputFile, _LK_UNLCK, 0xFFFFFFFFL );
		_close( nInputFile );
	}

	if ( nOutputFile > 0 ) {

		_close( nOutputFile );

		// if an error writing to the output file, delete it
		//   (unless we're appending
		if (( nReturn ) && ( fFlags & COPY_CREATE )) {
			SetFileMode( szOutputName, _A_NORMAL );
			remove( szOutputName );
		} else {
			// set the target file attributes
			SetFileMode( szOutputName, ( uAttribute | _A_ARCH ) );
		}
	}


FileCopyExit:
	// disable signal handling momentarily
	HoldSignals();
	
	return nReturn;
}



#pragma alloc_text( MISC_TEXT, wildname )


// expand a wildcard name
static void wildname( LPTSTR pszTarget, register LPTSTR pszSource, LPTSTR pszTemplate )
{
	register LPTSTR pszArg;
	TCHAR szString[MAXFILENAME+1];
	int nLength;

	char szShortName[MAXFILENAME+1];
	int nDestType = ifs_type( pszSource );

	
	// build a legal filename (right filename & extension size)
	insert_path( pszSource, fname_part( pszSource ), pszSource );

	// if source has path, copy it to target
	if (( pszArg = path_part( pszSource )) != NULL ) {
		strcpy( pszTarget, pszArg );
		pszSource += strlen( pszArg );
		pszTarget += strlen( pszArg );
	}

	// kludge for Win95 LFNs when copying to non-LFN-aware drive
	if ( nDestType == 0 ) {
		pszTemplate = strcpy( szShortName, pszTemplate );
		GetShortName( pszTemplate );
	}

	pszTemplate = fname_part( pszTemplate );

	while ( *pszSource ) {

		if ( *pszSource == _TEXT('*') ) {

			while (( *pszSource == _TEXT('*') ) || ( *pszSource == _TEXT('?') ))
				pszSource++;

			// we need to check for things like *abc
			sscanf( pszSource, _TEXT("%[^.*?]"), szString );
			nLength = strlen( szString );

			// insert substitute chars

			if ( fWin95LFN ) {
				// skip to extension
				while (( *pszTemplate ) && (( *pszTemplate != _TEXT('.') ) || ( *pszSource != _TEXT('.') )) && (( nLength == 0 ) || ( strnicmp( pszTemplate, szString, nLength ) != 0 )))
					*pszTarget++ = *pszTemplate++;
			} else {
				while (( *pszTemplate != _TEXT('.') ) && ( *pszTemplate ))
					*pszTarget++ = *pszTemplate++;
			}

		} else if ( *pszSource == _TEXT('?') ) {

			// single char substitution
			if (( *pszTemplate != _TEXT('.') ) && ( *pszTemplate ))
				*pszTarget++ = *pszTemplate++;
			pszSource++;

		} else if ( *pszSource == _TEXT('.') ) {

			while (( *pszTemplate ) && ( *pszTemplate++ != _TEXT('.') ))
				;
			*pszTarget++ = *pszSource++;		// copy '.'

		} else {

			*pszTarget++ = *pszSource++;
			if (( *pszTemplate != _TEXT('.') ) && ( *pszTemplate ))
				pszTemplate++;
		}
	}

	// strip trailing '.'
	if (( pszTarget[-1] == _TEXT('.') ) && ( pszTarget[-2] != _TEXT('.') ))
		pszTarget--;
	*pszTarget = _TEXT('\0');
}


// REN remains a near command dispatcher; implementation is in FILE_TEXT.
extern int _far FileRen_Cmd( LPTSTR );
// Near REN dispatcher; forward to the FILE_TEXT implementation.
int _near Ren_Cmd( LPTSTR pszCmdLine )
{
	return FileRen_Cmd( pszCmdLine );
}

// Far bridge to wildcard target-name expansion in wildname().
// Forward the target buffer, source pattern, and template filename.
void _far FileRenWildname( LPTSTR target, LPTSTR source, LPTSTR templateName )
{
	wildname( target, source, templateName );
}

// Far bridge to the near command parser.
// Forward the command line and execution options unchanged.
int _far FileRenCommand( LPTSTR pszLine, int fOptions )
{
	return command( pszLine, fOptions );
}


// DEL remains a near dispatcher; implementation is in FILE_TEXT.
extern int _far FileDel_Cmd( LPTSTR );
// Near DEL dispatcher; forward to the FILE_TEXT implementation.
int _near Del_Cmd( LPTSTR pszCmdLine )
{
	return FileDel_Cmd( pszCmdLine );
}


// ATTRIB command implementation resides in FILE_TEXT.
extern int _far FileAttrib_Cmd( LPTSTR );
// Near ATTRIB dispatcher; forward to the FILE_TEXT implementation.
int _near Attrib_Cmd( LPTSTR pszCmdLine )
{
	return FileAttrib_Cmd( pszCmdLine );
}


// Far bridge to show_atts() for formatting file attribute bits.
LPTSTR _far FileAttribShowBridge( int nAttribute )
{
	return show_atts( nAttribute );
}


#pragma alloc_text( SCREENIO_TEXT, show_atts )

LPTSTR _fastcall show_atts( register int nAttribute )
{

	static char szAttributes[6];
	strcpy( szAttributes, "_____" );

	if ( nAttribute & _A_RDONLY )
		szAttributes[0] = _TEXT('R');
	if ( nAttribute & _A_HIDDEN )
		szAttributes[1] = _TEXT('H');
	if ( nAttribute & _A_SYSTEM )
		szAttributes[2] = _TEXT('S');
	if ( nAttribute & _A_ARCH )
		szAttributes[3] = _TEXT('A');
	if ( nAttribute & _A_SUBDIR )
		szAttributes[4] = _TEXT('D');

	return szAttributes;
} 


// TOUCH remains a near command dispatcher; implementation is in FILE_TEXT.
extern int _far FileTouch_Cmd( LPTSTR );
// Near TOUCH dispatcher; forward to the FILE_TEXT implementation.
int _near Touch_Cmd( LPTSTR pszCmdLine )
{
	return FileTouch_Cmd( pszCmdLine );
}


// File display commands retain near dispatcher entry points.
extern int _far FileHead_Cmd( LPTSTR );
extern int _far FileTail_Cmd( LPTSTR );
extern int _far FileType_Cmd( LPTSTR );

// Near HEAD dispatcher; forward to the FILE_TEXT implementation.
int _near Head_Cmd( LPTSTR pszCmdLine )
{
	return FileHead_Cmd( pszCmdLine );
}

// Near TAIL dispatcher; forward to the FILE_TEXT implementation.
int _near Tail_Cmd( LPTSTR pszCmdLine )
{
	return FileTail_Cmd( pszCmdLine );
}

// Near TYPE dispatcher; forward to the FILE_TEXT implementation.
int _near Type_Cmd( LPTSTR pszCmdLine )
{
	return FileType_Cmd( pszCmdLine );
}

// Y [filename ...]
// Copy standard input to standard output, then TYPE any named files.
// Keep the near dispatch entry in its original segment.
extern int _far FileY_Cmd( LPTSTR );
// Near Y dispatcher; forward to the FILE_TEXT implementation.
int _near Y_Cmd( LPTSTR pszCmdLine )
{
	return FileY_Cmd( pszCmdLine );
}

// Far bridge to the near TYPE dispatcher for Y.
int _far FileYTypeBridge( LPTSTR pszCmdLine )
{
	return Type_Cmd( pszCmdLine );
}


// tee copies standard input to output & puts a copy in the specified file(s)
extern int _far FileTee_Cmd( LPTSTR );
// Near TEE dispatcher; forward to the FILE_TEXT implementation.
int _near Tee_Cmd( LPTSTR pszCmdLine )
{
	return FileTee_Cmd( pszCmdLine );
}
