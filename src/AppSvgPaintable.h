#pragma once

#include "stdafx.h"

G_BEGIN_DECLS

#if 1  // gobject definition :: AppSvgPaintable

G_DECLARE_FINAL_TYPE( AppSvgPaintable, app_svg_paintable, APP, SVG_PAINTABLE, GObject )

AppSvgPaintable* app_svg_paintable_new( const char* path );

#endif

G_END_DECLS
