// FILEUTIL.C - File command implementations in FILE_TEXT.
// Compiled with /NTFILE_TEXT; dispatch entry points remain near.

#include "product.h"
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <direct.h>
#include <dos.h>
#ifndef __WATCOMC__
#include <msdos.h>
#endif
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <io.h>
#include <malloc.h>
#include <share.h>
#include <string.h>
#include "4all.h"

// Shared date/time/size range filters are defined in FILECMDS.C.
extern RANGES aRanges;

// Fuzzy directory tree maintenance is implemented in SYSCMDS.C.
extern void UpdateFuzzyTree( LPTSTR, LPTSTR, int );


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


// Y [filename ...]
// Copy standard input to standard output, then TYPE any named files.
// The original near TYPE command is reached through a far bridge.
extern int _far FileYTypeBridge( LPTSTR );

int _far FileY_Cmd( LPTSTR pszCmdLine )
{
#ifdef __WATCOMC__
    extern unsigned *__io_mode;
#else
    extern char _osfile[];
#endif
    int nBytesRead, fEof;
    TCHAR *pszTemp, szYBuf[514];

    for ( fEof = 0; (( fEof == 0 ) && ( getline( STDIN, szYBuf, 512, EDIT_DATA ) > 0 )); ) {
        // Look for ^Z.
        if (( pszTemp = strchr( szYBuf, EoF )) != NULL ) {
            if ( pszTemp == szYBuf )
                break;
            *pszTemp = _TEXT('\0');
            fEof = 1;
        } else
            strcat( szYBuf, gszCRLF );

        nBytesRead = strlen( szYBuf );
        wwrite( STDOUT, szYBuf, nBytesRead );
    }

    // Clear the EOF flag.
#ifdef __WATCOMC__
    __io_mode[STDIN] &= ~( _EOF | _BINARY );
#else
    _osfile[STDIN] &= ~( FEOFLAG | FCRLF );
#endif

    return ((( pszCmdLine ) && ( *pszCmdLine )) ? FileYTypeBridge( pszCmdLine ) : 0 );
}


static int _fastcall FileOutput( LPTSTR, int );

#define FO_HEAD 1
#define FO_TAIL 2
#define FO_TYPE 4
#define FO_VERBOSE 8
#define FO_LINES 0x10
#define FO_NUMBERING 0x20
#define FO_PAUSE 0x40
#define FO_PIPE 0x80
#define FO_FOLLOW 0x100

// HEAD [options] [filename ...]
// Display the beginning of files or redirected standard input.
// Example: HEAD REPORT.TXT.
int _far FileHead_Cmd( LPTSTR pszCmdLine )
{
	int nReturn = 0;

	if (( nReturn = FileOutput( pszCmdLine, FO_HEAD )) == USAGE_ERR )
		return ( Usage( HEAD_USAGE ));

	return nReturn;
}


// TAIL [options] [filename ...]
// Display the end of files or redirected standard input.
// Example: TAIL LOG.TXT.
int _far FileTail_Cmd( LPTSTR pszCmdLine )
{
	register int nReturn = 0;

	if (( nReturn = FileOutput( pszCmdLine, FO_TAIL )) == USAGE_ERR )
		return ( Usage( TAIL_USAGE ));

	return nReturn;
}


#define TYPE_NUMBER 2
#define TYPE_PAUSE 4
#define TYPE_FNAME 8

// TYPE [options] [filename ...]
// Display file contents, optionally numbering lines or pausing by page.
// Example: TYPE /P README.TXT.
int _far FileType_Cmd( LPTSTR pszCmdLine )
{
	int nReturn = 0;

	if (( nReturn = FileOutput( pszCmdLine, FO_TYPE )) == USAGE_ERR )
		return ( Usage( TYPE_USAGE ));

	return nReturn;
}


// Shared HEAD, TAIL, and TYPE output worker.
// fOp selects the display operation; returns USAGE_ERR for invalid syntax.
// Handles file matching, redirected input, and display options.
static int _fastcall FileOutput( LPTSTR pszCmdLine, int fOp )
{
	register TCHAR *pszArg, *pszLine;
	int nArgc, nFind, nReturn = 0, fEOF, nBytes = 0, nEditFlags = EDIT_DATA, fFlags = 0, nColumn = 0, nFH = -1, fNoLF = 0;
	long lRow, lOptions, lDisplayLines = 10, lLines;
	unsigned long ulChars = (ULONG)-1L, ulAttributes = 0L;
	TCHAR chBKM;
	TCHAR szClip[MAXFILENAME], szSource[MAXFILENAME];
	FILESEARCH dir;

	LONG llEnd;

	if ( pszCmdLine == NULL )
		pszCmdLine = NULLSTR;

	gnInclusiveMode = gnExclusiveMode = gnOrMode = 0;

	// get date/time/size ranges
	if ( GetRange( pszCmdLine, &(dir.aRanges), 0 ) != 0 )
		return ERROR_EXIT;

	init_page_size();		// clear row & page length vars

	// TYPE command?
	if ( fOp & FO_TYPE ) {

		// check for and remove switches
		if ( GetSwitches( pszCmdLine, _TEXT("*LPV"), &lOptions, 0 ) != 0 )
			return USAGE_ERR;

		if ( lOptions & 1 )
			ulAttributes = 0x07;

		// print the line number?
		if ( lOptions & TYPE_NUMBER ) {
			fFlags |= FO_NUMBERING;
			nColumn = 7;
		}

		// pause after each page?
		if ( lOptions & TYPE_PAUSE ) {
			fFlags |= FO_PAUSE;
			gnPageLength = GetScrRows();
		}

		// header?
		if ( lOptions & TYPE_FNAME )
			fFlags |= FO_VERBOSE;
	}

	// save arguments on stack so we can overwrite with input buffer
	pszLine = strcpy( (LPTSTR)_alloca( ( strlen( pszCmdLine ) + 1 ) * sizeof(TCHAR) ), pszCmdLine );
	szSource[0] = _TEXT('\0');

	for ( nArgc = 0; ; nArgc++ ) {

		// check for input redirection - "head < file"
		if (( pszArg = ntharg( pszLine, nArgc )) == NULL ) {
			if ( szSource[0] )
				break;

			if ( QueryIsConsole( STDIN ) )
				return USAGE_ERR;

			pszArg = CONSOLE;
			nFH = STDIN;
		}

		fFlags &= ~FO_PIPE;

		if (( fOp & ( FO_HEAD | FO_TAIL ))  && ( *pszArg == gpIniptr->SwChr )) {

			// check for /A:xxx switch
			if (( _ctoupper( pszArg[1] ) == _TEXT('A') ) && ( pszArg[2] == _TEXT(':') )) {
				// set gnInclusiveMode and gnExclusiveMode
				GetSearchAttributes( pszArg + 2 );
				ulAttributes = 0x07;
				continue;
			}

			// number of characters to display
			if ( QueryIsSwitch( pszArg, _TEXT('C'), 0 )) {

				chBKM = _TEXT('\0');
				ulChars = (ULONG)-1L;
				pszArg += 2;
				if ( *pszArg == _TEXT(':'))
					pszArg++;
				sscanf( pszArg, _TEXT("%lu%c"), &ulChars, &chBKM );
				switch ( chBKM ) {
				case _TEXT('B'):
					ulChars *= 512;
					break;
				case _TEXT('k'):
					ulChars *= 1000;
					break;
				case _TEXT('K'):
					ulChars *= 1024;
					break;
				case _TEXT('m'):
					ulChars *= 1000000;
					break;
				case _TEXT('M'):
					ulChars *= 1048576;
				}
				continue;
			}

			// follow file update?
			if (( QueryIsSwitch( pszArg, _TEXT('F'), 0 )) && ( fOp & FO_TAIL )) {
				fFlags |= FO_FOLLOW;
				continue;
			}

			// number of lines to display
			if ( QueryIsSwitch( pszArg, _TEXT('N'), 0 )) {
				if ( isdigit( pszArg[2] ))
					pszArg += 2;
				else if (( pszArg[2] == _TEXT('\0')) && (( pszArg = ntharg( pszLine, nArgc+1 )) != NULL ) && ( isdigit ( *pszArg )))
					nArgc++;
				if ( pszArg )
					lDisplayLines = atoi( pszArg );
				continue;
			}

			// pause after each page?
			if ( QueryIsSwitch( pszArg, _TEXT('P' ), 0 )) {
				fFlags |= FO_PAUSE;
				gnPageLength = GetScrRows();
				continue;
			}

			// no header?
			if ( QueryIsSwitch( pszArg, _TEXT('Q' ), 0 )) {
				fFlags &= ~FO_VERBOSE;
				continue;
			}

			// header?
			if ( QueryIsSwitch( pszArg, _TEXT('V' ), 0 )) {
				fFlags |= FO_VERBOSE;
				continue;
			}

			return USAGE_ERR;
		}

		copy_filename( szSource, pszArg );
		mkfname( szSource, 0 );

		// display specified files (including hidden & read-only)
		for ( nFind = FIND_FIRST; ; nFind = FIND_NEXT ) {

			lLines = lDisplayLines;

			szClip[0] = _TEXT('\0');
			if ( QueryIsDevice( szSource )) {

				if ( nFind == FIND_NEXT )
					break;
				nFind = FIND_NEXT;		// only allow this once!

				if ( stricmp( szSource, CLIP ) == 0 ) {

					RedirToClip( szClip, 99 );
					if ( CopyFromClipboard( szClip ) != 0 )
						break;
					strcpy( gszCmdline, szClip );

				} else {

					// kludge for knuckleheads who try to type character devices!
					LPTSTR pName;

					pName = fname_part( szSource );

					// nothing to do on a TYPE NUL
					if (( pName != NULL ) && ( stricmp( pName, _TEXT("NUL")) == 0 ))
						break;

					if (( pName == NULL ) || ( stricmp( pName, CONSOLE ) != 0 )) {
						nReturn = error( ERROR_ACCESS_DENIED, szSource );
						break;
					}
				}

			} else if ( QueryIsPipeName( szSource )) {

				// only look for named pipes once
				if ( nFind == FIND_NEXT )
					break;
				strcpy( gszCmdline, szSource );
				ulChars = (ULONG)-1L;
				fFlags |= FO_PIPE;

				// kludge for CMD.EXE compatibility -- if no wildcards, match hidden names; otherwise, require /A:xx
			} else if ( find_file( nFind, szSource, ( ulAttributes | FIND_BYATTS | FIND_RANGE | FIND_EXCLUDE | (( strpbrk( szSource, WILD_CHARS ) == NULL ) ? 0x07 : 0 )), &(dir), gszCmdline ) == NULL ) {
				if ( nFind == FIND_FIRST )
					nReturn = ERROR_EXIT;
				break;
			}

			// print header for HEAD & TAIL
			if ( fFlags & FO_VERBOSE ) {
				if ( fNoLF ) {
					fNoLF = 0;
					qputs( _TEXT("\r\n") );
				}
				qputs( _TEXT("---- ") );
				more_page( gszCmdline, 5 );
			}

#
				if ( nFH != STDIN ) {

					if (( nFH = _sopen( gszCmdline, _O_BINARY | _O_RDONLY, _SH_DENYNO )) < 0 ) {
						nReturn = error( _doserrno, gszCmdline );
						if ( szClip[0] )
							break;
						continue;
					}
				}

				if ( setjmp( cv.env ) == -1 ) {

					if ( nFH != STDIN ) {
						_close( nFH );
						nFH = -1;
					}
					nReturn = CTRLC;
					break;
				}


				if (( fOp & FO_TAIL ) && ( ulChars != (ULONG)-1L )) {

					// first, get file size

						// back up "ulChars" (characters, not bytes!) from the end
						llEnd = _lseek( nFH, 0L, SEEK_END );

							_lseek( nFH, (( ulChars > (ULONG)llEnd ) ? -llEnd : -(LONG)ulChars ), SEEK_END );

					// now we'll fall through to the TYPE read routine & read the last "ulChars" of the file
				}

				// output the file to stdout, line-by-line
				if (( fOp & ( FO_HEAD | FO_TAIL )) && ( ulChars == (ULONG)-1L )) {

					if ( fOp & FO_TAIL ) {

						// TAIL - start at the end and count back "lLines"
						// seek to end, save size, then back up & start counting LF's

							llEnd = _lseek( nFH, 0L, SEEK_END );

						for ( lRow = lLines; ( lRow > 0 ); ) {

								if ( llEnd <= 0 ) {
									_lseek( nFH, 0L, SEEK_SET );
									break;
								}

								if (( llEnd -= CMDBUFSIZ ) < 0 )
									llEnd = 0;

								_lseek( nFH, llEnd, SEEK_SET );

								nBytes = _read( nFH, gszCmdline, CMDBUFSIZ );

								for ( --nBytes; ( --nBytes >= 0 ); ) {

									if (( gszCmdline[ nBytes ] == LF ) && ( --lRow <= 0 )) {
										nBytes++;

										_lseek( nFH, ( llEnd + nBytes ), SEEK_SET );
										break;
									}
								}

						}
					}


						for ( ; (( lLines > 0 ) && ( getline( nFH, gszCmdline, CMDBUFSIZ-1, nEditFlags ) > 0 )); lLines-- )
							more_page( gszCmdline, 0 );

					// close the file / pipe
					goto CloseHandles;

				} else if ( fFlags & ( FO_NUMBERING | FO_PAUSE ) || _isatty( STDOUT )) {

					// TYPE /L or /P, or neither flag if not redirected
					for ( lRow = 1; ( getline( nFH, gszCmdline, CMDBUFSIZ-1, nEditFlags ) > 0 ); lRow++ ) {
						if ( fFlags & FO_NUMBERING )
							printf( _TEXT("%4lu : "), lRow );
						more_page( gszCmdline, nColumn );
					}

					goto CloseHandles;
				}

			for ( fEOF = 0; ( fEOF == 0 ); ) {

				nBytes = 0;

				if (( nBytes = _read( nFH, gszCmdline, (unsigned int)(( ulChars < CMDBUFSIZ ) ? ulChars : CMDBUFSIZ ))) <= 0 )
					break;

				// make sure we're not at the end of the file ("type file >> file")
				fEOF = _eof( nFH );

				if ( nBytes ) // with /C option, file may end without \n
					fNoLF = gszCmdline[nBytes-1] != _TEXT('\n');

				if ( wwrite( STDOUT, gszCmdline, nBytes ) == -1 ) {

						error( _doserrno, szSource );
					break;
				}

				// adjust the number of chars left to read
				if ( ulChars != (ULONG)-1L ) {
					// if max number of chars, stop
					if ( ulChars < (ULONG)CMDBUFSIZ )
						break;
					ulChars -= CMDBUFSIZ;
				}
			}

CloseHandles:

			{
				if ( nFH == STDIN )
					break;

				_close( nFH );
				nFH = -1;

			}

			// delete temporary CLIP: file
			if ( szClip[0] ) {
				remove( szSource );
				break;
			}
		}
	}

	return nReturn;
}

// TOUCH implementation and recursive helper in FILE_TEXT.
typedef struct
{
	TCHAR szSource[MAXFILENAME];
	TCHAR szTarget[MAXFILENAME];
	LPTSTR pszFilename;
	long fFlags;
	long lMode;
	int nField;
} TOUCH_STRUCT;

#define TOUCH_CREATE 1
#define TOUCH_FORCE 2
#define TOUCH_QUIET 4
#define TOUCH_DUP 8
#define TOUCH_SUBDIRS 0x10
#define TOUCH_NOTHING 0x20
#define TOUCH_PRESERVE_DATE 0x40
#define TOUCH_PRESERVE_TIME 0x80

static int _touch( TOUCH_STRUCT *, DATETIME * );


// TOUCH [options] filename [filename ...]
// Change file timestamps; optionally create files or duplicate timestamps.
// Example: TOUCH /C LOG.TXT.
int _far FileTouch_Cmd( LPTSTR pszCmdLine )
{
	register LPTSTR pszArg, pszTemp;
	int nArgc, nReturn = 0, rc;
	unsigned int uHours, uMinutes, uSeconds;
	unsigned int uMonth, uDay, uYear;
	FILESEARCH dir;
	DATETIME sysDateTime;
	TOUCH_STRUCT Touch;

	if (( pszCmdLine == NULL ) || ( *pszCmdLine == _TEXT('\0') ))
		return ( Usage( TOUCH_USAGE ));

	Touch.nField = 0;
	Touch.fFlags = 0;
	Touch.lMode = 0x07;
	Touch.szSource[0] = _TEXT('\0');

	QueryDateTime( &sysDateTime );

	// get date/time/size ranges
	if ( GetRange( pszCmdLine, &aRanges, 0 ) != 0 )
		return ERROR_EXIT;

	gnInclusiveMode = gnExclusiveMode = gnOrMode = 0;

	for ( nArgc = 0; (( pszArg = ntharg( pszCmdLine, nArgc | 0x800 )) != NULL ); nArgc++ ) {

		if ( *pszArg == gpIniptr->SwChr ) {

		  for ( ; ; ) {

			// kludge to allow (illegal) syntax like "touch /F/Q file"
			if ( *pszArg == gpIniptr->SwChr )
				pszArg++;
			if ( *pszArg == _TEXT('\0') )
				break;

			rc = _ctoupper( *pszArg++ );
			switch ( rc ) {
			case 'A':       // retrieve based on specified attribs
				pszArg = GetSearchAttributes( pszArg );
				Touch.lMode |= 0x17;
				break;

			case 'E':
				Touch.lMode |= 0x100;
				break;

			case 'C':
			    // create the file if it doesn't exist
			    Touch.fFlags |= TOUCH_CREATE;
			    break;

			case 'F':
			    // ride roughshod over read-only files
			    Touch.fFlags |= TOUCH_FORCE;
			    break;

			case 'N':
				Touch.fFlags |= TOUCH_NOTHING;
				break;

			case 'Q':
			    Touch.fFlags |= TOUCH_QUIET;
			    break;

			case 'R':
			    // duplicate this file's date & time
			    Touch.fFlags |= TOUCH_DUP;

			    // check for optional write/access/create field
			    if ( *pszArg == _TEXT(':') ) {
					pszArg++;
			        if (( *pszArg == _TEXT('W') ) || ( *pszArg == _TEXT('w') )) {
						Touch.nField = 0;
						pszArg++;
			        } else if (( *pszArg == _TEXT('A') ) || ( *pszArg == _TEXT('a') )) {
						Touch.nField = 1;
						pszArg++;
			        } else if (( *pszArg == _TEXT('C') ) || ( *pszArg == _TEXT('c') )) {
						Touch.nField = 2;
						pszArg++;
			        }
			    }

			    if (( *pszArg == _TEXT('\0') ) && (( pszArg = ntharg( pszCmdLine, ++nArgc | 0x800 )) == NULL ))
					return ( Usage( TOUCH_USAGE ));

			    if ( find_file( FIND_FIRST, pszArg, Touch.lMode | FIND_CLOSE | FIND_NO_ERRORS, &dir, NULL ) != NULL ) {

	                if ( Touch.nField == 1)
                       FileTimeToDOSTime( &(dir.ftLastAccessTime), &(dir.ft.wr_time), &(dir.fd.wr_date) );
	    else if ( Touch.nField == 2)
	FileTimeToDOSTime( &(dir.ftCreationTime), &(dir.ft.wr_time), &(dir.fd.wr_date) );

					sysDateTime.year = dir.fd.file_date.years;
					if ( sysDateTime.year < 120 )
						sysDateTime.year += 1980;
					sysDateTime.month = dir.fd.file_date.months;
					sysDateTime.day = dir.fd.file_date.days;
					sysDateTime.hours = dir.ft.file_time.hours;
					sysDateTime.minutes = dir.ft.file_time.minutes;
					sysDateTime.seconds = dir.ft.file_time.seconds * 2;

				}

				pszArg = NULLSTR;
			    break;

			case 'D':
			case 'T':

			    if ( *pszArg == _TEXT(':') )
					pszArg++;
			    if (( *pszArg == _TEXT('W') ) || ( *pszArg == _TEXT('w') )) {
					Touch.nField = 0;
					pszArg++;
			    } else if (( *pszArg == _TEXT('A') ) || ( *pszArg == _TEXT('a') )) {
					Touch.nField = 1;
					pszArg++;
			    } else if (( *pszArg == _TEXT('C') ) || ( *pszArg == _TEXT('c') )) {
					Touch.nField = 2;
					pszArg++;
			    }

			    if ( rc == _TEXT('D') ) {

			        // valid date entry?
					if ( *pszArg == _TEXT('\0') ) {
						// keep the existing date
						Touch.fFlags |= TOUCH_PRESERVE_DATE;
					    break;
					}

					if ( !GetStrDate( pszArg, &uMonth, &uDay, &uYear )) {

						sysDateTime.year = uYear;
						sysDateTime.month = (unsigned char)uMonth;
						sysDateTime.day = (unsigned char)uDay;
						pszArg = NULLSTR;
						break;
					}

					return ( error( ERROR_4DOS_INVALID_DATE, pszArg ));

			    } else {

			        uSeconds = 0;

					if ( *pszArg == _TEXT('\0') ) {
						// keep existing time
						Touch.fFlags |= TOUCH_PRESERVE_TIME;
						break;
					}

			        if (( sscanf( pszArg, DATE_FMT, &uHours, &uMinutes, &uSeconds ) >= 2 ) && ( uHours < 24 ) && ( uMinutes < 60 ) && ( uSeconds < 60 )) {

						// check for AM/PM syntax
						if (( pszArg = strpbrk( pszArg, _TEXT("AP") )) != NULL ) {
							if (( uHours == 12 ) && ( *pszArg == _TEXT('A') ))
								uHours -= 12;
							else if (( *pszArg == _TEXT('P') ) && ( uHours < 12 ))
							    uHours += 12;
						}

				        sysDateTime.hours = (unsigned char)uHours;
				        sysDateTime.minutes = (unsigned char)uMinutes;
				        sysDateTime.seconds = (unsigned char)uSeconds;
						sysDateTime.hundredths = 0;
					    pszArg = NULLSTR;
					    break;
			        }

			        return ( error( ERROR_4DOS_INVALID_TIME, pszArg ));
			    }

			case 'S':
			    Touch.fFlags |= TOUCH_SUBDIRS;
			    break;

			default:
			    return ( Usage( TOUCH_USAGE ));
			}
		  }

		  continue;
		}

		mkfname( pszArg, 0 );
		strcpy( Touch.szSource, pszArg );

		// save the source filename part (for recursive calls and
		//   include lists)
		Touch.pszFilename = Touch.szSource + strlen( path_part( Touch.szSource));
		pszTemp = (LPTSTR)_alloca( ( strlen( Touch.pszFilename ) + 1 ) * sizeof(TCHAR) );
		Touch.pszFilename = strcpy( pszTemp, Touch.pszFilename );

			nReturn = _touch( &Touch, &sysDateTime );

		if (( setjmp( cv.env ) == -1 ) || ( nReturn == CTRLC ))
			break;

	}

	return (( Touch.szSource[0] == _TEXT('\0') ) ? Usage( TOUCH_USAGE ) : nReturn );
}


// Apply the requested timestamps to matching files.
// Touch holds the search and operation flags; sysDateTime supplies the timestamp.
// Recurse into subdirectories when requested.
static int _touch( TOUCH_STRUCT *Touch, DATETIME *sysDateTime )
{
	int nReturn = 0, rc, nFind;
	unsigned int uAttribute = 0;
	FILESEARCH dir;
	LPTSTR pszSourceName;

	// copy date/time range info
	memmove( &(dir.aRanges), &aRanges, sizeof(RANGES) );

	dir.hdir = INVALID_HANDLE_VALUE;

	// touch specified files
	for ( nFind = FIND_FIRST; ( nFind != 0 ); nFind = FIND_NEXT ) {

		if (( setjmp( cv.env ) == -1 ) || ( cv.fException )) {
			FindClose( dir.hdir );
			return CTRLC;
		}

		CheckForBreak();


		if ( find_file( nFind, Touch->szSource, ( Touch->lMode | FIND_NO_ERRORS | FIND_BYATTS | FIND_RANGE | FIND_EXCLUDE | FIND_NO_DOTNAMES ), &dir, Touch->szTarget ) == NULL ) {

			if ( nFind == FIND_FIRST ) {

				// if empty file list, ignore it
				if ( dir.pszFileList != NULL )
					break;

			    // if file doesn't exist, create it
				if ( Touch->fFlags & TOUCH_NOTHING ) {
					strcpy( Touch->szTarget, Touch->szSource );
					nFind = 0;
				} else if (( Touch->fFlags & TOUCH_CREATE ) && (( nFind = _sopen( Touch->szSource, _O_WRONLY | _O_BINARY | _O_CREAT | _O_TRUNC, _SH_DENYWR, _S_IWRITE | _S_IREAD )) >= 0 )) {
					_close( nFind );
					strcpy( Touch->szTarget, Touch->szSource );
					nFind = 0;
			    } else {
					if (( Touch->lMode & 0x100 ) == 0 )
						error( ERROR_FILE_NOT_FOUND, Touch->szSource );
					nReturn = ERROR_EXIT;
					break;
				}

			} else
			    break;
		}

		if ( Touch->fFlags & ( TOUCH_PRESERVE_DATE | TOUCH_PRESERVE_TIME )) {

			if ( Touch->nField == 1)
				FileTimeToDOSTime( &(dir.ftLastAccessTime), &(dir.ft.wr_time), &(dir.fd.wr_date) );
			else if ( Touch->nField == 2)
				FileTimeToDOSTime( &(dir.ftCreationTime), &(dir.ft.wr_time), &(dir.fd.wr_date) );

			if ( Touch->fFlags & TOUCH_PRESERVE_DATE ) {
				sysDateTime->year = dir.fd.file_date.years;
				if ( sysDateTime->year < 120 )
					sysDateTime->year += 1980;
				sysDateTime->month = dir.fd.file_date.months;
				sysDateTime->day = dir.fd.file_date.days;
			}

			if ( Touch->fFlags & TOUCH_PRESERVE_TIME ) {
				sysDateTime->hours = dir.ft.file_time.hours;
				sysDateTime->minutes = dir.ft.file_time.minutes;
				sysDateTime->seconds = dir.ft.file_time.seconds * 2;
			}

		}

		if (( Touch->fFlags & TOUCH_FORCE ) && (( Touch->fFlags & TOUCH_NOTHING ) == 0 )) {
			// set the target file attributes, less RDONLY
			QueryFileMode( Touch->szTarget, &uAttribute );
			SetFileMode( Touch->szTarget, uAttribute & 0x26 );
		}

		// display the time being set
		if (( Touch->fFlags & TOUCH_QUIET ) == 0 )
			printf( _TEXT("%s %02d:%02d:%02d  %s\r\n"), FormatDate( sysDateTime->month, sysDateTime->day, sysDateTime->year, 0x100 ), sysDateTime->hours, sysDateTime->minutes, sysDateTime->seconds, Touch->szTarget );

		if (( Touch->fFlags & TOUCH_NOTHING ) == 0 ) {
			if ((( rc =  SetFileDateTime( Touch->szTarget, 0, sysDateTime, Touch->nField )) != 0 ) && (( Touch->lMode & 0x100 ) == 0 ))
				nReturn = error( rc, Touch->szTarget );
		}

		// restore original attributes
		if (( Touch->fFlags & TOUCH_FORCE ) && (( Touch->fFlags & TOUCH_NOTHING ) == 0 ))
			SetFileMode( Touch->szTarget, uAttribute );
	}

	if ( Touch->fFlags & TOUCH_SUBDIRS ) {

		// modify matching subdirectory files

		CheckFreeStack( 1024 );

		// strip the filename & add wildcards for directory search
		sprintf( Touch->szSource, FMT_DOUBLE_STR, path_part( Touch->szSource ), WILD_FILE );

		// save the current subdirectory start
		pszSourceName = strchr( Touch->szSource, _TEXT('*') );

		// search for all subdirectories in this (sub)directory
		//   tell find_file() not to display errors, & to only
		//   return subdir names
		for ( nFind = FIND_FIRST; ( find_file( nFind, Touch->szSource, ( Touch->lMode | 0x10 | FIND_NO_ERRORS | FIND_ONLY_DIRS | FIND_NO_FILELISTS | FIND_EXCLUDE ), &dir, NULL ) != NULL ); nFind = FIND_NEXT ) {

			// make the new "source"
			sprintf( pszSourceName, FMT_PATH_STR, dir.szFileName, Touch->pszFilename );

			// process directory tree recursively
			nReturn = _touch( Touch, sysDateTime );

			if (( setjmp( cv.env ) == -1 ) || ( nReturn == CTRLC )) {
				// reset the directory search handle
				FindClose( dir.hdir );
				return CTRLC;
			}

			// restore the original name
			strcpy( pszSourceName, WILD_FILE );
		}
	}

	return nReturn;
}

// File attribute worker relocated to FILE_TEXT.
// ATTRIB command parsing and show_atts() remain in their original segments.
#define ATTRIB_DIRS 2
#define ATTRIB_NOERRORS 4
#define ATTRIB_NOTHING 8
#define ATTRIB_PAUSE 0x10
#define ATTRIB_QUIET 0x20
#define ATTRIB_SUBDIRS 0x40

typedef struct
{
	TCHAR szSource[MAXFILENAME];
	TCHAR szTarget[MAXFILENAME];
	LPTSTR szFilename;
	long fFlags;
	long lMode;

	char cAnd;
	char cOr;

} ATTRIB_STRUCT;


extern LPTSTR _far FileAttribShowBridge( int );

// ATTRIB command parser in FILE_TEXT; preserve the original command behavior.
int _far FileAttribWorker( ATTRIB_STRUCT * );

// ATTRIB [options] [+attribute | -attribute] [filename ...]
// Display or change file attributes, optionally including subdirectories.
// Examples: ATTRIB *.TXT; ATTRIB +R REPORT.TXT.
int _far FileAttrib_Cmd( LPTSTR pszCmdLine )
{
	register LPTSTR pszArg;
	LPTSTR pszTrailing;
	int nArgc, n, nReturn = 0;
	union {
		// file attribute structure

		unsigned char attribute;

		struct {
			unsigned rdonly : 1;
			unsigned hidden : 1;
			unsigned system : 1;
			unsigned reserved : 2;
			unsigned archive : 1;

		} bit;
	} and_mask, or_mask;
	ATTRIB_STRUCT Attrib;

	Attrib.lMode = 0;

	init_page_size();		// clear row & page length vars

	// get date/time/size ranges
	if ( GetRange( pszCmdLine, &aRanges, 0 ) != 0 )
		return ERROR_EXIT;

	// check for flags
	if ( GetSwitches( pszCmdLine, _TEXT("*DENPQS"), &(Attrib.fFlags), 0 ) != 0 )
		return (Usage( ATTRIB_USAGE ));

	if ( Attrib.fFlags & ATTRIB_PAUSE )
		gnPageLength = GetScrRows();

	Attrib.lMode |= (( Attrib.fFlags & ATTRIB_DIRS ) ? 0x17 : 0x07 );
	if ( gnInclusiveMode & _A_SUBDIR )
		Attrib.lMode |= _A_SUBDIR;

	// suppress error message from find_file
	if ( Attrib.fFlags & ATTRIB_NOERRORS )
		Attrib.lMode |= FIND_NO_ERRORS;

	and_mask.attribute = 0x37;

	or_mask.attribute = 0;

	nArgc = 0;
	strcpy( Attrib.szSource, WILD_FILE );	// default to *.*

	do {

	    if (( pszArg = ntharg( pszCmdLine, nArgc++ )) != NULL ) {

		if ( *pszArg == _TEXT('-') ) {

			// remove attribute (clear OR and AND bits)
			while ( *(++pszArg )) {

				switch ( _ctoupper( *pszArg )) {
				case 'A':
					or_mask.bit.archive = and_mask.bit.archive = 0;
					break;
				case 'H':
					or_mask.bit.hidden = and_mask.bit.hidden = 0;
					break;
				case 'R':
					or_mask.bit.rdonly = and_mask.bit.rdonly = 0;
					break;
				case 'S':
					or_mask.bit.system = and_mask.bit.system = 0;
					break;
				case '-':
					// kludge for RTPatch bug
					break;
				case '_':
					// ignore for ___A_ syntax
					break;
				case 'D':
					// ____D syntax
					Attrib.fFlags |= ATTRIB_DIRS;
					break;

				default:
					error( ERROR_INVALID_PARAMETER, pszArg );
					return (Usage( ATTRIB_USAGE ));
				}
			}

		} else if ( *pszArg == _TEXT('+') ) {

			// add attribute (set OR bits)
			while ( *(++pszArg )) {

				switch ( _ctoupper( *pszArg )) {
				case 'A':
					or_mask.bit.archive = 1;
					break;
				case 'H':
					or_mask.bit.hidden = 1;
					break;
				case 'R':
					or_mask.bit.rdonly = 1;
					break;
				case 'S':
					or_mask.bit.system = 1;
					break;
				case '+':
					// kludge for RTPatch bug
					break;
				case '_':
					// ignore for ___A_ syntax
					break;
				case 'D':
					// ____D syntax
					Attrib.fFlags |= ATTRIB_DIRS;
					break;

				default:
					error( ERROR_INVALID_PARAMETER, pszArg );
					return ( Usage( ATTRIB_USAGE ));
				}
			}

		} else {

		    // it must be a filename
		    copy_filename( Attrib.szSource, pszArg );

		    // check for trailing switches
		    // KLUDGE for COMMAND.COM compatibility (ATTRIB *.c +h)
		    for ( n = nArgc; (( pszTrailing = ntharg( pszCmdLine, n )) != NULL ); n++ ) {
			// check for another file spec
			if (( *pszTrailing != _TEXT('-') ) && ( *pszTrailing != _TEXT('+') ))
				goto change_atts;
		    }
		}

	    } else {

change_atts:
		if ( mkfname( Attrib.szSource, 0 ) == NULL )
			return ERROR_EXIT;

		// if source is a directory & no /D specified, add "\*.*"
		if ( Attrib.fFlags & ATTRIB_DIRS )
			strip_trailing( Attrib.szSource, _TEXT("\\/") );
		else
			AddWildcardIfDirectory( Attrib.szSource );

		// save the source filename part (for recursive calls and
		//   include lists)
		Attrib.szFilename = Attrib.szSource + strlen( path_part( Attrib.szSource));
		pszTrailing = (LPTSTR)_alloca( ( strlen( Attrib.szFilename ) + 1 ) * sizeof(TCHAR) );
		Attrib.szFilename = strcpy( pszTrailing, Attrib.szFilename );

		// change attributes
		Attrib.cAnd = and_mask.attribute;
		Attrib.cOr = or_mask.attribute;

		nReturn = FileAttribWorker( &Attrib );
		if (( setjmp( cv.env ) == -1 ) || ( nReturn == CTRLC ))
			break;

	    }

	} while (( nReturn != CTRLC ) && ( pszArg != NULL ));

	return nReturn;
}





// Apply or display attributes for matching files.
// Attrib holds the search options and AND/OR attribute masks.
// Recurse into subdirectories when requested.
int _far FileAttribWorker( register ATTRIB_STRUCT *Attrib )
{
	register unsigned int uOldAttribute;
	int nFind, nReturn = 0, rc, nAttribute;
	LPTSTR pszSource;
	FILESEARCH dir;

	// copy date/time range info
	memmove( &(dir.aRanges), &aRanges, sizeof(RANGES) );

	dir.hdir = INVALID_HANDLE_VALUE;

	// change attributes
	for ( nFind = FIND_FIRST; ( find_file( nFind, Attrib->szSource, ( Attrib->lMode | FIND_BYATTS | FIND_RANGE | FIND_EXCLUDE | FIND_NO_DOTNAMES ), &dir, Attrib->szTarget ) != NULL ); nFind = FIND_NEXT ) {

		if (( setjmp( cv.env ) == -1 ) || ( cv.fException )) {
			FindClose( dir.hdir );
			return CTRLC;
		}


		CheckForBreak();

		// kludge for Win32 GetFileAttributes API bug -- won't work with open files!!
		//  (If it isn't a filelist, we already have the attributes in "dir.attrib")

		// query file mode again because we might be using @filelists (lFileOffset >= 0)
		if ( dir.lFileOffset != -1L ) {
			if (( nReturn = QueryFileMode( Attrib->szTarget, (unsigned int *)&nAttribute )) == 0 )
				dir.attrib = nAttribute;
			else {
				if (( Attrib->fFlags & ATTRIB_NOERRORS ) == 0 )
					error( nReturn, Attrib->szTarget );
				nReturn = ERROR_EXIT;
				continue;
			}
		}

		// display old attributes, new attributes, and filename

		uOldAttribute = ( dir.attrib & 0x37 );

		dir.attrib = (( uOldAttribute & Attrib->cAnd ) | Attrib->cOr );

		// if attributes were changed, show new ones
		if (( Attrib->fFlags & ATTRIB_QUIET ) == 0 ) {

			qputs( FileAttribShowBridge( uOldAttribute ));

			if (( Attrib->cAnd != 0x37 ) || ( Attrib->cOr != 0 )) {

				printf( _TEXT(" -> %s"), FileAttribShowBridge( dir.attrib ));
			}

			printf( _TEXT("  %s"), Attrib->szTarget );
			crlf();
			_page_break();
		}

		// check for a change; ignore if same
		if ( uOldAttribute != dir.attrib && !( Attrib->fFlags & ATTRIB_NOTHING )) {

			// can't change directory attribute
			dir.attrib &= ~_A_SUBDIR;
			if (( rc = SetFileMode( Attrib->szTarget, dir.attrib )) != 0 ) {
				if (( Attrib->fFlags & ATTRIB_NOERRORS ) == 0 )
					error( rc, Attrib->szTarget );
				nReturn = ERROR_EXIT;
			}
		}
	}

	if ( Attrib->fFlags & ATTRIB_SUBDIRS ) {
	// modify matching subdirectory files too?

		CheckFreeStack( 1024 );

		// strip the filename & add wildcards for directory search
		sprintf( Attrib->szSource, FMT_DOUBLE_STR, path_part( Attrib->szSource ), WILD_FILE );

		// save the current subdirectory start
		pszSource = strchr( Attrib->szSource, _TEXT('*') );

		// search for all subdirectories in this (sub)directory
		//   tell find_file() not to display errors, & to only
		//   return subdir names
		for ( nFind = FIND_FIRST; ( find_file( nFind, Attrib->szSource, ( Attrib->lMode | 0x10 | FIND_NO_ERRORS | FIND_ONLY_DIRS | FIND_NO_FILELISTS | FIND_EXCLUDE ), &dir, NULL ) != NULL ); nFind = FIND_NEXT) {

			// make the new "source"
			sprintf( pszSource, FMT_PATH_STR, dir.szFileName, Attrib->szFilename );

			// process directory tree recursively
			nReturn = FileAttribWorker( Attrib );
			if (( setjmp( cv.env ) == -1 ) || ( nReturn == CTRLC )) {
				// reset the directory search handle
				FindClose( dir.hdir );
				return CTRLC;
			}

			// restore the original name
			strcpy( pszSource, WILD_FILE );
		}
	}

	return nReturn;
}

// REN command implementation in FILE_TEXT.
extern void _far FileRenWildname( LPTSTR, LPTSTR, LPTSTR );
extern int _far FileRenCommand( LPTSTR, int );
#define REN_BY_ATTRIBUTES 1
#define REN_NOERRORS 2
#define REN_NOTHING 4
#define REN_QUERY 8
#define REN_QUIET 0x10
#define REN_SUBDIRS 0x20
#define REN_TOTALS 0x40

// REN [options] source [source ...] target
// Rename files or directories, including wildcard target expansion.
// Example: REN *.TXT *.BAK.
int _far FileRen_Cmd( LPTSTR pszCmdLine )
{
	register LPTSTR pszArg;
	TCHAR szSource[MAXFILENAME], szTargetName[MAXFILENAME], szTarget[MAXFILENAME];
	int i, nArgc, nFind, nReturn = 0;
	unsigned long ulMode = 0;
	long fRen, lFilesRenamed = 0;
	FILESEARCH dir;

	// get date/time/size ranges
	if ( GetRange( pszCmdLine, &(dir.aRanges), 0 ) != 0 )
		return ERROR_EXIT;

	// check for and remove switches
	// get the target name, or abort if no target
	if (( GetSwitches( pszCmdLine, _TEXT("*ENPQST"), &fRen, 0 ) != 0 ) || (( pszArg = last_arg( pszCmdLine, &nArgc )) == NULL ))
		return ( Usage( RENAME_USAGE ));

	if ( fRen & REN_BY_ATTRIBUTES )
		ulMode |= 0x07;

	// if query, turn off quiet mode
	if ( fRen & REN_QUERY )
		fRen &= ~( REN_QUIET | REN_TOTALS );

	copy_filename( szTarget, pszArg );
	StripQuotes( szTarget );
	*gpNthptr = _TEXT('\0');		// remove target from command line

	// rename matching directories?
	if (( fRen & REN_SUBDIRS ) || ( strpbrk( szTarget, WILD_CHARS ) == NULL ))
		ulMode |= _A_SUBDIR;

	// suppress error message from find_file
	if ( fRen & REN_NOERRORS )
		ulMode |= FIND_NO_ERRORS;

	for ( nArgc = 0; (( pszArg = ntharg( pszCmdLine, nArgc)) != NULL ); nArgc++ ) {

		if ( mkfname( pszArg, (( fRen & REN_NOERRORS ) ? MKFNAME_NOERROR : 0 )) == NULL )
			continue;

		// strip trailing backslashes
		if ( ulMode & _A_SUBDIR )
			strip_trailing( pszArg+3, SLASHES );

		for ( nFind = FIND_FIRST; ; nFind = FIND_NEXT ) {

			if ( find_file( nFind, pszArg, ( ulMode | FIND_BYATTS | FIND_RANGE | FIND_EXCLUDE | FIND_NO_DOTNAMES ), &dir, szSource ) == NULL )
				break;

			if ( setjmp( cv.env ) == -1 ) {
				FindClose( dir.hdir );
				nReturn = CTRLC;
				goto rename_abort;
			}

			CheckForBreak();

			FileRenWildname( szTargetName, szTarget, szSource );

			if ( mkfname( szSource, 0 ) == NULL )
				return ERROR_EXIT;

			// nasty kludge to emulate COMMAND.COM & CMD.EXE stupid
			//   renaming convention (if source has path & target
			//   doesn't, rename into source directory)
			if ( path_part( szTargetName ) == NULL )
				strins( szTargetName, path_part( szSource ));

			if ( mkfname( szTargetName, 0 ) == NULL )
				return ERROR_EXIT;

			if ( is_dir( szTargetName )) {
				// destination is directory; create target name
				//   (unless we're renaming directories!)
				if ( !is_dir( szSource ))
					mkdirname( szTargetName, fname_part( szSource ));
			}

			if (( fRen & ( REN_QUIET | REN_TOTALS )) == 0 ) {

				printf( FMT_STR_TO_STR, szSource, szTargetName );

				if ( fRen & REN_QUERY ) {
					if ((( i = QueryInputChar( NULLSTR, YES_NO_ALL )) == ALL_CHAR ) || ( i == REST_CHAR ))
						fRen &= ~REN_QUERY;
					else if ( i == NO_CHAR )
						continue;
					else if ( i == ESCAPE ) {
						FindClose( dir.hdir );
						break;
					}
				} else
					crlf();
			}

			if (( fRen & REN_NOTHING ) == 0 ) {

				if ( rename( szSource, szTargetName ) == -1 ) {
					// check if error is with source or target
					nReturn = error( _doserrno, ((( _doserrno == ERROR_ACCESS_DENIED) || ( _doserrno == ERROR_NOT_SAME_DEVICE ) || ( _doserrno == ERROR_PATH_NOT_FOUND)) ? szTargetName : szSource ));

				} else {

					lFilesRenamed++;

					// rename description to target dir
					process_descriptions( szSource, szTargetName, DESCRIPTION_COPY );

					// update JPSTREE.IDX
					if ( is_dir( szTargetName )) {

						TCHAR szTemp[MAXFILENAME], szBuf[MAXLINESIZ];
						int n, fSave;
						FILESEARCH tdir;

						// remove source tree
						UpdateFuzzyTree( szSource, NULL, 1 );
						// add new target directory
						UpdateFuzzyTree( szTargetName, NULL, 0 );

						// now rename any subdirectories
						strcpy( szTemp, szTargetName );
						mkdirname( szTemp, WILD_FILE );

						if ( find_file( 0x4E, szTemp, 0x17 | FIND_NO_ERRORS | FIND_ONLY_DIRS | FIND_NO_DOTNAMES | FIND_CLOSE | FIND_NO_FILELISTS, &tdir, NULL ) != NULL ) {

							// save command line (it'll be destroyed by the command() call)
							pszCmdLine = strcpy( (LPTSTR)_alloca( ( strlen( pszCmdLine ) + 1 ) * sizeof(TCHAR) ), pszCmdLine );

							// build a temporary file
							szTemp[0] = _TEXT('\0');

							GetTempDirectory( szTemp );

							UniqueFileName( szTemp );

							sprintf( szBuf, _TEXT("*@tree /c \"%s\" >! \"%s\""), szTargetName, szTemp );

							// use TREE to build the directory list
							// make sure TREE isn't disabled!
							n = findcmd( _TEXT("TREE"), 1 );

							fSave = commands[ n ].fParse;
							commands[ n ].fParse &= ~CMD_DISABLED;
							i = FileRenCommand( szBuf, 0 );
							commands[ n ].fParse = fSave;

							// check for ^C or stack overflow
							if (( setjmp( cv.env ) == -1 ) || ( i == CTRLC ) || ( cv.fException )) {
								FindClose( dir.hdir );
								nReturn = CTRLC;
								goto rename_abort;
							}

							// insert temp file into JPSTREE.IDX
							UpdateFuzzyTree( szTargetName, szTemp, 0 );
							remove( szTemp );
						}
					}
				}
			} else if ( is_file( szTargetName ) )
				nReturn = error( _doserrno = ERROR_ACCESS_DENIED, szTargetName );
		}

		// delete the descriptions for renamed or missing files
		if ((( fRen & REN_NOTHING ) == 0 ) && ( lFilesRenamed != 0L )) {
			strcat( pszArg, WILD_FILE );
			process_descriptions( NULL, pszArg, DESCRIPTION_REMOVE );
		}
	}

rename_abort:
	if (( fRen & REN_QUIET ) == 0 )
		FilesProcessed( FILES_RENAMED, lFilesRenamed );

	return (( lFilesRenamed != 0L ) ? nReturn : ERROR_EXIT );
}
