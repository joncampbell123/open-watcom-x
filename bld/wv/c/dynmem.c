/****************************************************************************
*
*                            Open Watcom Project
*
* Copyright (c) 2002-2018 The Open Watcom Contributors. All Rights Reserved.
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
* Description:  Dynamic memory management routines for debugger.
*
****************************************************************************/


#include "dbglit.h"
#include <stdlib.h>
#ifdef _M_IX86
    #include <i86.h>
#endif
#ifdef __WATCOMC__
    /* it's important that <malloc> is included up here */
    #define __fmemneed foo
    #define __nmemneed bar
    #include <malloc.h>
    #undef __nmemneed
    #undef __fmemneed
#endif
#include "dbgdefn.h"
#include "dbgdata.h"
#include "dbgerr.h"
#include "dbgio.h"
#include "dui.h"
#ifdef TRMEM
#else
    #define TRMemAlloc(x)          malloc(x)
    #define TRMemRealloc(p,x)      realloc(p,x)
    #define TRMemFree(p)           free(p)
#endif
#include "dip.h"
#include "strutil.h"
#include "dbginit.h"

#include "guimem.h"
#if defined( GUI_IS_GUI )
    #include "cguimem.h"
    #include "wpimem.h"
    #ifdef __OS2__
        #include "os2mem.h"
    #endif
#else
    #include "stdui.h"
    #include "helpmem.h"
#endif
#ifdef TRMEM
    #include "trmem.h"
#endif


#if defined( __DOS__ )
#if !defined( __OSI__ )
extern int _d16ReserveExt( int );
#pragma aux _d16ReserveExt = \
        "mov cx,ax" \
        "shr eax,16" \
        "mov bx,ax" \
        "mov dx,1400H" \
        "mov ax,0ff00H" \
        "int 21H" \
        "ror eax,16" \
        "mov ax,dx" \
        "ror eax,16" \
    __parm      [__eax] \
    __value     [__eax] \
    __modify    [__ebx __ecx __edx]
#endif
#endif

#ifdef __WATCOMC__

#ifdef __386__
#define __fmemneed __nmemneed
#endif
extern int __saveregs __fmemneed( size_t size );

int __saveregs __fmemneed( size_t size )
{
    if( DIPMoreMem( size ) == DS_OK )
        return( true );
    if( DUIInfoRelease() )
        return( true );
    return( false );
}
#endif


#ifdef TRMEM

static FILE             *TrackFile = NULL;
static _trmem_hdl       TRMemHandle;


/* extern to avoid problems with taking address and overlays */
static bool Closing = false;

static void TRPrintLine( void *parm, const char *buff, size_t len )
/*****************************************************************/
{
    /* unused parameters */ (void)parm;

    if( !Closing )
        PopErrBox( buff );
    fwrite( buff, 1, len, TrackFile );
}

static void TRMemOpen( void )
/***************************/
{
    TRMemHandle = _trmem_open( malloc, free, realloc, NULL,
            NULL, TRPrintLine,
            _TRMEM_ALLOC_SIZE_0 | _TRMEM_REALLOC_SIZE_0 |
            _TRMEM_OUT_OF_MEMORY | _TRMEM_CLOSE_CHECK_FREE );
}

static void TRMemClose( void )
/****************************/
{
    _trmem_close( TRMemHandle );
}

static void * TRMemAlloc( size_t size )
/*************************************/
{
    return( _trmem_alloc( size, _trmem_guess_who(), TRMemHandle ) );
}

static void TRMemFree( void * ptr )
/*********************************/
{
    _trmem_free( ptr, _trmem_guess_who(), TRMemHandle );
}

static void * TRMemRealloc( void * ptr, size_t size )
/***************************************************/
{
    return( _trmem_realloc( ptr, size, _trmem_guess_who(), TRMemHandle ) );
}


void TRMemPrtUsage( void )
/************************/
{
    _trmem_prt_usage( TRMemHandle );
}

static unsigned TRMemPrtList( void )
/**********************************/
{
    return( _trmem_prt_list( TRMemHandle ) );
}

int TRMemValidate( void * ptr )
/*****************************/
{
    return( _trmem_validate( ptr, _trmem_guess_who(), TRMemHandle ) );
}

void TRMemCheck( void )
/*********************/
{
    _trmem_validate_all( TRMemHandle );
}

int TRMemChkRange( void * start, size_t len )
/*******************************************/
{
    return( _trmem_chk_range( start, len, _trmem_guess_who(), TRMemHandle ) );
}

static void MemTrackInit( void )
{
    char        name[FILENAME_MAX];

    TrackFile = stderr;
    if( DUIEnvLkup( "TRMEMFILE", name, sizeof( name ) ) ) {
        TrackFile = fopen( name, "w" );
    }
    TRMemOpen();
}

static const char UnFreed[] = { "Memory UnFreed" };
static const char TrackErr[] = { "Memory Tracker Errors Detected" };

static void MemTrackFini( void )
{
    Closing = true;
    if( TrackFile != stderr ) {
        fseek( TrackFile, 0, SEEK_END );
        if( ftell( TrackFile ) != 0 ) {
            PopErrBox( TrackErr );
        } else if( TRMemPrtList() != 0 ) {
            PopErrBox( UnFreed );
        }
        fclose( TrackFile );
    }
    TRMemClose();
}
#endif


/*
 * Dynamic Memory management routines
 */

void *DbgAlloc( size_t size )
{
    return( TRMemAlloc( size ) );
}

void *DbgMustAlloc( size_t size )
{
    void        *ptr;

    ptr = DbgAlloc( size );
    if( ptr == NULL ) {
        Error( ERR_NONE, LIT_ENG( ERR_NO_MEMORY ) );
    }
    return( ptr );
}

void *DbgRealloc( void *chunk, size_t size )
{
    return( TRMemRealloc( chunk, size ) );
}

void DbgFree( void *ptr )
{
    if( ptr != NULL ) {
        TRMemFree( ptr );
    }
}

void *ChkAlloc( size_t size, char *error )
{
    void *ret;

    ret = TRMemAlloc( size );
    if( ret == NULL )
        Error( ERR_NONE, error );
    return( ret );
}

#if defined( __DOS__ ) || defined( __NOUI__ )

#if defined( _M_I86 )
#define MAX_BLOCK (60U * 1024)
#elif defined( __DOS__ )
#define MAX_BLOCK (4U*1024*1024)
#else
#define MAX_BLOCK (1U*1024*1024)
#endif

static void MemExpand( void )
{
    unsigned long   size;
    void            **link;
    void            **p;
    size_t          alloced;

    if( MemSize == ~0 )
        return;
    link = NULL;
    alloced = MAX_BLOCK;
    for( size = MemSize; size > 0; size -= alloced ) {
        if( size < MAX_BLOCK )
            alloced = size;
        p = TRMemAlloc( alloced );
        if( p != NULL ) {
            *p = link;
            link = p;
        }
    }
    while( link != NULL ) {
        p = *link;
        TRMemFree( link );
        link = p;
    }
}
#endif

void SysSetMemLimit( void )
{
#if defined( __DOS__ )
#if !defined(__OSI__)
    _d16ReserveExt( MemSize + 1*1024UL*1024UL );
#endif
    MemExpand();
    if( _IsOff( SW_REMOTE_LINK ) && _IsOff( SW_KEEP_HEAP_ENABLED ) ) {
        _heapenable( 0 );
    }
#endif
}

#if defined( __NOUI__ )

void MemInit( void )
{
#ifdef TRMEM
    MemTrackInit();
#endif
    MemExpand();
}

void MemFini( void )
{
#ifdef TRMEM
    MemTrackFini();
#elif defined( __WATCOMC__ )
    static const char   Heap_Corupt[] = { "ERROR - Heap is corrupted - %s" };
    struct _heapinfo    h_info;
    int                 status;
    char                buf[50];
    char                *end;

    if( getenv( "TRMEMFILE" ) == NULL )
        return;
    h_info._pentry = NULL;
    while( (status = _heapwalk( &h_info )) == _HEAPOK ) {
#ifndef NDEBUG
        if( h_info._useflag == _USEDENTRY ) {
            end = Format( buf, "%s block",
                h_info._useflag == _USEDENTRY ? "Used" : "Free" );
            WriteText( STD_OUT, buf, end - buf );
        }
#endif
    }
    switch( status ) {
    case _HEAPBADBEGIN:
        end = Format( buf, Heap_Corupt, "bad header info" );
        WriteText( STD_OUT, buf, end - buf );
        break;
    case _HEAPBADPTR:
        end = Format( buf, Heap_Corupt, "bad pointer" );
        WriteText( STD_OUT, buf, end - buf );
        break;
    case _HEAPBADNODE:
        end = Format( buf, Heap_Corupt, "bad node" );
        WriteText( STD_OUT, buf, end - buf );
        break;
    default:
        break;
    }
#endif
}

#else   /* !defined( __NOUI__ ) */

#ifdef TRMEM
static _trmem_hdl  GUIMemHandle;

static FILE *GUIMemFP = NULL;   /* stream to put output on */
static int  GUIMemOpened = 0;

static void GUIMemPrintLine( void *parm, const char *buff, size_t len )
/*********************************************************************/
{
    /* unused parameters */ (void)parm;

    fwrite( buff, 1, len, GUIMemFP );
}

#endif

void GUIMemPrtUsage( void )
/*************************/
{
#ifdef TRMEM
    _trmem_prt_usage( GUIMemHandle );
#endif
}

void GUIMemRedirect( FILE *fp )
/*****************************/
{
#ifdef TRMEM
    GUIMemFP = fp;
#else
    /* unused parameters */ (void)fp;
#endif
}

void GUIMemOpen( void )
/*********************/
{
#ifdef TRMEM
    char * tmpdir;

    if( !GUIMemOpened ) {
        GUIMemFP = stderr;
        GUIMemHandle = _trmem_open( malloc, free, realloc, NULL,
            NULL, GUIMemPrintLine,
            _TRMEM_ALLOC_SIZE_0 | _TRMEM_REALLOC_SIZE_0 |
            _TRMEM_OUT_OF_MEMORY | _TRMEM_CLOSE_CHECK_FREE );

        tmpdir = getenv( "TRMEMFILE" );
        if( tmpdir != NULL ) {
            GUIMemFP = fopen( tmpdir, "w" );
        }
        GUIMemOpened = 1;
    }
#endif
}
#if !defined( GUI_IS_GUI )
void UIAPI UIMemOpen( void ) {}
#endif

void GUIMemClose( void )
/**********************/
{
#ifdef TRMEM
    _trmem_prt_list( GUIMemHandle );
    _trmem_close( GUIMemHandle );
    if( GUIMemFP != stderr ) {
        fclose( GUIMemFP );
    }
#endif
}
#if !defined( GUI_IS_GUI )
void UIAPI UIMemClose( void ) {}
#endif


/*
 * Alloc functions
 */

void *GUIMemAlloc( size_t size )
/******************************/
{
#ifdef TRMEM
    return( _trmem_alloc( size, _trmem_guess_who(), GUIMemHandle ) );
#else
    return( malloc( size ) );
#endif
}
#if defined( GUI_IS_GUI )
void *MemAlloc( size_t size )
{
    void        *ptr;

#ifdef TRMEM
    ptr = _trmem_alloc( size, _trmem_guess_who(), GUIMemHandle );
#else
    ptr = malloc( size );
#endif
    memset( ptr, 0, size );
    return( ptr );
}
void * _wpi_malloc( size_t size )
{
#ifdef TRMEM
    return( _trmem_alloc( size, _trmem_guess_who(), GUIMemHandle ) );
#else
    return( malloc( size ) );
#endif
}
#ifdef __OS2__
void *PMmalloc( size_t size )
{
#ifdef TRMEM
    return( _trmem_alloc( size, _trmem_guess_who(), GUIMemHandle ) );
#else
    return( malloc( size ) );
#endif
}
#endif
#else
void * UIAPI uimalloc( size_t size )
{
#ifdef TRMEM
    return( _trmem_alloc( size, _trmem_guess_who(), GUIMemHandle ) );
#else
    return( malloc( size ) );
#endif
}
LP_VOID UIAPI uifaralloc( size_t size )
{
#ifdef TRMEM
    return( _trmem_alloc( size, _trmem_guess_who(), GUIMemHandle ) );
#else
    return( malloc( size ) );
#endif
}
void *HelpMemAlloc( size_t size )
{
#ifdef TRMEM
    return( _trmem_alloc( size, _trmem_guess_who(), GUIMemHandle ) );
#else
    return( malloc( size ) );
#endif
}
#endif


/*
 * Free functions
 */

void GUIMemFree( void *ptr )
/**************************/
{
#ifdef TRMEM
    _trmem_free( ptr, _trmem_guess_who(), GUIMemHandle );
#else
    free( ptr );
#endif
}
#if defined( GUI_IS_GUI )
void MemFree( void *ptr )
{
#ifdef TRMEM
    _trmem_free( ptr, _trmem_guess_who(), GUIMemHandle );
#else
    free( ptr );
#endif
}
void _wpi_free( void *ptr )
{
#ifdef TRMEM
    _trmem_free( ptr, _trmem_guess_who(), GUIMemHandle );
#else
    free( ptr );
#endif
}
#ifdef __OS2__
void PMfree( void *ptr )
{
#ifdef TRMEM
    _trmem_free( ptr, _trmem_guess_who(), GUIMemHandle );
#else
    free( ptr );
#endif
}
#endif
#else
void UIAPI uifree( void *ptr )
{
#ifdef TRMEM
    _trmem_free( ptr, _trmem_guess_who(), GUIMemHandle );
#else
    free( ptr );
#endif
}
#if defined( __DOS__ )
#define MEM_NEAR_PTR(x)     (void *)FP_OFF( x )
#else
#define MEM_NEAR_PTR(x)     x
#endif
void UIAPI uifarfree( LP_VOID ptr )
{
    if( ptr != NULL ) {
#ifdef TRMEM
        _trmem_free( MEM_NEAR_PTR( ptr ), _trmem_guess_who(), GUIMemHandle );
#else
        free( MEM_NEAR_PTR( ptr ) );
#endif
    }
}
void HelpMemFree( void *ptr )
{
#ifdef TRMEM
    _trmem_free( ptr, _trmem_guess_who(), GUIMemHandle );
#else
    free( ptr );
#endif
}
#endif


/*
 * Realloc functions
 */

void *GUIMemRealloc( void *ptr, size_t size )
/*******************************************/
{
#ifdef TRMEM
    return( _trmem_realloc( ptr, size, _trmem_guess_who(), GUIMemHandle ) );
#else
    return( realloc( ptr, size ) );
#endif
}
#if defined( GUI_IS_GUI )
void * _wpi_realloc( void *ptr, size_t size )
{
#ifdef TRMEM
    return( _trmem_realloc( ptr, size, _trmem_guess_who(), GUIMemHandle ) );
#else
    return( realloc( ptr, size ) );
#endif
}
void *MemRealloc( void *ptr, size_t size )
{
#ifdef TRMEM
    return( _trmem_realloc( ptr, size, _trmem_guess_who(), GUIMemHandle ) );
#else
    return( realloc( ptr, size ) );
#endif
}
#ifdef __OS2__
void *PMrealloc( void *ptr, size_t size )
{
#ifdef TRMEM
    return( _trmem_realloc( ptr, size, _trmem_guess_who(), GUIMemHandle ) );
#else
    return( realloc( ptr, size ) );
#endif
}
#endif
#else
void * UIAPI uirealloc( void *ptr, size_t size )
{
#ifdef TRMEM
    return( _trmem_realloc( ptr, size, _trmem_guess_who(), GUIMemHandle ) );
#else
    return( realloc( ptr, size ) );
#endif
}
void *HelpMemRealloc( void *ptr, size_t size )
{
#ifdef TRMEM
    return( _trmem_realloc( ptr, size, _trmem_guess_who(), GUIMemHandle ) );
#else
    return( realloc( ptr, size ) );
#endif
}
#endif

#endif  /* !defined( __NOUI__ ) */
