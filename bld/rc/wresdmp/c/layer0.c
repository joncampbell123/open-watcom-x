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


#include "layer0.h"
#include "wio.h"
#include "rcrtns.h"
#include "reserr.h"

#include "clibext.h"


WResFileID res_open( const char *name, wres_open_mode omode )
{
    int     fd;

    omode=omode;
#if defined( __WATCOMC__ ) && defined( __QNX__ )
    /* This is a kludge fix to avoid turning on the O_TRUNC bit under QNX */
    fd = open( name, O_RDONLY );
    if( fd == -1 ) {
        WRES_ERROR( WRS_OPEN_FAILED );
    } else {
        setmode( fd, O_BINARY );
    }
#else
    fd = open( name, O_RDONLY | O_BINARY );
    if( fd == -1 ) {
        WRES_ERROR( WRS_OPEN_FAILED );
    }
#endif
    return( WRES_PH2FID( fd ) );
}

bool res_close( WResFileID fid )
{
    return( close( WRES_FID2PH( fid ) ) != 0 );
}

size_t res_read( WResFileID fid, void *buf, size_t size )
{
    return( posix_read( WRES_FID2PH( fid ), buf, size ) );
}

size_t res_write( WResFileID fid, const void *buf, size_t size )
{
    return( posix_write( WRES_FID2PH( fid ), buf, size ) );
}

bool res_seek( WResFileID fid, WResFileOffset pos, int where )
{
    if( where == SEEK_SET ) {
        /* fool the wres library into thinking that the resource information starts at offset 0 */
        return( lseek( WRES_FID2PH( fid ), pos + WResFileShift, where ) == -1 );
    }
    return( lseek( WRES_FID2PH( fid ), pos, where ) == -1 );
}

WResFileOffset res_tell( WResFileID fid )
{
    return( tell( WRES_FID2PH( fid ) ) );
}

bool res_ioerr( WResFileID fid, size_t rc )
/*****************************************/
{
    fid=fid;
    return( rc == -1 );
}

WResSetRtns( res_open, res_close, res_read, res_write, res_seek, res_tell, res_ioerr, RESALLOC, RESFREE );
