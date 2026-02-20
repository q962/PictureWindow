#pragma once

#include "stdafx.h"

G_BEGIN_DECLS

#if 1  // gobject definition :: AppWidget

G_DECLARE_DERIVABLE_TYPE( AppWidget, app_widget, APP, WIDGET, GtkWidget )

struct _AppWidgetClass {
	GtkWidgetClass parent_instance;
};

AppWidget* app_widget_new();

#endif

G_END_DECLS
