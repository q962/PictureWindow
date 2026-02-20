#pragma once

#include "stdafx.h"
#include "AppWidget.h"

G_BEGIN_DECLS

#if 1 // gobject definition :: AppIcon

 G_DECLARE_DERIVABLE_TYPE( AppIcon, app_icon, APP, ICON, AppWidget )

struct _AppIconClass {
	AppWidgetClass parent_instance;
};

AppIcon* app_icon_new();

#endif

G_END_DECLS

