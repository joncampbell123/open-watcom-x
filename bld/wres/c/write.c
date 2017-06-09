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


#include <string.h>
#include "layer0.h"
#include "filefmt.h"
#include "resfmt.h"
#include "mresfmt.h"
#include "write.h"
#include "reserr.h"
#include "read.h"
#include "wresrtns.h"

#define CONV_BUF_SIZE           512

static char     ConvBuffer[CONV_BUF_SIZE];

static size_t DefaultConversion( size_t len, const char *str, char *buf )
{
    size_t  i;

    if( buf != NULL ) {
        for( i = 0; i < len; i++ ) {
            *buf++ = *str++;
            *buf++ = 0;
        }
    }
    return( len * 2 );
}

bool ResWriteUint8( uint_8 newint, WResFileID fid )
/*************************************************/
{
    if( WRESWRITE( fid, &newint, sizeof( uint_8 ) ) != sizeof( uint_8 ) )
        return( WRES_ERROR( WRS_WRITE_FAILED ) );
    return( false );
}

bool ResWriteUint16( uint_16 newint, WResFileID fid )
/***************************************************/
{
    if( WRESWRITE( fid, &newint, sizeof( uint_16 ) ) != sizeof( uint_16 ) )
        return( WRES_ERROR( WRS_WRITE_FAILED ) );
    return( false );
}

bool ResWriteUint32( uint_32 newint, WResFileID fid )
/***************************************************/
{
    if( WRESWRITE( fid, &newint, sizeof( uint_32 ) ) != sizeof( uint_32 ) )
        return( WRES_ERROR( WRS_WRITE_FAILED ) );
    return( false );
}

bool ResWritePadDWord( WResFileID fid )
/*************************************/
{
    WResFileOffset  curr_pos;
    size_t          padding;
    uint_32         zero = 0;

    curr_pos = WRESTELL( fid );
    if( curr_pos == -1 )
        return( WRES_ERROR( WRS_TELL_FAILED ) );
    padding = RES_PADDING( curr_pos, sizeof( zero ) );
    if( padding != 0 ) {
        if( WRESWRITE( fid, &zero, padding ) != padding ) {
            return( WRES_ERROR( WRS_WRITE_FAILED ) );
        }
    }
    return( false );
}

bool WResWriteWResIDNameUni( const WResIDName *name, bool use_unicode, WResFileID fid )
/*************************************************************************************/
{
    bool            error;
    unsigned        numchars;
    char            *ptr;
    bool            freebuf;

    freebuf = false;
    error = false;
    if( name == NULL ) {
        /* a NULL name means write 0 length name */
        numchars = 0;
    } else {
        numchars = name->NumChars;
    }
    if( use_unicode ) {
        // for short strings use a static buffer in improve performance
        if( numchars <= CONV_BUF_SIZE / 2 ) {
            ptr = ConvBuffer;
        } else {
            freebuf = true;
            ptr = WRESALLOC( 2 * numchars );
        }
        numchars = ConvToUnicode( numchars, name->Name, ptr );
        error = ResWriteUint16( numchars / 2, fid );
    } else {
        /* in 16-bit resources the string can be no more than 255 characters */
        if( numchars > 255 )
            numchars = 255;
        ptr = (char *)name->Name;
        error = ResWriteUint8( numchars, fid );
    }
    if( !error && numchars > 0 ) {
        if( WRESWRITE( fid, ptr, numchars ) != numchars ) {
            error = WRES_ERROR( WRS_WRITE_FAILED );
        }
    }
    if( freebuf ) {
        WRESFREE( ptr );
    }
    return( error );
} /* WResWriteWResIDNameUni */

bool WResWriteWResIDName( const WResIDName *name, WResFileID fid )
/****************************************************************/
{
    return( WResWriteWResIDNameUni( name, false, fid ) );
}

bool WResWriteWResID( const WResID *name, WResFileID fid )
/********************************************************/
{
    bool        error;

    error = ResWriteUint8( name->IsName, fid );
    if( !error ) {
        if( name->IsName ) {
            error = WResWriteWResIDName( &(name->ID.Name), fid );
        } else {
            error = ResWriteUint16( name->ID.Num, fid );
        }
    }

    return( error );
} /* WResWriteWResID */

/*
 * WResWriteTypeRecord - write the type record to the current postion
 *                       in the file identified by fp
 */
bool WResWriteTypeRecord( const WResTypeInfo *type, WResFileID fid )
{
    size_t      size;

    if( type->TypeName.IsName ) {
        /* -1 because one of the chars in the name is declared in the struct */
        size = sizeof( WResTypeInfo ) + type->TypeName.ID.Name.NumChars - 1;
    } else {
        size = sizeof( WResTypeInfo );
    }
    if( WRESWRITE( fid, type, size ) != size )
        return( WRES_ERROR( WRS_WRITE_FAILED ) );
    return( false );
} /* WResWriteTypeRecord */

/*
 * WResWriteResRecord - write the resource record to the current position
 *                      in the file identified by  fp
 */
bool WResWriteResRecord( const WResResInfo *res, WResFileID fid )
{
    size_t      size;

    if( res->ResName.IsName ) {
        /* -1 because one of the chars in the name is declared in the struct */
        size = sizeof( WResResInfo ) + res->ResName.ID.Name.NumChars - 1;
    } else {
        size = sizeof( WResResInfo );
    }
    if( WRESWRITE( fid, (uint_8 *)res, size ) != size )
        return( WRES_ERROR( WRS_WRITE_FAILED ) );
    return( false );
} /* WResWriteResRecord */

/*
 * WResWriteLangRecord - write out a language record at the current file
 *                       position
 */
bool WResWriteLangRecord( const WResLangInfo *info, WResFileID fid )
{
    if( WRESWRITE( fid, info, sizeof( WResLangInfo ) ) != sizeof( WResLangInfo ) )
        return( WRES_ERROR( WRS_WRITE_FAILED ) );
    return( false );
}

bool WResWriteHeaderRecord( const WResHeader *header, WResFileID fid )
/********************************************************************/
{
    if( WRESSEEK( fid, 0L, SEEK_SET ) )
        return( WRES_ERROR( WRS_SEEK_FAILED ) );
    if( WRESWRITE( fid, header, sizeof( WResHeader ) ) != sizeof( WResHeader ) )
        return( WRES_ERROR( WRS_WRITE_FAILED ) );
    return( false );
} /* WResWriteHeaderRecord */

bool WResWriteExtHeader( const WResExtHeader *ext_head, WResFileID fid )
/**********************************************************************/
{
    if( WRESWRITE( fid, ext_head, sizeof( WResExtHeader ) ) != sizeof( WResExtHeader ) )
        return( WRES_ERROR( WRS_WRITE_FAILED ) );
    return( false );
}

bool ResWriteStringLen( const char *string, bool use_unicode, WResFileID fid, size_t len )
/****************************************************************************************/
{
    char            *buf = NULL;
    bool            error;

    if( use_unicode ) {
        if( len * 2 > CONV_BUF_SIZE ) {
            buf = WRESALLOC( 2 * len );
        } else {
            buf = ConvBuffer;
        }
        len = ConvToUnicode( len, string, buf );
        string = buf;
    }
    error = false;
    if( WRESWRITE( fid, string, len ) != len )
        error = WRES_ERROR( WRS_WRITE_FAILED );
    if( use_unicode ) {
        if( buf != ConvBuffer ) {
            WRESFREE( buf );
        }
    }
    return( error );
}

bool ResWriteString( const char *string, bool use_unicode, WResFileID fid )
/*************************************************************************/
{
    size_t  stringlen;

    /* if string is NULL output the null string */
    if( string == NULL ) {
        string = "";
    }

    /* the +1 is so we will output the '\0' as well */
    stringlen = strlen( string ) + 1;
    return( ResWriteStringLen( string, use_unicode, fid, stringlen ) );
}

bool ResWriteNameOrOrdinal( ResNameOrOrdinal *name, bool use_unicode, WResFileID fid )
/************************************************************************************/
{
    bool        error;

    if( name == NULL ) {
        error = ResWriteString( "", use_unicode, fid );
    } else {
        if( name->ord.fFlag == 0xff ) {
            if( use_unicode ) {
                error = ResWriteUint16( (uint_16)-1, fid );
            } else {
                error = ResWriteUint8( name->ord.fFlag, fid );
            }
            if( !error ) {
                error = ResWriteUint16( name->ord.wOrdinalID, fid );
            }
        } else {
            error = ResWriteString( name->name, use_unicode, fid );
        }
    }

    return( error );
} /* ResWriteNameOrOrdinal */

static size_t MResFindNameOrOrdSize( ResNameOrOrdinal *data, bool use_unicode )
/*****************************************************************************/
{
    size_t  size;

    if( data->ord.fFlag == 0xff ) {
        size = 4;
    } else {
        if( use_unicode ) {
            size = 2 * ( strlen( data->name ) + 1 );
        } else {
            size = strlen( data->name ) + 1;
        }
    }

    return( size );
}

static size_t MResFindHeaderSize( MResResourceHeader *header, bool use_unicode )
/******************************************************************************/
{
    size_t  headersize;
    size_t  namesize;
    size_t  typesize;
    size_t  padding;

    headersize = 2 * sizeof( uint_16 ) + 5 * sizeof( uint_32 );
    namesize = MResFindNameOrOrdSize( header->Name, use_unicode );
    typesize = MResFindNameOrOrdSize( header->Type, use_unicode );
    headersize += ( namesize + typesize );
    padding = RES_PADDING( typesize + namesize, sizeof( uint_32 ) );

    return( headersize + padding );
}

bool MResWriteResourceHeader( MResResourceHeader *currhead, WResFileID fid, bool iswin32 )
/****************************************************************************************/
{
    bool        error;

    if( !iswin32 ) {
        error = ResWriteNameOrOrdinal( currhead->Type, false, fid );
        if( !error ) {
            error = ResWriteNameOrOrdinal( currhead->Name, false, fid );
        }
        if( !error ) {
            error = ResWriteUint16( currhead->MemoryFlags, fid );
        }
        if( !error ) {
            error = ResWriteUint32( currhead->Size, fid );
        }
    } else {
        error = ResWriteUint32( currhead->Size, fid );
        if( !error ) {
            error = ResWriteUint32( MResFindHeaderSize( currhead, true ), fid  );
        }
        if( !error ) {
            error = ResWriteNameOrOrdinal( currhead->Type, true, fid );
        }
        if( !error ) {
            error = ResWriteNameOrOrdinal( currhead->Name, true, fid );
        }
        if( !error ) {
            error = ResWritePadDWord( fid );
        }
        if( !error ) {
            error = ResWriteUint32( currhead->DataVersion, fid );
        }
        if( !error ) {
            error = ResWriteUint16( currhead->MemoryFlags, fid );
        }
        if( !error ) {
            error = ResWriteUint16( currhead->LanguageId, fid );
        }
        if( !error ) {
            error = ResWriteUint32( currhead->Version, fid );
        }
        if( !error ) {
            error = ResWriteUint32( currhead->Characteristics, fid );
        }
    }

    return( error );
} /* MResWriteResourceHeader */

void WriteInitStatics( void )
/***************************/
{
    memset( ConvBuffer, 0, CONV_BUF_SIZE * sizeof( char ) );
}

ConvToUnicode_fn    *ConvToUnicode = DefaultConversion;
