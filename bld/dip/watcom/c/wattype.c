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
* Description:  Watcom debugging information type support.
*
****************************************************************************/


#include "dipwat.h"
#include "wattype.h"
#include "walloca.h"
#include "watlcl.h"
#include "watgbl.h"

#include "clibext.h"


#define NEXT_TYPE( p )  ((p) + GETU8(p))

typedef struct typeinfo {
    const char          *start;
    const char          *end;
    word                entry;
    imp_mod_handle      im;
    struct typeinfo     *prev;
} typeinfo;

static typeinfo *Type;

static void PushLoad( typeinfo *new )
{
    new->prev = Type;
    new->start = NULL;
    Type = new;
}

static void FreeLoad( void )
{
    if( Type->start != NULL ) {
        InfoSpecUnlock( Type->start );
        Type->start = NULL;
    }
}

static void PopLoad( void )
{
    FreeLoad();
    Type = Type->prev;
}

void KillTypeLoadStack( void )
{
    Type = NULL;
}

static dip_status LoadType( imp_image_handle *iih, imp_mod_handle im, word entry )
{
    FreeLoad();
    Type->start = InfoLoad( iih, im, DMND_TYPES, entry, NULL );
    if( Type->start == NULL ) {
        return( DS_FAIL );
    }
    Type->end = Type->start + InfoSize( iih, im, DMND_TYPES, entry );
    Type->entry = entry;
    Type->im = im;
    return( DS_OK );
}

static const char *NamePtr( const char *p )
{
    unsigned    index;

    switch( GETU8( p + 1 ) ) {
    case NAME_TYPE+TYPE_SCALAR:
        p += 3;
        break;
    case NAME_TYPE+TYPE_SCOPE:
        p += 2;
        break;
    case NAME_TYPE+TYPE_NAME:
        p = GetIndex( p + 2, &index );
        p = GetIndex( p, &index );
        break;
    case ENUM_TYPE+ENUM_CONST_BYTE:
        p += 3;
        break;
    case ENUM_TYPE+ENUM_CONST_WORD:
        p += 4;
        break;
    case ENUM_TYPE+ENUM_CONST_LONG:
        p += 6;
        break;
    case ENUM_TYPE+ENUM_CONST_I64:
        p += 10;
        break;
    case STRUCT_TYPE+ST_BIT_BYTE:
        p += 2;
        /* fall through */
    case STRUCT_TYPE+ST_FIELD_BYTE:
        p += 3;
        p = GetIndex( p, &index );
        break;
    case STRUCT_TYPE+ST_BIT_WORD:
        p += 2;
        /* fall through */
    case STRUCT_TYPE+ST_FIELD_WORD:
        p += 4;
        p = GetIndex( p, &index );
        break;
    case STRUCT_TYPE+ST_BIT_LONG:
        p += 2;
        /* fall through */
    case STRUCT_TYPE+ST_FIELD_LONG:
        p += 6;
        p = GetIndex( p, &index );
        break;
    case STRUCT_TYPE+ST_FIELD_LOC:
        p = SkipLocation( p + 3 );
        p = GetIndex( p, &index );
        break;
    case STRUCT_TYPE+ST_BIT_LOC:
        p = SkipLocation( p + 3 ) + 2;
        p = GetIndex( p, &index );
        break;
    default:
        p += GETU8( p );
        break;
    }
    return( p );
}

static const char *BaseTypePtr( const char *p )
{
    byte        kind;
    byte        subkind;
    unsigned    index;

    kind = GETU8( p + 1 );
    subkind = kind & SUBCLASS_MASK;
    switch( kind & CLASS_MASK ) {
    case NAME_TYPE:
        if( subkind == TYPE_NAME ) {
            p = GetIndex( p, &index );
            return( p );
        }
        break;
    case ARRAY_TYPE:
        {
            static const byte Incr[] = { 3, 4, 6, 0, 8, 10 };

            if( subkind == ARRAY_TYPE_INDEX ) {
                p = GetIndex( p + 2, &index );
            } else {
                p += Incr[subkind];
            }
        }
        return( p );
    case SUBRANGE_TYPE:
        {
            static const byte Incr[] = { 4, 6, 10 };

            p += Incr[subkind];
        }
        return( p );
    case POINTER_TYPE:
        return( p + 2 );
    case STRUCT_TYPE:
        switch( subkind ) {
        case STRUCT_LIST:
            return( NULL );
        case ST_BIT_BYTE:
            p += 2;
            /* fall through */
        case ST_FIELD_BYTE:
            p += 3;
            break;
        case ST_BIT_WORD:
            p += 2;
            /* fall through */
        case ST_FIELD_WORD:
            p += 4;
            break;
        case ST_BIT_LONG:
            p += 2;
            /* fall through */
        case ST_FIELD_LONG:
            p += 6;
            break;
        case ST_FIELD_LOC:
            p = SkipLocation( p + 3 );
            break;
        case ST_BIT_LOC:
            p = SkipLocation( p + 3 ) + 2;
            break;
        case ST_INHERIT:
            p = SkipLocation( p + 2 );
            break;
        }
        return( p );
     case PROC_TYPE:
         return( p + 2 );
    }
    return( NULL );
}

static unsigned long GetScalar( address addr, unsigned scalar )
{
    location_list       src_ll;
    location_list       dst_ll;
    union {
        unsigned_8      u8;
        signed_8        s8;
        unsigned_16     u16;
        signed_16       s16;
        unsigned_32     u32;
        signed_32       s32;
    }                   tmp;

    LocationCreate( &src_ll, LT_ADDR, &addr );
    LocationCreate( &dst_ll, LT_INTERNAL, &tmp );
    DCAssignLocation( &dst_ll, &src_ll, (scalar & SCLR_LEN_MASK) + 1 );
    switch( scalar ) {
    case SCLR_UNSIGNED + 0:
        return( tmp.u8 );
    case SCLR_UNSIGNED + 1:
        return( tmp.u16 );
    case SCLR_UNSIGNED + 3:
        return( tmp.u32 );
    case SCLR_INTEGER  + 0:
        return( tmp.s8 );
    case SCLR_INTEGER  + 1:
        return( tmp.s16 );
    case SCLR_INTEGER  + 3:
        return( tmp.s32 );
    }
    return( 0 );
}

#define TYPE_CACHE_SIZE 64              // must be power of 2

typedef struct {
    imp_image_handle    *iih;
    imp_mod_handle      im;
    unsigned            index;
    struct type_pos     t;
} type_cache;

static type_cache       TypeCache[TYPE_CACHE_SIZE];
static int              TypeCacheRover;

static dip_status FindTypeCache( imp_image_handle *iih, imp_mod_handle im,
                        unsigned index, imp_type_handle *ith )
{
    int         i;
    type_cache  *cache;

    for( i = 0; i < TYPE_CACHE_SIZE; ++i ) {
        cache = TypeCache + i;
        if( cache->iih == iih && cache->im == im && cache->index == index ) {
            ith->im = im;
            ith->f.all = 0;
            ith->t.entry = cache->t.entry;
            ith->t.offset = cache->t.offset;
            return( LoadType( iih, im, ith->t.entry ) );
        }
    }
    return( DS_FAIL );
}


static void SetTypeCache( imp_image_handle *iih, imp_mod_handle im,
                        unsigned index, imp_type_handle *ith )
{
    type_cache  *cache;

    cache = TypeCache + TypeCacheRover++;
    TypeCacheRover &= TYPE_CACHE_SIZE - 1;
    cache->iih = iih;
    cache->im = im;
    cache->index = index;
    cache->t.entry = ith->t.entry;
    cache->t.offset = ith->t.offset;
}


void ClearTypeCache( imp_image_handle *iih )
{
    int         i;

    for( i = 0; i < TYPE_CACHE_SIZE; ++i ) {
        if( TypeCache[i].iih == iih ) {
            TypeCache[i].iih = NULL;
        }
    }
}


static dip_status FindRawTypeHandle( imp_image_handle *iih, imp_mod_handle im,
                unsigned index, imp_type_handle *ith )
{
    const char  *p;
    byte        kind;
    word        entry;
    unsigned    count;

    if( index == 0 )
        return( DS_FAIL );
    if( FindTypeCache( iih, im, index, ith ) == DS_OK ) {
        return( DS_OK );
    }
    entry = 0;
    count = index;
    for( ;; ) {
        if( LoadType( iih, im, entry ) != DS_OK )
            break;
        for( p = Type->start; p < Type->end; p = NEXT_TYPE( p ) ) {
            kind = GETU8( p + 1 );
            switch( kind & CLASS_MASK ) {
            case ENUM_TYPE :
                if( (kind & SUBCLASS_MASK) == ENUM_LIST ) {
                    --count;
                }
                break;
            case STRUCT_TYPE :
                if( (kind & SUBCLASS_MASK) == STRUCT_LIST ) {
                    --count;
                }
                break;
            case NAME_TYPE:
                switch( kind & SUBCLASS_MASK ) {
                case TYPE_CUE_TABLE:
                    break;
                case TYPE_EOF:
                    FreeLoad();
                    return( DS_FAIL );
                default:
                    --count;
                    break;
                }
                break;
            default :
                --count;
            }
            if( count == 0 ) {
                ith->im = im;
                ith->f.all = 0;
                ith->t.entry = entry;
                ith->t.offset = p - Type->start;
                SetTypeCache( iih, im, index, ith );
                return( DS_OK );
            }
        }
        ++entry;
    }
    return( DS_FAIL );
}

static int CharName( const char *name, unsigned len )
{
    if( len > 4 ) {
        name += len - 5;
        if( name[0] != ' ' )
            return( 0 );
        ++name;
    }
    if( memcmp( name, "char", 4 ) == 0 )
        return( 1 );
    return( 0 );
}


static byte GetRealTypeHandle( imp_image_handle *iih, imp_type_handle *ith )
{
    const char  *p;
    const char  *start;
    unsigned    index;
    byte        is_char;
    unsigned    len;

    ith->f.s.chr = 0;
    is_char = 1;
    for( ;; ) {
        p = Type->start + ith->t.offset;
        if( GETU8( p + 1 ) != NAME_TYPE + TYPE_NAME ) {
            if( is_char
              && GETU8( p ) >= 7
              && GETU8( p + 1 ) == NAME_TYPE + TYPE_SCALAR
              && CharName( p + 3, GETU8( p ) - 3 ) ) {
                ith->f.s.chr = 1;
            }
            return( GETU8( p + 1 ) );
        }
        len = GETU8( p );
        start = p;
        p = GetIndex( p + 2, &index ); /* scope index */
        p = GetIndex( p, &index );
        len -= (p - start);
        if( !(is_char && len >= 4 && CharName( p, len )) ) {
            is_char = 0;
        }
        if( FindRawTypeHandle( iih, ith->im, index, ith ) != DS_OK )
            return( NO_TYPE );
        if( is_char )
            ith->f.s.chr = 1;
        is_char = 0;
    }
}

static dip_status DoFindTypeHandle( imp_image_handle *iih, imp_mod_handle im,
                unsigned index, imp_type_handle *ith )
{
    byte                type;
    const char          *p;
    imp_type_handle     base_ith;
    dip_status          ret;

    ret = FindRawTypeHandle( iih, im, index, ith );
    if( ret != DS_OK )
        return( ret );
    type = GetRealTypeHandle( iih, ith );
    switch( type & CLASS_MASK ) {
    case NO_TYPE:
        return( DS_FAIL );
    case ARRAY_TYPE:
        if( stricmp( ImpInterface.ModSrcLang( iih, im ), "fortran" ) == 0 ) {
            ith->f.s.col_major = 1;
            ith->f.s.array_ss = 0;
            base_ith = *ith;
            for( ;; ) {
                p = Type->start + base_ith.t.offset;
                if( (GETU8( p + 1 ) & CLASS_MASK) != ARRAY_TYPE )
                    break;
                p = BaseTypePtr( p );
                GetIndex( p, &index );
                FindRawTypeHandle( iih, base_ith.im, index, &base_ith );
                ith->f.s.array_ss++;
            }
        }
        break;
    }
    return( DS_OK );
}

dip_status FindTypeHandle( imp_image_handle *iih, imp_mod_handle im,
                unsigned index, imp_type_handle *ith )
{
    typeinfo            typeld;
    dip_status          ret;

    PushLoad( &typeld );
    ret = DoFindTypeHandle( iih, im, index, ith );
    PopLoad();
    return( ret );
}

typedef enum { ET_ENUM = 1, ET_TYPE = 2 } type_or_enum;

enum { STRUCT_IDX, CLASS_IDX, UNION_IDX, ENUM_IDX, NUM_IDX };
static const char *Scopes[] = { "struct", "class", "union", "enum", NULL };


struct name_state {
    unsigned    scope_idx[NUM_IDX];
    unsigned    curr_idx;
    unsigned    hit_eof         : 1;
    const char  *header;
};

static void InitNameState( struct name_state *state )
{
    memset( state, 0, sizeof( *state ) );
}

static const char *FindAName( struct name_state *state, const char *p,
                        type_or_enum which, lookup_item *li )
{
    const char  *name;
    size_t      len;
    int         (*comp)(void const*,void const*,size_t);
    unsigned    index;
    unsigned    i;
    unsigned    type;

    if( li->case_sensitive ) {
        comp = memcmp;
    } else {
        comp = memicmp;
    }
    for( ; p < Type->end; p = NEXT_TYPE( p ) ) {
        state->curr_idx++;
        switch( GETU8( p + 1 ) & CLASS_MASK ) {
        case ENUM_TYPE:
            if( GETU8( p + 1 ) == ENUM_TYPE + ENUM_LIST ) {
                state->header = p;
            } else {
                state->curr_idx--;
            }
            if( (which & ET_ENUM) == 0 )
                continue;
            break;
        case NAME_TYPE:
            if( (GETU8( p + 1 ) & SUBCLASS_MASK) == TYPE_EOF ) {
                state->hit_eof = 1;
                return( NULL );
            }
            if( (which & ET_TYPE) == 0 )
                continue;
            switch( GETU8( p + 1 ) & SUBCLASS_MASK ) {
            case TYPE_SCALAR:
                type = ST_TYPE;
                break;
            case TYPE_SCOPE:
                name = NamePtr( p );
                len = GETU8( p ) - ( name - p );
                for( i = 0; i < NUM_IDX; ++i ) {
                    if( memcmp( name, Scopes[i], len ) == 0 ) {
                        state->scope_idx[i] = state->curr_idx;
                        break;
                    }
                }
                continue;
            case TYPE_NAME:
                GetIndex( p + 2, &index );
                if( index == 0 ) {
                    type = ST_TYPE;
                } else {
                    for( i = 0; i < NUM_IDX; ++i ) {
                        if( state->scope_idx[i] == index ) {
                            break;
                        }
                    }
                    type = ST_STRUCT_TAG+i;
                }
                break;
            default:
                continue;
            }
            if( type == ST_TYPE && li->type == ST_NONE )
                break;
            if( type == li->type )
                break;
            continue;
        case STRUCT_TYPE:
            if( GETU8( p + 1 ) == STRUCT_TYPE + STRUCT_LIST ) {
                state->header = p;
            } else {
                state->curr_idx--;
            }
            continue;
        default:
            continue;
        }
        name = NamePtr( p );
        len = GETU8( p ) - ( name - p );
        if( len != li->name.len )
            continue;
        if( comp( name, li->name.start, len ) != 0 )
            continue;
        return( p );
    }
    return( NULL );
}

search_result LookupTypeName( imp_image_handle *iih, imp_mod_handle im,
                lookup_item *li, imp_type_handle *ith )
{
    word                entry;
    struct name_state   state;
    const char          *p;
    typeinfo            typeld;
    search_result       sr;

    PushLoad( &typeld );
    InitNameState( &state );
    sr = SR_NONE;
    entry = 0;
    for( ;; ) {
        if( LoadType( iih, im, entry ) != DS_OK )
            break;
        p = FindAName( &state, Type->start, ET_TYPE, li );
        if( p != NULL ) {
            ith->im = im;
            ith->f.all = 0;
            ith->t.entry = entry;
            ith->t.offset = p - Type->start;
            sr = SR_EXACT;
            break;
        }
        if( state.hit_eof )
            break;
        ++entry;
    }
    PopLoad();
    return( sr );
}

static search_result SearchEnumTypeName( imp_image_handle *iih, imp_mod_handle im,
                         lookup_item *li, void *d, type_or_enum which )
{
    word                entry;
    struct name_state   state;
    search_result       sr;
    imp_sym_handle      *ish;
    const char          *p;
    typeinfo            typeld;

    PushLoad( &typeld );
    sr = SR_NONE;
    InitNameState( &state );
    entry = 0;
    for( ;; ) {
        if( LoadType( iih, im, entry ) != DS_OK )
            break;
        p = Type->start;
        for( ;; ) {
            p = FindAName( &state, p, which, li );
            if( p == NULL )
                break;
            ish = DCSymCreate( iih, d );
            ish->im = im;
            ish->name_off = (byte)( NamePtr( p ) - p );
            ish->u.typ.t.entry = entry;
            ish->u.typ.t.offset = p - Type->start;
            if( (GETU8( p + 1 ) & CLASS_MASK) == ENUM_TYPE ) {
                ish->type = SH_CST;
                ish->u.typ.h.offset = state.header - Type->start;
                ish->u.typ.h.entry = entry;
            } else {
                ish->type = SH_TYP;
            }
            sr = SR_EXACT;
            /* we really should continue searching for more names that
               match, but we're going to early out because I know that
               the symbolic info format is too weak to have more than
               one type name or enum const that will match */
            PopLoad();
            return( sr );
        }
        if( state.hit_eof )
            break;
        ++entry;
    }
    PopLoad();
    return( sr );
}

search_result SearchEnumName( imp_image_handle *iih, imp_mod_handle im,
                         lookup_item *li, void *d )
{
    return( SearchEnumTypeName( iih, im, li, d, ET_ENUM ) );
}

search_result SearchTypeName( imp_image_handle *iih, imp_mod_handle im,
                         lookup_item *li, void *d )
{
    return( SearchEnumTypeName( iih, im, li, d, ET_TYPE ) );
}

walk_result DIPIMPENTRY( WalkTypeList )( imp_image_handle *iih, imp_mod_handle im,
                        DIP_IMP_TYPE_WALKER *wk, imp_type_handle *ith, void *d )
{
    const char  *p;
    byte        kind;
    typeinfo    typeld;
    walk_result wr;

    PushLoad( &typeld );
    ith->im = im;
    ith->f.all = 0;
    ith->t.entry = 0;
    for( ;; ) {
        if( LoadType( iih, im, ith->t.entry ) != DS_OK )
            break;
        for( p = Type->start; p < Type->end; p = NEXT_TYPE( p ) ) {
            kind = GETU8( p + 1 );
            switch( kind & CLASS_MASK ) {
            case NAME_TYPE:
                switch( kind & SUBCLASS_MASK ) {
                case TYPE_SCOPE:
                case TYPE_CUE_TABLE:
                    continue;
                case TYPE_EOF:
                    goto done;
                }
                break;
            case ENUM_TYPE :
                if( (kind & SUBCLASS_MASK) != ENUM_LIST ) {
                    continue;
                }
                break;
            case STRUCT_TYPE :
                if( (kind & SUBCLASS_MASK) != STRUCT_LIST ) {
                    continue;
                }
                break;
            case PROC_TYPE:
                if( (kind & SUBCLASS_MASK) == PROC_EXT_PARMS ) {
                    continue;
                }
                break;
            }
            ith->t.offset = p - Type->start;
            wr = wk( iih, ith, d );
            if( wr != WR_CONTINUE ) {
                PopLoad();
                return( wr );
            }
        }
        ith->t.entry++;
    }
done:
    PopLoad();
    return( WR_CONTINUE );
}

static void ScalarInfo( unsigned info, dip_type_info *ti )
{
    ti->size = (info & SCLR_LEN_MASK) + 1;
    switch( info & SCLR_CLASS_MASK ) {
    case SCLR_INTEGER:
        ti->modifier = TM_SIGNED;
        ti->kind = TK_INTEGER;
        break;
    case SCLR_UNSIGNED:
        ti->modifier = TM_UNSIGNED;
        ti->kind = TK_INTEGER;
        break;
    case SCLR_FLOAT:
        ti->kind = TK_REAL;
        break;
    case SCLR_VOID:
        ti->kind = TK_VOID;
        break;
    case SCLR_COMPLEX:
        ti->kind = TK_COMPLEX;
        break;
    }
}


static dip_status GetTypeInfo(imp_image_handle *iih, imp_type_handle *ith,
                    location_context *lc, dip_type_info *ti, unsigned *ndims )
{
    const char          *p;
    byte                subkind;
    imp_type_handle     tmp_ith;
    address             addr;
    array_info          info;
    byte                is_char;
    byte                save_major;
    dip_status          ret;
    unsigned            index;
    unsigned            count;
    unsigned long       max;
    addr_off            offset = 0;
    unsigned            skip;
    typeinfo            typeld;
    location_list       ll;

    PushLoad( &typeld );
    ti->kind = TK_NONE;
    ti->modifier = TM_NONE;
    ti->size = 0;
    if( ith->f.s.gbl ) {
        ti->kind = GblTypeClassify( ith->t.offset );
    } else if( ith->f.s.sclr ) {
        ScalarInfo( ith->t.offset, ti );
    } else {
        is_char = ith->f.s.chr;
        ret = LoadType( iih, ith->im, ith->t.entry );
        if( ret != DS_OK ) {
            PopLoad();
            return( ret );
        }
        if( !GetRealTypeHandle( iih, ith ) ) {
            PopLoad();
            return( DS_FAIL );
        }
        p = ith->t.offset + Type->start;
        subkind = GETU8( p + 1 ) & SUBCLASS_MASK;
        switch( GETU8( p + 1 ) & CLASS_MASK ) {
        case NAME_TYPE:
            if( subkind == TYPE_SCALAR ) {
                ScalarInfo( GETU8( p + 2 ), ti );
                if( is_char && ti->kind == TK_INTEGER && ti->size == 1 ) {
                    ti->kind = TK_CHAR;
                }
            }
            break;
        case ARRAY_TYPE:
            FreeLoad();
            save_major = ith->f.s.col_major;
            ith->f.s.col_major = 0;
            ImpInterface.TypeArrayInfo( iih, ith, lc, &info, NULL );
            ImpInterface.TypeBase( iih, ith, &tmp_ith, NULL, NULL );
            GetTypeInfo( iih, &tmp_ith, lc, ti, ndims );
            ith->f.s.col_major = save_major;
            if( ndims != NULL )
                ++*ndims;
            ti->size *= info.num_elts;
            ti->modifier = TM_NONE;
            ti->kind = TK_ARRAY;
            Type->start = NULL;
            break;
        case SUBRANGE_TYPE:
            ImpInterface.TypeBase( iih, ith, &tmp_ith, NULL, NULL );
            ImpInterface.TypeInfo( iih, &tmp_ith, lc, ti );
            break;
        case POINTER_TYPE:
            {
                static const char PSize[] = {2,4,4,2,4,4,4,6,4,6};
                #define N TM_NEAR
                #define F TM_FAR
                #define H TM_HUGE
                #define D TM_FLAG_DEREF
                static const char PMods[] = {N,F,H,N|D,F|D,H|D,N,F,N|D,F|D};

                ti->size = PSize[subkind];
                ti->modifier = PMods[subkind];
                ti->kind = TK_POINTER;
            }
            break;
        case ENUM_TYPE:
            ScalarInfo( GETU8( p + 4 ), ti );
            ti->kind = TK_ENUM;
            break;
        case STRUCT_TYPE:
            if( GETU8( p ) > 4 ) {
                ti->size = GETU32( p + 4 );
            } else {
                max = 0;
                for( count = GETU16( p + 2 ); count > 0; --count ) {
                    skip = 2;
                    p = NEXT_TYPE( p );
                    switch( GETU8( p + 1 ) ) {
                    case STRUCT_TYPE+ST_BIT_BYTE:
                        skip += 2;
                        /* fall through */
                    case STRUCT_TYPE+ST_FIELD_BYTE:
                        skip += 1;
                        offset = GETU8( p + 2 );
                        break;
                    case STRUCT_TYPE+ST_BIT_WORD:
                        skip += 2;
                        /* fall through */
                    case STRUCT_TYPE+ST_FIELD_WORD:
                        skip += 2;
                        offset = GETU16( p + 2 );
                        break;
                    case STRUCT_TYPE+ST_BIT_LONG:
                        skip += 2;
                        /* fall through */
                    case STRUCT_TYPE+ST_FIELD_LONG:
                        skip += 4;
                        offset = GETU32( p + 2 );
                        break;
                    case STRUCT_TYPE+ST_FIELD_LOC:
                    case STRUCT_TYPE+ST_BIT_LOC:
                    case STRUCT_TYPE+ST_INHERIT:
                        skip = 0;
                        break;
                    }
                    if( skip > 0 ) {
                        GetIndex( p + skip, &index );
                        /* Pulling a trick. We know that the size
                           field will only be absent in older objects,
                           and thus will only have one type section per
                           module. */
                        FindTypeHandle( iih, ith->im, index, &tmp_ith );
                        GetTypeInfo( iih, &tmp_ith, lc, ti, NULL );
                        offset += ti->size;
                        if( offset > max ) {
                            max = offset;
                        }
                    }
                }
                ti->size = max;
            }
            ti->kind = TK_STRUCT;
            break;
        case PROC_TYPE:
            {
                static const char FMods[] = {
                    TM_NEAR,TM_FAR,TM_NEAR,TM_FAR
                };

                ti->modifier = FMods[subkind];
                ti->kind = TK_FUNCTION;
            }
            break;
        case CHAR_TYPE:
            switch( subkind ) {
            case CHAR_BYTE_LEN:
                ti->size = GETU8( p + 2 );
                break;
            case CHAR_WORD_LEN:
                ti->size = GETU16( p + 2 );
                break;
            case CHAR_LONG_LEN:
                ti->size = GETU32( p + 2);
                break;
            case CHAR_DESC_LEN:
                GetAddress( iih, p + 3, &addr, 0 );
                ti->size = GetScalar( addr, GETU8( p + 2 ) );
                break;
            case CHAR_DESC386_LEN:
                GetAddress( iih, p + 3, &addr, 1 );
                ti->size = GetScalar( addr, GETU8( p + 2 ) );
                break;
            case CHAR_DESC_LOC:
                EvalLocation( iih, lc, p + 3, &ll );
                ti->size = GetScalar( ll.e[0].u.addr, GETU8( p + 2 ) );
                break;
            }
            ti->kind = TK_STRING;
        }
    }
    PopLoad();
    return( DS_OK );
}

dip_status DIPIMPENTRY( TypeInfo )(imp_image_handle *iih, imp_type_handle *ith,
                        location_context *lc, dip_type_info *ti )
{
    return( GetTypeInfo( iih, ith, lc, ti, NULL ) );
}

dip_status DIPIMPENTRY( TypeBase )(imp_image_handle *iih, imp_type_handle *ith,
                 imp_type_handle *base_ith,
                 location_context *lc, location_list *ll )
{
    unsigned    index;
    const char  *p;
    dip_status  ret;
    typeinfo    typeld;

    /* unused parameters */ (void)lc; (void)ll;

    PushLoad( &typeld );
    *base_ith = *ith;
    ret = LoadType( iih, ith->im, ith->t.entry );
    if( ret != DS_OK ) {
        PopLoad();
        return( ret );
    }
    p = ith->t.offset + Type->start;
    if( base_ith->f.s.col_major ) {
        if( base_ith->f.s.array_ss > 1 ) {
            base_ith->f.s.array_ss--;
        } else {
            base_ith->f.s.col_major = 0;
            do {
                p = BaseTypePtr( p );
                GetIndex( p, &index );
                FindRawTypeHandle( iih, base_ith->im, index, base_ith );
                p = base_ith->t.offset + Type->start;
            } while( (GETU8( p + 1 ) & CLASS_MASK) == ARRAY_TYPE );
        }
        PopLoad();
    } else if( GETU8( p + 1 ) == ENUM_TYPE+ENUM_LIST ) {
        base_ith->t.offset = GETU8( p + 4 );
        base_ith->f.s.sclr = 1;
        PopLoad();
    } else {
        p = BaseTypePtr( p );
        if( p == NULL )
            return( DS_FAIL );
        GetIndex( p, &index );
        PopLoad();
        FindTypeHandle( iih, ith->im, index, base_ith );
    }
    return( DS_OK );
}

dip_status DIPIMPENTRY( TypeArrayInfo )(imp_image_handle *iih, imp_type_handle *ith,
             location_context *lc, array_info *ai, imp_type_handle *index_ith )
{
    const char          *p;
    unsigned            index_idx;
    unsigned            scalar;
    imp_type_handle     tmp_ith;
    address             addr;
    byte                is_32;
    long                hi = 0;
    dip_type_info       info;
    unsigned            count;
    dip_status          ret;
    typeinfo            typeld;

    if( ith->f.s.col_major ) {
        tmp_ith = *ith;
        tmp_ith.f.s.col_major = 0;
        for( count = tmp_ith.f.s.array_ss-1; count != 0; --count ) {
            ImpInterface.TypeBase( iih, &tmp_ith, &tmp_ith, NULL, NULL );
        }
        ImpInterface.TypeArrayInfo( iih, &tmp_ith, lc, ai, index_ith );
        ai->num_dims = tmp_ith.f.s.array_ss - ai->num_dims + 1;
        ai->column_major = 1;
        return( DS_OK );
    }
    PushLoad( &typeld );
    ret = LoadType( iih, ith->im, ith->t.entry );
    if( ret != DS_OK ) {
        PopLoad();
        return( ret );
    }
    is_32 = 1;
    index_idx = 0;
    scalar = SCLR_UNSIGNED | 3;
    p = ith->t.offset + Type->start;
    switch( GETU8( p + 1 ) ) {
    case ARRAY_TYPE + ARRAY_BYTE_INDEX:
        ai->num_elts = GETU8( p + 2 ) + 1;
        ai->low_bound = 0;
        break;
    case ARRAY_TYPE+ARRAY_WORD_INDEX:
        ai->num_elts = GETU16( p + 2 ) + 1;
        ai->low_bound = 0;
        break;
    case ARRAY_TYPE+ARRAY_LONG_INDEX:
        ai->num_elts = GETU32( p + 2 ) + 1;
        ai->low_bound = 0;
        break;
    case ARRAY_TYPE+ARRAY_TYPE_INDEX:
        GetIndex( p + 2, &index_idx );
        ret = FindTypeHandle( iih, ith->im, index_idx, &tmp_ith );
        if( ret != DS_OK ) {
            PopLoad();
            return( ret );
        }
        ret = LoadType( iih, tmp_ith.im, tmp_ith.t.entry );
        if( ret != DS_OK ) {
            PopLoad();
            return( ret );
        }
        p = tmp_ith.t.offset + Type->start;
        switch( GETU8( p + 1 ) ) {
        case SUBRANGE_TYPE+SUBRANGE_BYTE:
            ai->low_bound = GETS8( p + 2 );
            hi = GETS8( p + 3 );
            break;
        case SUBRANGE_TYPE+SUBRANGE_WORD:
            ai->low_bound = GETS16( p + 2 );
            hi = GETS16( p + 4 );
            break;
        case SUBRANGE_TYPE+SUBRANGE_LONG:
            ai->low_bound = GETS32( p + 2 );
            hi = GETS32( p + 6 );
            break;
        }
        ai->num_elts = (hi - ai->low_bound) + 1;
        break;
    case ARRAY_TYPE+ARRAY_DESC_INDEX:
        is_32 = 0;
        /* fall through */
    case ARRAY_TYPE+ARRAY_DESC386_INDEX:
        GetAddress( iih, p + 4, &addr, is_32 );
        scalar = GETU8( p + 2 );
        ai->low_bound = GetScalar( addr, scalar );
        addr.mach.offset += (scalar & SCLR_LEN_MASK) + 1;
        ai->num_elts = GetScalar( addr, GETU8( p + 3 ) );
        break;
    }
    PopLoad();
    ImpInterface.TypeBase( iih, ith, &tmp_ith, NULL, NULL );
    ai->num_dims = 1;
    ret = GetTypeInfo( iih, &tmp_ith, lc, &info, &ai->num_dims );
    if( ret != DS_OK )
        return( ret );
    ai->stride = info.size;
    ai->column_major = 0;
    if( index_ith != NULL ) {
        if( index_idx != 0 ) {
            FindTypeHandle( iih, ith->im, index_idx, index_ith );
        } else {
            index_ith->im = ith->im;
            index_ith->f.all = 0;
            index_ith->f.s.sclr = 1;
            index_ith->t.offset = scalar;
        }
    }
    return( DS_OK );
}

dip_status DIPIMPENTRY( TypeProcInfo )(imp_image_handle *iih, imp_type_handle *ith,
                 imp_type_handle *parm_ith, unsigned num )
{
    const char  *p;
    const char  *end;
    unsigned    index;
    dip_status  ret;
    typeinfo    typeld;

    PushLoad( &typeld );
    ret = LoadType( iih, ith->im, ith->t.entry );
    if( ret != DS_OK ) {
        PopLoad();
        return( ret );
    }
    p = Type->start + ith->t.offset;
    end = NEXT_TYPE( p );
    p = GetIndex( p + 2, &index );
    if( num == 0 ) {
        /* nothing to do */
    } else if( num <= GETU8( p ) ) {
        ++p;
        for( ;; ) {
            if( p == end ) {
                /* handle EXT_PARMS record */
                p += 2;
                end = NEXT_TYPE( p );
            }
            p = GetIndex( p, &index );
            --num;
            if( num == 0 ) {
                break;
            }
        }
    } else {
        PopLoad();
        return( DS_NO_PARM );
    }
    PopLoad();
    return( FindTypeHandle( iih, ith->im, index, parm_ith ) );
}

dip_status DIPIMPENTRY( TypePtrAddrSpace )( imp_image_handle *iih,
                    imp_type_handle *ith, location_context *lc, address *addr )
{
    const char          *p;
    const char          *end;
    dip_status          ret;
    location_list       ll;
    unsigned            dummy;
    typeinfo            typeld;

    PushLoad( &typeld );
    ret = LoadType( iih, ith->im, ith->t.entry );
    if( ret != DS_OK ) {
        PopLoad();
        return( ret );
    }
    p = Type->start + ith->t.offset;
    end = NEXT_TYPE( p );
    p = GetIndex( p + 2, &dummy );
    if( p >= end ) {
        PopLoad();
        return( DS_FAIL );
    }
    LocationCreate( &ll, LT_ADDR, addr );
    PushBaseLocation( &ll );
    ret = EvalLocation( iih, lc, p, &ll );
    PopLoad();
    if( ret != DS_OK )
        return( ret );
    if( ll.num != 1 || ll.e[0].type != LT_ADDR ) {
        return( DS_ERR|DS_BAD_LOCATION );
    }
    *addr = ll.e[0].u.addr;
    return( DS_OK );
}

unsigned SymHdl2CstName( imp_image_handle *iih, imp_sym_handle *ish,
                        char *buff, unsigned buff_size )
{
    const char  *p;
    unsigned    len;
    typeinfo    typeld;

    PushLoad( &typeld );
    if( LoadType( iih, ish->im, ish->u.typ.t.entry ) != DS_OK ) {
        PopLoad();
        return( 0 );
    }
    p = Type->start + ish->u.typ.t.offset;
    len = GETU8( p ) - ish->name_off;
    if( buff_size > 0 ) {
        --buff_size;
        if( buff_size > len )
            buff_size = len;
        memcpy( buff, p + ish->name_off, buff_size );
        buff[buff_size] = '\0';
    }
    PopLoad();
    return( len );
}

unsigned SymHdl2TypName( imp_image_handle *iih, imp_sym_handle *ish,
                        char *buff, unsigned buff_size )
{
    return( SymHdl2CstName( iih, ish, buff, buff_size ) );
}

unsigned SymHdl2MbrName( imp_image_handle *iih, imp_sym_handle *ish,
                        char *buff, unsigned buff_size )
{
    return( SymHdl2CstName( iih, ish, buff, buff_size ) );
}

dip_status SymHdl2CstValue( imp_image_handle *iih, imp_sym_handle *ish, void *d )
{
    const char          *p;
    const char          *e;
    unsigned_64         val;
    dip_status          ret;
    typeinfo            typeld;

    PushLoad( &typeld );
    ret = LoadType( iih, ish->im, ish->u.typ.t.entry );
    if( ret != DS_OK ) {
        PopLoad();
        return( ret );
    }
    memset( &val, 0, sizeof( val ) );
    p = Type->start + ish->u.typ.t.offset;
    switch( GETU8( p + 1 ) ) {
    case ENUM_TYPE+ENUM_CONST_BYTE:
        val.u._32[0] = GETS8( p + 2 );
        break;
    case ENUM_TYPE+ENUM_CONST_WORD:
        val.u._32[0] = GETS16( p + 2 );
        break;
    case ENUM_TYPE+ENUM_CONST_LONG:
        val.u._32[0] = GETS32( p + 2 );
        break;
    case ENUM_TYPE+ENUM_CONST_I64:
        memcpy( &val, p + 2, sizeof( val ) );
        break;
    }
    e = Type->start + ish->u.typ.h.offset;
    memcpy( d, &val, (GETU8( e + 4 ) & SCLR_LEN_MASK) + 1 );
    PopLoad();
    return( DS_OK );
}

dip_status SymHdl2CstType( imp_image_handle *iih, imp_sym_handle *ish,
                        imp_type_handle *ith )
{
    /* unused parameters */ (void)iih;

    ith->im = ish->im;
    ith->t = ish->u.typ.h;
    ith->f.all = 0;
    return( DS_OK );
}

dip_status SymHdl2TypType( imp_image_handle *iih, imp_sym_handle *ish,
                        imp_type_handle *ith )
{
    /* unused parameters */ (void)iih;

    ith->im = ish->im;
    ith->t = ish->u.typ.t;
    ith->f.all = 0;
    return( DS_OK );
}

dip_status SymHdl2MbrType( imp_image_handle *iih, imp_sym_handle *ish,
                        imp_type_handle *ith )
{
    const char  *p;
    unsigned    index;
    dip_status  ret;
    typeinfo    typeld;

    PushLoad( &typeld );
    ret = LoadType( iih, ish->im, ish->u.typ.t.entry );
    if( ret != DS_OK ) {
        PopLoad();
        return( ret );
    }
    p = Type->start + ish->u.typ.t.offset;
    p = BaseTypePtr( p );
    GetIndex( p, &index );
    PopLoad();
    return( FindTypeHandle( iih, ish->im, index, ith ) );
}


struct pending_entry {
    struct pending_entry        *prev;
    unsigned short              off;
};

struct anc_graph {
    struct anc_graph            *prev;
    union {
        struct pending_entry    *todo;
        struct {
            unsigned short      off;
            unsigned short      count;
        }                       s;
    }                           u;
    word                        entry;
};

dip_status SymHdl2MbrLoc( imp_image_handle *iih, imp_sym_handle *ish,
                         location_context *lc, location_list *ll )
{
    const char          *p;
    unsigned            count;
    struct anc_graph    *pending, *new, *tmp;
    imp_type_handle     new_ith;
    unsigned            index;
    dip_status          ok;
    location_list       adj;
    addr_off            offset = 0;
    unsigned            bit_start;
    unsigned            bit_len;
    location_info       info;
    typeinfo            typeld;

    PushLoad( &typeld );
    ok = LoadType( iih, ish->im, ish->u.typ.t.entry );
    if( ok != DS_OK ) {
        PopLoad();
        return( ok );
    }
    p = Type->start + ish->u.typ.t.offset;
    switch( GETU8( p + 1 ) ) {
    case STRUCT_TYPE+ST_FIELD_LOC:
    case STRUCT_TYPE+ST_BIT_LOC:
        info = InfoLocation( p + 3 );
        break;
    default:
        info = NEED_BASE;
        break;
    }
    if( info & (NEED_BASE|EMPTY_EXPR) ) {
        ok = LoadType( iih, ish->im, ish->u.typ.h.entry );
        if( ok != DS_OK ) {
            PopLoad();
            return( ok );
        }
        p = Type->start + ish->u.typ.h.offset;
        pending = NULL;
        count = GETU16( p + 2 );
        for( ;; ) {
            if( count == 0 ) {
                if( pending == NULL )
                    return( DS_FAIL );
                ok = LoadType( iih, ish->im, pending->entry );
                if( ok != DS_OK ) {
                    PopLoad();
                    return( ok );
                }
                count = pending->u.s.count;
                p = Type->start + pending->u.s.off;
                pending = pending->prev;
            }
            --count;
            p = NEXT_TYPE( p );
            if( GETU8( p + 1 ) == STRUCT_TYPE+ST_INHERIT ) {
                new = walloca( sizeof( *new ) );
                new->entry = Type->entry;
                new->u.s.off = p - Type->start;
                new->u.s.count = count;
                new->prev = pending;
                pending = new;
                p = SkipLocation( p + 2 );
                GetIndex( p, &index );
                ok = DoFindTypeHandle( iih, ish->im, index, &new_ith );
                if( ok != DS_OK ) {
                    PopLoad();
                    return( ok );
                }
                p = Type->start + new_ith.t.offset;
                count = GETU16( p + 2 );
            } else {
                if( ish->u.typ.t.entry == Type->entry
                  && ish->u.typ.t.offset == (p - Type->start) ) {
                    break;
                }
            }
        }
        FreeLoad();
        /* reverse the inheritance list */
        new = NULL;
        while( pending != NULL ) {
            tmp = pending;
            pending = tmp->prev;
            tmp->prev = new;
            new = tmp;
        }
        /* do the adjustors at each level */
        ok = DCItemLocation( lc, CI_OBJECT, ll );
        if( ok != DS_OK ) {
            PopLoad();
            DCStatus( ok );
            return( ok );
        }
        while( new != NULL ) {
            ok = LoadType( iih, ish->im, new->entry );
            if( ok != DS_OK ) {
                PopLoad();
                return( ok );
            }
            PushBaseLocation( ll );
            p = new->u.s.off + Type->start;
            ok = EvalLocation( iih, lc, p + 2, &adj );
            if( ok != DS_OK ) {
                PopLoad();
                return( ok );
            }
            if( adj.num != 1 || adj.e[0].type != LT_ADDR ) {
                PopLoad();
                DCStatus( DS_ERR|DS_BAD_LOCATION );
                return( DS_ERR|DS_BAD_LOCATION );
            }
            LocationAdd( ll, adj.e[0].u.addr.mach.offset * 8 );
            new = new->prev;
        }
        ok = LoadType( iih, ish->im, ish->u.typ.t.entry );
        if( ok != DS_OK ) {
            PopLoad();
            return( ok );
        }
        p = Type->start + ish->u.typ.t.offset;
    }
    /* do field offset and bit field selection */
    bit_start = 0;
    bit_len = 0;
    switch( GETU8( p + 1 ) ) {
    case STRUCT_TYPE + ST_BIT_BYTE:
        bit_start = GETU8( p + 3 );
        bit_len   = GETU8( p + 4 );
        /* fall through */
    case STRUCT_TYPE + ST_FIELD_BYTE:
        offset = GETU8( p + 2 );
        break;
    case STRUCT_TYPE + ST_BIT_WORD:
        bit_start = GETU8( p + 4 );
        bit_len   = GETU8( p + 5 );
        /* fall through */
    case STRUCT_TYPE + ST_FIELD_WORD:
        offset = GETU16( p + 2 );
        break;
    case STRUCT_TYPE+ST_BIT_LONG:
        bit_start = GETU8( p + 6 );
        bit_len   = GETU8( p + 7 );
        /* fall through */
    case STRUCT_TYPE+ST_FIELD_LONG:
        offset = GETU32( p + 2 );
        break;
    case STRUCT_TYPE+ST_FIELD_LOC:
    case STRUCT_TYPE+ST_BIT_LOC:
        if( info & (NEED_BASE|EMPTY_EXPR) )
            PushBaseLocation( ll );
        ok = EvalLocation( iih, lc, p + 3, ll );
        if( ok != DS_OK ) {
            PopLoad();
            return( ok );
        }
        if( GETU8( p + 1 ) == STRUCT_TYPE+ST_BIT_LOC ) {
            p = SkipLocation( p + 3 );
            bit_start = GETU8( p );
            bit_len = GETU8( p + 1 );
        }
        offset = 0;
    }
    PopLoad();
    LocationAdd( ll, offset * 8 + bit_start );
    LocationTrunc( ll, bit_len );
    return( DS_OK );
}

dip_status SymHdl2MbrInfo( imp_image_handle *iih, imp_sym_handle *ish,
                        sym_info *si, location_context *lc )
{
    const char          *p;
    unsigned            index;
    dip_status          ret;
    unsigned            attrib;
    imp_type_handle     tmp_ith;
    imp_sym_handle      func_ish;
    byte                kind;
    location_list       ll;
    typeinfo            typeld;

    PushLoad( &typeld );
    ret = LoadType( iih, ish->im, ish->u.typ.t.entry );
    if( ret != DS_OK ) {
        PopLoad();
        return( ret );
    }
    p = Type->start + ish->u.typ.t.offset;
    switch( GETU8( p + 1 ) ) {
    case STRUCT_TYPE+ST_FIELD_LOC:
    case STRUCT_TYPE+ST_BIT_LOC:
        attrib = GETU8( p + 2 );
        break;
    default:
        attrib = 0;
        break;
    }
    p = BaseTypePtr( p );
    GetIndex( p, &index );
    ret = FindRawTypeHandle( iih, ish->im, index, &tmp_ith );
    if( ret != DS_OK )
        return( ret );
    kind = GetRealTypeHandle( iih, &tmp_ith ) & CLASS_MASK;
    PopLoad();
    switch( kind ) {
    case PROC_TYPE:
        si->kind = SK_PROCEDURE;
        ret = SymHdl2MbrLoc( iih, ish, lc, &ll );
        if( ret == DS_OK ) {
            func_ish.im = ish->im;
            if( LookupLclAddr( iih, ll.e[0].u.addr, &func_ish ) == SR_EXACT ) {
                ret = SymHdl2LclInfo( iih, &func_ish, si );
            }
        }
        break;
    default:
        si->kind = SK_DATA;
        break;
    }
    si->is_member = 1;
    if( attrib & 0x01 )
        si->compiler = 1;
    if( attrib & 0x02 )
        si->is_public = 1;
    if( attrib & 0x04 )
        si->is_protected = 1;
    if( attrib & 0x08 )
        si->is_private = 1;
    return( ret );
}

dip_status DIPIMPENTRY( TypeThunkAdjust )( imp_image_handle *iih,
                    imp_type_handle *oith, imp_type_handle *mith,
                    location_context *lc, address *addr )
{
    const char          *p;
    unsigned            count;
    struct anc_graph    *pending, *new, *tmp;
    imp_type_handle     new_ith;
    unsigned            index;
    dip_status          ok;
    location_list       adj;
    typeinfo            typeld;

    if( oith->im != mith->im )
        return( DS_FAIL );
    PushLoad( &typeld );
    ok = LoadType( iih, oith->im, oith->t.entry );
    if( ok != DS_OK ) {
        PopLoad();
        return( ok );
    }
    p = Type->start + oith->t.offset;
    pending = NULL;
    count = GETU16( p + 2 );
    for( ;; ) {
        if( count == 0 ) {
            if( pending == NULL )
                return( DS_FAIL );
            ok = LoadType( iih, oith->im, pending->entry );
            if( ok != DS_OK ) {
                PopLoad();
                return( ok );
            }
            count = pending->u.s.count;
            p = Type->start + pending->u.s.off;
            pending = pending->prev;
        }
        --count;
        p = NEXT_TYPE( p );
        if( GETU8( p + 1 ) == STRUCT_TYPE+ST_INHERIT ) {
            new = walloca( sizeof( *new ) );
            new->entry = Type->entry;
            new->u.s.off = p - Type->start;
            new->u.s.count = count;
            new->prev = pending;
            pending = new;
            p = SkipLocation( p + 2 );
            GetIndex( p, &index );
            ok = DoFindTypeHandle( iih, oith->im, index, &new_ith );
            if( ok != DS_OK ) {
                PopLoad();
                return( ok );
            }
            if( new_ith.t.offset == mith->t.offset
              && new_ith.t.entry == mith->t.entry )
                break;
            p = Type->start + new_ith.t.offset;
            count = GETU16( p + 2 );
        }
    }
    /* reverse the inheritance list */
    new = NULL;
    while( pending != NULL ) {
        tmp = pending;
        pending = tmp->prev;
        tmp->prev = new;
        new = tmp;
    }
    /* do the adjustors at each level */
    while( new != NULL ) {
        ok = LoadType( iih, oith->im, new->entry );
        if( ok != DS_OK ) {
            PopLoad();
            return( ok );
        }
        p = new->u.s.off + Type->start;
        ok = EvalLocation( iih, lc, p + 2, &adj );
        if( !ok ) {
            PopLoad();
            return( ok );
        }
        if( adj.num != 1 || adj.e[0].type != LT_ADDR ) {
            PopLoad();
            DCStatus( DS_ERR|DS_BAD_LOCATION );
            return( DS_ERR|DS_BAD_LOCATION );
        }
        addr->mach.offset += adj.e[0].u.addr.mach.offset;
        new = new->prev;
    }
    PopLoad();
    return( DS_OK );
}

search_result SearchMbr( imp_image_handle *iih, imp_type_handle *ith,
                 lookup_item *li, void *d )
{
    const char          *p;
    unsigned            count;
    search_result       sr;
    struct anc_graph    *pending, *new;
    imp_type_handle     new_ith;
    unsigned            index;
    int                 (*comp)(void const*,void const*,size_t);
    imp_sym_handle      *ish;
    const char          *name;
    size_t              len;
    typeinfo            typeld;

    PushLoad( &typeld );
    if( LoadType( iih, ith->im, ith->t.entry ) != DS_OK ) {
        PopLoad();
        return( SR_NONE );
    }
    p = Type->start + ith->t.offset;
    if( (GETU8( p + 1 ) & CLASS_MASK) != STRUCT_TYPE ) {
        PopLoad();
        return( SR_NONE );
    }
    if( li->case_sensitive ) {
        comp = memcmp;
    } else {
        comp = memicmp;
    }
    sr = SR_NONE;
    pending = NULL;
    count = GETU16( p + 2 );
    for( ;; ) {
        if( count == 0 ) {
            if( pending == NULL )
                break;
            if( LoadType( iih, ith->im, pending->entry ) != DS_OK ) {
                PopLoad();
                return( WR_STOP );
            }
            count = pending->u.s.count;
            p = Type->start + pending->u.s.off;
            pending = pending->prev;
        }
        --count;
        p = NEXT_TYPE( p );
        if( GETU8( p + 1 ) == STRUCT_TYPE+ST_INHERIT ) {
            new = walloca( sizeof( *new ) );
            new->entry = Type->entry;
            new->u.s.off = p - Type->start;
            new->u.s.count = count;
            new->prev = pending;
            pending = new;
            p = SkipLocation( p + 2 );
            GetIndex( p, &index );
            if( DoFindTypeHandle( iih, ith->im, index, &new_ith ) != DS_OK ) {
                PopLoad();
                return( WR_STOP );
            }
            p = Type->start + new_ith.t.offset;
            count = GETU16( p + 2 );
        } else {
            name = NamePtr( p );
            len = GETU8( p ) - ( name - p );
            if( len == li->name.len && comp( name, li->name.start, len ) == 0 ) {
                ish = DCSymCreate( iih, d );
                ish->u.typ.t.offset = p - Type->start;
                ish->u.typ.t.entry = Type->entry;
                ish->name_off = (byte)( name - p );
                ish->im = ith->im;
                ish->u.typ.h = ith->t;
                ish->type = SH_MBR;
                sr = SR_EXACT;
            }
        }
    }
    PopLoad();
    return( sr );
}

walk_result WalkTypeSymList( imp_image_handle *iih, imp_type_handle *ith,
                 DIP_IMP_SYM_WALKER *wk, imp_sym_handle *ish, void *d )
{
    const char                  *p;
    unsigned                    count;
    walk_result                 wr;
    struct anc_graph            *pending, *new;
    imp_type_handle             new_ith;
    unsigned                    index;
    struct pending_entry        *list, *new_entry, *used;
    typeinfo                    typeld;

    PushLoad( &typeld );
    if( LoadType( iih, ith->im, ith->t.entry ) != DS_OK ) {
        PopLoad();
        return( WR_STOP );
    }
    ish->im = ith->im;
    ish->u.typ.h = ith->t;
    list = NULL;
    used = NULL;
    pending = NULL;
    p = Type->start + ith->t.offset;
    switch( GETU8( p + 1 ) & CLASS_MASK ) {
    case STRUCT_TYPE:
        ish->type = SH_MBR;
        goto do_walk;
    case ENUM_TYPE:
        ish->type = SH_CST;
do_walk:
        count = GETU16( p + 2 );
        wr = WR_CONTINUE;
        for( ;; ) {
            while( count != 0 ) {
                /* structure list is backwards -- reverse it */
                p = NEXT_TYPE( p );
                if( used == NULL ) {
                    new_entry = walloca( sizeof( *new_entry ) );
                } else {
                    new_entry = used;
                    used = used->prev;
                }
                new_entry->off = p - Type->start;
                new_entry->prev = list;
                list = new_entry;
                --count;
            }
            if( list == NULL ) {
                if( pending == NULL )
                    break;
                wk( iih, SWI_INHERIT_END, NULL, d );
                if( LoadType( iih, ith->im, pending->entry ) != DS_OK ) {
                    PopLoad();
                    return( WR_STOP );
                }
                list = pending->u.todo;
                pending = pending->prev;
            }
            p = list->off + Type->start;
            if( GETU8( p + 1 ) == STRUCT_TYPE+ST_INHERIT ) {
                if( wk( iih, SWI_INHERIT_START, NULL, d ) == WR_CONTINUE ) {
                    if( list->prev != NULL ) {
                        new = walloca( sizeof( *new ) );
                        new->entry = Type->entry;
                        new->u.todo = list->prev;
                        new->prev = pending;
                        pending = new;
                    }
                    p = SkipLocation( p + 2 );
                    GetIndex( p, &index );
                    if( DoFindTypeHandle( iih, ith->im, index, &new_ith ) != DS_OK ) {
                        PopLoad();
                        return( WR_STOP );
                    }
                    list = NULL;
                    p = Type->start + new_ith.t.offset;
                    count = GETU16( p + 2 );
                    /* setting count will cause the list to be reversed */
                    continue;
                }
            } else {
                ish->u.typ.t.offset = p - Type->start;
                ish->u.typ.t.entry = Type->entry;
                ish->name_off = (byte)( NamePtr( p ) - p );
                wr = wk( iih, SWI_SYMBOL, ish, d );
                if( wr != WR_CONTINUE ) {
                    break;
                }
            }
            new_entry = list->prev;
            list->prev = used;
            used = list;
            list = new_entry;
        }
        break;
    default:
        wr = WR_STOP;
        break;
    }
    PopLoad();
    return( wr );
}

imp_mod_handle DIPIMPENTRY( TypeMod )( imp_image_handle *iih, imp_type_handle *ith )
{
    /* unused parameters */ (void)iih;

    return( ith->im );
}

const char *FindSpecCueTable( imp_image_handle *iih, imp_mod_handle im, const char **base )
{
    typeinfo            typeld;
    const char          *p;
    word                entry;
    unsigned long       offset;
    unsigned            size;

    PushLoad( &typeld );
    if( LoadType( iih, im, 0 ) != DS_OK )
        goto missing;
    for( p = Type->start; p < Type->end; p = NEXT_TYPE( p ) ) {
        if( GETU8( p + 1 ) == (NAME_TYPE+TYPE_CUE_TABLE) ) {
            offset = GETU32( p + 2 );
            entry = 0;
            for( ;; ) {
                size = InfoSize( iih, im, DMND_TYPES, entry );
                if( size == 0 )
                    goto missing;
                if( size > offset )
                    break;
                offset -= size;
                ++entry;
            }
            if( LoadType( iih, im, entry ) != DS_OK )
                goto missing;
            p = Type->start;
            *base = p;
            InfoSpecLock( p ); /* so that the PopLoad doesn't free it */
            PopLoad();
            return( p + (unsigned)offset );
        }
    }
missing:
    *base = NULL;
    PopLoad();
    return( NULL );
}

int DIPIMPENTRY( TypeCmp )( imp_image_handle *iih, imp_type_handle *ith1, imp_type_handle *ith2 )
{
    /* unused parameters */ (void)iih;

    if( ith1->im < ith2->im )
        return( -1 );
    if( ith1->im > ith2->im )
        return( 1 );
    if( ith1->t.entry < ith2->t.entry )
        return( -1 );
    if( ith1->t.entry > ith2->t.entry )
        return( 1 );
    if( ith1->t.offset < ith2->t.offset )
        return( -1 );
    if( ith1->t.offset > ith2->t.offset )
        return( 1 );
    return( 0 );
}


size_t DIPIMPENTRY( TypeName )( imp_image_handle *iih, imp_type_handle *ith,
                unsigned num, symbol_type *tag, char *buff, size_t buff_size )
{
    /* unused parameters */ (void)iih; (void)ith; (void)num; (void)tag; (void)buff; (void)buff_size;

    //NYI: stub implementation
    return( 0 );
}

dip_status DIPIMPENTRY( TypeAddRef )( imp_image_handle *iih, imp_type_handle *ith )
{
    /* unused parameters */ (void)iih; (void)ith;

    return(DS_OK);
}

dip_status DIPIMPENTRY( TypeRelease )( imp_image_handle *iih, imp_type_handle *ith )
{
    /* unused parameters */ (void)iih; (void)ith;

    return(DS_OK);
}

dip_status DIPIMPENTRY( TypeFreeAll )( imp_image_handle *iih )
{
    /* unused parameters */ (void)iih;

    return(DS_OK);
}

