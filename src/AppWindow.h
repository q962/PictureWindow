#pragma once

#include "stdafx.h"

G_BEGIN_DECLS

#if 1  // gobject definition :: AppWindow

#define APP_WINDOW_SIGNAL_VISIBLE_POPOVER "visible-popover"  // (GtkPopover*)

G_DECLARE_FINAL_TYPE( AppWindow, app_window, APP, WINDOW, GtkApplicationWindow )

AppWindow* app_window_new( const char** paths );

#endif

G_END_DECLS
