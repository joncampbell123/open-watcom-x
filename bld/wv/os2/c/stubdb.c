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
* Description:  Debugger stub functions.
*
****************************************************************************/


#include <stdio.h>
#include <ctype.h>
#include <process.h>
#define INCL_DOS
#include <os2.h>
#include "_srcmgt.h"
#include "dbgdata.h"
#include "dbgmem.h"
#include "dbglit.h"
#include "mad.h"
#include "dui.h"
#include "dbgvar.h"
#include "dbgstk.h"
#include "srcmgt.h"
#include "trapaccs.h"
#include "dbgscrn.h"
#include "strutil.h"
#include "dbgscan.h"
#include "dbgutil.h"
#include "dbgsrc.h"
#include "dbgexec.h"
#include "dbgexpr2.h"
#include "dbgmain.h"
#include "dbgbrk.h"
#include "dbgass.h"
#include "dbgpend.h"
#include "envlkup.h"
#include "dbgcmd.h"
#include "dbgtrace.h"
#include "trpld.h"
#include "dipimp.h"
#include "dipinter.h"
#include "dbgdot.h"
#include "dlgcmd.h"
#include "dbgwintr.h"

/*************************************************/
/* TODO! review all these prototypes declaration if they are local(static) or external */

bool DlgNewWithSym( const char *title, char *buff, int buff_len );
bool DlgUpTheStack( void );
bool DlgAreYouNuts( unsigned long mult );
bool DlgBackInTime( bool warn );
bool DlgIncompleteUndo( void );
bool DlgBreak( address addr );
void ProcAccel( void );
void ProcDisplay( void );
void ProcFont( void );
void ProcHelp( void );
#ifndef NDEBUG
void ProcInternal( void );
#endif
void ProcPaint( void );
void ProcView( void );
void ProcConfigFile( void );
void ConfigDisp( void );
void ConfigFont( void );
void ConfigPaint( void );
int TabIntervalGet( void );
void TabIntervalSet( int new );
void VarSaveWndToScope( void *wnd );
void VarRestoreWndFromScope( void *wnd );
void PopErrBox( const char *buff );
/*************************************************/

extern void             *WndAsmInspect( address addr );

static bool             Done;
extern char             *CmdData;
extern stack_entry      *ExprSP;

unsigned                NumLines;
unsigned                NumColumns;

unsigned DUIConfigScreen( void )
{
    return( 0 );
}

bool DUIClose( void )
{
    Done = true;
    return( true );
}

#if 0
// The following routine is cut & pasted verbatim from dbgwvar.c
// (which we really don't want to drag in here)
var_node *VarGetDisplayPiece( var_info *i, int row, int piece, int *pdepth, int *pinherit )
{
    var_node    *row_v;
    var_node    *v;

    if( piece >= VAR_PIECE_LAST ) return( NULL );
    if( VarFirstNode( i ) == NULL ) return( NULL );
    if( row >= VarRowTotal( i ) ) return( NULL );
    row_v = VarFindRowNode( i, row );
    if( !row_v->value_valid ) {
        VarSetValue( row_v, LIT_ENG( Quest_Marks ) );
        row_v->value_valid = false;
    }
    if( !row_v->gadget_valid ) {
        VarSetGadget( row_v, VARGADGET_NONE );
        row_v->gadget_valid = false;
    }
    v = row_v;
    if( piece == VAR_PIECE_NAME ||
        ( piece == VAR_PIECE_GADGET && row_v->gadget_valid ) ||
        ( piece == VAR_PIECE_VALUE && row_v->value_valid ) ) {
        VarError = false;
    } else if( _IsOff( SW_TASK_RUNNING ) ) {
        if( row == i->exprsp_cacherow && i->exprsp_cache != NULL ) {
            VarError = false;
            v = i->exprsp_cache;
        } else if( row == i->exprsp_cacherow && i->exprsp_cache_is_error ) {
            VarError = true;
            v = NULL;
        } else {
            VarErrState();
            v = VarFindRow( i, row );
            VarOldErrState();
            i->exprsp_cacherow = row;
            i->exprsp_cache = v;
            i->exprsp_cache_is_error = VarError;
        }
        if( v == NULL ) {
            if( !VarError ) return( NULL );
            v = row_v;
        }
        VarNodeInvalid( v );
        VarErrState();
        ExprValue( ExprSP );
        VarSetGadget( v, VarGetGadget( v ) );
        VarSetOnTop( v, VarGetOnTop( v ) );
        VarSetValue( v, VarGetValue( i, v ) );
        VarOldErrState();
        VarDoneRow( i );
    }
    VarGetDepths( i, v, pdepth, pinherit );
    return( v );
}
#endif

var_info        Locals;
HEV             Requestsem;
HEV             Requestdonesem;

static void DumpLocals( void )
{
    address     addr;
    int         row;
    int         depth;
    int         inherit;
    var_node    *v;

    if( _IsOff( SW_TASK_RUNNING ) ) {
        VarErrState();
        VarInfoRefresh( VAR_LOCALS, &Locals, &addr, NULL );
        VarOkToCache( &Locals, true );
    }
    for( row = 0; (v = VarGetDisplayPiece( &Locals, row, VAR_PIECE_GADGET, &depth, &inherit )) != NULL; ++row ) {
        v = VarGetDisplayPiece( &Locals, row, VAR_PIECE_NAME, &depth, &inherit );
        v = VarGetDisplayPiece( &Locals, row, VAR_PIECE_VALUE, &depth, &inherit );
        switch( v->gadget ) {
        case VARGADGET_NONE:
            printf( "  " );
            break;
        case VARGADGET_OPEN:
            printf( "+ " );
            break;
        case VARGADGET_CLOSED:
            printf( "- " );
            break;
        case VARGADGET_POINTS:
            printf( "->" );
            break;
        case VARGADGET_UNPOINTS:
            printf( "<-" );
            break;
        }
        VarBuildName( &Locals, v, true );
        printf( " %-20s %s\n", TxtBuff, v->value );
    }
    if( _IsOff( SW_TASK_RUNNING ) ) {
        VarOkToCache( &Locals, false );
        VarOldErrState();
    }
}

static void DumpSource( void )
{
    char        buff[256];
    DIPHDL( cue, ch );

    if( _IsOn( SW_TASK_RUNNING ) ) {
        printf( "I don't know where the task is. It's running\n" );
    }
    if( DeAliasAddrCue( NO_MOD, GetCodeDot(), ch ) == SR_NONE ||
        !DUIGetSourceLine( ch, buff, sizeof( buff ) ) ) {
        UnAsm( GetCodeDot(), buff, sizeof( buff ) );
    }
    printf( "%s\n", buff );
}

enum {
    REQ_NONE,
    REQ_BYE,
    REQ_GO,
    REQ_TRACE_OVER,
    REQ_TRACE_INTO
} Req = REQ_NONE;

bool RequestDone;

static void APIENTRY ControlFunc( ULONG parm )
{
    ULONG   ulCount;

    parm = parm;
    do {
        DosWaitEventSem( Requestsem, SEM_INDEFINITE_WAIT ); // wait for Request
        DosResetEventSem( Requestsem, &ulCount );
        switch( Req ) {
        case REQ_GO:
            Go( true );
            break;
        case REQ_TRACE_OVER:
            ExecTrace( TRACE_OVER, DbgLevel );
            break;
        case REQ_TRACE_INTO:
            ExecTrace( TRACE_INTO, DbgLevel );
            break;
        }
        DoInput();
        _SwitchOff( SW_TASK_RUNNING );
        DosPostEventSem( Requestdonesem );
    } while( Req != REQ_BYE );
    return; // thread over!
}

static void RunRequest( int req )
{
    ULONG   ulCount;

    if( _IsOn( SW_TASK_RUNNING ) ) return;
    DosWaitEventSem( Requestdonesem, SEM_INDEFINITE_WAIT ); // wait for last request to finish
    DosResetEventSem( Requestdonesem, &ulCount );
    Req = req;
    _SwitchOn( SW_TASK_RUNNING );
    DosPostEventSem( Requestsem ); // tell worker to go
}

void DlgCmd( void )
{
    char        buff[256];

    printf( "DBG>" );
    fflush( stdout );   // not really necessary
    gets( buff );
    if( buff[0] != NULLCHAR && buff[1] == NULLCHAR ) {
        switch( tolower( buff[0] ) ) {
        case 'u':
            WndAsmInspect( GetCodeDot() );
            break;
        case 's':
            DumpSource();
            break;
        case 'l':
            DumpLocals();
            break;
        case 'i':
            RunRequest( REQ_TRACE_INTO );
            break;
        case 'o':
            RunRequest( REQ_TRACE_OVER );
            break;
        case 'g':
            RunRequest( REQ_GO );
            break;
        case 'x':
            if( _IsOn( SW_REMOTE_LINK ) ) {
                printf( "Can't break remote task!\n" );
            } else {
                HMODULE hmod;
                PFN     proc = NULL;

                DosQueryModuleHandle( TrapParms, &hmod );
                DosQueryProcAddr( hmod, 5, 0, &proc );
//                if( proc != NULL )
//                    proc();
            }
            // break the task
            break;
        default:
            printf( "Error - unrecognized command\n" );
        }
    } else {
        DoCmd( DupStr( buff ) );
        DoInput();
    }
}

int main( int argc, char **argv )
{
    char        buff[256];
    TID         tid;
    APIRET      rc;

    MemInit();
    getcmd( buff );
    CmdData = buff;
    DebugMain();
    _SwitchOff( SW_ERROR_STARTUP );
    DoInput();
    VarInitInfo( &Locals );
    DosCreateEventSem( NULL, &Requestsem, 0, false );
    DosCreateEventSem( NULL, &Requestdonesem, 0, false );
    DosPostEventSem( Requestdonesem ); // signal req done
    rc = DosCreateThread( &tid, ControlFunc, 0, 0, 32768 );
    if( rc != 0 ) {
        printf( "Stubugger: Error creating thread!\n" );
    }
    while( !Done ) {
        DlgCmd();
    }
    DosCloseEventSem( Requestsem );
    DosCloseEventSem( Requestdonesem );
    DebugFini();
    RunRequest( REQ_BYE );
    MemFini();
    return( 0 );
}

// Minimalist DUI callback routines

extern char *DUILoadString( dui_res_id id )
{
    char        buff[256];
    char        *ret;
    int         size;

    size = WinLoadString( 0, NULLHANDLE, id, sizeof( buff ), buff );
//    size = LoadString( GetModuleHandle( NULL ), id, buff, sizeof( buff ) );
    buff[size++] = NULLCHAR;
    ret = DbgAlloc( size );
    memcpy( ret, buff, size );
    return( ret );
}

void DUIMsgBox( const char *text )
{
    printf( "MSG %s\n", text );
}

bool DUIDlgTxt( const char *text )
{
    printf( "DLG %s\n", text );
    return( true );
}

void DUIInfoBox( const char *text )
{
    printf( "INF %s\n", text );
}

void DUIErrorBox( const char *text )
{
    printf( "ERR %s\n", text );
}

void DUIStatusText( const char *text )
{
    printf( "STA %s\n", text );
}

bool DUIDlgGivenAddr( const char *title, address *value )
{
    // needed when segment's don't map (from new/sym command)
    return( false );
}

bool DlgNewWithSym( const char *title, char *buff, int buff_len )
{
    // used by print command with no arguments
    return( true );
}

bool DlgUpTheStack( void )
{
    // used when trying to trace, but we've unwound the stack a bit
    return( false );
}

bool DlgAreYouNuts( unsigned long mult )
{
    // used when too many break on write points are set
    return( false );
}

bool DlgBackInTime( bool warn )
{
    // used when trying to trace, but we've backed up over a call or asynch
    warn = warn;
    return( false );
}

bool DlgIncompleteUndo( void )
{
    // used when trying to trace, but we've backed up over a call or asynch
    return( false );
}

bool DlgBreak( address addr )
{
    // used when an error occurs in the break point expression or it is entered wrong
    return( false );
}

bool DUIInfoRelease( void )
{
    // used when we're low on memory
    return( false );
}
void DUIUpdate( update_list flags )
{
    // flags indicates what conditions have changed.  They should be saved
    // until an appropriate time, then windows updated accordingly
}

void DUIStop( void )
{
    // close down the UI - we're about to change modes.
}

void DUIFini( void )
{
    // finish up the UI
}

void DUIInit( void )
{
    // Init the UI
}

extern void DUIFreshAll( void )
{
    // refresh all screens - initialization has been done
    UpdateFlags = 0;
}

extern bool DUIStopRefresh( bool stop )
{
    // temporarily turn off/on screen refreshing, cause we're going to run a
    // big command file and we don't want flashing.
    return( false );
}

extern void DUIShow( void )
{
    // show the main screen - the splash page has been closed
}

extern void DUIWndUser( void )
{
    // switch to the program screen
}

extern void DUIWndDebug( void )
{
    // switch to the debugger screen
}

extern void DUIShowLogWindow( void )
{
    // bring up the log window, cause some printout is coming
}

extern int DUIGetMonitorType( void )
{
    // stub for old UI
    return( 1 );
}

extern int DUIScreenSizeY( void )
{
    // stub for old UI
    return( 0 );
}

extern int DUIScreenSizeX( void )
{
    // stub for old UI
    return( 0 );
}

extern void DUIArrowCursor( void )
{
    // we're about to suicide, so restore the cursor to normal
}

bool DUIAskIfAsynchOk( void )
{
    // we're about to try to replay across an asynchronous event.  Ask user
    return( false );
}

extern void DUIFlushKeys( void )
{
    // we're about to suicide - clear the keyboard typeahead
}

extern void DUIPlayDead( bool dead )
{
    // the app is about to run - make the debugger play dead
}

extern void DUISysEnd( bool pause )
{
    // done calling system();
}

extern void DUISysStart( void )
{
    // about to call system();
}

extern void DUIRingBell( void )
{
    // ring ring (error)
}

extern int DUIDisambiguate( const ambig_info *ambig, int count )
{
    // the expression processor detected an ambiguous symbol.  Ask user which one
    return( 0 );
}

extern void *DUIHourGlass( void *x )
{
    return( x );
}

void ProcAccel( void )
{
    // stub for old UI
    FlushEOC();
}

void ProcDisplay( void )
{
    // stub for old UI
    FlushEOC();
}

void ProcFont( void )
{
    // stub for old UI
    FlushEOC();
}

void ProcHelp( void )
{
    // stub for old UI
    FlushEOC();
}

#ifndef NDEBUG
void ProcInternal( void )
{
    // stub for old UI
    FlushEOC();
}
#endif

void ProcPaint( void )
{
    // stub for old UI
    FlushEOC();
}

void ProcView( void )
{
    // stub for old UI
    FlushEOC();
}

void DUIProcWindow( void )
{
    // stub for old UI
    FlushEOC();
}

void ProcConfigFile( void )
{
    // called when main config file processed
    FlushEOC();
}

void ConfigDisp( void )
{
    // stub for old UI
}

void ConfigFont( void )
{
    // stub for old UI
}

void ConfigPaint( void )
{
    // stub for old UI
}

extern void DClickSet( void )
{
    // stub for old UI
    FlushEOC();
}

extern void DClickConf( void )
{
    // stub for old UI
}

extern void InputSet( void )
{
    // stub for old UI
    FlushEOC();
}

extern void InputConf( void )
{
    // stub for old UI
}

extern void MacroSet( void )
{
    // stub for old UI
    FlushEOC();
}

extern void MacroConf( void )
{
    // stub for old UI
}

extern  void    FiniMacros( void )
{
    // stub for old UI
}
int TabIntervalGet( void )
{
    // stub for old UI
    return( 0 );
}
void TabIntervalSet( int new )
{
    // stub for old UI
}

extern void TabSet( void )
{
    // stub for old UI
    FlushEOC();
}

extern void TabConf( void )
{
    // stub for old UI
}

extern void SearchSet( void )
{
    // stub for old UI
    FlushEOC();
}

extern void SearchConf( void )
{
    // stub for old UI
}

extern void DUIFingClose( void )
{
    // close the splash page
}

extern void DUIFingOpen( void )
{
    // open a splash page
}

extern void AsmChangeOptions( void )
{
    // assembly window options changed
}

extern void RegChangeOptions( void )
{
    // reg window options changed
}

extern void VarChangeOptions( void )
{
    // var window options changed
}

extern void FuncChangeOptions( void )
{
    // func window options changed
}

extern void GlobChangeOptions( void )
{
    // glob window options changed
}

extern void ModChangeOptions( void )
{
    // mod window options changed
}

extern void WndVarInspect( const char *buff )
{
}

extern void *WndAsmInspect( address addr )
{
    // used by examine/assembly command
    int         i;
    char        buff[256];
    mad_disasm_data     *dd;

    _AllocA( dd, MADDisasmDataSize() );
    for( i = 0; i < 10; ++i ) {
        MADDisasm( dd, &addr, 0 );
        MADDisasmFormat( dd, MDP_ALL, CurrRadix, buff, sizeof( buff ) );
        InsMemRef( dd );
        printf( "%-40s%s\n", buff, TxtBuff );
    }
    return( NULL );
}

extern void *WndSrcInspect( address addr )
{
    // used by examine/source command
    return( NULL );
}

extern void WndMemInspect( address addr, char *next, unsigned len,
                           mad_type_handle type )
{
    // used by examine/byte/word/etc command
}

extern void WndIOInspect(address*addr,mad_type_handle type)
{
    // used by examine/iobyte/ioword/etc command
}

extern void WndTmpFileInspect( const char *file )
{
    // used by capture command
    file = file;
}

extern void GraphicDisplay( void )
{
    // used by print/window command
}

extern void VarUnMapScopes( image_entry *img )
{
    // unmap variable scopes - prog about to restart
    img = img;
}

extern void VarReMapScopes( image_entry *img )
{
    // remap variable scopes - prog about to restart
    img = img;
}

extern void VarFreeScopes( void )
{
    // free variable scope info
}

extern void SetLastExe( const char *name )
{
    // remember last exe debugged name
    name = name;
}

extern void DUIProcPendingPaint( void )
{
    // a paint command was issued - update the screen (stub)
}

void VarSaveWndToScope( void *wnd )
{
}

void VarRestoreWndFromScope( void *wnd )
{
}

void PopErrBox( const char *buff )
{
    printf( "%s: %s\n", buff, LIT_ENG( Debugger_Startup_Error ) );
//    MessageBox( (HWND) NULL, buff, LIT_ENG( Debugger_Startup_Error ),
//            MB_OK | MB_ICONHAND | MB_SYSTEMMODAL );
}

void DUIEnterCriticalSection( void )
{
}

void DUIExitCriticalSection( void )
{
}

void DUIInitLiterals( void )
{
}

void DUIFiniLiterals( void )
{
}

bool DUIGetSourceLine( cue_handle *ch, char *buff, unsigned len )
{
    void        *viewhndl;

    viewhndl = OpenSrcFile( ch );
    if( viewhndl == NULL ) return( false );
    buff[FReadLine( viewhndl, DIPCueLine( ch ), 0, buff, len )] = NULLCHAR;
    FDoneSource( viewhndl );
    return( true );
}

bool DUIIsDBCS( void )
{
    return( false );
}

size_t DUIEnvLkup( const char *name, char *buff, size_t buff_len )
{
    return( EnvLkup( name, buff, buff_len ) );
}

void DUIDirty( void )
{
}


void DUISrcOrAsmInspect( address addr )
{
}

void DUIAddrInspect( address addr )
{
}

extern void DUIRemoveBreak( brkp *bp )
/************************************/
{
    RemovePoint( bp );
}

extern void SetMADMenuItems( void )
/**********************************/
{
}

extern void FPUChangeOptions( void )
/**********************************/
{
}

extern void MMXChangeOptions( void )
/**********************************/
{
}

extern void XMMChangeOptions( void )
/**********************************/
{
}

bool DUIImageLoaded( image_entry *image, bool load,
                     bool already_stopping, bool *force_stop )
/************************************************************/
{
    char buff[256];

    already_stopping=already_stopping;
    force_stop= force_stop;
    if( load ) {
        sprintf( buff, "%s '%s'", LIT_ENG( DLL_Loaded ), image->image_name );
    } else {
        sprintf( buff, "%s '%s'", LIT_ENG( DLL_UnLoaded ), image->image_name );
    }
    DUIDlgTxt( buff );
    return( false );
}

void DUICopySize( void *cookie, unsigned long size )
/**************************************************/
{
    size = size;
    cookie = cookie;
}

void DUICopyCopied( void *cookie, unsigned long size )
/****************************************************/
{
    size = size;
    cookie = cookie;
}

bool DUICopyCancelled( void * cookie )
/************************************/
{
    cookie = cookie;
    return( false );
}

unsigned DUIDlgAsyncRun( void )
/*****************************/
{
    return( 0 );
}

void DUISetNumLines( int num )
{
    num = num;
}

void DUISetNumColumns( int num )
{
    num = num;
}

void DUIInitRunThreadInfo( void )
{
}

void DUIScreenOptInit( void )
{
}

bool DUIScreenOption( const char *start, unsigned len, int pass )
{
    start=start;len=len;pass=pass;
    return( true );
}

#if defined( __GUI__ )
unsigned OnAnotherThreadAccess( trap_elen in_num, in_mx_entry_p in_mx, trap_elen out_num, mx_entry_p out_mx )
{
    return( TrapAccess( in_num, in_mx, out_num, out_mx ) );
}

unsigned OnAnotherThreadSimpAccess( trap_elen in_len, in_data_p in_data, trap_elen out_len, out_data_p out_data )
{
    return( TrapSimpAccess( in_len, in_data, out_len, out_data ) );
}
#endif
