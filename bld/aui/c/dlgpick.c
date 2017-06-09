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
* Description:  WHEN YOU FIGURE OUT WHAT THIS MODULE DOES, PLEASE
*               DESCRIBE IT HERE!
*
****************************************************************************/


#include "auipvt.h"

static int              DlgChosen;
static const void       *_data_handle;
static GUIPICKGETTEXT   *_getstring;
static int              _items;

static void PickInit( gui_window *gui, gui_ctl_id list_id )
{
    GUIAddTextList( gui, list_id, _items, _data_handle, _getstring );
    GUISetCurrSelect( gui, list_id, DlgChosen );
}

int DlgPickWithRtn2( const char *title, const void *data_handle, int def, GUIPICKGETTEXT *getstring, int items, WNDPICKER *pickfn )
{
    DlgChosen = def;
    _data_handle = data_handle;
    _getstring = getstring;
    _items = items;
    return( pickfn( title, &PickInit ) );
}


static int DoDlgPick( const char *title, GUIPICKCALLBACK *pickinit )
{
    return( GUIDlgPickWithRtn( title, pickinit, DlgOpen ) );
}


int DlgPickWithRtn( const char *title, const void *data_handle, int def, GUIPICKGETTEXT *getstring, int items )
{
    return( DlgPickWithRtn2( title, data_handle, def, getstring, items, DoDlgPick ) );
}

static const char *DlgPickText( const void *data_handle, int item )
{
    return( ((const char **)data_handle)[item] );
}

int DlgPick( const char *title, const void *data_handle, int def, int items )
{
    return( DlgPickWithRtn( title, data_handle, def, DlgPickText, items ) );
}
