#pragma once

#include "stdafx.h"
#include "AppWidget.h"

G_BEGIN_DECLS

#if 1  // gobject definition :: AppViewNavigator

#define APP_VIEW_NAVIGATOR_SIGNAL_TO_POSITION "to-position"  // (double, double)

G_DECLARE_FINAL_TYPE( AppViewNavigator, app_view_navigator, APP, VIEW_NAVIGATOR, AppWidget )

AppViewNavigator* app_view_navigator_new();

void app_view_navigator_set_paintable( AppViewNavigator* self, GdkPaintable* paintable );

void app_view_navigator_set_view_rect( AppViewNavigator* self, graphene_rect_t* rect );

#endif

G_END_DECLS
