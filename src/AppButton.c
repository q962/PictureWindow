#include "AppButton.h"

#if 1  // gobject definition :: AppButton

struct _AppButton {
	AppWidget parent_instance;
};

enum {
	PROP_0,
	PROP_CHILD,
	PROP_N,
};

static guint _SIGNAL_CLICKED = 0;

G_DEFINE_TYPE( AppButton, app_button, app_widget_get_type() )

#if 1  // static function

static void _on_released( GtkGestureClick* event, gint n_press, gdouble x, gdouble y, gpointer user_data )
{
	AppButton* self = user_data;

	g_signal_emit( self, _SIGNAL_CLICKED, 0 );
}

#endif

#if 1  // base class virtual function

static void app_button_constructed( GObject* object )
{
	AppButton* self = ( AppButton* )object;

	G_OBJECT_CLASS( app_button_parent_class )->constructed( object );
}

static void app_button_dispose( GObject* object )
{
	AppButton* self = ( AppButton* )object;

	G_OBJECT_CLASS( app_button_parent_class )->dispose( object );
}

static void app_button_finalize( GObject* object )
{
	AppButton* self = ( AppButton* )object;

	G_OBJECT_CLASS( app_button_parent_class )->finalize( object );
}

static void app_button_init( AppButton* self )
{
	GtkEventController* click_controller = ( gpointer )gtk_gesture_click_new();
	g_signal_connect( click_controller, "released", G_CALLBACK( _on_released ), self );
	gtk_widget_add_controller( GTK_WIDGET( self ), click_controller );
}

static void app_button_class_init( AppButtonClass* klass )
{
	GObjectClass*   base_class   = ( GObjectClass* )klass;
	AppWidgetClass* parent_class = ( AppWidgetClass* )klass;

	base_class->constructed = app_button_constructed;
	base_class->dispose     = app_button_dispose;
	base_class->finalize    = app_button_finalize;

	_SIGNAL_CLICKED = g_signal_new( APP_BUTTON_SIGNAL_CLICKED,
	                                G_TYPE_FROM_CLASS( klass ),
	                                G_SIGNAL_RUN_FIRST,
	                                0,
	                                NULL,
	                                NULL,
	                                NULL,
	                                G_TYPE_NONE,  //
	                                0 );
}

#endif

#if 1  // public function

AppButton* app_button_new( GtkWidget* widget )
{
	AppButton* self = g_object_new( app_button_get_type(), "widget", widget, NULL );
	return self;
}

#endif

#endif
