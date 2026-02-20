#include "AppViewNavigator.h"

#if 1  // gobject definition :: AppViewNavigator

struct _AppViewNavigator {
	AppWidget parent_instance;

	graphene_rect_t view_rect;

	double mouse_x, mouse_y;

	gboolean is_drag;

	GdkPaintable* paintable;
};

enum {
	PROP_0,
	PROP_VIEW_RECT,
	PROP_N,
};

static GParamSpec* pspecs[ PROP_N ] = {};

static guint SIGNAL_TO_POSITION = 0;

G_DEFINE_TYPE( AppViewNavigator, app_view_navigator, app_widget_get_type() )

#if 1  // static function

static void _emit_to_position( AppViewNavigator* self )
{
	if ( !self->paintable )
		return;

	int width  = gtk_widget_get_width( ( gpointer )self );
	int height = gtk_widget_get_height( ( gpointer )self );

	int picture_width  = gdk_paintable_get_intrinsic_width( self->paintable );
	int picture_height = gdk_paintable_get_intrinsic_height( self->paintable );

	g_signal_emit( self,
	               SIGNAL_TO_POSITION,
	               0,
	               self->mouse_x / ( ( double )width / picture_width ),
	               self->mouse_y / ( ( double )height / picture_height ) );
}

static void _on_motion( GtkEventController* motion_controller, gdouble x, gdouble y, gpointer user_data )
{
	AppViewNavigator* self = user_data;

	self->mouse_x = x;
	self->mouse_y = y;

	if ( self->is_drag ) {
		_emit_to_position( self );
	}
}

static void _on_motion_enter( GtkEventController* motion_controller, gpointer user_data )
{
	AppViewNavigator* self = user_data;
}

static void _on_motion_leave( GtkEventController* motion_controller, gpointer user_data )
{
	AppViewNavigator* self = user_data;
}

static void _on_click_pressed( GtkGestureClick* event, gint n_press, gdouble x, gdouble y, AppViewNavigator* self )
{
	self->is_drag = TRUE;
}

static void _on_click_released( GtkGestureClick* event, gint n_press, gdouble x, gdouble y, AppViewNavigator* self )
{
	self->is_drag = FALSE;

	_emit_to_position( self );
}

#endif

#if 1  // base class virtual function

static GtkSizeRequestMode app_view_navigator_get_request_mode( GtkWidget* widget )
{
	return GTK_SIZE_REQUEST_HEIGHT_FOR_WIDTH;
}

static void app_view_navigator_measure( GtkWidget*     widget,
                                        GtkOrientation orientation,
                                        int            for_size,
                                        int*           minimum,
                                        int*           natural,
                                        int*           minimum_baseline,
                                        int*           natural_baseline )
{
	AppViewNavigator* self = ( gpointer )widget;

	if ( !self->paintable )
		return;

	int picture_width  = gdk_paintable_get_intrinsic_width( self->paintable );
	int picture_height = gdk_paintable_get_intrinsic_height( self->paintable );

	double ratio = ( double )picture_height / ( double )picture_width;

	if ( ratio < 1.0 ) {
		if ( orientation == GTK_ORIENTATION_HORIZONTAL ) {
			*minimum = 0;
			*natural = 300;
		}
		else {
			*minimum = 0;
			*natural = MAX( for_size * ratio, 50 );
		}
	}
	else {
		if ( orientation == GTK_ORIENTATION_HORIZONTAL ) {
			*minimum = 0;
			*natural = MAX( ( 300.0 / picture_height ) * picture_width, 50 );
		}
		else {
			*minimum = 0;
			*natural = MAX( for_size * ratio, 50 );
		}
	}
}

static void app_view_navigator_size_allocate( GtkWidget* widget, int width, int height, int baseline )
{
	AppViewNavigator* self = ( gpointer )widget;
}

static void app_view_navigator_snapshot_append_view_rect( AppViewNavigator* self,
                                                          GtkSnapshot*      snapshot,
                                                          graphene_rect_t*  self_rect )
{
	int picture_width  = gdk_paintable_get_intrinsic_width( self->paintable );
	int picture_height = gdk_paintable_get_intrinsic_height( self->paintable );

	float x  = self_rect->origin.x + self->view_rect.origin.x / ( picture_width / self_rect->size.width );
	float y  = self_rect->origin.y + self->view_rect.origin.y / ( picture_height / self_rect->size.height );
	float w  = self->view_rect.size.width / ( picture_width / self_rect->size.width );
	float h  = self->view_rect.size.height / ( picture_height / self_rect->size.height );
	float cl = 30.0;
	float r  = 2;

	if ( w < 0.0000001 || h < 0.000001 ) {
		return;
	}

	GskPathBuilder* builder = gsk_path_builder_new();

	gsk_path_builder_move_to( builder, x, y + cl );
	gsk_path_builder_conic_to( builder, x, y, x + cl, y, r );

	gsk_path_builder_move_to( builder, x + w - cl, y );
	gsk_path_builder_conic_to( builder, x + w, y, x + w, y + cl, r );

	gsk_path_builder_move_to( builder, x, y + h - cl );
	gsk_path_builder_conic_to( builder, x, y + h, x + cl, y + h, r );

	gsk_path_builder_move_to( builder, x + w, y + h - cl );
	gsk_path_builder_conic_to( builder, x + w, y + h, x + w - cl, y + h, r );

	GskPath* path = gsk_path_builder_free_to_path( builder );
	{
		GskStroke* stroke = gsk_stroke_new( 6 );
		gsk_stroke_set_line_cap( stroke, GSK_LINE_CAP_ROUND );

		// float dash[ 2 ] = { 10, 10 };
		// gsk_stroke_set_dash( stroke, dash, 2 );
		// gsk_stroke_set_dash_offset( stroke, 0 );

		GdkRGBA color = { 0.0, 0.0, 0.0, 1 };
		gtk_snapshot_append_stroke( snapshot, path, stroke, &color );

		gsk_stroke_free( stroke );
	}

	gsk_path_unref( path );
}

static void app_view_navigator_snapshot( GtkWidget* widget, GtkSnapshot* snapshot )
{
	AppViewNavigator* self = ( gpointer )widget;

	if ( !self->paintable )
		return;

	int width  = gtk_widget_get_width( widget );
	int height = gtk_widget_get_height( widget );

	int picture_width  = gdk_paintable_get_intrinsic_width( self->paintable );
	int picture_height = gdk_paintable_get_intrinsic_height( self->paintable );

	double ratio = ( double )picture_height / ( double )picture_width;

	graphene_rect_t rect = GRAPHENE_RECT_INIT( 0, 0, width, width * ratio );

	rect.origin.y = ( height - rect.size.height ) / 2;
	if ( GDK_IS_TEXTURE( self->paintable ) ) {
		gtk_snapshot_append_texture( snapshot, ( gpointer )self->paintable, &rect );
	}
	else {
		gtk_snapshot_translate( snapshot, &GRAPHENE_POINT_INIT( rect.origin.x, rect.origin.y ) );

		gdk_paintable_snapshot( self->paintable, snapshot, rect.size.width, rect.size.height );
	}

	app_view_navigator_snapshot_append_view_rect( self, snapshot, &rect );
}

static void app_view_navigator_constructed( GObject* object )
{
	AppViewNavigator* self = ( AppViewNavigator* )object;

	G_OBJECT_CLASS( app_view_navigator_parent_class )->constructed( object );
}

static void app_view_navigator_dispose( GObject* object )
{
	AppViewNavigator* self = ( AppViewNavigator* )object;

	g_clear_object( &self->paintable );

	G_OBJECT_CLASS( app_view_navigator_parent_class )->dispose( object );
}

static void app_view_navigator_finalize( GObject* object )
{
	AppViewNavigator* self = ( AppViewNavigator* )object;

	G_OBJECT_CLASS( app_view_navigator_parent_class )->finalize( object );
}

static void app_view_navigator_init( AppViewNavigator* self )
{
	gtk_widget_set_overflow( ( gpointer )self, GTK_OVERFLOW_HIDDEN );

	{
		GtkEventController* motion_controller = ( gpointer )gtk_event_controller_motion_new();
		gtk_event_controller_set_propagation_phase( motion_controller, GTK_PHASE_CAPTURE );
		g_signal_connect( motion_controller, "motion", G_CALLBACK( _on_motion ), self );
		g_signal_connect( motion_controller, "enter", G_CALLBACK( _on_motion_enter ), self );
		g_signal_connect( motion_controller, "leave", G_CALLBACK( _on_motion_leave ), self );
		gtk_widget_add_controller( ( gpointer )self, motion_controller );

		GtkEventController* click_controller = ( gpointer )gtk_gesture_click_new();
		gtk_event_controller_set_propagation_phase( click_controller, GTK_PHASE_CAPTURE );
		gtk_gesture_single_set_button( ( gpointer )click_controller, GDK_BUTTON_PRIMARY );
		g_signal_connect( click_controller, "pressed", G_CALLBACK( _on_click_pressed ), self );
		g_signal_connect( click_controller, "released", G_CALLBACK( _on_click_released ), self );
		gtk_widget_add_controller( ( gpointer )self, click_controller );
	}
}

static void app_view_navigator_class_init( AppViewNavigatorClass* klass )
{
	GObjectClass*   base_class   = ( GObjectClass* )klass;
	GtkWidgetClass* widget_class = ( GtkWidgetClass* )klass;

	base_class->constructed = app_view_navigator_constructed;
	base_class->dispose     = app_view_navigator_dispose;
	base_class->finalize    = app_view_navigator_finalize;

	widget_class->get_request_mode = app_view_navigator_get_request_mode;
	widget_class->measure          = app_view_navigator_measure;
	widget_class->size_allocate    = app_view_navigator_size_allocate;
	widget_class->snapshot         = app_view_navigator_snapshot;

	SIGNAL_TO_POSITION = g_signal_new( APP_VIEW_NAVIGATOR_SIGNAL_TO_POSITION,
	                                   app_view_navigator_get_type(),
	                                   G_SIGNAL_RUN_LAST,
	                                   0,
	                                   NULL,
	                                   NULL,
	                                   NULL,
	                                   G_TYPE_NONE,
	                                   2,
	                                   G_TYPE_DOUBLE,
	                                   G_TYPE_DOUBLE );

	gtk_widget_class_set_css_name( widget_class, "AppViewNavigator" );
}

#endif

#if 1  // public function

AppViewNavigator* app_view_navigator_new()
{
	AppViewNavigator* self = g_object_new( app_view_navigator_get_type(), NULL );
	return self;
}

void app_view_navigator_set_paintable( AppViewNavigator* self, GdkPaintable* paintable )
{
	g_return_if_fail( APP_IS_VIEW_NAVIGATOR( self ) );
	g_return_if_fail( GDK_IS_PAINTABLE( paintable ) );

	g_set_object( &self->paintable, paintable );
}

void app_view_navigator_set_view_rect( AppViewNavigator* self, graphene_rect_t* rect )
{
	g_return_if_fail( APP_IS_VIEW_NAVIGATOR( self ) );

	self->view_rect = *rect;

	gtk_widget_queue_draw( ( gpointer )self );
}

#endif

#endif
