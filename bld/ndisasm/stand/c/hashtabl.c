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
* Description:  hash table routines
*
*
****************************************************************************/


#include <stdio.h>
#include "dis.h"
#include "global.h"
#include "hashtabl.h"
#include "memfuncs.h"


#define _HashCompare( a, b, c )  (a->compare( b, c ) == 0)

int NumberCmp( hash_value n1, hash_value n2 )
{
    if( n1 == n2 )
        return( 0 );
    if( n1 < n2 )
        return( -1 );
    return( 1 );
}

static hash_value stringEncode( const char *string )
{
    const unsigned char *p;
    uint_32             g;
    uint_32             h;

    h = 0;
    for( p = (const unsigned char *)string; *p != 0; p++ ) {
        h = ( h << 4 ) + *p;
        if( (g = (h & 0xf0000000)) != 0 ) {
            h = h ^ ( g >> 24 );
            h = h ^ g;
        }
    }
    return( (hash_value)h );
}

static hash_value hashFunc( hash_table hash_tbl, hash_value key )
{
    return( key % hash_tbl->size );
}

return_val HashTableInsert( hash_table hash_tbl, hash_value key, hash_data data )
{
    hash_value              hash_val;
    hash_struct             *hash_ptr;
    hash_struct             *new_element;

    if( hash_tbl->type == HASH_STRING ) {
        hash_val = hashFunc( hash_tbl, stringEncode( (char *)key ) );
    } else {
        hash_val = hashFunc( hash_tbl, key );
    }
    for( hash_ptr = hash_tbl->table[hash_val]; hash_ptr != NULL; hash_ptr = hash_ptr->next ) {
        if( _HashCompare( hash_tbl, hash_ptr->key, key ) ) {
            hash_ptr->data = data;
            return( RC_OKAY );
        }
    }
    new_element = (hash_struct *)MemAlloc( sizeof( hash_struct ) );
    if( new_element == NULL ) {
        return( RC_OUT_OF_MEMORY );
    }
    new_element->key = key;
    new_element->data = data;
    new_element->next = hash_tbl->table[hash_val];
    hash_tbl->table[hash_val] = new_element;
    return( RC_OKAY );
}

hash_data *HashTableQuery( hash_table hash_tbl, hash_value key )
{
    hash_value          hash_val;
    hash_struct *       hash_ptr;

    if( hash_tbl->type == HASH_STRING ) {
        hash_val = hashFunc( hash_tbl, stringEncode( (char *)key ) );
    } else {
        hash_val = hashFunc( hash_tbl, key );
    }
    for( hash_ptr = hash_tbl->table[hash_val]; hash_ptr != NULL; hash_ptr = hash_ptr->next ) {
        if( _HashCompare( hash_tbl, hash_ptr->key, key ) ) {
            return( &(hash_ptr->data) );
        }
    }
    return( NULL );
}

hash_table HashTableCreate( hash_table_size size, hash_table_type type, hash_table_comparison_func func )
{
    hash_table          hash_tbl;
    hash_table_size     loop;

    hash_tbl = (hash_table)MemAlloc( sizeof( hash_table_struct ) );
    if( hash_tbl == NULL )
        return( NULL );
    hash_tbl->table = (hash_struct **)MemAlloc( size * sizeof( hash_struct * ) );
    if( hash_tbl->table == NULL )
        return( NULL );
    hash_tbl->size = size;
    hash_tbl->type = type;
    hash_tbl->compare = func;
    for( loop = 0; loop < size; loop ++ ) {
        hash_tbl->table[loop] = NULL;
    }
    return( hash_tbl );
}

void HashTableFree( hash_table hash_tbl )
{
    hash_table_size     loop;
    hash_struct         *hash_ptr;
    hash_struct         *next_hash_ptr;

    if( !hash_tbl ) return;

    for( loop = 0; loop < hash_tbl->size; loop++ ) {
        for( hash_ptr = hash_tbl->table[loop]; hash_ptr != NULL; hash_ptr = next_hash_ptr ) {
            next_hash_ptr = hash_ptr->next;
            MemFree( hash_ptr );
        }
    }
    MemFree( hash_tbl->table );
    MemFree( hash_tbl );
}
