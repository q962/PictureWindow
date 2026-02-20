#include "AppPictureList.h"

#if 1  // gobject definition :: AppPictureList

G_DEFINE_ENUM_TYPE( PictureLoopMode,
                    picture_loop_mode,
                    G_DEFINE_ENUM_VALUE( PICTURE_LOOP_RANDOM, "random" ),
                    G_DEFINE_ENUM_VALUE( PICTURE_LOOP_SEQUENTIAL, "sequential" ) );

struct _AppPictureList {
	GObject parent_instance;

	PictureLoopMode mode;

	GPtrArray* picture_list;

	guint   index;
	GArray* history;
};

enum {
	PROP_0,
	PROP_N,
};

static guint SIGNAL_N_ITEMS = 0;

G_DEFINE_TYPE( AppPictureList, app_picture_list, g_object_get_type() )

#if 1  // static function

#endif

#if 1  // base class virtual function

static void app_picture_list_set_property( GObject* object, guint property_id, const GValue* value, GParamSpec* pspec )
{
	AppPictureList* self = ( AppPictureList* )object;

	switch ( property_id ) {
		default: G_OBJECT_WARN_INVALID_PROPERTY_ID( object, property_id, pspec ); break;
	}
}

static void app_picture_list_get_property( GObject* object, guint property_id, GValue* value, GParamSpec* pspec )
{
	AppPictureList* self = ( AppPictureList* )object;

	switch ( property_id ) {
		default: G_OBJECT_WARN_INVALID_PROPERTY_ID( object, property_id, pspec ); break;
	}
}

static void app_picture_list_constructed( GObject* object )
{
	AppPictureList* self = ( AppPictureList* )object;

	G_OBJECT_CLASS( app_picture_list_parent_class )->constructed( object );
}

static void app_picture_list_dispose( GObject* object )
{
	AppPictureList* self = ( AppPictureList* )object;

	G_OBJECT_CLASS( app_picture_list_parent_class )->dispose( object );
}

static void app_picture_list_finalize( GObject* object )
{
	AppPictureList* self = ( AppPictureList* )object;

	g_ptr_array_free( self->picture_list, TRUE );
	g_array_free( self->history, TRUE );

	G_OBJECT_CLASS( app_picture_list_parent_class )->finalize( object );
}

static void app_picture_list_init( AppPictureList* self )
{
	self->picture_list = g_ptr_array_new_with_free_func( g_free );
	self->history      = g_array_new( TRUE, TRUE, sizeof( guint ) );

	g_array_set_size( self->history, 1 );
}

static void app_picture_list_class_init( AppPictureListClass* klass )
{
	GObjectClass* base_class   = ( GObjectClass* )klass;
	GObjectClass* parent_class = ( GObjectClass* )klass;

	base_class->constructed  = app_picture_list_constructed;
	base_class->dispose      = app_picture_list_dispose;
	base_class->finalize     = app_picture_list_finalize;
	base_class->set_property = app_picture_list_set_property;
	base_class->get_property = app_picture_list_get_property;

	SIGNAL_N_ITEMS = g_signal_new( APP_PICTURE_LIST_SIGNAL_N_ITEMS,
	                               G_TYPE_FROM_CLASS( klass ),
	                               G_SIGNAL_RUN_LAST,
	                               0,
	                               NULL,
	                               NULL,
	                               NULL,
	                               G_TYPE_NONE,
	                               1,
	                               G_TYPE_UINT );
}

#endif

#if 1  // public function

AppPictureList* app_picture_list_new( PictureLoopMode mode, const char* path )
{
	AppPictureList* self = g_object_new( app_picture_list_get_type(), NULL );

	app_picture_list_set_mode( self, mode );
	app_picture_list_load( self, path );

	return self;
}

void app_picture_list_load( AppPictureList* self, const char* path )
{
	g_return_if_fail( APP_IS_PICTURE_LIST( self ) );

	if ( path ) {
		GDir* dir = g_dir_open( path, 0, NULL );
		if ( dir ) {
			const char* name = NULL;

			GString* path_concat = g_string_new( path );
			guint    path_len    = path_concat->len;

			while ( ( name = g_dir_read_name( dir ) ) ) {
				g_string_set_size( path_concat, path_len );
				g_string_append_printf( path_concat, "/%s", name );

				gboolean is_media = FALSE;

				const char* dot_p = strrchr( name, '.' );
				if ( dot_p ) {
					is_media = g_str_has_suffix( dot_p, ".jpg" )      //
					           || g_str_has_suffix( dot_p, ".png" )   //
					           || g_str_has_suffix( dot_p, ".gif" )   //
					           || g_str_has_suffix( dot_p, ".jpeg" )  //
					           || g_str_has_suffix( dot_p, ".webp" )  //
					           || g_str_has_suffix( dot_p, ".svg" )   //
					           || g_str_has_suffix( dot_p, ".svgz" )  //
					           || g_str_has_suffix( dot_p, ".t" );
				}
				if ( is_media ) {
					g_ptr_array_add( self->picture_list, g_strdup( path_concat->str ) );
				}
			}

			g_string_free( path_concat, TRUE );
			g_dir_close( dir );

			g_signal_emit( self, SIGNAL_N_ITEMS, 0, self->picture_list->len );
		}
		else {
			app_picture_list_append( self, path );
		}
	}
}

void app_picture_list_append( AppPictureList* self, const char* path )
{
	g_return_if_fail( APP_IS_PICTURE_LIST( self ) );

	if ( !g_file_test( path, G_FILE_TEST_IS_REGULAR ) ) {
		return;
	}

	g_ptr_array_add( self->picture_list, g_strdup( path ) );

	g_signal_emit( self, SIGNAL_N_ITEMS, 0, self->picture_list->len );
}

void app_picture_list_appends( AppPictureList* self, const char** paths, guint count )
{
	g_return_if_fail( APP_IS_PICTURE_LIST( self ) );

	guint loaded = 0;

	for ( int i = 0; i < count; i++ ) {
		const char* path = paths[ i ];

		if ( !g_file_test( path, G_FILE_TEST_IS_REGULAR ) ) {
			continue;
		}

		loaded += 1;

		g_ptr_array_add( self->picture_list, g_strdup( path ) );
	}

	if ( loaded )
		g_signal_emit( self, SIGNAL_N_ITEMS, 0, self->picture_list->len );
}

void app_picture_list_append_next( AppPictureList* self, const char* path )
{
	g_return_if_fail( APP_IS_PICTURE_LIST( self ) );

	if ( !g_file_test( path, G_FILE_TEST_IS_REGULAR ) ) {
		return;
	}

	g_ptr_array_add( self->picture_list, g_strdup( path ) );

	guint picture_index = self->picture_list->len - 1;

	g_array_insert_val( self->history, self->index + 1, picture_index );

	g_signal_emit( self, SIGNAL_N_ITEMS, 0, self->picture_list->len );
}

void app_picture_list_append_nexts( AppPictureList* self, const char** paths, int count )
{
	guint loaded = 0;

	guint picture_index = g_array_index( self->history, guint, self->index );

	for ( int i = 0; i < count; i++ ) {
		const char* path = paths[ i ];

		if ( !g_file_test( path, G_FILE_TEST_IS_REGULAR ) ) {
			continue;
		}

		loaded += 1;

		guint new_picture_index = picture_index + i;

		g_ptr_array_insert( self->picture_list, new_picture_index, g_strdup( path ) );
		g_array_insert_val( self->history, self->index + i + 1, new_picture_index );
	}

	if ( loaded )
		g_signal_emit( self, SIGNAL_N_ITEMS, 0, self->picture_list->len );
}

void app_picture_list_remove_index( AppPictureList* self, guint index )
{
	g_return_if_fail( APP_IS_PICTURE_LIST( self ) );
	g_return_if_fail( index < self->picture_list->len );

	g_ptr_array_remove_index( self->picture_list, index );

	for ( int i = self->history->len - 1; i > 0; i-- ) {
		guint history_index = g_array_index( self->history, guint, i );

		if ( index == history_index ) {
			g_array_remove_index( self->history, i );
		}
	}

	g_signal_emit( self, SIGNAL_N_ITEMS, 0, self->picture_list->len );
}

void app_picture_list_remove_all( AppPictureList* self )
{
	g_return_if_fail( APP_IS_PICTURE_LIST( self ) );

	self->index = 0;

	g_ptr_array_set_size( self->picture_list, 0 );
	g_array_set_size( self->history, 1 );

	g_signal_emit( self, SIGNAL_N_ITEMS, 0, self->picture_list->len );
}

void app_picture_list_set_mode( AppPictureList* self, PictureLoopMode mode )
{
	g_return_if_fail( APP_IS_PICTURE_LIST( self ) );
	g_return_if_fail( mode > PICTURE_LOOP_0 && mode < PICTURE_LOOP_N );

	self->mode = mode;
}

gboolean app_picture_list_can_prev( AppPictureList* self )
{
	g_return_val_if_fail( APP_IS_PICTURE_LIST( self ), FALSE );

	if ( self->picture_list->len == 0 )
		return FALSE;

	if ( self->index <= 1 )
		return FALSE;

	return TRUE;
}
const char* app_picture_list_prev( AppPictureList* self )
{
	g_return_val_if_fail( APP_IS_PICTURE_LIST( self ), NULL );

	if ( self->picture_list->len == 0 )
		return NULL;

	if ( self->index <= 1 )
		return NULL;

	self->index -= 1;

	guint index = g_array_index( self->history, guint, self->index );

	return g_ptr_array_index( self->picture_list, index );
}

const char* app_picture_list_next( AppPictureList* self )
{
	g_return_val_if_fail( APP_IS_PICTURE_LIST( self ), NULL );

	if ( self->picture_list->len == 0 )
		return NULL;

	guint prev_index = self->index;

	self->index += 1;

	if ( self->index >= self->history->len ) {
		switch ( self->mode ) {
			case PICTURE_LOOP_RANDOM: {
				guint random = g_random_int_range( 0, self->picture_list->len );
				g_array_append_val( self->history, random );

			} break;
			case PICTURE_LOOP_SEQUENTIAL: {
				guint picture_index = 0;

				if ( prev_index == 0 ) {
					// first load
				}
				else {
					picture_index = g_array_index( self->history, guint, prev_index );

					if ( picture_index + 1 >= self->picture_list->len ) {
						picture_index = 0;
					}
					else {
						picture_index = picture_index + 1;
					}
				}
				g_array_append_val( self->history, picture_index );

			} break;
			default: break;
		}
	}

	guint picture_index = g_array_index( self->history, guint, self->index );

	return g_ptr_array_index( self->picture_list, picture_index );
}

const char* app_picture_list_index( AppPictureList* self )
{
	g_return_val_if_fail( APP_IS_PICTURE_LIST( self ), NULL );

	guint picture_index = g_array_index( self->history, guint, self->index );

	return g_ptr_array_index( self->picture_list, picture_index );
}

void app_picture_list_iter_reset( AppPictureList* self )
{
	g_return_if_fail( APP_IS_PICTURE_LIST( self ) );

	self->index = 0;

	g_array_set_size( self->history, self->index + 1 );
}

void app_picture_list_truncation( AppPictureList* self )
{
	g_return_if_fail( APP_IS_PICTURE_LIST( self ) );

	g_array_set_size( self->history, self->index + 1 );
}

guint app_picture_list_n_items( AppPictureList* self )
{
	g_return_val_if_fail( APP_IS_PICTURE_LIST( self ), 0 );

	return self->picture_list->len;
}

#endif

#endif
