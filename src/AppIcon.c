#include "AppIcon.h"

#if 1 // gobject definition :: AppIcon

typedef struct {
	int spik;
} AppIconPrivate;

enum {
	PROP_0,
	PROP_N,
};

G_DEFINE_TYPE_WITH_PRIVATE( AppIcon, app_icon, app_widget_get_type() )

#define SELF_PRIVATE app_icon_get_instance_private
#define SELFDATAVAL( SELF, NAME ) AppIconPrivate* NAME = SELF_PRIVATE( ( SELF ) )
#define SELFDATA2( SELF ) ( ( AppIconPrivate* )SELF_PRIVATE( ( SELF ) ) )
#define SELFDATA SELFDATA2( self )

#if 1 // static function

#endif

#if 1 // base class virtual function

static void app_icon_constructed( GObject* object ) {
	AppIcon* self = ( AppIcon* )object;

	G_OBJECT_CLASS (app_icon_parent_class)->constructed(object);
}

static void app_icon_dispose( GObject* object ) {
	AppIcon* self = ( AppIcon* )object;

	G_OBJECT_CLASS (app_icon_parent_class)->dispose(object);
}

static void app_icon_finalize( GObject* object ) {
	AppIcon* self = ( AppIcon* )object;

	G_OBJECT_CLASS (app_icon_parent_class)->finalize(object);
}

static void app_icon_init(AppIcon* self) {
	AppIconPrivate* priv = SELFDATA;
}

static void app_icon_class_init(AppIconClass* klass) {
	GObjectClass* base_class = (GObjectClass*)klass;
	AppWidgetClass* parent_class = (AppWidgetClass*)klass;

	base_class->constructed = app_icon_constructed;
	base_class->dispose = app_icon_dispose;
	base_class->finalize = app_icon_finalize;
}

#endif

#if 1 // public function

 AppIcon* app_icon_new() {
	AppIcon* self = g_object_new( app_icon_get_type(), NULL );
	return self;
}

#endif

#endif

