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


#include <stddef.h>
#include <limits.h>
#define INCL_ERRORS
#define INCL_BASE
#include <os2.h>
#include "dbgdefn.h"
#if !defined( BUILD_RFX )
#include "dbgdata.h"
#else
#include "rfxdata.h"
#endif
#include "dbgmem.h"
#include "dbgio.h"
#include "doserr.h"
#include "filelcl.h"

#define READONLY    0
#define WRITEONLY   1
#define READWRITE   2
#define FROMEND     2

const file_components   LclFile = { '.', ':', { '\\', '/' }, { '\r', '\n' } };
const char              LclPathSep = { ';' };

static const ULONG      local_seek_method[] = { FILE_BEGIN, FILE_CURRENT, FILE_END };

void LocalErrMsg( sys_error code, char *buff )
{
    char        *s;
    char        *d;
    ULONG       msg_len;
    char        ch;

    if( DosGetMessage( NULL, 0, buff, 50, code, "OSO001.MSG", &msg_len ) != 0 ) {
        GetDOSErrMsg( code, buff );
        return;
    }
    buff[msg_len] = NULLCHAR;
    s = d = buff;
    if( s[0] == 'S' && s[1] == 'Y' && s[2] == 'S' ) {
        /* Got the SYSxxxx: at the front. Take it off. */
        s += 3;
        while( (ch = *s++) != ':' ) {
            if( ch < '0' || ch > '9' ) {
                s = buff;
                break;
            }
        }
    }
    while( *s == ' ' )
        ++s;
    while( (ch = *s++) != NULLCHAR ) {
        if( ch == '\n' )
            ch = ' ';
        if( ch != '\r' ) {
            *d++ = ch;
        }
    }
    while( d > buff && d[-1] == ' ' )
        --d;
    *d = NULLCHAR;
}

sys_handle LocalOpen( const char *name, obj_attrs oattrs )
{
    HFILE       hdl;
    ULONG       action;
    ULONG       openflags;
    ULONG       openmode;
    APIRET      rc;

    if( (oattrs & OP_WRITE) == 0 ) {
        openmode = READONLY;
        oattrs &= ~(OP_CREATE | OP_TRUNC);
    } else if( oattrs & OP_READ ) {
        openmode = READWRITE;
    } else {
        openmode = WRITEONLY;
    }
    openmode |= 0x20c0;
    openflags = 0;
    if( oattrs & OP_CREATE )
        openflags |= 0x10;
    openflags |= (oattrs & OP_TRUNC) ? 0x02 : 0x01;
    rc = DosOpen( name,         /* name */
                &hdl,           /* handle to be filled in */
                &action,        /* action taken */
                0,              /* initial allocation */
                0,              /* normal file */
                openflags,      /* open the file */
                openmode,       /* deny-none, inheritance */
                0 );            /* reserved */
    if( rc != 0 ) {
        StashErrCode( rc, OP_LOCAL );
        return( NIL_SYS_HANDLE );
    }
    return( hdl );
}

size_t LocalRead( sys_handle filehndl, void *ptr, size_t len )
{
    ULONG       read_len;
    APIRET      ret;
    size_t      total;
    unsigned    piece_len;

    piece_len = INT_MAX;
    total = 0;
    while( len > 0 ) {
        if( piece_len > len )
            piece_len = (unsigned)len;
        ret = DosRead( filehndl, ptr, piece_len, &read_len );
        if( ret != 0 ) {
            StashErrCode( ret, OP_LOCAL );
            return( ERR_RETURN );
        }
        total += read_len;
        if( read_len != piece_len )
            break;
        ptr = (char *)ptr + read_len;
        len -= read_len;
    }
    return( total );
}

size_t LocalWrite( sys_handle filehndl, const void *ptr, size_t len )
{
    ULONG       write_len;
    APIRET      ret;
    size_t      total;
    unsigned    piece_len;

    piece_len = INT_MAX;
    total = 0;
    while( len > 0 ) {
        if( piece_len > len )
            piece_len = (unsigned)len;
        ret = DosWrite( filehndl, (PVOID)ptr, piece_len, &write_len );
        if( ret != 0 ) {
            StashErrCode( ret, OP_LOCAL );
            return( ERR_RETURN );
        }
        total += write_len;
        if( write_len != piece_len )
            break;
        ptr = (char *)ptr + write_len;
        len -= write_len;
    }
    return( total );
}

unsigned long LocalSeek( sys_handle hdl, unsigned long len, seek_method method )
{
    ULONG           new;
    APIRET          ret;

    ret = DosSetFilePtr( hdl, len, local_seek_method[method], &new );
    if( ret != 0 ) {
        StashErrCode( ret, OP_LOCAL );
        return( ERR_SEEK );
    }
    return( new );
}

error_handle LocalClose( sys_handle filehndl )
{
    APIRET      ret;

    ret = DosClose( filehndl );
    return( StashErrCode( ret, OP_LOCAL ) );
}

error_handle LocalErase( const char *name )
{
    APIRET      ret;

    ret = DosDelete( name );
    return( StashErrCode( ret, OP_LOCAL ) );
}

sys_handle LocalHandleSys( file_handle fh )
{
    return( (sys_handle)fh );
}
