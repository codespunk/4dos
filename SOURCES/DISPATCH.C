

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

// UTILITY_TEXT command dispatchers

// Near command-table entry; implementation resides in UTILITY_TEXT.
extern int _far UtilityBeep_Cmd( LPTSTR );
int _near Beep_Cmd( LPTSTR pszCmdLine )
{
	return UtilityBeep_Cmd( pszCmdLine );
}

// Near command-table entry; implementation resides in UTILITY_TEXT.
extern int _far UtilityDelay_Cmd( LPTSTR );
int _near Delay_Cmd( LPTSTR pszCmdLine )
{
	return UtilityDelay_Cmd( pszCmdLine );
}

// Near command-table entry; implementation resides in UTILITY_TEXT.
extern int _far UtilityBreak_Cmd( char * );
int _near Break_Cmd( register char *pszCmdLine )
{
	return UtilityBreak_Cmd( pszCmdLine );
}

// Near command-table entry; implementation resides in UTILITY_TEXT.
extern int _far UtilityCls_Cmd( char * );
int _near Cls_Cmd( char *pszCmdLine )
{
	return UtilityCls_Cmd( pszCmdLine );
}

// Near command-table entry; implementation resides in UTILITY_TEXT.
extern int _far UtilityIdle_Cmd( char * );
int _near Idle_Cmd( LPTSTR pszCmdLine )
{
	return UtilityIdle_Cmd( pszCmdLine );
}

// Near command-table entry; implementation resides in UTILITY_TEXT.
extern int _far UtilityTruename_Cmd( char * );
int _near Truename_Cmd( char *pszCmdLine )
{
	return UtilityTruename_Cmd( pszCmdLine );
}

// Near command-table entry; implementation resides in UTILITY_TEXT.
extern int _far UtilityVer_Cmd( char * );
int _near Ver_Cmd( char *pszCmdLine )
{
	return UtilityVer_Cmd( pszCmdLine );
}

// SCREEN_TEXT command dispatchers

// Near command-table entry; implementation resides in SCREEN_TEXT.
extern int _far ScreenEcho_Cmd( LPTSTR );
int _near Echo_Cmd( LPTSTR pszCmdLine )
{
	return ScreenEcho_Cmd( pszCmdLine );
}

// Near command-table entry; implementation resides in SCREEN_TEXT.
extern int _far ScreenEchoErr_Cmd( LPTSTR );
int _near EchoErr_Cmd( LPTSTR pszCmdLine )
{
	return ScreenEchoErr_Cmd( pszCmdLine );
}

// Near command-table entry; implementation resides in SCREEN_TEXT.
extern int _far ScreenEchos_Cmd( LPTSTR );
int _near Echos_Cmd( LPTSTR pszCmdLine )
{
	return ScreenEchos_Cmd( pszCmdLine );
}

// Near command-table entry; implementation resides in SCREEN_TEXT.
extern int _far ScreenEchosErr_Cmd( LPTSTR );
int _near EchosErr_Cmd( LPTSTR pszCmdLine )
{
	return ScreenEchosErr_Cmd( pszCmdLine );
}

// Near command-table entry; implementation resides in SCREEN_TEXT.
extern int _far ScreenInkey_Cmd( LPTSTR );
int _near Inkey_Cmd( LPTSTR pszCmdLine )
{
	return ScreenInkey_Cmd( pszCmdLine );
}

// Near command-table entry; implementation resides in SCREEN_TEXT.
extern int _far ScreenInput_Cmd( LPTSTR );
int _near Input_Cmd( LPTSTR pszCmdLine )
{
	return ScreenInput_Cmd( pszCmdLine );
}

// Near command-table entry; implementation resides in SCREEN_TEXT.
extern int _far ScreenPause_Cmd( LPTSTR );
int _near Pause_Cmd( LPTSTR pszCmdLine )
{
	return ScreenPause_Cmd( pszCmdLine );
}

// Near command-table entry; implementation resides in SCREEN_TEXT.
extern int _far ScreenScr_Cmd( LPTSTR );
int _near Scr_Cmd( LPTSTR pszCmdLine )
{
	return ScreenScr_Cmd( pszCmdLine );
}

// Near command-table entry; implementation resides in SCREEN_TEXT.
extern int _far ScreenScrput_Cmd( LPTSTR );
int _near Scrput_Cmd( LPTSTR pszCmdLine )
{
	return ScreenScrput_Cmd( pszCmdLine );
}

// Near command-table entry; implementation resides in SCREEN_TEXT.
extern int _far ScreenVScrput_Cmd( LPTSTR );
int _near VScrput_Cmd( LPTSTR pszCmdLine )
{
	return ScreenVScrput_Cmd( pszCmdLine );
}

// FILE_TEXT command dispatchers

// Near command-table entry; implementation resides in FILE_TEXT.
extern int _far FileDescribe_Cmd( LPTSTR );
int _near Describe_Cmd( LPTSTR pszCmdLine )
{
	return FileDescribe_Cmd( pszCmdLine );
}

// Near command-table entry; implementation resides in FILE_TEXT.
extern int _far FileCopy_Cmd( LPTSTR );
int _near Copy_Cmd( LPTSTR pszCmdLine )
{
	return FileCopy_Cmd( pszCmdLine );
}

// Near command-table entry; implementation resides in FILE_TEXT.
extern int _far FileMove_Cmd( LPTSTR );
int _near Mv_Cmd( LPTSTR pszCmdLine )
{
	return FileMove_Cmd( pszCmdLine );
}

// Near command-table entry; implementation resides in FILE_TEXT.
extern int _far FileRen_Cmd( LPTSTR );
int _near Ren_Cmd( LPTSTR pszCmdLine )
{
	return FileRen_Cmd( pszCmdLine );
}

// Near command-table entry; implementation resides in FILE_TEXT.
extern int _far FileDel_Cmd( LPTSTR );
int _near Del_Cmd( LPTSTR pszCmdLine )
{
	return FileDel_Cmd( pszCmdLine );
}

// Near command-table entry; implementation resides in FILE_TEXT.
extern int _far FileAttrib_Cmd( LPTSTR );
int _near Attrib_Cmd( LPTSTR pszCmdLine )
{
	return FileAttrib_Cmd( pszCmdLine );
}

// Near command-table entry; implementation resides in FILE_TEXT.
extern int _far FileTouch_Cmd( LPTSTR );
int _near Touch_Cmd( LPTSTR pszCmdLine )
{
	return FileTouch_Cmd( pszCmdLine );
}

// Near command-table entry; implementation resides in FILE_TEXT.
extern int _far FileHead_Cmd( LPTSTR );
int _near Head_Cmd( LPTSTR pszCmdLine )
{
	return FileHead_Cmd( pszCmdLine );
}

// Near command-table entry; implementation resides in FILE_TEXT.
extern int _far FileTail_Cmd( LPTSTR );
int _near Tail_Cmd( LPTSTR pszCmdLine )
{
	return FileTail_Cmd( pszCmdLine );
}

// Near command-table entry; implementation resides in FILE_TEXT.
extern int _far FileType_Cmd( LPTSTR );
int _near Type_Cmd( LPTSTR pszCmdLine )
{
	return FileType_Cmd( pszCmdLine );
}

// Near command-table entry; implementation resides in FILE_TEXT.
extern int _far FileY_Cmd( LPTSTR );
int _near Y_Cmd( LPTSTR pszCmdLine )
{
	return FileY_Cmd( pszCmdLine );
}

// Near command-table entry; implementation resides in FILE_TEXT.
extern int _far FileTee_Cmd( LPTSTR );
int _near Tee_Cmd( LPTSTR pszCmdLine )
{
	return FileTee_Cmd( pszCmdLine );
}

// DIR_TEXT command dispatchers

// Near command-table entry; implementation resides in DIR_TEXT.
extern int _far DirTree_Cmd( LPTSTR );
int _near Tree_Cmd( LPTSTR pszCmdLine )
{
	return DirTree_Cmd( pszCmdLine );
}

// Near command-table entry; implementation resides in DIR_TEXT.
extern int _far DirList_Cmd( LPTSTR );
int _near Dir_Cmd( LPTSTR pszCmdLine )
{
	return DirList_Cmd( pszCmdLine );
}

// Near command-table entry; implementation resides in DIR_TEXT.
extern int _far DirCd_Cmd( LPTSTR );
int _near Cd_Cmd( LPTSTR pszCmdLine )
{
	return DirCd_Cmd( pszCmdLine );
}

// Near command-table entry; implementation resides in DIR_TEXT.
extern int _far DirCdd_Cmd( LPTSTR );
int _near Cdd_Cmd( LPTSTR pszCmdLine )
{
	return DirCdd_Cmd( pszCmdLine );
}

// Near command-table entry; implementation resides in DIR_TEXT.
extern int _far DirMd_Cmd( LPTSTR );
int _near Md_Cmd( LPTSTR pszCmdLine )
{
	return DirMd_Cmd( pszCmdLine );
}

// Near command-table entry; implementation resides in DIR_TEXT.
extern int _far DirPushd_Cmd( LPTSTR );
int _near Pushd_Cmd( LPTSTR pszCmdLine )
{
	return DirPushd_Cmd( pszCmdLine );
}

// Near command-table entry; implementation resides in DIR_TEXT.
extern int _far DirPopd_Cmd( LPTSTR );
int _near Popd_Cmd( LPTSTR pszCmdLine )
{
	return DirPopd_Cmd( pszCmdLine );
}

// Near command-table entry; implementation resides in DIR_TEXT.
extern int _far DirDirs_Cmd( LPTSTR );
int _near Dirs_Cmd( LPTSTR pszCmdLine )
{
	return DirDirs_Cmd( pszCmdLine );
}

// Near command-table entry; implementation resides in DIR_TEXT.
extern int _far DirRd_Cmd( LPTSTR );
int _near Rd_Cmd( LPTSTR pszCmdLine )
{
	return DirRd_Cmd( pszCmdLine );
}

// ENV_TEXT command dispatchers

// Near command-table entry; implementation resides in ENV_TEXT.
extern int _far EnvAlias_Cmd( LPTSTR );
int _near Alias_Cmd( LPTSTR pszCmdLine )
{
	return EnvAlias_Cmd( pszCmdLine );
}

// Near command-table entry; implementation resides in ENV_TEXT.
extern int _far EnvFunction_Cmd( LPTSTR );
int _near Function_Cmd( LPTSTR pszCmdLine )
{
	return EnvFunction_Cmd( pszCmdLine );
}

// Near command-table entry; implementation resides in ENV_TEXT.
extern int _far EnvSet_Cmd( LPTSTR );
int _near Set_Cmd( LPTSTR pszCmdLine )
{
	return EnvSet_Cmd( pszCmdLine );
}

// Near command-table entry; implementation resides in ENV_TEXT.
extern int _far EnvUnalias_Cmd( LPTSTR );
int _near Unalias_Cmd( LPTSTR pszCmdLine )
{
	return EnvUnalias_Cmd( pszCmdLine );
}

// Near command-table entry; implementation resides in ENV_TEXT.
extern int _far EnvUnfunction_Cmd( LPTSTR );
int _near Unfunction_Cmd( LPTSTR pszCmdLine )
{
	return EnvUnfunction_Cmd( pszCmdLine );
}

// Near command-table entry; implementation resides in ENV_TEXT.
extern int _far EnvUnset_Cmd( LPTSTR );
int _near Unset_Cmd( LPTSTR pszCmdLine )
{
	return EnvUnset_Cmd( pszCmdLine );
}

// Near command-table entry; implementation resides in ENV_TEXT.
extern int _far EnvEset_Cmd( LPTSTR );
int _near Eset_Cmd( LPTSTR pszCmdLine )
{
	return EnvEset_Cmd( pszCmdLine );
}

// BATCH_TEXT command dispatchers

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

// Near command-table entry; implementation resides in BATCH_TEXT.
extern int _far BatchCall_Cmd( LPTSTR );
int _near Call_Cmd( LPTSTR pszCmdLine )
{
	return BatchCall_Cmd( pszCmdLine );
}

// Near command-table entry; implementation resides in BATCH_TEXT.
extern int _far BatchRet_Cmd( LPTSTR );
int _near Ret_Cmd( LPTSTR pszCmdLine )
{
	return BatchRet_Cmd( pszCmdLine );
}

// Near command-table entry; implementation resides in BATCH_TEXT.
extern int _far BatchCancel_Cmd( LPTSTR );
int _near Cancel_Cmd( LPTSTR pszCmdLine )
{
	return BatchCancel_Cmd( pszCmdLine );
}

// Near command-table entry; implementation resides in BATCH_TEXT.
extern int _far BatchQuit_Cmd( LPTSTR );
int _near Quit_Cmd( LPTSTR pszCmdLine )
{
	return BatchQuit_Cmd( pszCmdLine );
}

// Near command-table entry; implementation resides in BATCH_TEXT.
extern int _far BatchGoto_Cmd( LPTSTR );
int _near Goto_Cmd( LPTSTR pszCmdLine )
{
	return BatchGoto_Cmd( pszCmdLine );
}

// Near command-table entry; implementation resides in BATCH_TEXT.
extern int _far BatchGosub_Cmd( LPTSTR );
int _near Gosub_Cmd( LPTSTR pszCmdLine )
{
	return BatchGosub_Cmd( pszCmdLine );
}

// Near command-table entry; implementation resides in BATCH_TEXT.
extern int _far BatchFor_Cmd( LPTSTR );
int _near For_Cmd( LPTSTR pszCmdLine )
{
	return BatchFor_Cmd( pszCmdLine );
}

// Near command-table entry; implementation resides in BATCH_TEXT.
extern int _far BatchDo_Cmd( LPTSTR );
int _near Do_Cmd( LPTSTR pszCmdLine )
{
	return BatchDo_Cmd( pszCmdLine );
}

// Near command-table entry; implementation resides in BATCH_TEXT.
extern int _far BatchIf_Cmd( LPTSTR );
int _near If_Cmd( LPTSTR pszCmdLine )
{
	return BatchIf_Cmd( pszCmdLine );
}

// Near command-table entry; implementation resides in BATCH_TEXT.
extern int _far BatchIff_Cmd( LPTSTR );
int _near Iff_Cmd( LPTSTR pszCmdLine )
{
	return BatchIff_Cmd( pszCmdLine );
}

// Near command-table entry; implementation resides in BATCH_TEXT.
extern int _far BatchSwitch_Cmd( LPTSTR );
int _near Switch_Cmd( LPTSTR pszCmdLine )
{
	return BatchSwitch_Cmd( pszCmdLine );
}

// Near command-table entry; implementation resides in BATCH_TEXT.
extern int _far BatchCase_Cmd( LPTSTR );
int _near Case_Cmd( LPTSTR pszCmdLine )
{
	return BatchCase_Cmd( pszCmdLine );
}

// Near command-table entry; implementation resides in BATCH_TEXT.
extern int _far BatchSetlocal_Cmd( LPTSTR );
int _near Setlocal_Cmd( LPTSTR pszCmdLine )
{
	return BatchSetlocal_Cmd( pszCmdLine );
}

// Near command-table entry; implementation resides in BATCH_TEXT.
extern int _far BatchEndlocal_Cmd( LPTSTR );
int _near Endlocal_Cmd( LPTSTR pszCmdLine )
{
	return BatchEndlocal_Cmd( pszCmdLine );
}

// Miscellaneous command entry points

// REM - do nothing
int _near Remark_Cmd( LPTSTR pszCmdLine )
{
	return 0;
}
