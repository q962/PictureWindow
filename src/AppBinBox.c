#include "AppBinBox.h"

#if 1  // gobject definition :: AppBinBox

struct _AppBinBox {
	GtkWidget parent_instance;
};

enum {
	PROP_0,
	PROP_N,
};

G_DEFINE_TYPE( AppBinBox, app_bin_box, gtk_widget_get_type() )

#if 1  // static function

#endif

#if 1  // base class virtual function

static void app_bin_box_set_property( GObject* object, guint property_id, const GValue* value, GParamSpec* pspec )
{
	AppBinBox* self = ( AppBinBox* )object;

	switch ( property_id ) {
		default: G_OBJECT_WARN_INVALID_PROPERTY_ID( object, property_id, pspec ); break;
	}
}

static void app_bin_box_get_property( GObject* object, guint property_id, GValue* value, GParamSpec* pspec )
{
	AppBinBox* self = ( AppBinBox* )object;

	switch ( property_id ) {
		default: G_OBJECT_WARN_INVALID_PROPERTY_ID( object, property_id, pspec ); break;
	}
}

static void app_bin_box_constructed( GObject* object )
{
	AppBinBox* self = ( AppBinBox* )object;

	G_OBJECT_CLASS( app_bin_box_parent_class )->constructed( object );
}

static void app_bin_box_dispose( GObject* object )
{
	AppBinBox* self = ( AppBinBox* )object;

	for ( GtkWidget* child = NULL; ( child = gtk_widget_get_first_child( GTK_WIDGET( object ) ) ); )
		gtk_widget_unparent( child );

	G_OBJECT_CLASS( app_bin_box_parent_class )->dispose( object );
}

static void app_bin_box_finalize( GObject* object )
{
	AppBinBox* self = ( AppBinBox* )object;

	G_OBJECT_CLASS( app_bin_box_parent_class )->finalize( object );
}

static void app_bin_box_init( AppBinBox* self ) {}

static void app_bin_box_class_init( AppBinBoxClass* klass )
{
	GObjectClass*   base_class   = ( GObjectClass* )klass;
	GtkWidgetClass* parent_class = ( GtkWidgetClass* )klass;

	base_class->constructed  = app_bin_box_constructed;
	base_class->dispose      = app_bin_box_dispose;
	base_class->finalize     = app_bin_box_finalize;
	base_class->set_property = app_bin_box_set_property;
	base_class->get_property = app_bin_box_get_property;

	gtk_widget_class_set_css_name( parent_class, "AppBinBox" );
	gtk_widget_class_set_layout_manager_type( parent_class, gtk_bin_layout_get_type() );
}

#endif

#if 1  // public function

AppBinBox* app_bin_box_new()
{
	AppBinBox* self = g_object_new( app_bin_box_get_type(), NULL );
	return self;
}

#endif

#endif
