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
* Description:  Installer data source file I/O routines.
*
****************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "wio.h"
#include "setup.h"
#include "setupio.h"
#include "zip.h"


typedef enum ds_type {
    DS_INVALID,
    DS_FILE,
    DS_ZIP
} ds_type;

typedef struct file_handle_t {
    ds_type                 type;
    union {
        FILE                *fp;
        struct zip_file     *zf;
    } u;
} *file_handle;

static ds_type          srcType;
static struct zip       *srcZip;

/* At the moment the incoming path may have either forward or backward
 * slashes as path separators. However, ziplib only likes forward slashes,
 * so we must manually flip them.
 */
static char *flipBackSlashes( const char *old_path )
{
    char    *new_path;

    new_path = strdup( old_path );
    if( new_path != NULL ) {
        char        *s;

        /* Need to flip slashes - at the moment they may be
         * forward or backward, and ziplib requires forward
         * slashes only.
         */
        s = new_path;
        while( *s ) {
            if( *s == '\\' )
                *s = '/';
            ++s;
        }
    }
    return( new_path );
}

int FileInit( const VBUF *archive )
{
    int             zerr;

    /* Attempt to open a ZIP archive */
    srcZip = zip_open_vbuf( archive, 0, &zerr );
    if( srcZip != NULL ) {
        srcType = DS_ZIP;
    } else {
        srcType = DS_FILE;
    }
    return( 0 );
}


int FileFini( void )
{
    if( srcType == DS_ZIP ) {
        zip_close( srcZip );
    }
    return( 0 );
}


int FileIsPlainFS( void )
{
    return( srcType == DS_FILE );
}


int FileIsArchive( void )
{
    return( srcType == DS_ZIP );
}


int FileStat( const VBUF *path, struct stat *buf )
{
    int                 rc;
    struct zip_stat     zs;

    rc = -1;
    if( srcType == DS_ZIP ) {
        char        *alt_path;

        /* First try a file inside a ZIP archive */
        alt_path = flipBackSlashes( VbufString( path ) );
        if( alt_path != NULL ) {
            rc = zip_stat( srcZip, alt_path, 0, &zs );
            if( rc == 0 ) {
                memset( buf, 0, sizeof( *buf ) );
                buf->st_ino   = zs.index;
                buf->st_size  = zs.size;
                buf->st_mtime = zs.mtime;
            }
            free( alt_path );
        }
    }
    if( rc != 0 ) {
        /* If that fails, try local file */
        rc = stat_vbuf( path, buf );
    }
    return( rc );
}


file_handle FileOpen( const VBUF *path, const char *flags )
{
    file_handle     fh;
    char            *alt_path;

    fh = malloc( sizeof( *fh ) );
    if( fh == NULL )
        return( NULL );

    fh->u.zf = NULL;
    if( srcType == DS_ZIP ) {
        /* First try opening the file inside a ZIP archive */
        alt_path = flipBackSlashes( VbufString( path ) );
        if( alt_path != NULL ) {
            fh->u.zf = zip_fopen( srcZip, alt_path, 0 );
            fh->type = DS_ZIP;
            free( alt_path );
        }
    }
    if( fh->u.zf == NULL ) {
        /* If that fails, try opening the file directly */
        fh->u.fp = fopen_vbuf( path, flags );
        fh->type = DS_FILE;
    }
    if( fh->type == DS_FILE && fh->u.fp == NULL ) {
        free( fh );
        fh = NULL;
    }
    return( fh );
}


int FileClose( file_handle fh )
{
    int             rc;

    switch( fh->type ) {
    case DS_FILE:
        rc = fclose( fh->u.fp );
        break;
    case DS_ZIP:
        rc = zip_fclose( fh->u.zf );
        break;
    default:
        rc = -1;
    }

    free( fh );
    return( rc );
}


long FileSeek( file_handle fh, long offset, int origin )
{
    long            pos;

    switch( fh->type ) {
    case DS_FILE:
        if( fseek( fh->u.fp, offset, origin ) ) {
            pos = -1;
        } else {
            pos = ftell( fh->u.fp );
        }
        break;
    case DS_ZIP:
        /* I really want to be able to seek! */
    default:
        pos = -1;
    }

    return( pos );
}


size_t FileRead( file_handle fh, void *buffer, size_t length )
{
    size_t          amt;

    switch( fh->type ) {
    case DS_FILE:
        amt = fread( buffer, 1, length, fh->u.fp );
        break;
    case DS_ZIP:
        amt = zip_fread( fh->u.zf, buffer, length );
        break;
    default:
        amt = 0;
    }

    return( amt );
}
