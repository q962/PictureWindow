#include "AppPictureView.h"
#include "AppSvgPaintable.h"

#include <math.h>
#include <glycin-gtk4.h>

#define FCLAMP( x, min, max ) ( fmin( fmax( ( x ), ( min ) ), ( max ) ) )

G_DEFINE_ENUM_TYPE( AppPictureViewMode,
                    app_picture_view_mode,
                    G_DEFINE_ENUM_VALUE( APP_PICTURE_VIEW_MODE_NONE, "none" ),
                    G_DEFINE_ENUM_VALUE( APP_PICTURE_VIEW_MODE_MANUAL_ZOOM, "manual_zoom" ),
                    G_DEFINE_ENUM_VALUE( APP_PICTURE_VIEW_MODE_FIT_CONTAIN, "fit_contain" ),
                    G_DEFINE_ENUM_VALUE( APP_PICTURE_VIEW_MODE_CONTER, "conter" ) )

#if 1  // gobject definition :: AppPictureView

struct _AppPictureView {
	AppWidget parent_instance;

	GdkPaintable* paintable;

	AppPictureViewMode view_mode;

	gboolean space_pressed;

	double mouse_x, mouse_y;
	double mouse_begin_x, mouse_begin_y;

	guint  picture_width;
	guint  picture_height;
	double picture_x;
	double picture_y;

	double zoom_level;
	double zoom_factor;
	double zoom, zoom_begin, zoom_target;

	double min_level, max_level;

	gboolean is_zooming;
	gint64   zoom_begin_time;

	gboolean is_offset;
	gboolean is_dragging;
	double   drag_begin_x, drag_begin_y;
	double   drag_picture_begin_x;
	double   drag_picture_begin_y;

	gboolean is_moving;
	gint64   move_begin_time;

	double move_picture_begin_x;
	double move_picture_begin_y;
	double move_picture_end_x;
	double move_picture_end_y;

	guint tick_id;

	GtkPicture* picture;

	GlyImage*     gly_image;
	GCancellable* gly_image_cancellable;

	gboolean can_play;
	gboolean should_play;
	gint64   play_begin_time;
	gint64   play_duration;
	guint    play_tick_id;

	gboolean can_move;
};

enum {
	PROP_0,
	PROP_ZOOM,
	PROP_ZOOM_TARGET,
	PROP_PICTURE_VIEW_SIZE,
	PROP_PICTURE_POSITION,
	PROP_PICTURE_SIZE,
	PROP_VIEW_MODE,
	PROP_N,
};

static GParamSpec* pspecs[ PROP_N ] = {};

static guint _SIGNAL_MOVING = 0;

G_DEFINE_TYPE( AppPictureView, app_picture_view, app_widget_get_type() )

#if 1  // static function

static void _set_offset( AppPictureView* self, double x, double y )
{
	if ( self->is_moving )
		return;

	double width  = ( double )gtk_widget_get_width( ( gpointer )self );
	double height = ( double )gtk_widget_get_height( ( gpointer )self );

	double picture_view_width  = self->picture_width * self->zoom;
	double picture_view_height = self->picture_height * self->zoom;

	if ( picture_view_width < width ) {
		self->picture_x = FCLAMP( x, -picture_view_width / 2.0, width - picture_view_width / 2.0 );
	}
	else {
		self->picture_x = FCLAMP( x, -picture_view_width + width / 2.0, width / 2.0 );
	}

	if ( picture_view_height < height ) {
		self->picture_y = FCLAMP( y, -picture_view_height / 2.0, height - picture_view_height / 2.0 );
	}
	else {
		self->picture_y = FCLAMP( y, -picture_view_height + height / 2.0, height / 2.0 );
	}

	gtk_widget_queue_draw( ( gpointer )self );
}

static gboolean _on_tick( GtkWidget* widget, GdkFrameClock* clock, gpointer user_data );

static void _start_ticking( AppPictureView* self )
{
	GdkFrameClock* frame_clock = gtk_widget_get_frame_clock( ( gpointer )self );

	if ( !self->tick_id ) {
		gdk_frame_clock_begin_updating( frame_clock );
		self->tick_id = gtk_widget_add_tick_callback( ( gpointer )self, _on_tick, self, NULL );
	}
}

static double _compute_zoom_level( double zoom_factor, double zoom )
{
	return log( zoom ) / log( zoom_factor );
}

static void _set_zoom( AppPictureView* self, double to_zoom, gboolean enable_animation )
{
	if ( self->zoom == to_zoom )
		return;

	self->zoom_level = _compute_zoom_level( self->zoom_factor, to_zoom );

	self->zoom_begin  = self->zoom;
	self->zoom_target = to_zoom;

	self->mouse_begin_x = self->mouse_x;
	self->mouse_begin_y = self->mouse_y;

	g_object_notify_by_pspec( ( gpointer )self, pspecs[ PROP_ZOOM_TARGET ] );

	GtkSettings* settings              = gtk_settings_get_default();
	gboolean     gtk_enable_animations = FALSE;
	g_object_get( settings, "gtk-enable-animations", &gtk_enable_animations, NULL );

	if ( gtk_enable_animations && enable_animation ) {
		GdkFrameClock* frame_clock = gtk_widget_get_frame_clock( ( gpointer )self );

		self->zoom_begin_time = gdk_frame_clock_get_frame_time( frame_clock );

		_start_ticking( self );
	}
	else {
		self->zoom = self->zoom_target;

		g_object_notify_by_pspec( ( gpointer )self, pspecs[ PROP_ZOOM ] );

		gtk_widget_queue_draw( ( gpointer )self );
	}
}

static void _zoom_to_level( AppPictureView* self, double level, gboolean enable_animation )
{
	level = FCLAMP( level, self->min_level, self->max_level );

	self->view_mode = APP_PICTURE_VIEW_MODE_MANUAL_ZOOM;

	double zoom = pow( self->zoom_factor, level );
	_set_zoom( self, zoom, enable_animation );

	double picture_view_width  = self->picture_width * zoom;
	double picture_view_height = self->picture_height * zoom;

	int width  = gtk_widget_get_width( ( gpointer )self );
	int height = gtk_widget_get_height( ( gpointer )self );

	if ( ( double )width == picture_view_width || ( double )height == picture_view_height ) {
		self->is_zooming = FALSE;
	}
	else {
		self->is_zooming = TRUE;
	}

	g_object_notify_by_pspec( ( gpointer )self, pspecs[ PROP_VIEW_MODE ] );
}

static void _zoom_change( AppPictureView* self, int delta, gboolean enable_animation )
{
	if ( delta == 0 )
		return;

	if ( self->zoom_level < self->min_level ) {
		if ( delta < 0 )
			return;
	}
	else if ( self->zoom_level > self->max_level ) {
		if ( delta > 0 )
			return;
	}

	_zoom_to_level( self, self->zoom_level + delta, enable_animation );
}

static double easeOutQuart( double progress )
{
	return 1.0 - pow( 1.0 - progress, 4 );
}

static gboolean _on_tick_do_zoom( AppPictureView* self, GdkFrameClock* clock, gint64 frame_time, double duration )
{
	gboolean ret = FALSE;

	if ( self->zoom_begin_time == 0 )
		return TRUE;

	double progress = ( frame_time - self->zoom_begin_time ) / 1000.0 / duration;

	if ( progress >= 1.0 ) {
		progress = 1.0;

		self->zoom_begin_time = 0;
		ret                   = TRUE;
	}

	double step = easeOutQuart( progress );

	double old_zoom = self->zoom;

	self->zoom = self->zoom_begin + ( self->zoom_target - self->zoom_begin ) * step;

	{
		double rel_x = self->mouse_begin_x - self->picture_x;
		double rel_y = self->mouse_begin_y - self->picture_y;

		double ratio = self->zoom / old_zoom;

		_set_offset( self,
		             self->mouse_begin_x - ( rel_x * ratio ),  //
		             self->mouse_begin_y - ( rel_y * ratio ) );
	}

	return ret;
}

static gboolean _on_tick_do_move( AppPictureView* self, GdkFrameClock* clock, gint64 frame_time, double duration )
{
	gboolean ret = FALSE;

	if ( self->move_begin_time == 0 )
		return TRUE;

	double progress = ( frame_time - self->move_begin_time ) / 1000.0 / duration;

	if ( progress >= 1.0 ) {
		progress = 1.0;

		self->is_moving       = FALSE;
		self->move_begin_time = 0;
		ret                   = TRUE;
	}

	double step = easeOutQuart( progress );

	self->picture_x = self->move_picture_begin_x + ( self->move_picture_end_x - self->move_picture_begin_x ) * step;
	self->picture_y = self->move_picture_begin_y + ( self->move_picture_end_y - self->move_picture_begin_y ) * step;

	_set_offset( self, self->picture_x, self->picture_y );

	return ret;
}

static gboolean _on_tick( GtkWidget* widget, GdkFrameClock* clock, gpointer user_data )
{
	AppPictureView* self = user_data;

	gint64 frame_time = gdk_frame_clock_get_frame_time( clock );

	double duration = 300.0;

	gboolean ret = TRUE;

	double picture_x = self->picture_x;
	double picture_y = self->picture_y;
	double zoom      = self->zoom;

	ret = _on_tick_do_zoom( self, clock, frame_time, duration ) && ret;
	ret = _on_tick_do_move( self, clock, frame_time, duration ) && ret;

	if ( zoom != self->zoom )
		g_object_notify_by_pspec( ( gpointer )self, pspecs[ PROP_ZOOM ] );

	if ( picture_x != self->picture_x || picture_y != self->picture_y )
		g_object_notify_by_pspec( ( gpointer )self, pspecs[ PROP_PICTURE_POSITION ] );

	gtk_widget_queue_draw( ( gpointer )self );

	if ( ret ) {
		self->tick_id = 0;
		gdk_frame_clock_end_updating( clock );

		return G_SOURCE_REMOVE;
	}

	return G_SOURCE_CONTINUE;
}

static gboolean _on_tick_do_play( GtkWidget* widget, GdkFrameClock* clock, gpointer user_data )
{
	AppPictureView* self = user_data;

	gint64 frame_time = gdk_frame_clock_get_frame_time( clock );

	if ( self->play_begin_time == 0 ) {
		self->play_begin_time = frame_time;
		return G_SOURCE_CONTINUE;
	}

	gint64 duration = self->play_duration;

	if ( frame_time > self->play_begin_time + duration ) {
		self->play_begin_time = frame_time;

		GError* error = NULL;

		GlyFrame* frame = gly_image_next_frame( self->gly_image, &error );
		if ( error ) {
			g_warning( "AppPictureView next frame: %s", error->message );
			g_clear_error( &error );
			self->can_play = false;
		}
		else {
			self->play_duration = gly_frame_get_delay( frame );

			GdkPaintable* paintable = ( gpointer )gly_gtk_frame_get_texture( frame );

			app_picture_view_set_paintable( self, paintable );

			g_object_unref( paintable );
			g_object_unref( frame );
		}
	}

	if ( !self->can_play ) {
		self->play_tick_id = 0;
		return G_SOURCE_REMOVE;
	}

	return G_SOURCE_CONTINUE;
}

static void _on_motion( GtkEventControllerMotion* e, gdouble x, gdouble y, gpointer user_data )
{
	AppPictureView* self = user_data;

	self->mouse_x = x;
	self->mouse_y = y;

	if ( self->is_dragging ) {
		double offset_x = self->mouse_x - self->drag_begin_x;
		double offset_y = self->mouse_y - self->drag_begin_y;

		if ( offset_x == 0 && offset_y == 0 ) {
			self->is_offset = FALSE;
		}
		else {
			self->is_offset = TRUE;

			g_object_notify_by_pspec( ( gpointer )self, pspecs[ PROP_PICTURE_POSITION ] );
		}

		g_signal_emit( self, _SIGNAL_MOVING, 0, self->is_offset );

		_set_offset( self,
		             self->drag_picture_begin_x + offset_x,  //
		             self->drag_picture_begin_y + offset_y );
	}
}

static double _compute_fit_contain_zoom( AppPictureView* self )
{
	int width  = gtk_widget_get_width( ( gpointer )self );
	int height = gtk_widget_get_height( ( gpointer )self );

	double width_zoom  = ( double )width / self->picture_width;
	double height_zoom = ( double )height / self->picture_height;

	double zoom = MIN( width_zoom, height_zoom );

	return zoom;
}

static void _app_picture_view_to_position( AppPictureView* self, double x, double y, gboolean enable_animation )
{
	self->move_picture_begin_x = self->picture_x;
	self->move_picture_begin_y = self->picture_y;

	self->move_picture_end_x = x;
	self->move_picture_end_y = y;

	GtkSettings* settings              = gtk_settings_get_default();
	gboolean     gtk_enable_animations = FALSE;
	g_object_get( settings, "gtk-enable-animations", &gtk_enable_animations, NULL );

	if ( gtk_enable_animations && enable_animation ) {
		GdkFrameClock* frame_clock = gtk_widget_get_frame_clock( ( gpointer )self );

		self->move_begin_time = gdk_frame_clock_get_frame_time( frame_clock );
		self->is_moving       = TRUE;
		_start_ticking( self );
	}
	else {
		self->is_moving = FALSE;

		_set_offset( self, self->move_picture_end_x, self->move_picture_end_y );

		g_object_notify_by_pspec( ( gpointer )self, pspecs[ PROP_PICTURE_POSITION ] );
	}
}

static void _app_picture_view_to_center( AppPictureView* self, gboolean enable_animation )
{
	g_return_if_fail( APP_IS_PICTURE_VIEW( self ) );

	self->is_offset  = FALSE;
	self->is_zooming = FALSE;

	double picture_view_width_target  = self->picture_width * self->zoom_target;
	double picture_view_height_target = self->picture_height * self->zoom_target;

	int width  = gtk_widget_get_width( ( gpointer )self );
	int height = gtk_widget_get_height( ( gpointer )self );

	double picture_end_x = width / 2.0 - picture_view_width_target / 2.0;
	double picture_end_y = height / 2.0 - picture_view_height_target / 2.0;

	_app_picture_view_to_position( self, picture_end_x, picture_end_y, enable_animation );
}

static void _app_picture_view_to_fit_contain( AppPictureView* self, gboolean enable_animation )
{
	double zoom = _compute_fit_contain_zoom( self );

	_set_zoom( self, zoom, enable_animation );

	_app_picture_view_to_center( self, enable_animation );
}

static void _set_zoom_to_default( AppPictureView* self, gboolean enable_animation )
{
	double zoom = self->zoom;

	int width  = gtk_widget_get_width( ( gpointer )self );
	int height = gtk_widget_get_height( ( gpointer )self );

	double current_picture_view_width  = self->picture_width * self->zoom;
	double current_picture_view_height = self->picture_height * self->zoom;

	gboolean apply_fit_contain = FALSE;

	if ( self->is_offset ) {
		apply_fit_contain = TRUE;
	}
	else {
		if ( current_picture_view_width == width || current_picture_view_height == height ) {
			zoom             = 1.0;
			self->zoom_level = 0;

			self->view_mode = APP_PICTURE_VIEW_MODE_CONTER;
		}
		else {
			apply_fit_contain = TRUE;
		}
	}

	if ( apply_fit_contain ) {
		zoom = _compute_fit_contain_zoom( self );

		self->view_mode = APP_PICTURE_VIEW_MODE_FIT_CONTAIN;
	}

	self->is_offset  = FALSE;
	self->is_zooming = FALSE;

	_set_zoom( self, zoom, enable_animation );

	_app_picture_view_to_center( self, enable_animation );

	g_object_notify_by_pspec( ( gpointer )self, pspecs[ PROP_VIEW_MODE ] );
}
static void _on_click_pressed( GtkGestureClick* event, gint n_press, gdouble x, gdouble y, gpointer user_data )
{
	AppPictureView* self = user_data;

	switch ( gtk_gesture_single_get_current_button( ( gpointer )event ) ) {
		case GDK_BUTTON_PRIMARY: {
			self->is_moving       = FALSE;
			self->move_begin_time = 0;

			if ( !app_picture_view_contains( self, x, y ) ) {
				break;
			}
			if ( self->can_move ) {
				self->is_dragging = TRUE;

				self->drag_begin_x         = self->mouse_x;
				self->drag_begin_y         = self->mouse_y;
				self->drag_picture_begin_x = self->picture_x;
				self->drag_picture_begin_y = self->picture_y;
			}

			gtk_widget_queue_draw( ( gpointer )self );

			gtk_widget_grab_focus( ( gpointer )self );

		} break;
		case GDK_BUTTON_MIDDLE: {
		} break;
		case GDK_BUTTON_SECONDARY: {
		} break;
		case 8: {
		} break;
		case 9: {
		} break;
	}
}

static void _on_click_released( GtkGestureClick* event, gint n_press, gdouble x, gdouble y, gpointer user_data )
{
	AppPictureView* self = user_data;

	switch ( gtk_gesture_single_get_current_button( ( gpointer )event ) ) {
		case GDK_BUTTON_PRIMARY: {
			self->is_dragging = FALSE;

			g_signal_emit( self, _SIGNAL_MOVING, 0, FALSE );

		} break;
		case GDK_BUTTON_MIDDLE: {
			_set_zoom_to_default( self, TRUE );
		} break;
		case GDK_BUTTON_SECONDARY: {
		} break;
		case 8: {
		} break;
		case 9: {
		} break;
	}
}

static gboolean _on_key_pressed( GtkEventController* event,
                                 guint               keyval,
                                 guint               keycode,
                                 GdkModifierType     state,
                                 gpointer            user_data )
{
	AppPictureView* self = user_data;

	if ( self->space_pressed ) {
		return TRUE;
	}

	if ( keyval != GDK_KEY_space )
		return FALSE;

	self->space_pressed = TRUE;

	self->is_dragging          = TRUE;
	self->drag_begin_x         = self->mouse_x;
	self->drag_begin_y         = self->mouse_y;
	self->drag_picture_begin_x = self->picture_x;
	self->drag_picture_begin_y = self->picture_y;

	g_signal_emit( self, _SIGNAL_MOVING, 0, TRUE );

	return TRUE;
}

static gboolean _on_key_released( GtkEventControllerKey* event,
                                  guint                  keyval,
                                  guint                  keycode,
                                  GdkModifierType        state,
                                  gpointer               user_data )
{
	AppPictureView* self = user_data;

	switch ( keyval ) {
		case GDK_KEY_space: {
			self->space_pressed = FALSE;

			self->is_dragging = FALSE;

			g_signal_emit( self, _SIGNAL_MOVING, 0, FALSE );

		} break;

		case GDK_KEY_equal:  // GDK_KEY_minus:
		case GDK_KEY_KP_Add: {
			_zoom_change( self, 1, TRUE );
		} break;

		case GDK_KEY_minus:
		case GDK_KEY_KP_Subtract: {
			_zoom_change( self, -1, TRUE );
		} break;

		// case GDK_KEY_equal:
		case GDK_KEY_plus: {
			_set_zoom_to_default( self, TRUE );
		} break;

		default: return FALSE;
	}

	return TRUE;
}

static void _on_scroll( GtkEventControllerScroll* controller, double dx, double dy, AppPictureView* self )
{
	_zoom_change( self, -dy, TRUE );
}

static void _on_paintable_invalidate_contents( AppPictureView* self )
{
	gtk_widget_queue_draw( ( gpointer )self );
}

static void _on_paintable_invalidate_size( AppPictureView* self )
{
	self->picture_width  = gdk_paintable_get_intrinsic_width( ( gpointer )self->paintable );
	self->picture_height = gdk_paintable_get_intrinsic_height( ( gpointer )self->paintable );

	double min_width_ratio  = ( double )100 / ( double )self->picture_width;
	double min_height_ratio = ( double )100 / ( double )self->picture_height;

	self->min_level = _compute_zoom_level( self->zoom_factor, fmin( min_width_ratio, min_height_ratio ) );

	double max_ratio = ( self->picture_width * 15 ) / ( double )self->picture_width;

	self->max_level = _compute_zoom_level( self->zoom_factor, max_ratio );

	gtk_widget_queue_draw( ( gpointer )self );
}

static GdkPaintable* _load_picture( AppPictureView* self, const char* picture_path )
{
	GFile* file = g_file_new_for_path( picture_path );

	GError* error = NULL;

	GdkPaintable* paintable = NULL;

	GlyLoader* loader = gly_loader_new( file );
	GlyImage*  image  = gly_loader_load( loader, &error );
	GlyFrame*  frame  = NULL;

	G_STMT_START
	{
		if ( error ) {
			g_warning( "gly_loader_load: %s", error->message );
			break;
		}

		frame = gly_image_next_frame( image, &error );

		if ( error ) {
			g_warning( "gly_image_next_frame: %s", error->message );
			break;
		}

		paintable = ( gpointer )gly_gtk_frame_get_texture( frame );

		self->play_duration = gly_frame_get_delay( frame );
		self->can_play      = gly_frame_get_delay( frame ) != 0;
	}
	G_STMT_END;

	self->gly_image = image;

	g_clear_error( &error );
	g_clear_object( &frame );
	g_clear_object( &loader );
	g_clear_object( &file );

	return paintable;
}

#endif

#if 1  // base class virtual function

static void app_picture_view_measure( GtkWidget*     widget,
                                      GtkOrientation orientation,
                                      int            for_size,
                                      int*           minimum_size,
                                      int*           natural_size,
                                      int*           minimum_baseline,
                                      int*           natural_baseline )
{
	AppPictureView* self = ( AppPictureView* )widget;

	for ( GtkWidget* child = gtk_widget_get_first_child( widget ); child != NULL;
	      child            = gtk_widget_get_next_sibling( child ) ) {
		if ( gtk_widget_should_layout( child ) )
			gtk_widget_measure( child, orientation, for_size, NULL, NULL, NULL, NULL );
	}
}

static void app_picture_view_size_allocate( GtkWidget* widget, int width, int height, int baseline )
{
	AppPictureView* self = ( AppPictureView* )widget;

	if ( self->view_mode == APP_PICTURE_VIEW_MODE_NONE ) {
		_app_picture_view_to_fit_contain( self, FALSE );
		self->view_mode = APP_PICTURE_VIEW_MODE_FIT_CONTAIN;

		g_object_notify_by_pspec( ( gpointer )self, pspecs[ PROP_VIEW_MODE ] );
	}

	if ( !self->is_zooming && !self->is_offset ) {
		if ( self->view_mode == APP_PICTURE_VIEW_MODE_FIT_CONTAIN ) {
			double width_zoom  = ( double )width / self->picture_width;
			double height_zoom = ( double )height / self->picture_height;

			double zoom = MIN( width_zoom, height_zoom );
			_set_zoom( self, zoom, FALSE );
			_app_picture_view_to_center( self, FALSE );
		}
		else if ( self->view_mode == APP_PICTURE_VIEW_MODE_CONTER ) {
			_app_picture_view_to_center( self, FALSE );
		}
	}
	else
		_set_offset( self, self->picture_x, self->picture_y );
}

static void app_picture_view_snapshot( GtkWidget* widget, GtkSnapshot* snapshot )
{
	AppPictureView* self = ( AppPictureView* )widget;

	GdkPaintable* paintable = self->paintable;

	if ( self->zoom > 1.0 && GDK_IS_TEXTURE( self->paintable ) ) {
		gtk_snapshot_append_scaled_texture(
		  snapshot,
		  ( gpointer )self->paintable,
		  GSK_SCALING_FILTER_NEAREST,
		  &GRAPHENE_RECT_INIT(
		    self->picture_x, self->picture_y, self->picture_width * self->zoom, self->picture_height * self->zoom ) );
	}
	else {
		gtk_snapshot_translate( snapshot, &GRAPHENE_POINT_INIT( self->picture_x, self->picture_y ) );

		gdk_paintable_snapshot(
		  paintable, snapshot, self->picture_width * self->zoom, self->picture_height * self->zoom );
	}
}

static void app_picture_view_map( GtkWidget* widget )
{
	AppPictureView* self = ( AppPictureView* )widget;

	GTK_WIDGET_CLASS( app_picture_view_parent_class )->map( widget );
}

static void app_picture_view_set_property( GObject* object, guint property_id, const GValue* value, GParamSpec* pspec )
{
	AppPictureView* self = ( AppPictureView* )object;

	switch ( property_id ) {
		default: G_OBJECT_WARN_INVALID_PROPERTY_ID( object, property_id, pspec ); break;
	}
}

static void app_picture_view_get_property( GObject* object, guint property_id, GValue* value, GParamSpec* pspec )
{
	AppPictureView* self = ( AppPictureView* )object;

	switch ( property_id ) {
		case PROP_ZOOM: {
			g_value_set_double( value, self->zoom );
		} break;

		case PROP_ZOOM_TARGET: {
			g_value_set_double( value, self->zoom_target );
		} break;

		case PROP_PICTURE_VIEW_SIZE: {
			GVariant* variant =
			  g_variant_new( "(dd)", self->picture_width * self->zoom, self->picture_height * self->zoom );
			g_value_set_variant( value, variant );
		} break;

		case PROP_PICTURE_SIZE: {
			GVariant* variant = g_variant_new( "(uu)", self->picture_width, self->picture_height );
			g_value_set_variant( value, variant );
		} break;

		case PROP_PICTURE_POSITION: {
			GVariant* variant = g_variant_new( "(dd)", self->picture_x, self->picture_y );
			g_value_set_variant( value, variant );
		} break;

		case PROP_VIEW_MODE: {
			g_value_set_enum( value, self->view_mode );
		} break;

		default: G_OBJECT_WARN_INVALID_PROPERTY_ID( object, property_id, pspec ); break;
	}
}

static void app_picture_view_constructed( GObject* object )
{
	AppPictureView* self = ( AppPictureView* )object;

	G_OBJECT_CLASS( app_picture_view_parent_class )->constructed( object );
}

static void app_picture_view_dispose( GObject* object )
{
	AppPictureView* self = ( AppPictureView* )object;

	g_clear_object( &self->paintable );

	for ( GtkWidget* child = NULL; ( child = gtk_widget_get_first_child( GTK_WIDGET( object ) ) ); )
		gtk_widget_unparent( child );

	G_OBJECT_CLASS( app_picture_view_parent_class )->dispose( object );
}

static void app_picture_view_finalize( GObject* object )
{
	AppPictureView* self = ( AppPictureView* )object;

	g_clear_object( &self->gly_image );

	G_OBJECT_CLASS( app_picture_view_parent_class )->finalize( object );
}

static void app_picture_view_init( AppPictureView* self )
{
	self->min_level = -8;
	self->max_level = 8;

	self->zoom_factor = 1.5;

	self->zoom        = 1.0;
	self->zoom_target = 1.0;

	self->can_move = TRUE;

	gtk_widget_set_focusable( ( gpointer )self, TRUE );

	GtkEventController* scroll_controller = gtk_event_controller_scroll_new(  //
	  GTK_EVENT_CONTROLLER_SCROLL_DISCRETE |                                  //
	  GTK_EVENT_CONTROLLER_SCROLL_BOTH_AXES );

	g_signal_connect( scroll_controller, "scroll", G_CALLBACK( _on_scroll ), self );
	gtk_widget_add_controller( ( gpointer )self, scroll_controller );

	GtkEventController* key_controller = gtk_event_controller_key_new();
	gtk_event_controller_set_propagation_phase( key_controller, GTK_PHASE_CAPTURE );
	g_signal_connect( key_controller, "key-pressed", G_CALLBACK( _on_key_pressed ), self );
	g_signal_connect( key_controller, "key-released", G_CALLBACK( _on_key_released ), self );
	gtk_widget_add_controller( ( gpointer )self, key_controller );

	GtkEventController* motion_controller = ( gpointer )gtk_event_controller_motion_new();
	gtk_event_controller_set_propagation_phase( motion_controller, GTK_PHASE_CAPTURE );
	g_signal_connect( motion_controller, "motion", G_CALLBACK( _on_motion ), self );
	gtk_widget_add_controller( ( gpointer )self, motion_controller );

	GtkEventController* click_controller = ( gpointer )gtk_gesture_click_new();
	gtk_gesture_single_set_button( ( gpointer )click_controller, 0 );
	g_signal_connect( click_controller, "pressed", G_CALLBACK( _on_click_pressed ), self );
	g_signal_connect( click_controller, "released", G_CALLBACK( _on_click_released ), self );
	gtk_widget_add_controller( ( gpointer )self, click_controller );
}

static void app_picture_view_class_init( AppPictureViewClass* klass )
{
	GObjectClass*   base_class   = ( GObjectClass* )klass;
	GtkWidgetClass* widget_class = ( GtkWidgetClass* )klass;
	AppWidgetClass* parent_class = ( AppWidgetClass* )klass;

	base_class->constructed  = app_picture_view_constructed;
	base_class->dispose      = app_picture_view_dispose;
	base_class->finalize     = app_picture_view_finalize;
	base_class->set_property = app_picture_view_set_property;
	base_class->get_property = app_picture_view_get_property;

	widget_class->measure       = app_picture_view_measure;
	widget_class->size_allocate = app_picture_view_size_allocate;
	widget_class->snapshot      = app_picture_view_snapshot;

	widget_class->map = app_picture_view_map;

	gtk_widget_class_set_css_name( widget_class, "AppPictureView" );

#if 1  // signals

	_SIGNAL_MOVING = g_signal_new( APP_PICTURE_VIEW_SIGNAL_MOVING,
	                               G_TYPE_FROM_CLASS( klass ),
	                               G_SIGNAL_RUN_FIRST,
	                               0,
	                               NULL,
	                               NULL,
	                               NULL,
	                               G_TYPE_NONE,  //
	                               1,
	                               G_TYPE_BOOLEAN );

#endif

#if 1  // GParamSpec

	pspecs[ PROP_ZOOM ] = g_param_spec_double( APP_PICTURE_VIEW_PROP_ZOOM,
	                                           "zoom",
	                                           "zoom",  //
	                                           0,
	                                           G_MAXDOUBLE,
	                                           1,
	                                           G_PARAM_READABLE | G_PARAM_STATIC_STRINGS );

	pspecs[ PROP_ZOOM_TARGET ] = g_param_spec_double( APP_PICTURE_VIEW_PROP_ZOOM_TARGET,
	                                                  "zoom_target",
	                                                  "zoom_target",  //
	                                                  0,
	                                                  G_MAXDOUBLE,
	                                                  1,
	                                                  G_PARAM_READABLE | G_PARAM_STATIC_STRINGS );

	pspecs[ PROP_PICTURE_SIZE ] = g_param_spec_variant( APP_PICTURE_VIEW_PROP_PICTURE_SIZE,
	                                                    "picture_size",
	                                                    "picture_size",  //
	                                                    G_VARIANT_TYPE( "(uu)" ),
	                                                    NULL,
	                                                    G_PARAM_READABLE | G_PARAM_STATIC_STRINGS );

	pspecs[ PROP_PICTURE_VIEW_SIZE ] = g_param_spec_variant( APP_PICTURE_VIEW_PROP_PICTURE_VIEW_SIZE,
	                                                         "picture_view_size",
	                                                         "picture_view_size",  //
	                                                         G_VARIANT_TYPE( "(dd)" ),
	                                                         NULL,
	                                                         G_PARAM_READABLE | G_PARAM_STATIC_STRINGS );

	pspecs[ PROP_PICTURE_POSITION ] = g_param_spec_variant( APP_PICTURE_VIEW_PROP_PICTURE_POSITION,
	                                                        "picture_position",
	                                                        "picture_position",  //
	                                                        G_VARIANT_TYPE( "(dd)" ),
	                                                        NULL,
	                                                        G_PARAM_READABLE | G_PARAM_STATIC_STRINGS );

	pspecs[ PROP_VIEW_MODE ] = g_param_spec_enum( APP_PICTURE_VIEW_PROP_VIEW_MODE,
	                                              "view_mode",
	                                              "view_mode",  //
	                                              app_picture_view_mode_get_type(),
	                                              APP_PICTURE_VIEW_MODE_NONE,
	                                              G_PARAM_READABLE | G_PARAM_STATIC_STRINGS );

	g_object_class_install_properties( base_class, PROP_N, pspecs );

#endif
}

#endif

#if 1  // public function

AppPictureView* app_picture_view_new( const char* path, GdkPaintable* paintable )
{
	g_return_val_if_fail( path || GDK_IS_PAINTABLE( paintable ), NULL );

	AppPictureView* self = g_object_new( app_picture_view_get_type(), NULL );

	if ( path ) {
		if ( g_str_has_suffix( path, ".svg" ) ) {
			self->paintable = ( gpointer )app_svg_paintable_new( path );
		}
		else {
			self->paintable = _load_picture( self, path );
		}
	}
	else {
		self->paintable = g_object_ref( paintable );
	}

	if ( !self->paintable ) {
		g_object_ref_sink( self );
		g_object_unref( self );
		return NULL;
	}

	GdkPaintableFlags flags = gdk_paintable_get_flags( self->paintable );

	if ( ( flags & GDK_PAINTABLE_STATIC_CONTENTS ) == 0 )
		g_signal_connect_swapped(
		  self->paintable, "invalidate-contents", G_CALLBACK( _on_paintable_invalidate_contents ), self );

	if ( ( flags & GDK_PAINTABLE_STATIC_SIZE ) == 0 )
		g_signal_connect_swapped(
		  self->paintable, "invalidate-size", G_CALLBACK( _on_paintable_invalidate_size ), self );

	_on_paintable_invalidate_size( self );

	return self;
}

void app_picture_view_set_paintable( AppPictureView* self, GdkPaintable* paintable )
{
	g_return_if_fail( APP_IS_PICTURE_VIEW( self ) );
	g_return_if_fail( GDK_IS_PAINTABLE( paintable ) );

	g_set_object( &self->paintable, paintable );

	int picture_width  = gdk_paintable_get_intrinsic_width( ( gpointer )self->paintable );
	int picture_height = gdk_paintable_get_intrinsic_height( ( gpointer )self->paintable );

	if ( self->picture_width != picture_width || self->picture_height != picture_height ) {
		_on_paintable_invalidate_size( self );

		app_picture_view_to_fit_contain( self );
	}

	gtk_widget_queue_draw( ( gpointer )self );
}

GdkPaintable* app_picture_view_get_paintable( AppPictureView* self )
{
	g_return_val_if_fail( APP_IS_PICTURE_VIEW( self ), NULL );

	return GDK_PAINTABLE( self->paintable );
}

void app_picture_view_to_fit_contain( AppPictureView* self )
{
	g_return_if_fail( APP_IS_PICTURE_VIEW( self ) );

	_app_picture_view_to_fit_contain( self, TRUE );

	self->view_mode = APP_PICTURE_VIEW_MODE_FIT_CONTAIN;

	g_object_notify_by_pspec( ( gpointer )self, pspecs[ PROP_VIEW_MODE ] );
}

void app_picture_view_to_center( AppPictureView* self )
{
	g_return_if_fail( APP_IS_PICTURE_VIEW( self ) );

	_app_picture_view_to_center( self, TRUE );

	self->view_mode = APP_PICTURE_VIEW_MODE_CONTER;

	g_object_notify_by_pspec( ( gpointer )self, pspecs[ PROP_VIEW_MODE ] );
}

void app_picture_view_center_to( AppPictureView* self, double x, double y, gboolean enable_animation )
{
	g_return_if_fail( APP_IS_PICTURE_VIEW( self ) );

	int width  = gtk_widget_get_width( ( gpointer )self );
	int height = gtk_widget_get_height( ( gpointer )self );

	x = -x * self->zoom + ( width / 2.0 );
	y = -y * self->zoom + ( height / 2.0 );

	_app_picture_view_to_position( self, x, y, enable_animation );
}

void app_picture_view_set_zoom( AppPictureView* self, double zoom, gboolean enable_animation )
{
	g_return_if_fail( APP_IS_PICTURE_VIEW( self ) );

	double min_zoom = pow( self->zoom_factor, self->min_level );
	double max_zoom = pow( self->zoom_factor, self->max_level );

	zoom = FCLAMP( zoom, min_zoom, max_zoom );

	_set_zoom( self, zoom, enable_animation );
}

void app_picture_view_set_zoom_level( AppPictureView* self, double level, gboolean enable_animation )
{
	g_return_if_fail( APP_IS_PICTURE_VIEW( self ) );

	_zoom_to_level( self, level, enable_animation );
}

void app_picture_view_adjust_zoom_level( AppPictureView* self, double delta, gboolean enable_animation )
{
	g_return_if_fail( APP_IS_PICTURE_VIEW( self ) );

	_zoom_change( self, delta, enable_animation );
}

gboolean app_picture_view_contains( AppPictureView* self, double x, double y )
{
	g_return_val_if_fail( APP_IS_PICTURE_VIEW( self ), FALSE );

	if ( self->picture_width == 0 || self->picture_height == 0 )
		return FALSE;

	if (                                                                                //
	  x > self->picture_x && x < self->picture_x + self->picture_width * self->zoom &&  //
	  y > self->picture_y && y < self->picture_y + self->picture_height * self->zoom    //
	)
		return TRUE;

	return FALSE;
}

AppPictureViewMode app_picture_view_get_mode( AppPictureView* self )
{
	g_return_val_if_fail( APP_IS_PICTURE_VIEW( self ), APP_PICTURE_VIEW_MODE_NONE );

	return self->view_mode;
}

void app_picture_view_play( AppPictureView* self )
{
	g_return_if_fail( APP_IS_PICTURE_VIEW( self ) );

	if ( !self->can_play )
		return;

	if ( self->should_play )
		return;

	self->should_play  = TRUE;
	self->play_tick_id = gtk_widget_add_tick_callback( ( gpointer )self, _on_tick_do_play, self, NULL );
}

void app_picture_view_stop( AppPictureView* self )
{
	g_return_if_fail( APP_IS_PICTURE_VIEW( self ) );

	if ( !self->can_play )
		return;

	if ( !self->should_play )
		return;

	self->should_play = FALSE;

	gtk_widget_remove_tick_callback( ( gpointer )self, self->play_tick_id );
	self->play_tick_id = 0;
}

void app_picture_view_config( AppPictureView* self, AppPictureViewConfig config, ... )
{
	g_return_if_fail( APP_IS_PICTURE_VIEW( self ) );
	g_return_if_fail( config > APP_PICTURE_VIEW_CONFIG_0 && config < APP_PICTURE_VIEW_CONFIG_N );

	va_list ap;
	va_start( ap, config );

	switch ( config ) {
		case APP_PICTURE_VIEW_CONFIG_CAN_MOVE: {
			gboolean can   = va_arg( ap, gboolean );
			self->can_move = can;
		} break;

		default: break;
	}

	va_end( ap );
}

void app_picture_view_get_config( AppPictureView* self, AppPictureViewConfig config, ... )
{
	g_return_if_fail( APP_IS_PICTURE_VIEW( self ) );
	g_return_if_fail( config > APP_PICTURE_VIEW_CONFIG_0 && config < APP_PICTURE_VIEW_CONFIG_N );

	va_list ap;
	va_start( ap, config );

	switch ( config ) {
		case APP_PICTURE_VIEW_CONFIG_CAN_MOVE: {
			gboolean* can = va_arg( ap, gboolean* );
			if ( can ) {
				*can = self->can_move;
			}
		} break;

		default: break;
	}

	va_end( ap );
}

#endif

#endif
