#include "AppFolderHistory.h"

#if 1  // gobject definition :: AppFolderHistory

struct _AppFolderHistory {
	GObject parent_instance;

	guint max_count;

	GListStore* store;
};

enum {
	PROP_0,
	PROP_ITEM_TYPE,
	PROP_N_ITEMS,
	PROP_N,
};

static GParamSpec* props[ PROP_N ] = {};

static void g_list_model_iface_init( GListModelInterface* iface );

G_DEFINE_TYPE_WITH_CODE( AppFolderHistory,
                         app_folder_history,
                         g_object_get_type(),
                         G_IMPLEMENT_INTERFACE( G_TYPE_LIST_MODEL, g_list_model_iface_init ) )

#if 1  // static function

static GType app_folder_history_get_item_type( GListModel* list )
{
	AppFolderHistory* self = ( AppFolderHistory* )list;

	return g_list_model_get_item_type( G_LIST_MODEL( self->store ) );
}

static guint app_folder_history_get_n_items( GListModel* list )
{
	AppFolderHistory* self = ( AppFolderHistory* )list;

	return g_list_model_get_n_items( G_LIST_MODEL( self->store ) );
}

static gpointer app_folder_history_get_item( GListModel* list, guint position )
{
	AppFolderHistory* self = ( AppFolderHistory* )list;

	return g_list_model_get_item( G_LIST_MODEL( self->store ), position );
}

static void _on_items_changed( GListModel* model, guint position, guint removed, guint added, AppFolderHistory* self )
{
	g_list_model_items_changed( ( gpointer )self, position, removed, added );
}

static void _on_n_items_changed( GListModel* model, GParamSpec* specp, AppFolderHistory* self )
{
	g_object_notify_by_pspec( ( gpointer )self, specp );
}

#endif

#if 1  // base class virtual function

static void g_list_model_iface_init( GListModelInterface* iface )
{
	iface->get_item_type = app_folder_history_get_item_type;
	iface->get_n_items   = app_folder_history_get_n_items;
	iface->get_item      = app_folder_history_get_item;
}

static void app_folder_history_set_property( GObject*      object,
                                             guint         property_id,
                                             const GValue* value,
                                             GParamSpec*   pspec )
{
	AppFolderHistory* self = ( AppFolderHistory* )object;

	switch ( property_id ) {
		case PROP_ITEM_TYPE: {
			g_object_set_property( ( gpointer )self->store, "item-type", value );
		} break;
		default: G_OBJECT_WARN_INVALID_PROPERTY_ID( object, property_id, pspec ); break;
	}
}

static void app_folder_history_get_property( GObject* object, guint property_id, GValue* value, GParamSpec* pspec )
{
	AppFolderHistory* self = ( AppFolderHistory* )object;

	switch ( property_id ) {
		case PROP_ITEM_TYPE: {
			g_value_set_gtype( value, app_folder_history_get_item_type( G_LIST_MODEL( self->store ) ) );
		} break;
		case PROP_N_ITEMS: {
			g_value_set_uint64( value, app_folder_history_get_n_items( G_LIST_MODEL( self->store ) ) );
		} break;
		default: G_OBJECT_WARN_INVALID_PROPERTY_ID( object, property_id, pspec ); break;
	}
}

static void app_folder_history_constructed( GObject* object )
{
	AppFolderHistory* self = ( AppFolderHistory* )object;

	G_OBJECT_CLASS( app_folder_history_parent_class )->constructed( object );
}

static void app_folder_history_dispose( GObject* object )
{
	AppFolderHistory* self = ( AppFolderHistory* )object;

	G_OBJECT_CLASS( app_folder_history_parent_class )->dispose( object );
}

static void app_folder_history_finalize( GObject* object )
{
	AppFolderHistory* self = ( AppFolderHistory* )object;

	G_OBJECT_CLASS( app_folder_history_parent_class )->finalize( object );
}

static void app_folder_history_init( AppFolderHistory* self )
{
	self->store = g_list_store_new( GTK_TYPE_STRING_OBJECT );

	g_signal_connect( self->store, "items-changed", G_CALLBACK( _on_items_changed ), self );
	g_signal_connect( self->store, "notify::n-items", G_CALLBACK( _on_n_items_changed ), self );
}

static void app_folder_history_class_init( AppFolderHistoryClass* klass )
{
	GObjectClass* base_class   = ( GObjectClass* )klass;
	GObjectClass* parent_class = ( GObjectClass* )klass;

	base_class->constructed  = app_folder_history_constructed;
	base_class->dispose      = app_folder_history_dispose;
	base_class->finalize     = app_folder_history_finalize;
	base_class->set_property = app_folder_history_set_property;
	base_class->get_property = app_folder_history_get_property;

	props[ PROP_ITEM_TYPE ] =
	  g_param_spec_gtype( "item-type", NULL, NULL, GTK_TYPE_STRING_OBJECT, G_PARAM_READABLE | G_PARAM_STATIC_STRINGS );
	props[ PROP_N_ITEMS ] =
	  g_param_spec_uint( "n-items", NULL, NULL, 0, G_MAXUINT, 0, G_PARAM_READABLE | G_PARAM_STATIC_STRINGS );

	g_object_class_install_properties( base_class, PROP_N, props );
}

#endif

#if 1  // public function

static AppFolderHistory* instance = NULL;

static guint max_count = 10;

AppFolderHistory* app_folder_history_get()
{
	if ( instance )
		return g_object_ref( instance );

	AppFolderHistory* self = g_object_new( app_folder_history_get_type(), NULL );

	self->max_count = max_count;

	instance = self;

	return g_object_ref( instance );
}

void app_folder_history_append( AppFolderHistory* self, const char* path )
{
	g_return_if_fail( APP_IS_FOLDER_HISTORY( self ) );

	guint n_items = g_list_model_get_n_items( G_LIST_MODEL( self->store ) );

	g_object_freeze_notify( ( gpointer )self->store );

	if ( n_items >= self->max_count ) {
		g_list_store_remove( self->store, 0 );
	}

	GtkStringObject* path_str = gtk_string_object_new( path );
	g_list_store_append( self->store, path_str );
	g_object_unref( path_str );

	g_object_thaw_notify( ( gpointer )self->store );
}

void app_folder_history_clear( AppFolderHistory* self )
{
	g_return_if_fail( APP_IS_FOLDER_HISTORY( self ) );

	g_list_store_remove_all( self->store );
}

const char* app_folder_history_index( AppFolderHistory* self, guint index )
{
	g_return_val_if_fail( APP_IS_FOLDER_HISTORY( self ), NULL );

	GtkStringObject* str_obj = g_list_model_get_item( G_LIST_MODEL( self->store ), index );

	const char* path = gtk_string_object_get_string( str_obj );

	g_object_unref( str_obj );

	return path;
}

#endif

#endif
