#include "stdafx.h"

#include <locale.h>
#include <stdlib.h>

#include "utils/res-utils.h"
#include "AppWindow.h"
#include "AppFolderHistory.h"

GSettings* app_settings = NULL;

static void _quit_app( GObject* source_object, GAsyncResult* res, gpointer application )
{
	g_application_release( ( gpointer )application );
}

static void _activate_quit( GSimpleAction* simple, GVariant* parameter, gpointer application )
{
	g_application_release( ( gpointer )application );
}

static GtkCssProvider* css_provider = NULL;
static gboolean        _reload_css( gpointer user_data )
{
	if ( css_provider )
		gtk_style_context_remove_provider_for_display( gdk_display_get_default(), ( gpointer )css_provider );

	css_provider = gtk_css_provider_new();
	gtk_css_provider_load_from_resource( css_provider, "/io/github/q962/PictureWindow/css/main.css" );
	gtk_style_context_add_provider_for_display(
	  gdk_display_get_default(), GTK_STYLE_PROVIDER( css_provider ), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION );

	GtkSettings* gtk_settings = gtk_settings_get_default();
	g_object_bind_property(
	  gtk_settings, "gtk-interface-color-scheme", css_provider, "prefers-color-scheme", G_BINDING_SYNC_CREATE );

	g_clear_object( &css_provider );

	return G_SOURCE_CONTINUE;
}

static void _startup( GtkApplication* application )
{
	_reload_css( NULL );
	// g_timeout_add( 1000, _reload_css, NULL );

	const GActionEntry entries[] = { { "quit", _activate_quit } };

	g_action_map_add_action_entries( G_ACTION_MAP( application ), entries, G_N_ELEMENTS( entries ), application );
}

static void _activate( GtkApplication* application )
{
	GtkWindow* win = ( gpointer )app_window_new( NULL );

	gtk_application_add_window( application, win );

	gtk_window_present( win );
}

static void _open_file( GtkApplication* application, GFile** files, gint n_files, gchar* hint, gpointer user_data )
{
	GPtrArray* file_array = g_ptr_array_new();

	for ( int i = 0; i < n_files; i++ ) {
		GFile* file = files[ i ];
		g_ptr_array_add( file_array, ( gpointer )g_file_peek_path( file ) );
	}
	g_ptr_array_add( file_array, NULL );

	GtkWindow* win = ( gpointer )app_window_new( ( const char** )file_array->pdata );

	g_ptr_array_unref( file_array );

	gtk_application_add_window( application, win );

	gtk_window_present( win );
}

int main( int argc, char** argv )
{
	g_setenv( "PANGOCAIRO_BACKEND", "fc", TRUE );

#ifdef G_OS_WIN32
	g_setenv( "GSK_RENDERER", "cairo", TRUE );
#endif

	app_settings = g_settings_new( APPID );

	GtkApplication* app = gtk_application_new( APPID, G_APPLICATION_HANDLES_OPEN | G_APPLICATION_NON_UNIQUE );

	setlocale( LC_ALL, "" );
	bindtextdomain( GETTEXT_PACKAGE, app_share_path( "locale", NULL ) );
	bind_textdomain_codeset( GETTEXT_PACKAGE, "UTF-8" );
	textdomain( GETTEXT_PACKAGE );

	g_application_set_version( ( gpointer )app, "1.0.0" );

	g_signal_connect( app, "startup", G_CALLBACK( _startup ), NULL );
	g_signal_connect( app, "activate", G_CALLBACK( _activate ), NULL );
	g_signal_connect( app, "open", G_CALLBACK( _open_file ), NULL );

	g_application_run( ( gpointer )app, argc, argv );

	g_object_unref( app );

	return EXIT_SUCCESS;
}
