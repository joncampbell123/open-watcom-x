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


#include <limits.h>
#include <stddef.h>
#include <stdio.h>
#include "wio.h"
#include "dbgdefn.h"
#if !defined( BUILD_RFX )
#include "dbgdata.h"
#else
#include "rfxdata.h"
#endif
#include "dbgmem.h"
#include "dbgio.h"
#include "errno.h"
#include "strutil.h"
#include "filelcl.h"

#include "clibext.h"


const file_components   LclFile = { '.', ':', { '\\', '/' }, { '\r', '\n' } };
const char              LclPathSep = { ';' };

static const int        local_seek_method[] = { SEEK_SET, SEEK_CUR, SEEK_END };

//NYI: should use native NT calls

void LocalErrMsg( sys_error code, char *buff )
{
    StrCopy( strerror( code ), buff );
}

sys_handle LocalOpen( const char *name, obj_attrs oattrs )
{
    unsigned    openmode;
    int         ret;

    if( (oattrs & OP_WRITE) == 0 ) {
        openmode = O_RDONLY;
    } else if( oattrs & OP_READ ) {
        openmode = O_RDWR;
    } else {
        openmode = O_WRONLY;
    }
    if( oattrs & OP_CREATE )
        openmode |= O_CREAT;
    if( oattrs & OP_TRUNC )
        openmode |= O_TRUNC;
    ret = open( name, openmode | O_BINARY, 0666 );
    if( ret == -1 ) {
        StashErrCode( errno, OP_LOCAL );
        return( NIL_SYS_HANDLE );
    }
//    fcntl( ret, F_SETFD, (int)FD_CLOEXEC );
    return( ret );
}

size_t LocalRead( sys_handle filehndl, void *ptr, size_t len )
{
    ssize_t ret;

    ret = posix_read( filehndl, ptr, len );
    if( ret < 0 ) {
        StashErrCode( errno, OP_LOCAL );
        return( ERR_RETURN );
    }
    return( ret );
}

size_t LocalWrite( sys_handle filehndl, const void *ptr, size_t len )
{
    ssize_t ret;

    ret = posix_write( filehndl, ptr, len );
    if( ret < 0 ) {
        StashErrCode( errno, OP_LOCAL );
        return( ERR_RETURN );
    }
    return( ret );
}

unsigned long LocalSeek( sys_handle hdl, unsigned long len, seek_method method )
{
    off_t       ret;

    ret = lseek( hdl, len, local_seek_method[method] );
    if( ret == (off_t)-1 ) {
        StashErrCode( errno, OP_LOCAL );
        return( ERR_SEEK );
    }
    return( ret );
}

error_handle LocalClose( sys_handle filehndl )
{
    if( close( filehndl ) == 0 )
        return( 0 );
    return( StashErrCode( errno, OP_LOCAL ) );
}

error_handle LocalErase( const char *name )
{
    if( remove( name ) == 0 )
        return( 0 );
    return( StashErrCode( errno, OP_LOCAL ) );
}

sys_handle LocalHandleSys( file_handle fh )
{
    return( (sys_handle)fh );
}
