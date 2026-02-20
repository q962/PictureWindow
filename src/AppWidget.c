#include "AppWidget.h"

#if 1  // gobject definition :: AppWidget

typedef struct {
	int spik;
} AppWidgetPrivate;

enum {
	PROP_0,
	PROP_N,
};

G_DEFINE_TYPE_WITH_PRIVATE( AppWidget, app_widget, gtk_widget_get_type() )

#define SELF_PRIVATE app_widget_get_instance_private
#define SELFDATAVAL( SELF, NAME ) AppWidgetPrivate* NAME = SELF_PRIVATE( ( SELF ) )
#define SELFDATA2( SELF ) ( ( AppWidgetPrivate* )SELF_PRIVATE( ( SELF ) ) )
#define SELFDATA SELFDATA2( self )

#if 1  // static function

#endif

#if 1  // base class virtual function

static void app_widget_constructed( GObject* object )
{
	AppWidget* self = ( AppWidget* )object;

	G_OBJECT_CLASS( app_widget_parent_class )->constructed( object );
}

static void app_widget_dispose( GObject* object )
{
	AppWidget* self = ( AppWidget* )object;

	G_OBJECT_CLASS( app_widget_parent_class )->dispose( object );
}

static void app_widget_finalize( GObject* object )
{
	AppWidget* self = ( AppWidget* )object;

	G_OBJECT_CLASS( app_widget_parent_class )->finalize( object );
}

static void app_widget_init( AppWidget* self )
{
	AppWidgetPrivate* priv = SELFDATA;
}

static void app_widget_class_init( AppWidgetClass* klass )
{
	GObjectClass*   base_class   = ( GObjectClass* )klass;
	GtkWidgetClass* parent_class = ( GtkWidgetClass* )klass;

	base_class->constructed = app_widget_constructed;
	base_class->dispose     = app_widget_dispose;
	base_class->finalize    = app_widget_finalize;
}

#endif

#if 1  // public function

AppWidget* app_widget_new()
{
	AppWidget* self = g_object_new( app_widget_get_type(), NULL );
	return self;
}

#endif

#endif
