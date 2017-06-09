/****************************************************************************
*
*                            Open Watcom Project
*
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
* Description:  WHEN YOU FIGURE OUT WHAT THIS FILE DOES, PLEASE
*               DESCRIBE IT HERE!
*
****************************************************************************/


#include "dbgdefn.h"
#include "dbgdata.h"
#include "dbgwind.h"
#include "dbgerr.h"
#include "dbgmem.h"
#include "dbgio.h"
#include "guidlg.h"
#include "dlgcmd.h"
#include "strutil.h"
#include "dbgscan.h"
#include "dbgutil.h"
#include "dbgcmd.h"
#include "dbginit.h"
#include "dlgscomp.h"


typedef struct dlg_command {
    char            cmd[TXT_LEN];
} dlg_command;

extern void             DlgSetHistory( gui_window *gui, void *history, char *cmd, int edit, int list );
extern bool             DlgHistoryKey( gui_window *gui, void *param, int edit, int list );
extern void             DlgClickHistory( gui_window *gui, int edit, int list );


static bool CmdEvent( gui_window *gui, gui_event gui_ev, void *param )
{
    dlg_command *dlg;
    char        *text;
    gui_ctl_id  id;

    dlg = GUIGetExtra( gui );
    switch( gui_ev ) {
    case GUI_INIT_DIALOG:
#ifdef __OS2__
        GUILimitEditText( gui, CTL_CMD_EDIT, TXT_LEN ); // allow more than 32 chars input
#endif
        DlgSetHistory( gui, CmdHistory, dlg->cmd, CTL_CMD_EDIT, CTL_CMD_LIST );
        GUISetFocus( gui, CTL_CMD_EDIT );
        return( true );
    case GUI_KEY_CONTROL:
        return( DlgHistoryKey( gui, param, CTL_CMD_EDIT, CTL_CMD_LIST ) );
    case GUI_CONTROL_DCLICKED:
    case GUI_CONTROL_CLICKED:
        GUI_GETID( param, id );
        switch( id ) {
        case CTL_CMD_LIST:
            DlgClickHistory( gui, CTL_CMD_EDIT, CTL_CMD_LIST );
            if( gui_ev == GUI_CONTROL_CLICKED )
                return( true );
            /* fall through */
        case CTL_CMD_SYMBOL:
            SymComplete( gui, CTL_CMD_EDIT );
            return( true );
        case CTL_CMD_OK:
            text = GUIGetText( gui, CTL_CMD_EDIT );
            if( text != NULL ) {
                if( text[0] != NULLCHAR )
                    WndSaveToHistory( CmdHistory, text );
                DoCmd( DupStr( text ) );
                GUIMemFree( text );
            }
            break;
        }
        GUICloseDialog( gui );
        return( true );
    case GUI_DESTROY:
        WndFree( dlg );
        return( true );
    default:
        return( false );
    }
}


void    DlgCmd( void )
{
    dlg_command *dlg;

    dlg = WndMustAlloc( sizeof( dlg_command ) );
    ResDlgOpen( &CmdEvent, dlg, DIALOG_CMD );
}
