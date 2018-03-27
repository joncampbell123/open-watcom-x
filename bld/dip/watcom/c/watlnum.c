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


#include "dipwat.h"
#include "dbcue.h"
#include "wataddr.h"
#include "watlcl.h"
#include "watmod.h"
#include "wattype.h"
#include "watgbl.h"

#define NO_LINE         ((word)-1)

#define NEXT_SEG( ptr )     (V2Lines ? ((line_segment *)((ptr)->u.v2.line+(ptr)->u.v2.count)) \
                                     : ((line_segment *)((ptr)->u.v3.line+(ptr)->u.v3.count)))
#define LINE_SEG( ptr )     (V2Lines ? (ptr)->u.v2.segment : (ptr)->u.v3.segment)
#define LINE_COUNT( ptr )   (V2Lines ? (ptr)->u.v2.count : (ptr)->u.v3.count)
#define LINE_LINE( ptr)     (V2Lines ? (ptr)->u.v2.line : (ptr)->u.v3.line)

#define MIDIDX16(lo,hi) ((unsigned_16)(((unsigned_32)lo + (unsigned_32)hi) / 2))

typedef struct {
    union {
        v2_line_segment     v2;
        v3_line_segment     v3;
    } u;
} line_segment;

typedef struct search_info {
    imp_cue_handle      imp_cueh;
    line_number         line;
    addr_off            off;
    search_result       have;
    bool                found;
    const void          *special_table;
    enum { ST_UNKNOWN, ST_NO, ST_YES } have_spec_table;
} search_info;

static line_segment     *LinStart;
static line_segment     *LinEnd;
static bool             V2Lines;

static bool CueFind( const char *base, line_number cue, cue_state *ret )
{
    const cue_state *curr;
    unsigned_16     lo;
    unsigned_16     mid;
    bool            ok;
    unsigned_16     hi;

    if( base == NULL )
        return( 0 );
    hi = GETU16( base );
    base =  base + 2;
    ok = false;
    lo = 0;
    for( ;; ) {
        mid = MIDIDX16( lo, hi );
        curr = ((const cue_state *)base) + mid;      // compare keys
        if( mid == lo )
            break;
        if( cue < curr->cue ) {         // key < mid
            hi = mid;
        } else if( cue > curr->cue ) {  // key > mid
            lo = mid;
        } else {                        // key == mid
            break;
        }
    }
    if( cue >= curr->cue ) {
        ok = true;
    }
    *ret = *curr;
    ret->line += cue - curr->cue;
    ret->fno += 2;      /* 0 => no file, 1 => primary file */
    return( ok );
}

static line_number SpecCueLine( imp_image_handle *iih, imp_cue_handle *imp_cueh, line_number cue )
{
    cue_state           info;
    line_number         line;
    const char          *start;
    const char          *base;

    line = 0;
    start = FindSpecCueTable( iih, imp_cueh->im, &base );
    if( start != NULL ) {
        if( CueFind( start, cue, &info ) ){
            line = info.line;
        }
        InfoSpecUnlock( base );
    }
    return( line );
}

static cue_fileid SpecCueFileId( imp_image_handle *iih, imp_cue_handle *imp_cueh, line_number cue )
{
    cue_state           info;
    cue_fileid          fno;
    const char          *start;
    const char          *base;

    fno = 0;
    start = FindSpecCueTable( iih, imp_cueh->im, &base );
    if( start != NULL ) {
        if( CueFind( start, cue, &info ) ){
            fno = info.fno;
        }
        InfoSpecUnlock( base );
    }
    return( fno );
}

static size_t SpecCueFile( imp_image_handle *iih, imp_cue_handle *imp_cueh,
                    unsigned file_id, char *buff, size_t buff_size )
{
    unsigned_16         size;
    size_t              len;
    const unsigned_16   *index;
    const char          *name;
    const char          *start;
    const char          *base;

    len = 0;
    start = FindSpecCueTable( iih, imp_cueh->im, &base );
    if( start != NULL ) {
        size  = GETU16( start );
        start += 2;
        start += size * sizeof( cue_state );
        size  = GETU16( start );
        start += 2;
        index = (const unsigned_16 *)start;
        name = start + size * sizeof( unsigned_16 );
        name += index[file_id - 2]; /* see comment in CueFind */
        len = strlen( name );
        if( buff_size > 0 ) {
            --buff_size;
            if( buff_size > len )
                buff_size = len;
            memcpy( buff, name, buff_size );
            buff[buff_size] = '\0';
        }
        InfoSpecUnlock( base );
    }
    return( len );
}


/*
 * GetLineInfo -- get the line number infomation for a module
 */

static void UnlockLine( void )
{
    if( LinStart != NULL ) {
        InfoSpecUnlock( (const char *)LinStart );
        LinStart = NULL; /* so we don't unlock the same section twice */
    }
}


static dip_status GetLineInfo( imp_image_handle *iih, imp_mod_handle im, word entry )
{
    if( entry != 0 )
        UnlockLine();
    LinStart = (line_segment *)InfoLoad( iih, im, DMND_LINES, entry, NULL );
    if( LinStart == NULL )
        return( DS_FAIL );
    LinEnd = (line_segment *)( (byte *)LinStart + InfoSize( iih, im, DMND_LINES, entry ) );
    V2Lines = iih->v2;
    return( DS_OK );
}


static line_dbg_info *FindLineOff( addr_off off, addr_off adj,
                        const char *start, const char *end,
                        struct search_info *close, imp_image_handle *iih )
{
    line_dbg_info   *ln_ptr;
    int             low, high, target;
    addr_off        chk;
    const char      *dummy;

    low = 0;
    /* get number of entries minus one */
    high = ( end - start ) / sizeof( line_dbg_info ) - 1;
    /* point at first entry */
    ln_ptr = (line_dbg_info *)start;
    while( low <= high ) {
        target = (low + high) >> 1;
        chk = ln_ptr[target].code_offset + adj;
        if( off < chk ) {
            high = target - 1;
        } else if( off > chk ) {
            low = target + 1;
        } else {                  /* exact match */
            if( ln_ptr[target].line >= PRIMARY_RANGE ) {
                /* a special cue - have to make sure we have the table */
                if( close->have_spec_table == ST_UNKNOWN ) {
                    if( FindSpecCueTable( iih, close->imp_cueh.im, &dummy ) != NULL ) {
                        close->have_spec_table = ST_YES;
                    } else {
                        close->have_spec_table = ST_NO;
                    }
                }
                if( close->have_spec_table == ST_YES ) {
                    return( ln_ptr + target );
                }
            } else {
                return( ln_ptr + target );
            }
            /* if it's a special & we don't have the table, ignore entry */
            high = target - 1;
        }
    }
    if( high < 0 )
        return( NULL );
    if( ln_ptr[high].line >= PRIMARY_RANGE ) {
        /* a special cue - have to make sure we have the table */
        if( close->have_spec_table == ST_UNKNOWN ) {
            if( FindSpecCueTable( iih, close->imp_cueh.im, &dummy ) != NULL ) {
                close->have_spec_table = ST_YES;
            } else {
                close->have_spec_table = ST_NO;
            }
        }
        if( close->have_spec_table == ST_NO ) {
            /* if it's a special & we don't have the table, ignore entry */
            for( ;; ) {
                --high;
                if( high < 0 )
                    return( NULL );
                if( ln_ptr[high].line < PRIMARY_RANGE ) {
                    break;
                }
            }
        }
    }
    return( ln_ptr + high );
}

#define BIAS( p )       ((byte *)(p) - (byte *)LinStart)
#define UNBIAS( o )     ((void *)((byte *)LinStart + (o)))

static void SearchSection( imp_image_handle *iih, struct search_info *close, address addr )
{
    line_segment        *seg;
    line_segment        *next;
    line_dbg_info       *info;
    mem_block           block;

    close->found = false;
    for( seg = LinStart; seg < LinEnd; seg = next ) {
        next = NEXT_SEG( seg );
        block = FindSegBlock( iih, close->imp_cueh.im, LINE_SEG( seg ) );
        if( DCSameAddrSpace( block.start, addr ) != DS_OK )
            continue;
        if( block.start.mach.offset > addr.mach.offset )
            continue;
        if( block.start.mach.offset + block.len <= addr.mach.offset )
            continue;
        info = FindLineOff( addr.mach.offset, block.start.mach.offset,
                            (const char *)LINE_LINE( seg ), (const char *)next, close, iih );
        if( info == NULL )
            continue;
        if( close->have == SR_NONE
                || info->code_offset + block.start.mach.offset > close->off ) {
            close->found = true;
            close->imp_cueh.seg_bias = BIAS( seg );
            close->imp_cueh.info_bias = BIAS( info );
            close->off = block.start.mach.offset + info->code_offset;
            if( close->off == addr.mach.offset ) {
                close->have = SR_EXACT;
            } else {
                close->have = SR_CLOSEST;
            }
        }
    }
}

search_result DIPIMPENTRY( AddrCue )( imp_image_handle *iih, imp_mod_handle im,
                address addr, imp_cue_handle *imp_cueh )
{
    struct search_info  close;
    word                save_entry = 0;

    close.imp_cueh.im = im;
    close.have = SR_NONE;
    close.have_spec_table = ST_UNKNOWN;
    close.off = 0;
    for( close.imp_cueh.entry = 0; GetLineInfo( iih, close.imp_cueh.im, close.imp_cueh.entry ) == DS_OK; ++close.imp_cueh.entry ) {
        SearchSection( iih, &close, addr );
        if( close.found )
            save_entry = close.imp_cueh.entry;
        if( close.have == SR_EXACT ) {
            break;
        }
    }
    *imp_cueh = close.imp_cueh;
    imp_cueh->entry = save_entry;
    UnlockLine();
    return( close.have );
}

static void ScanSection( struct search_info *close, cue_fileid file_id, line_number search_line )
{
    line_dbg_info       *curr;
    line_segment        *ptr;
    line_segment        *next;
    line_number         line;
    cue_state           spec;

    close->found = false;
    for( ptr = LinStart; ptr < LinEnd; ptr = next ) {
        next = NEXT_SEG( ptr );
        if( search_line == 0 ) {
            search_line = LINE_LINE( ptr )[0].line;
        }
        for( curr = LINE_LINE( ptr ); curr < (line_dbg_info *)next; ++curr ) {
            spec.fno = 1;
            line = curr->line;
            if( line >= PRIMARY_RANGE ) {
                if( !CueFind( close->special_table, line - PRIMARY_RANGE, &spec ) ) {
                    continue;
                }
                line = spec.line;
            }
            if( spec.fno != file_id )
                continue;
            if( line > search_line )
                continue;
            if( line <= close->line )
                continue;
            close->found = true;
            close->imp_cueh.seg_bias = BIAS( ptr );
            close->imp_cueh.info_bias = BIAS( curr );
            close->line = line;
            if( line == search_line ) {
                close->have = SR_EXACT;
                return;
            }
            close->have = SR_CLOSEST;
        }
    }
}

search_result DIPIMPENTRY( LineCue )( imp_image_handle *iih, imp_mod_handle im,
                        cue_fileid file, unsigned long line, unsigned col,
                        imp_cue_handle *imp_cueh )
{
    struct search_info  close;
    word                save_entry = 0;
    const char          *base;

    /* unused parameters */ (void)col;

    if( file == 0 )
        file = 1;
    close.imp_cueh.im = im;
    close.have = SR_NONE;
    close.line = 0;
    close.special_table = FindSpecCueTable( iih, im, &base );
    for( close.imp_cueh.entry = 0; GetLineInfo( iih, im, close.imp_cueh.entry ) == DS_OK; ++close.imp_cueh.entry ) {
        ScanSection( &close, file, line );
        if( close.found )
            save_entry = close.imp_cueh.entry;
        if( close.have == SR_EXACT ) {
            break;
        }
    }
    *imp_cueh = close.imp_cueh;
    imp_cueh->entry = save_entry;
    UnlockLine();
    if( base != NULL )
        InfoSpecUnlock( base );
    return( close.have );
}

unsigned long DIPIMPENTRY( CueLine )( imp_image_handle *iih, imp_cue_handle *imp_cueh )
{
    line_dbg_info   *info;
    line_number     line;

    LinStart = NULL;
    line = 0;
    if( imp_cueh->entry != NO_LINE && GetLineInfo( iih, imp_cueh->im, imp_cueh->entry ) == DS_OK ) {
        info = UNBIAS( imp_cueh->info_bias );
        line = info->line;
        UnlockLine();
        if( line >= PRIMARY_RANGE ) {
            return( SpecCueLine( iih, imp_cueh, line - PRIMARY_RANGE ) );
        }
    }
    return( line );
}

unsigned DIPIMPENTRY( CueColumn )( imp_image_handle *iih, imp_cue_handle *imp_cueh )
{
    /* unused parameters */ (void)iih; (void)imp_cueh;

    return( 0 );
}

address DIPIMPENTRY( CueAddr )( imp_image_handle *iih, imp_cue_handle *imp_cueh )
{
    address             addr;
    line_dbg_info       *info;
    line_segment        *seg;

    LinStart = NULL;
    addr = NilAddr;
    if( imp_cueh->entry != NO_LINE && GetLineInfo( iih, imp_cueh->im, imp_cueh->entry ) == DS_OK ) {
        seg = UNBIAS( imp_cueh->seg_bias );
        addr = FindSegBlock( iih, imp_cueh->im, LINE_SEG( seg ) ).start;
        info = UNBIAS( imp_cueh->info_bias );
        addr.mach.offset += info->code_offset;
        UnlockLine();
    }
    return( addr );
}


walk_result DIPIMPENTRY( WalkFileList )( imp_image_handle *iih, imp_mod_handle im,
            DIP_IMP_CUE_WALKER *wk, imp_cue_handle *imp_cueh, void *d )
{
    line_dbg_info       *curr;
    line_segment        *ptr;
    line_segment        *next;
    walk_result         wr;

    //NYI: handle special cues
    imp_cueh->im = im;
    for( imp_cueh->entry = 0; GetLineInfo( iih, im, imp_cueh->entry ) == DS_OK; ++imp_cueh->entry ) {
        for( ptr = LinStart; ptr < LinEnd; ptr = next ) {
            next = NEXT_SEG( ptr );
            imp_cueh->seg_bias = BIAS( ptr );
            for( curr = LINE_LINE( ptr ); curr < (line_dbg_info *)next; ++curr ) {
                imp_cueh->info_bias = BIAS( curr );
                if( curr->line < PRIMARY_RANGE ) {
                    wr = wk( iih, imp_cueh, d );
                    UnlockLine();
                    return( wr );
                }
            }
        }
        UnlockLine();
    }
    if( imp_cueh->entry == 0 ) {
        /* Module with no line cues. Fake one up. */
        imp_cueh->entry = NO_LINE;
        return( wk( iih, imp_cueh, d ) );
    }
    return( WR_CONTINUE );
}

imp_mod_handle DIPIMPENTRY( CueMod )( imp_image_handle *iih, imp_cue_handle *imp_cueh )
{
    /* unused parameters */ (void)iih;

    return( imp_cueh->im );
}

cue_fileid DIPIMPENTRY( CueFileId )( imp_image_handle *iih, imp_cue_handle *imp_cueh )
{
    line_dbg_info   *info;
    line_number     line;

    LinStart = NULL;
    if( imp_cueh->entry != NO_LINE && GetLineInfo( iih, imp_cueh->im, imp_cueh->entry ) == DS_OK ) {
        info = UNBIAS( imp_cueh->info_bias );
        line = info->line;
        UnlockLine();
        if( line >= PRIMARY_RANGE ) {
            return( SpecCueFileId( iih, imp_cueh, line - PRIMARY_RANGE ) );
        }
    }
    return( 1 );
}

size_t DIPIMPENTRY( CueFile )( imp_image_handle *iih, imp_cue_handle *imp_cueh,
                        char *buff, size_t buff_size )
{
    cue_fileid      id;

    id = ImpInterface.CueFileId( iih, imp_cueh );
    switch( id ) {
    case 0:
        return( 0 );
    case 1:
        return( PrimaryCueFile( iih, imp_cueh, buff, buff_size ) );
    default:
        return( SpecCueFile( iih, imp_cueh, id, buff, buff_size ) );
    }
}


static dip_status AdjForward( imp_image_handle *iih, imp_cue_handle *imp_cueh )
{
    line_dbg_info       *info;
    line_segment        *seg;
    word                num_entries;
    dip_status          status;

    num_entries = ModPointer( iih, imp_cueh->im )->di[DMND_LINES].u.entries;
    while( (status = GetLineInfo( iih, imp_cueh->im, imp_cueh->entry )) == DS_OK ) {
        seg = UNBIAS( imp_cueh->seg_bias );
        info = UNBIAS( imp_cueh->info_bias );
        ++info;
        for( ;; ) {
            if( info < (line_dbg_info *)NEXT_SEG( seg ) ) {
                imp_cueh->seg_bias = BIAS( seg );
                imp_cueh->info_bias = BIAS( info );
                UnlockLine();
                return( DS_OK );
            }
            seg = NEXT_SEG( seg );
            if( seg >= LinEnd )
                break;
            info = LINE_LINE( seg );
        }
        imp_cueh->entry++;
        if( imp_cueh->entry >= num_entries ) {
            imp_cueh->entry = 0;
            imp_cueh->seg_bias = BIAS( LinStart );
            info = LINE_LINE( LinStart );
            imp_cueh->info_bias = BIAS( info );
            UnlockLine();
            return( DS_WRAPPED );
        }
        imp_cueh->seg_bias = BIAS( seg );
        imp_cueh->info_bias = BIAS( info );
    }
    return( status );
}

static line_segment *FindPrevSeg( line_segment *seg )
{
    line_segment        *curr;
    line_segment        *next;

    curr = LinStart;
    for( ;; ) {
        next = NEXT_SEG( curr );
        if( next >= seg )
            return( curr );
        curr = next;
    }
}

static dip_status AdjBackward( imp_image_handle *iih, imp_cue_handle *imp_cueh )
{
    line_dbg_info       *info;
    line_segment        *seg;
    word                num_entries;
    dip_status          status;

    LinStart = NULL;
    while( (status = GetLineInfo( iih, imp_cueh->im, imp_cueh->entry )) == DS_OK ) {
        seg = UNBIAS( imp_cueh->seg_bias );
        info = UNBIAS( imp_cueh->info_bias );
        --info;
        for( ;; ) {
            if( info >= LINE_LINE( seg ) ) {
                imp_cueh->seg_bias = BIAS( seg );
                imp_cueh->info_bias = BIAS( info );
                UnlockLine();
                return( DS_OK );
            }
            if( seg == LinStart )
                break;
            seg = FindPrevSeg( seg );
            info = LINE_LINE( seg ) + LINE_COUNT( seg ) - 1;
        }
        if( imp_cueh->entry == 0 ) {
            num_entries = ModPointer( iih, imp_cueh->im )->di[DMND_LINES].u.entries;
            imp_cueh->entry = num_entries - 1;
            status = GetLineInfo( iih, imp_cueh->im, imp_cueh->entry );
            if( status != DS_OK )
                return( status );
            seg = FindPrevSeg( seg );
            info = LINE_LINE( seg ) + LINE_COUNT( seg ) - 1;
            imp_cueh->seg_bias = BIAS( seg );
            imp_cueh->info_bias = BIAS( info );
            UnlockLine();
            return( DS_WRAPPED );
        }
        imp_cueh->entry--;
        if( imp_cueh->entry == 0 ) {
            /* special handling since we're walking backwards */
            UnlockLine();
        }
        imp_cueh->seg_bias = BIAS( seg );
        imp_cueh->info_bias = BIAS( info );
    }
    return( status );
}

dip_status DIPIMPENTRY( CueAdjust )( imp_image_handle *iih, imp_cue_handle *src_imp_cueh,
                        int adj, imp_cue_handle *dst_imp_cueh )
{
    dip_status  status;
    dip_status  ok;

    if( src_imp_cueh->entry == NO_LINE )
        return( DS_BAD_PARM );
    //NYI: handle special cues
    *dst_imp_cueh = *src_imp_cueh;
    ok = DS_OK;
    for( ; adj > 0; --adj ) {
        status = AdjForward( iih, dst_imp_cueh );
        if( status & DS_ERR )
            return( status );
        if( status != DS_OK ) {
            ok = status;
        }
    }
    for( ; adj < 0; ++adj ) {
        status = AdjBackward( iih, dst_imp_cueh );
        if( status & DS_ERR )
            return( status );
        if( status != DS_OK ) {
            ok = status;
        }
    }
    return( ok );
}

int DIPIMPENTRY( CueCmp )( imp_image_handle *iih, imp_cue_handle *imp_cueh1, imp_cue_handle *imp_cueh2 )
{
    /* unused parameters */ (void)iih;

    if( imp_cueh1->im < imp_cueh2->im )
        return( -1 );
    if( imp_cueh1->im > imp_cueh2->im )
        return( 1 );
    if( imp_cueh1->entry < imp_cueh2->entry )
        return( -1 );
    if( imp_cueh1->entry > imp_cueh2->entry )
        return( 1 );
    if( imp_cueh1->info_bias < imp_cueh2->info_bias )
        return( -1 );
    if( imp_cueh1->info_bias > imp_cueh2->info_bias )
        return( 1 );
    return( 0 );
}
