#include "AppSvgPaintable.h"

#include <librsvg/rsvg.h>

#if 1  // gobject definition :: AppSvgPaintable

struct _AppSvgPaintable {
	GObject parent_instance;

	RsvgHandle* handle;

	char* path;
};

enum {
	PROP_0,
	PROP_N,
};

static void gdk_paintable_iface_init( GdkPaintableInterface* iface );

G_DEFINE_TYPE_WITH_CODE( AppSvgPaintable,
                         app_svg_paintable,
                         g_object_get_type(),
                         G_IMPLEMENT_INTERFACE( GDK_TYPE_PAINTABLE, gdk_paintable_iface_init ) )

#if 1  // static function

static void _load_svg( AppSvgPaintable* self, const char* path )
{
	GError*     error  = NULL;
	GFile*      file   = g_file_new_for_path( path );
	RsvgHandle* handle = rsvg_handle_new_from_gfile_sync( file, RSVG_HANDLE_FLAGS_NONE, NULL, &error ); /* 1 */

	if ( !handle ) {
		g_critical( "could not load: %s", error->message );
	}

	self->handle = handle;
}

#endif

#if 1  // base class virtual function

#if 1  // gdk_paintable_iface_init

static void gdk_paintable_iface_snapshot( GdkPaintable* paintable, GdkSnapshot* snapshot, double width, double height )
{
	AppSvgPaintable* self = ( gpointer )paintable;

	GError* error = NULL;

	int picture_width  = width;
	int picture_height = height;

	cairo_t* cr = gtk_snapshot_append_cairo( snapshot, &GRAPHENE_RECT_INIT( 0, 0, picture_width, picture_height ) );

	RsvgRectangle viewport = {
		.x      = 0.0,
		.y      = 0.0,
		.width  = picture_width,
		.height = picture_height,
	};

	if ( !rsvg_handle_render_document( self->handle, cr, &viewport, &error ) ) {
		g_critical( "could not render: %s", error->message );
	}
}

static GdkPaintable* gdk_paintable_iface_get_current_image( GdkPaintable* paintable )
{
	AppSvgPaintable* self = ( gpointer )paintable;
	;
}

static GdkPaintableFlags gdk_paintable_iface_get_flags( GdkPaintable* paintable )
{
	return GDK_PAINTABLE_STATIC_SIZE;
}

static int gdk_paintable_iface_get_intrinsic_width( GdkPaintable* paintable )
{
	AppSvgPaintable* self = ( gpointer )paintable;

	double width = 0;

	if ( self->handle )
		rsvg_handle_get_intrinsic_size_in_pixels( self->handle, &width, NULL );

	return width;
}

static int gdk_paintable_iface_get_intrinsic_height( GdkPaintable* paintable )
{
	AppSvgPaintable* self = ( gpointer )paintable;

	double height = 0;

	if ( self->handle )
		rsvg_handle_get_intrinsic_size_in_pixels( self->handle, NULL, &height );

	return height;
}

static double gdk_paintable_iface_get_intrinsic_aspect_ratio( GdkPaintable* paintable )
{
	AppSvgPaintable* self = ( gpointer )paintable;

	double width  = 0;
	double height = 0;

	if ( self->handle ) {
		rsvg_handle_get_intrinsic_size_in_pixels( self->handle, &width, &height );

		return width / height;
	}

	return 0.0;
}

static void gdk_paintable_iface_init( GdkPaintableInterface* iface )
{
	iface->snapshot                   = gdk_paintable_iface_snapshot;
	iface->get_current_image          = gdk_paintable_iface_get_current_image;
	iface->get_flags                  = gdk_paintable_iface_get_flags;
	iface->get_intrinsic_width        = gdk_paintable_iface_get_intrinsic_width;
	iface->get_intrinsic_height       = gdk_paintable_iface_get_intrinsic_height;
	iface->get_intrinsic_aspect_ratio = gdk_paintable_iface_get_intrinsic_aspect_ratio;
}

#endif

static void app_svg_paintable_set_property( GObject* object, guint property_id, const GValue* value, GParamSpec* pspec )
{
	AppSvgPaintable* self = ( AppSvgPaintable* )object;

	switch ( property_id ) {
		default: G_OBJECT_WARN_INVALID_PROPERTY_ID( object, property_id, pspec ); break;
	}
}

static void app_svg_paintable_get_property( GObject* object, guint property_id, GValue* value, GParamSpec* pspec )
{
	AppSvgPaintable* self = ( AppSvgPaintable* )object;

	switch ( property_id ) {
		default: G_OBJECT_WARN_INVALID_PROPERTY_ID( object, property_id, pspec ); break;
	}
}

static void app_svg_paintable_constructed( GObject* object )
{
	AppSvgPaintable* self = ( AppSvgPaintable* )object;

	G_OBJECT_CLASS( app_svg_paintable_parent_class )->constructed( object );
}

static void app_svg_paintable_dispose( GObject* object )
{
	AppSvgPaintable* self = ( AppSvgPaintable* )object;

	G_OBJECT_CLASS( app_svg_paintable_parent_class )->dispose( object );
}

static void app_svg_paintable_finalize( GObject* object )
{
	AppSvgPaintable* self = ( AppSvgPaintable* )object;

	g_clear_object( &self->handle );

	G_OBJECT_CLASS( app_svg_paintable_parent_class )->finalize( object );
}

static void app_svg_paintable_init( AppSvgPaintable* self ) {}

static void app_svg_paintable_class_init( AppSvgPaintableClass* klass )
{
	GObjectClass* base_class   = ( GObjectClass* )klass;
	GObjectClass* parent_class = ( GObjectClass* )klass;

	base_class->constructed  = app_svg_paintable_constructed;
	base_class->dispose      = app_svg_paintable_dispose;
	base_class->finalize     = app_svg_paintable_finalize;
	base_class->set_property = app_svg_paintable_set_property;
	base_class->get_property = app_svg_paintable_get_property;
}

#endif

#if 1  // public function

AppSvgPaintable* app_svg_paintable_new( const char* path )
{
	AppSvgPaintable* self = g_object_new( app_svg_paintable_get_type(), NULL );

	_load_svg( self, path );

	return self;
}

#endif

#endif
