#pragma once

#include "stdafx.h"
#include "AppWidget.h"

G_BEGIN_DECLS

#if 1  // gobject definition :: AppButton

#define APP_BUTTON_SIGNAL_CLICKED "clicked"

G_DECLARE_FINAL_TYPE( AppButton, app_button, APP, BUTTON, AppWidget )

AppButton* app_button_new( GtkWidget* widget );

#endif

G_END_DECLS
