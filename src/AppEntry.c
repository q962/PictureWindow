#include "AppEntry.h"

#if 1  // gobject definition :: AppEntry

struct _AppEntry {
	GtkEntry parent_instance;

	char* before;
	char* defalut_val;
	char* after;

	guint id;
};

enum {
	PROP_0,
	PROP_N,
};

G_DEFINE_TYPE( AppEntry, app_entry, gtk_entry_get_type() )

#if 1  // static function

static void _limit_selection_bounds( AppEntry* self )
{
	const char* buffer = gtk_editable_get_text( ( gpointer )self );

	int char_count = g_utf8_strlen( buffer, -1 );

	int start_pos = 0;
	int end_pos   = 0;

	gtk_editable_get_selection_bounds( ( gpointer )self, &start_pos, &end_pos );

	if ( start_pos == end_pos )
		return;

	int new_start_pos = start_pos;
	int new_end_pos   = end_pos;

	do {
		if ( self->after ) {
			int right_edge_position = char_count - g_utf8_strlen( self->after, -1 );

			if ( start_pos >= right_edge_position && end_pos >= right_edge_position ) {
				new_start_pos = right_edge_position;
				new_end_pos   = right_edge_position;

				break;
			}
			else {
				new_end_pos = MIN( right_edge_position, end_pos );
			}
		}
		if ( self->before ) {
			int left_edge_position = g_utf8_strlen( self->before, -1 );

			if ( start_pos <= left_edge_position && end_pos <= left_edge_position ) {
				new_start_pos = left_edge_position;
				new_end_pos   = left_edge_position;

				break;
			}
			else {
				new_start_pos = MAX( left_edge_position, start_pos );
			}
		}
	} while ( 0 );

	if ( start_pos != new_start_pos || end_pos != new_end_pos ) {
		gtk_editable_select_region( ( gpointer )self, new_start_pos, new_end_pos );
	}
}

static void _on_cursor_position_changed( AppEntry* self )
{
	if ( gtk_editable_get_selection_bounds( ( gpointer )self, NULL, NULL ) ) {
		g_signal_handlers_block_by_func( self, G_CALLBACK( _on_cursor_position_changed ), NULL );

		_limit_selection_bounds( self );

		g_signal_handlers_unblock_by_func( self, G_CALLBACK( _on_cursor_position_changed ), NULL );

		return;
	}

	const char* buffer = gtk_editable_get_text( ( gpointer )self );

	int position   = gtk_editable_get_position( ( gpointer )self );
	int char_count = g_utf8_strlen( buffer, -1 );

	int selection_bound = 0;
	g_object_get( ( gpointer )self, "selection-bound", &selection_bound, NULL );

	g_signal_handlers_block_by_func( self, G_CALLBACK( _on_cursor_position_changed ), NULL );
	{
		if ( self->after ) {
			int right_edge_position = char_count - g_utf8_strlen( self->after, -1 );

			if ( position > right_edge_position ) {
				gtk_editable_set_position( ( gpointer )self, right_edge_position );
			}
		}
		if ( self->before ) {
			int left_edge_position = g_utf8_strlen( self->before, -1 );

			if ( position < left_edge_position ) {
				gtk_editable_set_position( ( gpointer )self, left_edge_position );
			}
		}
	}

	g_signal_handlers_unblock_by_func( self, G_CALLBACK( _on_cursor_position_changed ), NULL );
}

static void _on_delete_text( GtkEditable* editable, gint start_pos, gint end_pos, gpointer user_data )
{
	AppEntry* self = ( gpointer )user_data;

	const char* buffer = gtk_editable_get_text( ( gpointer )editable );

	int char_count = g_utf8_strlen( buffer, -1 );

	int left_edge_position  = self->before ? g_utf8_strlen( self->before, -1 ) : start_pos;
	int right_edge_position = self->after ? char_count - g_utf8_strlen( self->after, -1 ) : end_pos;

	if ( start_pos < left_edge_position || end_pos > right_edge_position ) {
		g_signal_stop_emission_by_name( editable, "delete-text" );
	}
}

static void _on_text_changed( GtkEditable* editable, GParamSpec* pspec, AppEntry* self )
{
	g_signal_handlers_block_by_func( editable, G_CALLBACK( _on_delete_text ), self );

	const char* buffer = gtk_editable_get_text( editable );

	char* str =
	  g_strdup_printf( "%s%s%s",
	                   ( self->before && ( !g_str_has_prefix( buffer, self->before ) ) ) ? self->before : "",  //
	                   buffer,
	                   ( self->after && ( !g_str_has_suffix( buffer, self->after ) ) ) ? self->after : "" );

	if ( g_strcmp0( str, buffer ) != 0 )
		gtk_editable_set_text( editable, str );

	g_free( str );

	g_signal_handlers_unblock_by_func( editable, G_CALLBACK( _on_delete_text ), self );
}

#endif

#if 1  // base class virtual function

static void app_entry_snapshot( GtkWidget* widget, GtkSnapshot* snapshot )
{
	GTK_WIDGET_CLASS( app_entry_parent_class )->snapshot( widget, snapshot );
}

static void app_entry_set_property( GObject* object, guint property_id, const GValue* value, GParamSpec* pspec )
{
	AppEntry* self = ( AppEntry* )object;

	switch ( property_id ) {
		default: G_OBJECT_WARN_INVALID_PROPERTY_ID( object, property_id, pspec ); break;
	}
}

static void app_entry_get_property( GObject* object, guint property_id, GValue* value, GParamSpec* pspec )
{
	AppEntry* self = ( AppEntry* )object;

	switch ( property_id ) {
		default: G_OBJECT_WARN_INVALID_PROPERTY_ID( object, property_id, pspec ); break;
	}
}

static void app_entry_constructed( GObject* object )
{
	AppEntry* self = ( AppEntry* )object;

	G_OBJECT_CLASS( app_entry_parent_class )->constructed( object );
}

static void app_entry_dispose( GObject* object )
{
	AppEntry* self = ( AppEntry* )object;

	G_OBJECT_CLASS( app_entry_parent_class )->dispose( object );
}

static void app_entry_finalize( GObject* object )
{
	AppEntry* self = ( AppEntry* )object;

	g_free( self->before );
	g_free( self->defalut_val );
	g_free( self->after );

	G_OBJECT_CLASS( app_entry_parent_class )->finalize( object );
}

static void app_entry_init( AppEntry* self )
{
	GtkWidget* text = gtk_widget_get_first_child( ( gpointer )self );
	g_signal_connect( text, "delete-text", G_CALLBACK( _on_delete_text ), self );
	g_signal_connect( text, "notify::text", G_CALLBACK( _on_text_changed ), self );

	g_signal_connect( self, "notify::cursor-position", G_CALLBACK( _on_cursor_position_changed ), NULL );
}

static void app_entry_class_init( AppEntryClass* klass )
{
	GObjectClass*   base_class   = ( GObjectClass* )klass;
	GtkWidgetClass* widget_class = ( GtkWidgetClass* )klass;
	GtkEntryClass*  parent_class = ( GtkEntryClass* )klass;

	base_class->constructed  = app_entry_constructed;
	base_class->dispose      = app_entry_dispose;
	base_class->finalize     = app_entry_finalize;
	base_class->set_property = app_entry_set_property;
	base_class->get_property = app_entry_get_property;

	widget_class->snapshot = app_entry_snapshot;
}

#endif

#if 1  // public function

AppEntry* app_entry_new( char* before, char* defalut_val, char* after )
{
	AppEntry* self = g_object_new( app_entry_get_type(), NULL );

	g_set_str( &self->before, before );
	g_set_str( &self->defalut_val, defalut_val );
	g_set_str( &self->after, after );

	if ( defalut_val )
		gtk_editable_set_text( ( gpointer )self, defalut_val );

	return self;
}

#endif

#endif
