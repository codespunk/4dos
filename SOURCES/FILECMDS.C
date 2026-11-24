

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

int fDiskFull;
int fClip;
RANGES aRanges;				// date/time/size ranges


// display the number of files copied/moved/renamed/deleted, with special
//   kludge for "1 file" vs. "2 files"
void _fastcall FilesProcessed( LPTSTR szFormat, long lFiles )
{
	printf( szFormat, lFiles, (( lFiles == 1L ) ? ONE_FILE : MANY_FILES ));
}


// COPY remains a near dispatcher; implementation resides in FILE_TEXT.
extern int _far FileCopy_Cmd( LPTSTR );
// Near COPY dispatcher; forward to the FILE_TEXT implementation.
int _near Copy_Cmd( LPTSTR pszCmdLine )
{
	return FileCopy_Cmd( pszCmdLine );
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
