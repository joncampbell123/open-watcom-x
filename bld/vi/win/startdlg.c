/****************************************************************************
*
*                            Open Watcom Project
*
* Copyright (c) 2015-2016 The Open Watcom Contributors. All Rights Reserved.
*    Portions Copyright (c) 1983-2002 Sybase, Inc. All Rights Reserved.
*
*  ========================================================================
*
*    This file contains Original Code and/or Modifications of Original
*    Code as defined in and that are subject to the Sybase Open Watcom
*    Public License version 1.0 (the 'License'). You may not use this file
*    except in compliance with the License. BY USING THIS FILE YOU AGREE TO
*    ALL TERMS AND CONDITIONS OF THE LICENSE. A copy of the License is
*    provided with the Original Code and Modifications, and is also
*    available at www.sybase.com/developer/opensource.
*
*    The Original Code and all software distributed under the License are
*    distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
*    EXPRESS OR IMPLIED, AND SYBASE AND ALL CONTRIBUTORS HEREBY DISCLAIM
*    ALL SUCH WARRANTIES, INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF
*    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR
*    NON-INFRINGEMENT. Please see the License for the specific language
*    governing rights and limitations under the License.
*
*  ========================================================================
*
* Description:  Startup splash screen.
*
****************************************************************************/


#include "vi.h"
#include "startup.h"
#include "banner.h"
#include "wclbproc.h"


/* Allow Easy Flash Screen suppression */
//#define NOSPLASH

#ifndef NOSPLASH

/* Local Windows CALLBACK function prototypes */
WINEXPORT INT_PTR CALLBACK StartupDlgProc( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam );

/*
 * StartupDlgProc - callback routine for startup modeless dialog
 */
WINEXPORT INT_PTR CALLBACK StartupDlgProc( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam )
{
    lparam = lparam;
    wparam = wparam;
    hwnd = hwnd;
    switch( msg ) {
        RECT        r;
        int         maxx, maxy;
        int         width;
        int         height;
        int         newx, newy;
        static char vers[] = banner1p2( _VI_VERSION_ );

    case WM_INITDIALOG:
        GetWindowRect( hwnd, &r );
        maxx = GetSystemMetrics( SM_CXSCREEN );
        maxy = GetSystemMetrics( SM_CYSCREEN );
        width = r.right - r.left;
        height = r.bottom - r.top;
        newx = (maxx - width) / 2;
        newy = (maxy - height) / 2;
        SetWindowPos( hwnd, HWND_TOPMOST, newx, newy, 0, 0, SWP_NOSIZE );
        SetDlgItemText( hwnd, STARTUP_VERSION, vers );
        return( TRUE );
    }
    return( FALSE );

} /* StartupDlgProc */

static HWND     startDlgWindow;
static DLGPROC  startDlgProc;

#endif

/*
 * ShowStartupDialog - show the startup dialog
 */
void ShowStartupDialog( void )
{
#ifndef NOSPLASH
    startDlgProc = MakeProcInstance_DLG( StartupDlgProc, InstanceHandle );
    startDlgWindow = CreateDialog( InstanceHandle, "Startup", (HWND)NULLHANDLE, startDlgProc );
#endif

} /* ShowStartupDialog */


/*
 * CloseStartupDialog - close the startup dialog
 */
void CloseStartupDialog( void )
{
#ifndef NOSPLASH
    if( BAD_ID( startDlgWindow ) ) {
        return;
    }
    DestroyWindow( startDlgWindow );
    startDlgWindow = NO_WINDOW;
    FreeProcInstance_DLG( startDlgProc );
#endif

} /* CloseStartupDialog */
