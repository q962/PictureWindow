#include "AppEntryBufferVerify.h"

#if 1  // gobject definition :: AppEntryBufferVerify

struct _AppEntryBufferVerify {
	GtkEntryBuffer parent_instance;

	GtkEntryBuffer* buffer;

	GRegex* regex;
};

enum {
	PROP_0,
	PROP_N,
};

G_DEFINE_TYPE( AppEntryBufferVerify, app_entry_buffer_verify, gtk_entry_buffer_get_type() )

#if 1  // static function

#endif

#if 1  // base class virtual function

static void app_entry_buffer_verify_set_property( GObject*      object,
                                                  guint         property_id,
                                                  const GValue* value,
                                                  GParamSpec*   pspec )
{
	AppEntryBufferVerify* self = ( AppEntryBufferVerify* )object;

	switch ( property_id ) {
		default: G_OBJECT_WARN_INVALID_PROPERTY_ID( object, property_id, pspec ); break;
	}
}

static void app_entry_buffer_verify_get_property( GObject* object, guint property_id, GValue* value, GParamSpec* pspec )
{
	AppEntryBufferVerify* self = ( AppEntryBufferVerify* )object;

	switch ( property_id ) {
		default: G_OBJECT_WARN_INVALID_PROPERTY_ID( object, property_id, pspec ); break;
	}
}

static guint app_entry_buffer_verify_insert_text( GtkEntryBuffer* buffer,
                                                  guint           position,
                                                  const char*     chars,
                                                  guint           n_chars )
{
	AppEntryBufferVerify* self = ( AppEntryBufferVerify* )buffer;

	gtk_entry_buffer_set_text(
	  self->buffer, gtk_entry_buffer_get_text( buffer ), gtk_entry_buffer_get_length( buffer ) );

	gtk_entry_buffer_insert_text( self->buffer, position, chars, n_chars );

	const char* buffer_str = gtk_entry_buffer_get_text( self->buffer );

	if ( g_regex_match( self->regex, buffer_str, G_REGEX_MATCH_DEFAULT, NULL ) ) {
		return GTK_ENTRY_BUFFER_CLASS( app_entry_buffer_verify_parent_class )
		  ->insert_text( buffer, position, chars, n_chars );
	}

	return 0;
}

static void app_entry_buffer_verify_constructed( GObject* object )
{
	AppEntryBufferVerify* self = ( AppEntryBufferVerify* )object;

	G_OBJECT_CLASS( app_entry_buffer_verify_parent_class )->constructed( object );
}

static void app_entry_buffer_verify_dispose( GObject* object )
{
	AppEntryBufferVerify* self = ( AppEntryBufferVerify* )object;

	g_clear_object( &self->buffer );

	G_OBJECT_CLASS( app_entry_buffer_verify_parent_class )->dispose( object );
}

static void app_entry_buffer_verify_finalize( GObject* object )
{
	AppEntryBufferVerify* self = ( AppEntryBufferVerify* )object;

	G_OBJECT_CLASS( app_entry_buffer_verify_parent_class )->finalize( object );
}

static void app_entry_buffer_verify_init( AppEntryBufferVerify* self )
{
	self->buffer = gtk_entry_buffer_new( NULL, -1 );
}

static void app_entry_buffer_verify_class_init( AppEntryBufferVerifyClass* klass )
{
	GObjectClass*        base_class   = ( GObjectClass* )klass;
	GtkEntryBufferClass* parent_class = ( GtkEntryBufferClass* )klass;

	base_class->constructed  = app_entry_buffer_verify_constructed;
	base_class->dispose      = app_entry_buffer_verify_dispose;
	base_class->finalize     = app_entry_buffer_verify_finalize;
	base_class->set_property = app_entry_buffer_verify_set_property;
	base_class->get_property = app_entry_buffer_verify_get_property;

	parent_class->insert_text = app_entry_buffer_verify_insert_text;
}

#endif

#if 1  // public function

AppEntryBufferVerify* app_entry_buffer_verify_new( const char* regex_str )
{
	GError* error = NULL;
	GRegex* regex = g_regex_new( regex_str, G_REGEX_OPTIMIZE, 0, &error );
	if ( error ) {
		g_warning( "regex error: %s", error->message );
		return NULL;
	}

	AppEntryBufferVerify* self = g_object_new( app_entry_buffer_verify_get_type(), NULL );

	self->regex = regex;

	return self;
}

#endif

#endif
