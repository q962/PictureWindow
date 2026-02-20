#pragma once

#include "stdafx.h"
#include "AppWidget.h"

G_BEGIN_DECLS

#if 1  // gobject definition :: AppPictureView

// (gboolean is_moving)
#define APP_PICTURE_VIEW_SIGNAL_MOVING "moving"

#define APP_PICTURE_VIEW_PROP_ZOOM "zoom"                            // double
#define APP_PICTURE_VIEW_PROP_ZOOM_TARGET "zoom-target"              // double
#define APP_PICTURE_VIEW_PROP_PICTURE_VIEW_SIZE "picture-view-size"  // (dd)
#define APP_PICTURE_VIEW_PROP_PICTURE_POSITION "picture-position"    // (dd)
#define APP_PICTURE_VIEW_PROP_PICTURE_SIZE "picture-size"            // (uu)
#define APP_PICTURE_VIEW_PROP_VIEW_MODE "view-mode"                  // AppPictureViewMode

typedef enum {
	APP_PICTURE_VIEW_MODE_NONE,
	APP_PICTURE_VIEW_MODE_MANUAL_ZOOM,
	APP_PICTURE_VIEW_MODE_FIT_CONTAIN,
	APP_PICTURE_VIEW_MODE_CONTER
} AppPictureViewMode;

typedef enum {
	APP_PICTURE_VIEW_CONFIG_0,
	APP_PICTURE_VIEW_CONFIG_CAN_MOVE,  // boolean
	APP_PICTURE_VIEW_CONFIG_N,
} AppPictureViewConfig;

G_DECLARE_FINAL_TYPE( AppPictureView, app_picture_view, APP, PICTURE_VIEW, AppWidget )

/// @brief
/// @param path
/// @param paintable (ref)
/// @return
AppPictureView* app_picture_view_new( const char* path, GdkPaintable* paintable );

void          app_picture_view_set_paintable( AppPictureView* self, GdkPaintable* paintable );
GdkPaintable* app_picture_view_get_paintable( AppPictureView* self );

void app_picture_view_to_fit_contain( AppPictureView* self );
void app_picture_view_to_center( AppPictureView* self );

/// @brief 将指定位置居中显示
/// @param self
/// @param x 原始尺寸下的坐标
/// @param y 原始尺寸下的坐标
void app_picture_view_center_to( AppPictureView* self, double x, double y, gboolean enable_animation );

void app_picture_view_set_zoom( AppPictureView* self, double zoom, gboolean enable_animation );
void app_picture_view_set_zoom_level( AppPictureView* self, double level, gboolean enable_animation );
void app_picture_view_adjust_zoom_level( AppPictureView* self, double delta, gboolean enable_animation );

gboolean app_picture_view_contains( AppPictureView* self, double x, double y );

AppPictureViewMode app_picture_view_get_mode( AppPictureView* self );

void app_picture_view_play( AppPictureView* self );
void app_picture_view_stop( AppPictureView* self );

void app_picture_view_config( AppPictureView* self, AppPictureViewConfig config, ... );
void app_picture_view_get_config( AppPictureView* self, AppPictureViewConfig config, ... );

#endif

G_END_DECLS
