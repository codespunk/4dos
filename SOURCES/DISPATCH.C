

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


// DISPATCH.C - near command dispatchers for segmented implementations

#include "product.h"
#include "4all.h"

// Keep this compilation unit in _TEXT: the command table uses near pointers.

// Near dispatcher retained for the existing command table.
extern int _far UtilityBeep_Cmd( LPTSTR );
int _near Beep_Cmd( LPTSTR pszCmdLine )
{
	return UtilityBeep_Cmd( pszCmdLine );
}

// Near dispatcher retained for the existing command table.
extern int _far UtilityDelay_Cmd( LPTSTR );
int _near Delay_Cmd( LPTSTR pszCmdLine )
{
	return UtilityDelay_Cmd( pszCmdLine );
}

// Near dispatcher for the command table; implementation resides in SCREENIO_TEXT.
extern int _far ScreenEcho_Cmd( LPTSTR );
int _near Echo_Cmd( LPTSTR pszCmdLine )
{
	return ScreenEcho_Cmd( pszCmdLine );
}

// Near dispatcher for the command table; implementation resides in SCREENIO_TEXT.
extern int _far ScreenEchoErr_Cmd( LPTSTR );
int _near EchoErr_Cmd( LPTSTR pszCmdLine )
{
	return ScreenEchoErr_Cmd( pszCmdLine );
}

// Near dispatcher for the command table; implementation resides in SCREENIO_TEXT.
extern int _far ScreenEchos_Cmd( LPTSTR );
int _near Echos_Cmd( LPTSTR pszCmdLine )
{
	return ScreenEchos_Cmd( pszCmdLine );
}

// Near dispatcher for the command table; implementation resides in SCREENIO_TEXT.
extern int _far ScreenEchosErr_Cmd( LPTSTR );
int _near EchosErr_Cmd( LPTSTR pszCmdLine )
{
	return ScreenEchosErr_Cmd( pszCmdLine );
}

// Near dispatcher for the command table; implementation resides in SCREENIO_TEXT.
extern int _far ScreenInkey_Cmd( LPTSTR );
int _near Inkey_Cmd( LPTSTR pszCmdLine )
{
	return ScreenInkey_Cmd( pszCmdLine );
}

// Near dispatcher for the command table; implementation resides in SCREENIO_TEXT.
extern int _far ScreenInput_Cmd( LPTSTR );
int _near Input_Cmd( LPTSTR pszCmdLine )
{
	return ScreenInput_Cmd( pszCmdLine );
}

// Near dispatcher for the command table; implementation resides in SCREENIO_TEXT.
extern int _far ScreenPause_Cmd( LPTSTR );
int _near Pause_Cmd( LPTSTR pszCmdLine )
{
	return ScreenPause_Cmd( pszCmdLine );
}

// Near dispatcher for the command table; implementation resides in SCREENIO_TEXT.
extern int _far ScreenScr_Cmd( LPTSTR );
int _near Scr_Cmd( LPTSTR pszCmdLine )
{
	return ScreenScr_Cmd( pszCmdLine );
}

// Near dispatcher for the command table; implementation resides in SCREENIO_TEXT.
extern int _far ScreenScrput_Cmd( LPTSTR );
int _near Scrput_Cmd( LPTSTR pszCmdLine )
{
	return ScreenScrput_Cmd( pszCmdLine );
}

// Near dispatcher for the command table; implementation resides in SCREENIO_TEXT.
extern int _far ScreenVScrput_Cmd( LPTSTR );
int _near VScrput_Cmd( LPTSTR pszCmdLine )
{
	return ScreenVScrput_Cmd( pszCmdLine );
}

// REM - do nothing
int _near Remark_Cmd( LPTSTR pszCmdLine )
{
	return 0;
}

// Near command-table entry; implementation resides in BATCH_TEXT.
extern int _far BatchLfnFor_Cmd( LPTSTR );
int _near LfnFor_Cmd( LPTSTR pszCmdLine )
{
	return BatchLfnFor_Cmd( pszCmdLine );
}

// Near command-table entry; implementation resides in BATCH_TEXT.
extern int _far BatchShift_Cmd( LPTSTR );
int _near Shift_Cmd( LPTSTR pszCmdLine )
{
	return BatchShift_Cmd( pszCmdLine );
}

// Near command-table entry; implementation resides in BATCH_TEXT.
extern int _far BatchBattext_Cmd( LPTSTR );
int _near Battext_Cmd( LPTSTR pszCmdLine )
{
	return BatchBattext_Cmd( pszCmdLine );
}

// Near command-table entry; implementation resides in BATCH_TEXT.
extern int _far BatchOn_Cmd( LPTSTR );
int _near On_Cmd( LPTSTR pszCmdLine )
{
	return BatchOn_Cmd( pszCmdLine );
}

// Near command-table entry; implementation resides in BATCH_TEXT.
extern int _far BatchLoadbtm_Cmd( LPTSTR );
int _near Loadbtm_Cmd( LPTSTR pszCmdLine )
{
	return BatchLoadbtm_Cmd( pszCmdLine );
}
