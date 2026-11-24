

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


// DIRUTIL.C - System commands for 4DOS
//   Copyright (c) 1988 - 2002 Rex C. Conn  All rights reserved

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


// Directory command implementations compiled with /NTDIR_TEXT.

// MD [/S] [/N] directory ...
// Create one or more directories. /S creates missing parent directories.
// /N suppresses updates to the fuzzy directory index.
// Examples: MD TEMP; MD /S C:\WORK\SOURCE; MD /N TEMP.
int _far Md_Cmd( LPTSTR pszCmdLine )
{
	register int nReturn = 0;
	unsigned int nArg, fUpdateFuzzy;
	long fMD;
	LPTSTR pszDirectory;
	LPTSTR pszPath;

	// check for and remove switches; abort if no directory name arguments
	if (( GetSwitches( pszCmdLine, _TEXT("SN"), &fMD, 0 ) != 0 ) || ( first_arg( pszCmdLine ) == NULL ))
		return ( Usage( MD_USAGE ));
	fUpdateFuzzy = (( fMD & 2 ) == 0 );


	for ( nArg = 0; (( pszDirectory = ntharg( pszCmdLine, nArg | 0x800 )) != NULL ); nArg++) {

		mkfname( pszDirectory, 0 );

		// remove trailing '/' or '\'
		strip_trailing( pszDirectory+3, SLASHES );

		// create directory tree if /S specified
		if ( fMD & 1 ) {

			// skip UNC name
			if ( pszDirectory[0] == _TEXT('\\') ) {
				for ( pszPath = pszDirectory + 2; (( *pszPath != _TEXT('\0') ) && ( *pszPath++ != _TEXT('\\') )); )
					;
			} else
				pszPath = pszDirectory + 3;

			for ( _doserrno = 0; ( *pszPath != _TEXT('\0') ); pszPath++ ) {

				if (( *pszPath == _TEXT('\\') ) || ( *pszPath == _TEXT('/') )) {
					*pszPath = _TEXT('\0');

					if (( MakeDirectory( pszDirectory, fUpdateFuzzy ) == -1 ) && ( _doserrno != ERROR_ACCESS_DENIED ))

						break;
					*pszPath = _TEXT('\\');
				}
			}
		}

		if ( MakeDirectory( pszDirectory, fUpdateFuzzy ) == -1 )
			nReturn = error( _doserrno, pszDirectory );
	}

	return nReturn;
}

// RD directory ...
// Remove matching empty directories, honoring ranges and exclusions.
// Update the fuzzy index and remove descriptions from the parent directory.
// Examples: RD TEMP; RD OLD1 OLD2; RD TEMP*.
int _far Rd_Cmd( LPTSTR pszCmdLine )
{
	register int nFind;
	unsigned int nArg, nReturn;
	TCHAR szSource[MAXFILENAME+1], *pszDirectory;
	FILESEARCH dir;

	if ( GetRange( pszCmdLine, &(dir.aRanges), 0 ) != 0 )
		return ERROR_EXIT;

	if ( first_arg( pszCmdLine ) == NULL )
		return ( Usage( RD_USAGE ));

	dir.hdir = INVALID_HANDLE_VALUE;
	if (( setjmp( cv.env ) == -1 ) || ( cv.fException )) {
		FindClose( dir.hdir );
		return CTRLC;
	}

	for ( nReturn = nArg = 0; (( pszDirectory = ntharg( pszCmdLine, nArg )) != NULL ); nArg++ ) {

		// expand filename & remove any trailing backslash
		mkfname( pszDirectory, 0 );
		strip_trailing( pszDirectory+3, SLASHES );

		for ( nFind = FIND_FIRST; ; nFind = FIND_NEXT ) {

			if ( find_file( nFind, pszDirectory, ( _A_SUBDIR | _A_RDONLY | _A_HIDDEN | _A_SYSTEM | _A_ARCH | FIND_ONLY_DIRS | FIND_RANGE | FIND_EXCLUDE ), &dir, szSource ) == NULL )
				break;

			if ( DestroyDirectory( szSource ) != 0 )
				nReturn = error( _doserrno, szSource );

		}

		// check for directory not found!
		if ( nFind == FIND_FIRST )
			nReturn = ERROR_EXIT;

		// remove descriptions from parent directory
		process_descriptions( NULL, pszDirectory, DESCRIPTION_REMOVE );
	}

	return nReturn;
}

// CD [/N] [directory]
// Display the current directory (or that of a specified drive), or change it.
// /N disables fuzzy matching; successful changes save the previous directory.
// Examples: CD; CD C:; CD ..; CD /N TEMP; CD -.
int _far Cd_Cmd( LPTSTR pszCmdLine )
{
	register LPTSTR pszArg;
	int fFlags = CD_SAVELASTDIR;
	long fCD = 0;

	if (( pszCmdLine ) && ( *pszCmdLine )) {

		while (( pszArg = first_arg( pszCmdLine )) != NULL ) {
			if ( stricmp( pszArg, _TEXT("/N") ) == 0 ) {
				fCD |= 2;
				pszCmdLine = skipspace( pszCmdLine + 2 );
			} else
				break;
		}
	}

	// disable fuzzy searching
	if ( fCD & 2 )
		fFlags |= CD_NOFUZZY;

	// if no args or arg == disk spec, print current working directory
	if ( pszCmdLine )
		StripQuotes( pszCmdLine );

	if (( pszCmdLine == NULL ) || ( *pszCmdLine == _TEXT('\0') ) || (( _stricmp( pszCmdLine + 1, _TEXT(":") ) == 0 ) && (( fCD & 1 ) == 0 ))) {

		if (( pszArg = gcdir( pszCmdLine, 0 )) == NULL )
			return ERROR_EXIT;	// invalid drive

		printf( FMT_STR_CRLF, pszArg );
		return 0;
	}


	return ( __cd( pszCmdLine, fFlags ));
}

extern TCHAR gaPushdQueue[ DIRSTACKSIZE ];

// PUSHD [directory]
// Save the current drive and directory on the directory stack.
// If a destination is supplied, change to it before saving the old path.
// Examples: PUSHD; PUSHD D:\WORK.
int _far Pushd_Cmd( LPTSTR pszCmdLine )
{
	LPTSTR pszCurrentDirectory;
	TCHAR szDirectory[MAXPATH];

	if (( pszCurrentDirectory = gcdir( NULL, 0 )) == NULL )	// get current directory
		return ERROR_EXIT;

	strcpy( szDirectory, pszCurrentDirectory );

	// change disk and/or directory
	//   (if no arguments, just save the current directory)
	if (( pszCmdLine ) && ( *pszCmdLine ) && ( __cd( pszCmdLine, CD_CHANGEDRIVE | CD_SAVELASTDIR ) != 0 ))
		return ERROR_EXIT;

	// save the previous disk & directory
	SaveDirectory( gaPushdQueue, szDirectory );

	return 0;
}

// POPD [*]
// Restore the most recently saved drive and directory, removing its entry.
// Remove the entry even if the directory change fails; * clears the stack.
// Examples: POPD; POPD *.
int _far Popd_Cmd( LPTSTR pszCmdLine )
{
	LPTSTR pszDirectory;
	register int nRet;

	if (( pszCmdLine ) && ( *pszCmdLine )) {

		if ( *pszCmdLine == _TEXT('*') ) {
			// clear directory stack
			gaPushdQueue[0] = _TEXT('\0');
			gaPushdQueue[1] = _TEXT('\0');
			return 0;
		}

		return ( Usage( POPD_USAGE ));
	}

	if ( gaPushdQueue[0] == _TEXT('\0') )
		return ( error( ERROR_4DOS_DIR_STACK_EMPTY, NULL ));

	// get last (newest) entry in queue
	for ( pszDirectory = (char *)(long)( end_of_env( gaPushdQueue ) - 1 ); (( pszDirectory > gaPushdQueue ) && ( pszDirectory[-1] != _TEXT('\0') )); pszDirectory-- )
		;

	// change drive and directory
	nRet = __cd( pszDirectory, CD_CHANGEDRIVE | CD_SAVELASTDIR );

	// remove most recent entry from DIRSTACK
	pszDirectory[0] = _TEXT('\0');
	pszDirectory[1] = _TEXT('\0');

	return nRet;
}

// DIRS
// Display the directory stack from oldest to newest without modifying it.
// Report an error if the stack is empty. Example: DIRS.
int _far Dirs_Cmd( LPTSTR pszCmdLine )
{
	TCHAR _far *pszDirectory;

	if ( gaPushdQueue[0] == _TEXT('\0') )
		return (error( ERROR_4DOS_DIR_STACK_EMPTY, NULL ));

	for ( pszDirectory = gaPushdQueue; ( *pszDirectory != _TEXT('\0') ); pszDirectory = next_env( pszDirectory ))
		printf( FMT_FAR_STR_CRLF, pszDirectory );

	return 0;
}


extern void UpdateFuzzyTree( LPTSTR, LPTSTR, int );
extern int _far DirExecuteCommand( LPTSTR, int );

// CDD [/N] directory | /A | /S | /U | /D
// Change drive and directory, saving the previous location; /N disables fuzzy matching.
// /A lists current directories on ready drives C: through Z:.
// /S builds the fuzzy index; /U updates and /D removes selected trees.
// Examples: CDD D:\WORK; CDD -; CDD /A; CDD /S C:; CDD /U C:.
int _far Cdd_Cmd( LPTSTR pszCmdLine )
{
	register LPTSTR pszPtr, pszArg;
	TCHAR szDrives[128], szTreePath[MAXFILENAME], szTemp[MAXFILENAME];
	int i, n, nReturn = 0, fFlags = CD_CHANGEDRIVE | CD_SAVELASTDIR, fSave, fUpdate;

	if (( pszCmdLine ) && ( *pszCmdLine )) {

		if ( _strnicmp( pszCmdLine, _TEXT("/U"), 2 ) == 0 ) {
			// update tree(s)
			fUpdate = 1;
			pszCmdLine[1] = _TEXT('S');
		} else if ( _strnicmp( pszCmdLine, _TEXT("/D"), 2 ) == 0 ) {
			// delete tree(s)
			fUpdate = 2;
			pszCmdLine[1] = _TEXT('S');
		} else
			fUpdate = 0;

		// disable fuzzy searching
		if ( _strnicmp( pszCmdLine, _TEXT("/N "), 3 ) == 0 ) {
			fFlags |= CD_NOFUZZY;
			strcpy( pszCmdLine, skipspace( pszCmdLine + 3 ));

			// if /A, display current directory on all drives C: - Z:
		} else if ( _stricmp( pszCmdLine, _TEXT("/A") ) == 0 ) {

			for ( n = 3; ( n <= 26 ); n++ ) {

				if ( QueryDriveReady( n )) {
					sprintf( szDrives, _TEXT("%c:"), n + 64 );
					if (( pszPtr = gcdir( szDrives, 1 )) != NULL )
						printf( FMT_STR_CRLF, pszPtr );
				}
			}

			return 0;

		} else if ( _strnicmp( pszCmdLine, _TEXT("/S"), 2 ) == 0 ) {

			LPTSTR pszSave;

			// get tree index location
			if ( gpIniptr->TreePath != INI_EMPTYSTR )
				pszPtr = (LPTSTR)( gpIniptr->StrData + gpIniptr->TreePath );

			else
				pszPtr = _TEXT("C:\\");
			strcpy( szTreePath, pszPtr );
			mkdirname( szTreePath, _TEXT("jpstree.idx") );
			AddQuotes( szTreePath );

			// scan drive for wildcard searches
			pszSave = strcpy( (LPTSTR)_alloca( ( strlen( pszCmdLine ) + 1 ) * sizeof(TCHAR) ), pszCmdLine );

			pszPtr = skipspace( pszSave + 2 );
			if ( *pszPtr == _TEXT('\0') ) {
				// default to all hard disks
				SetDriveString( szDrives );
FormatDriveNames:
				for ( pszPtr = szDrives; ( *pszPtr != _TEXT('\0') ); pszPtr += 4 )
					strins( pszPtr+1, _TEXT(":\\ ") );
				pszPtr = szDrives;

			} else {
				pszPtr = skipspace( pszPtr );
				if ( strpbrk( pszPtr, _TEXT(":\\. \t") ) == NULL ) {
					strcpy( szDrives, pszPtr );
					goto FormatDriveNames;
				}
				pszPtr = strcpy( (LPTSTR)_alloca( ( strlen( pszPtr ) + 1 ) * sizeof(TCHAR) ), pszPtr );
			}

			for ( i = 0; (( pszArg = ntharg( pszPtr, i )) != NULL ); i++ ) {

				if ( fUpdate ) {
					// remove existing tree from JPSTREE.IDX
					strcpy( szTemp, pszArg );
					UpdateFuzzyTree( szTemp, NULL, 1 );
					if ( fUpdate == 2 )
						continue;
				}

				// use TREE to build the directory list
				printf( BUILDING_INDEX, pszArg );
				if ( fUpdate ) {
					// build a temporary file
					pszArg = strcpy( (LPTSTR)_alloca( ( strlen( pszArg ) + 1 ) * sizeof(TCHAR) ), pszArg );
					szTemp[0] = _TEXT('\0');

					GetTempDirectory( szTemp );

					UniqueFileName( szTemp );
					AddQuotes( szTemp );
				}

				sprintf( pszCmdLine, _TEXT("*@tree /c%s %s %s %s"), (( gpIniptr->CompleteHidden ) ? _TEXT("/h") : NULLSTR ), pszArg, (( i == 0 ) ? _TEXT(">!") : _TEXT(">>!") ), (( fUpdate ) ? szTemp : szTreePath ));

				// make sure TREE isn't disabled!
				n = findcmd( _TEXT("TREE"), 1 );
				fSave = commands[ n ].fParse;
				commands[ n ].fParse &= ~CMD_DISABLED;

				nReturn = DirExecuteCommand( pszCmdLine, 0 );

				commands[ n ].fParse = fSave;

				// check for ^C or stack overflow
				if (( setjmp( cv.env ) == -1 ) || ( nReturn == CTRLC ) || ( cv.fException ))
					return CTRLC;

				if ( fUpdate ) {
					// insert temp file into JPSTREE.IDX
					UpdateFuzzyTree( pszArg, szTemp, 0 );
					remove( szTemp );
				}
			}

			return 0;
		}
	}

	return ((( pszCmdLine == NULL ) || ( *pszCmdLine == _TEXT('\0') )) ? Usage( CDD_USAGE ) : __cd( pszCmdLine, fFlags ));
}


// Append pszDirectory to the double-NUL-terminated lpszDirectoryStack.
// Use DirHistorySize for directory history, DIRSTACKSIZE for the PUSHD stack.
// Remove duplicate history entries and discard oldest entries to make room.
void _fastcall SaveDirectory( TCHAR _far *lpszDirectoryStack, LPTSTR pszDirectory )
{
	register int nLength;
	int uSize;
	TCHAR _far *pszNewEntry, _far *pszDup;

	if ( lpszDirectoryStack == glpDirHistory )
		uSize = gpIniptr->DirHistorySize;
	else
		uSize = DIRSTACKSIZE;

	nLength = ( strlen( pszDirectory ) + 1 );

	// if CD / CDD history, remove duplicates
	if (( pszDup = lpszDirectoryStack ) == glpDirHistory ) {

		TCHAR _far *lpNext;

		while ( *pszDup != _TEXT('\0') ) {

			lpNext = next_env( pszDup );
			if ( _fstricmp( pszDup, pszDirectory ) == 0 ) {
				// remove duplicate
				_fmemmove( pszDup, lpNext, (int)(( end_of_env( pszDup ) - lpNext ) + 1 ) * sizeof(TCHAR) );
			} else
				pszDup = lpNext;
		}
	}

	// if directory queue overflow, remove the oldest entry
	while (((( pszNewEntry = end_of_env( lpszDirectoryStack )) - lpszDirectoryStack ) + nLength ) >= ( uSize - 2 ))
		_fmemmove( lpszDirectoryStack, next_env( lpszDirectoryStack ), (int)(( pszNewEntry - next_env( lpszDirectoryStack )) + 1 ) * sizeof(TCHAR) );

	_fstrcpy( pszNewEntry, pszDirectory );
	pszNewEntry[ nLength ] = _TEXT('\0');
}


// Create pszDirectory using _mkdir; return -1 on failure, 0 on success.
// If fUpdateFuzzy is set, replace any existing entry in JPSTREE.IDX.
int _fastcall MakeDirectory( LPTSTR pszDirectory, int fUpdateFuzzy )
{
	if ( _mkdir( pszDirectory ) == -1 )
		return -1;

	if ( fUpdateFuzzy ) {
		// delete the subdirectory from JPSTREE.IDX (if it exists), then add it again
		UpdateFuzzyTree( pszDirectory, NULL, 1 );
		UpdateFuzzyTree( pszDirectory, NULL, 0 );
	}

	return 0;
}


// Remove pszDirectory using _rmdir; return -1 on failure, 0 on success.
// Remove its fuzzy index entry only after the directory is removed.
int _fastcall DestroyDirectory( LPTSTR pszDirectory )
{
	if ( _rmdir( pszDirectory ) == -1 )
		return -1;

	UpdateFuzzyTree( pszDirectory, NULL, 1 );

	return 0;
}
