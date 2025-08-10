/****************************************************************************
*
*                            Open Watcom Project
*
* Copyright (c) 2025      The Open Watcom Contributors. All Rights Reserved.
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


#include "variety.h"
#include <stddef.h>
#include <string.h>


#if defined( _M_I86 )

#if defined(__SMALL_DATA__)

extern void     movebwd( char _WCFAR *d, const char _WCFAR *s, size_t len);
#pragma aux movebwd = \
        "std"           \
        "push ds"       \
        "xchg si,ax"    \
        "mov  ds,ax"    \
        "dec  si"       \
        "dec  di"       \
        "shr  cx,1"     \
        "rep  movsw"    \
        "adc  cx,cx"    \
        "inc  si"       \
        "inc  di"       \
        "rep  movsb"    \
        "pop  ds"       \
        "cld"           \
    __parm              [__es __di] [__si __ax] [__cx] \
    __value             \
    __modify __exact    [__di __si __cx __ax]

extern void     movefwd( char _WCFAR *d, const char _WCFAR *s, size_t len);
#pragma aux movefwd = \
        "push ds"       \
        "xchg si,ax"    \
        "mov  ds,ax"    \
        "shr  cx,1"     \
        "rep  movsw"    \
        "adc  cx,cx"    \
        "rep  movsb"    \
        "pop  ds"       \
    __parm              [__es __di] [__si __ax] [__cx] \
    __value             \
    __modify __exact    [__di __si __cx __ax]

#else

extern  void    movebwd( char _WCFAR *d, const char _WCFAR *s, size_t len);
#pragma aux     movebwd =  \
        "std"           \
        "push ds"       \
        "xchg si,ax"    \
        "mov  ds,ax"    \
        "dec  si"       \
        "dec  di"       \
        "shr  cx,1"     \
        "rep  movsw"    \
        "adc  cx,cx"    \
        "inc  si"       \
        "inc  di"       \
        "rep  movsb"    \
        "pop  ds"       \
        "cld"           \
    __parm              [__es __di] [__si __ax] [__cx] \
    __value             \
    __modify __exact    [__di __si __cx __ax]

extern void     movefwd( char _WCFAR *d, const char _WCFAR *s, size_t len);
#pragma aux movefwd = \
        "push ds"       \
        "xchg si,ax"    \
        "mov  ds,ax"    \
        "shr  cx,1"     \
        "rep  movsw"    \
        "adc  cx,cx"    \
        "rep  movsb"    \
        "pop  ds"       \
    __parm              [__es __di] [__si __ax] [__cx] \
    __value             \
    __modify __exact    [__di __si __cx __ax]

#endif

#elif defined( __386__ )

#if defined(__FLAT__)

extern void     movefwd( char _WCFAR *d, const char _WCFAR *s, size_t len);
#pragma aux movefwd = \
        "push es"       \
        "mov  es,edx"   \
        "push ds"       \
        "xchg esi,eax"  \
        "mov  ds,eax"   \
        "shr  ecx,1"    \
        "rep  movsw"    \
        "adc  ecx,ecx"  \
        "rep  movsb"    \
        "pop  ds"       \
        "pop  es"       \
    __parm              [__dx __edi] [__si __eax] [__ecx] \
    __value             \
    __modify __exact    [__eax __ecx __edi __esi]

#else

extern void     movefwd( char _WCFAR *d, const char _WCFAR *s, size_t len);
#pragma aux movefwd = \
        "push ds"       \
        "xchg esi,eax"  \
        "mov  ds,eax"   \
        "shr  ecx,1"    \
        "rep  movsw"    \
        "adc  ecx,ecx"  \
        "rep  movsb"    \
        "pop  ds"       \
    __parm              [__es __edi] [__si __eax] [__ecx] \
    __value             \
    __modify __exact    [__eax __ecx __edi __esi]

#endif

#else

#error platform not supported

#endif


_WCRTLINK void _WCFAR *_fmemmove( void _WCFAR *vd, const void _WCFAR *vs, size_t len )
{
    char _WCFAR *d = vd;
    const char _WCFAR *s = vs;
    if( s == d
      || len == 0 ) {
        return( d );
    }
    if( s < d
      && s + len > d ) {  /* if buffers are overlapped*/
#if defined(__HUGE__) || defined(__386__)
        d += len;
        s += len;
        while( len != 0 ) {
            d--;
            s--;
            *d = *s;
            len--;
        }
#else
        movebwd( ( d + len ) - 1, ( s + len ) - 1, len );
#endif
    } else {
        movefwd( d, s, len );
    }
    return( d );
}
