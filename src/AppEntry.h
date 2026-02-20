#pragma once

#include "stdafx.h"

G_BEGIN_DECLS

#if 1  // gobject definition :: AppEntry

G_DECLARE_FINAL_TYPE( AppEntry, app_entry, APP, ENTRY, GtkEntry )

AppEntry* app_entry_new( char* before, char* defalut_val, char* after );

#endif

G_END_DECLS
