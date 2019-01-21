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
* Description:  dynamic string (VBUF) related declarations
*
****************************************************************************/


#define VBUF_INIT_BUF       ""
#define VBUF_INIT_LEN       1
#define VBUF_INIT_USED      0

#define VBUF_INIT_STRUCT    { VBUF_INIT_BUF, VBUF_INIT_LEN, VBUF_INIT_USED }

typedef struct vbuf{            // VBUF structure
    char           *buf;        // - buffer pointer
    size_t         len;         // - buffer size
    size_t         used;        // - amount used in buffer
} VBUF;

// PROTOTYPES:

void VbufReqd(                  // ENSURE BUFFER IS OF SUFFICIENT SIZE
    VBUF *vbuf,                 // - VBUF structure
    size_t reqd )               // - required size
;
void VbufSetLen(                // SET BUFFER LENGTH
    VBUF *vbuf,                 // - VBUF structure
    size_t size )               // - new length
;
void VbufInit(                  // INITIALIZE BUFFER STRUCTURE
    VBUF *vbuf )                // - VBUF structure
;
void VbufFree(                  // FREE BUFFER
    VBUF *vbuf )                // - VBUF structure
;
int VbufCompVbuf(               // COMPARE A VBUF
    const VBUF *vbuf1,          // - VBUF structure
    const VBUF *vbuf2,          // - VBUF structure
    bool igncase )              // - bool ignore case
;
int VbufCompStr(                // COMPARE A VBUFs
    const VBUF *vbuf1,          // - VBUF structure
    const char *str,            // - string
    bool igncase )              // - bool ignore case
;
int VbufCompBuffer(             // COMPARE A VBUFs
    const VBUF *vbuf1,          // - VBUF structure
    char const *buffer,         // - buffer
    size_t size,                // - size of buffer
    bool igncase )              // - bool ignore case
;
void VbufConcVbuf(              // CONCATENATE A VBUF TO VBUF
    VBUF *vbuf1,                // - VBUF structure
    const VBUF *vbuf2 )         // - VBUF structure
;
void VbufPrepVbuf(              // PREPEND A VBUF TO VBUF
    VBUF *vbuf1,                // - VBUF structure
    const VBUF *vbuf2 )         // - VBUF structure to be prepended
;
void VbufSetVbuf(               // SET A VBUF
    VBUF *vbuf1,                // - VBUF structure
    const VBUF *vbuf2 )         // - VBUF structure
;
void VbufConcVbufPos(           // CONCATENATE A VBUF TO VBUF
    VBUF *vbuf1,                // - VBUF structure
    const VBUF *vbuf2,          // - VBUF structure
    size_t start_pos )          // - start position in second VBUF
;
void VbufPrepVbufPos(           // PREPEND A VBUF TO VBUF
    VBUF *vbuf1,                // - VBUF structure
    const VBUF *vbuf2,          // - VBUF structure to be prepended
    size_t start_pos )          // - start position in second VBUF
;
void VbufSetVbufPos(            // SET A VBUF TO VBUF
    VBUF *vbuf1,                // - VBUF structure
    const VBUF *vbuf2,          // - VBUF structure
    size_t start_pos )          // - start position in second VBUF
;
void VbufConcBuffer(            // CONCATENATE A BUFFER TO VBUF
    VBUF *vbuf,                 // - VBUF structure
    char const *buffer,         // - buffer
    size_t size )               // - size of buffer
;
void VbufPrepBuffer(            // PREPEND A BUFFER TO VBUF
    VBUF *vbuf,                 // - VBUF structure
    char const *buffer,         // - buffer
    size_t size )               // - size of buffer
;
void VbufSetBuffer(             // SET A BUFFER
    VBUF *vbuf,                 // - VBUF structure
    char const *buffer,         // - buffer
    size_t size )               // - size of buffer
;
void VbufConcStr(               // CONCATENATE A STRING TO VBUF
    VBUF *vbuf,                 // - VBUF structure
    char const *string )        // - string to be concatenated
;
void VbufPrepStr(               // PREPEND STRING TO VBUF
    VBUF *vbuf,                 // - VBUF structure
    char const *string )        // - string to be prepended
;
void VbufSetStr(                // SET STRING TO vbuf->buf
    VBUF *vbuf,                 // - VBUF structure
    char const *string )        // - string to be concatenated
;
void VbufConcChr(               // CONCATENATE A CHAR TO VBUF
    VBUF *vbuf,                 // - VBUF structure
    char chr )                  // - char to be concatenated
;
void VbufPrepChr(               // PREPEND CHAR TO vbuf->buf
    VBUF *vbuf,                 // - VBUF structure
    char chr )                  // - char to be prepended
;
void VbufSetChr(                // CONCATENATE CHAR TO vbuf->buf
    VBUF *vbuf,                 // - VBUF structure
    char chr )                  // - char to be concatenated
;
#if 0
void VbufConcDecimal(           // CONCATENATE A DECIMAL TO VBUF
    VBUF *vbuf,                 // - VBUF structure
    unsigned value )            // - value to be concatenated
;
void VbufConcI64(               // CONCATENATE A I64 TO VBUF
    VBUF *vbuf,                 // - VBUF structure
    signed_64 value )           // - value to be concatenated
;
#endif
void VbufConcInteger(           // CONCATENATE A INTEGER TO VBUF
    VBUF *vbuf,                 // - VBUF structure
    int value,                  // - value to be concatenated
    int digits )                // - minimal number of digits, prepend leading '0' if necessary
;
void VbufSetInteger(            // SET A INTEGER TO VBUF
    VBUF *vbuf,                 // - VBUF structure
    int value,                  // - value to be concatenated
    int digits )                // - minimal number of digits, prepend leading '0' if necessary
;
void VbufTruncWhite(            // TRUNCATE TRAILING WHITESPACE FROM vbuf->buf
    VBUF *vbuf )                // - VBUF structure
;
void VbufAddDirSep(             // TERMINATE A VBUF AS PATH BY DIR_SEP
    VBUF *vbuf )                // - VBUF structure
;
void VbufRemDirSep(             // REMOVE DIR_SEP FROM A VBUF END
    VBUF *vbuf )                // - VBUF structure
;
void VbufMakepath(              // SET A FILE PATH NAME TO VBUF
    VBUF *vbuf,                 // - full file path
    const VBUF *drive,          // - file drive
    const VBUF *dir,            // - file directory
    const VBUF *name,           // - file name
    const VBUF *ext )           // - file extension
;
void VbufSplitpath(             // GET A FILE PATH COMPONENTS FROM VBUF
    const VBUF *full,           // - full file path
    VBUF *drive,                // - file drive
    VBUF *dir,                  // - file directory
    VBUF *name,                 // - file name
    VBUF *ext )                 // - file extension
;
void VbufFullpath(              // GET A FULL FILE PATH TO VBUF
    VBUF *vbuf,                 // - VBUF structure
    const VBUF *file )          // - file name VBUF
;
bool VbufSetPathDrive(          // SET A DRIVE FOR FILE PATH IN VBUF
    VBUF *vbuf,                 // - full file path
    char drive )                // - drive character
;
void VbufSetPathExt(            // SET A FILE EXTENSION FOR FILE PATH IN VBUF
    VBUF *vbuf,                 // - full file path
    const VBUF *new_ext )       // - file extension
;
#define VbufLen(v)              ((v)->used)                         // RETURN LENGTH OF BUFFER
;
#define VbufRewind(v)           VbufSetLen(v,0)                     // CLEAN BUFFER
;
#define VbufSetPosBack(v,n)     VbufSetLen(v,(v)->used - n)         // SHORTEN BUFFER LENGTH
;
#define VbufBuffer(v)           ((const unsigned char *)(v)->buf)   // RETURN POINTER OF BUFFER
;
#define VbufString(v)           ((const char *)(v)->buf)            // RETURN POINTER OF BUFFER
;
#define VbufSize(v)             ((v)->len)                          // RETURN SIZE OF BUFFER
;
