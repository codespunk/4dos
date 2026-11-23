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


// TEE [/A] filename [filename ...]
// Copy standard input to standard output and one or more files.
// Examples: DIR | TEE LIST.TXT; TEE /A LOG.TXT.
int _far FileTee_Cmd( LPTSTR pszCmdLine )
{
	register LPTSTR pszArg;
	register int i;
	int nBytesRead, fEof, nMode = 0, nReturn = 0, nFiles = 1, naFH[20], fTeeClip = -1;
	TCHAR *pszTemp, szClip[MAXFILENAME], szTeeBuffer[CMDBUFSIZ+2];
	long fTee;

	// check for "append" switch
	if (( GetSwitches( pszCmdLine, _TEXT("A"), &fTee, 0 ) != 0 ) || ( first_arg( pszCmdLine ) == NULL ))
		return ( Usage( TEE_USAGE ));

	nMode = (( fTee == 0 ) ? (_O_RDWR | _O_CREAT | _O_BINARY | _O_TRUNC) : (_O_RDWR | _O_CREAT | _O_BINARY | _O_APPEND));

	naFH[0] = STDOUT;
	szClip[0] = _TEXT('\0');

	if ( setjmp( cv.env ) == -1 )
		nReturn = CTRLC;

	else {

		for ( i = 0; (( pszArg = ntharg( pszCmdLine, i )) != NULL ) && ( nFiles < 20 ); i++, nFiles++ ) {

			mkfname( pszArg, 0 );

			// TEE'ing to CLIP: ?
			if ( stricmp( pszArg, CLIP ) == 0 ) {
				RedirToClip( szClip, 99 );
				pszArg = szClip;
			}

// FIXME - add FTP support!

			if (( naFH[nFiles] = _sopen( pszArg, nMode, _SH_DENYWR, ( _S_IREAD | _S_IWRITE ))) < 0 ) {
				nReturn = error( _doserrno, pszArg );
				goto tee_bye;
			}

			// save file handle for CLIP:
			if ( pszArg == szClip )
				fTeeClip = naFH[nFiles];
		}

		for ( fEof = 0; (( fEof == 0 ) && ( getline( STDIN, szTeeBuffer, CMDBUFSIZ, EDIT_DATA ) > 0 )); ) {

			// look for ^Z
			if (( pszTemp = strchr( szTeeBuffer, EoF )) != NULL ) {
				if ( pszTemp == szTeeBuffer )
					break;
				*pszTemp = _TEXT('\0');
				fEof = 1;
			} else
				strcat( szTeeBuffer, gszCRLF );
			nBytesRead = strlen( szTeeBuffer );

			for ( i = 0; ( i < nFiles ); i++ )
				wwrite( naFH[i], szTeeBuffer, nBytesRead );
		}
	}

tee_bye:
	// close the output files (this looks a bit peculiar because we have
	//   to allow for a ^C during the close(), & still close everything
	for ( ; ( nFiles > 1 ); nFiles-- ) {

		// is one of the TEE arguments CLIP: ?
		if ( fTeeClip == naFH[nFiles-1] )
			CopyToClipboard( fTeeClip );

		_close( naFH[nFiles-1] );
	}

	if ( szClip[0] )
		remove( szClip );


	return nReturn;
}


// DESCRIBE filename [description]
// Read or modify descriptions attached to matching files.
// Example: DESCRIBE REPORT.TXT "Quarterly report".
int _far FileDescribe_Cmd( LPTSTR pszCmdLine )
{
	register int nFind;
	register LPTSTR pszArg;
	int fHaveDescription, rc, nReturn = 0, nArg;
	unsigned int uMaxLength;
	TCHAR szSource[MAXFILENAME], szFileName[MAXFILENAME], szNewDescription[514];
	long fDescribe;
	FILESEARCH dir;

	// get file date/time ranges
	if ( GetRange( pszCmdLine, &(dir.aRanges), 0 ) != 0 )
		return ERROR_EXIT;

	// check for and remove /A:-rhsda switch
	if (( GetSwitches( pszCmdLine, _TEXT("*"), &fDescribe, 1 ) != 0 ) || ( pszCmdLine == NULL ) || ( *pszCmdLine == _TEXT('\0') ))
		return ( Usage( DESCRIBE_USAGE ));

	dir.hdir = INVALID_HANDLE_VALUE;
	if (( setjmp( cv.env ) == -1 ) || ( cv.fException )) {
		FindClose( dir.hdir );
		return CTRLC;
	}

	for ( nArg = 0; (( pszArg = ntharg( pszCmdLine, nArg )) != NULL ); nArg++ ) {

		copy_filename( szSource, pszArg );

		// get file description
		szNewDescription[0] = _TEXT('\0');
		fHaveDescription = 0;
		for ( rc = nArg; (( pszArg = ntharg( pszCmdLine, rc )) != NULL ); rc++ ) {

			if ( rc > 0 ) {

				// is this argument predefined as a description?
				if (( strnicmp( pszArg, _TEXT("/D"), 2 ) == 0 ) && ( pszArg[2] == DOUBLE_QUOTE )) {
					pszArg += 2;
					goto HaveDescription;
				}

				// make sure it's not an NTFS/LFN filename
				if (( *pszArg == DOUBLE_QUOTE ) && (( ifs_type( pszArg ) == 0 ) || (( is_file( pszArg ) == 0 ) && ( is_dir( pszArg ) == 0 )))) {
HaveDescription:
					fHaveDescription = sscanf( pszArg + 1, _TEXT("%511[^\"]"), szNewDescription );
					break;
				}
			}
		}

		// if the current argument is a description, skip it
		if ( rc == nArg )
			continue;

		for ( nFind = FIND_FIRST; ( find_file( nFind, szSource, (0x17L | FIND_BYATTS | FIND_RANGE | FIND_EXCLUDE | FIND_NO_DOTNAMES ), &dir, szFileName ) != NULL ); nFind = FIND_NEXT ) {

			mkfname( szFileName, 0 );

			// display current description & get new one
			if ( fHaveDescription == 0 ) {

				szNewDescription[0] = _TEXT('\0');
				process_descriptions( szFileName, szNewDescription, DESCRIPTION_READ | DESCRIPTION_PROCESS );
				if ( _isatty(STDIN))
					printf( DESCRIBE_PROMPT, szFileName );

				// if input is redirected, exit if EoF
				if (( uMaxLength = strlen( szNewDescription )) < gpIniptr->DescriptMax )
					uMaxLength = gpIniptr->DescriptMax;
				else if ( uMaxLength > 511 )
					uMaxLength = 511;

				if ( getline( STDIN, szNewDescription, uMaxLength, EDIT_ECHO | EDIT_SCROLL_CONSOLE ) == 0 ) {
					if ( _isatty( STDIN ) == 0 )
						return nReturn;
				}
			}

			// write the new description
			if (( rc = process_descriptions( szFileName, szNewDescription, ( DESCRIPTION_READ | DESCRIPTION_WRITE | DESCRIPTION_CREATE | DESCRIPTION_REMOVE | DESCRIPTION_PROCESS ))) != 0 )
				nReturn = error( rc, DESCRIPTION_FILE );
		}
	}

	return nReturn;
}
