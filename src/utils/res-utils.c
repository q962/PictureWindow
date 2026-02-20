#include "res-utils.h"

static void _path_compression( GString* path )
{
	if ( !path || path->len == 0 )
		return;

	char* buf  = path->str;
	gsize read = 0, write = 0;

	while ( read < path->len ) {
		char c = buf[ read ];

		if ( c == '/' || c == '\\' ) {
			buf[ write++ ] = G_DIR_SEPARATOR;

			while ( read < path->len && ( buf[ read ] == '/' || buf[ read ] == '\\' ) )
				read++;
		}
		else {
			buf[ write++ ] = buf[ read++ ];
		}
	}

	g_string_set_size( path, write );
}

static void _va_append( GString* path, const char* arg1, va_list v_args )
{
	va_list args;
	va_copy( args, v_args );

	const char* arg = arg1;
	do {
		if ( !arg )
			break;

		if ( arg[ 0 ] == '\0' )
			goto _continue;

		if ( arg[ 0 ] != '/' || arg[ 0 ] != '\\' )
			g_string_append( path, G_DIR_SEPARATOR_S );
		g_string_append( path, arg );
	_continue:;
	} while ( ( arg = va_arg( args, const char* ) ) );

	_path_compression( path );

	va_end( args );
}

const char* gres_path_translate( const char* path )
{
	static char _path_buf[ 4096 ] = "";
	if ( !path )
		return NULL;

	if ( path[ 0 ] == '/' ) {
		path = path + 1;
	}

	g_snprintf( _path_buf, sizeof( _path_buf ) - 1, APPRESPREFIX "%s", path );

	return _path_buf;
}

const char* app_exe_path()
{
	static GString* exe_path = NULL;
	if ( g_once_init_enter_pointer( ( gpointer* )&exe_path ) ) {
		GString* str = g_string_new( NULL );

#ifdef G_OS_WIN32
		{
			wchar_t* path   = g_malloc( 4096 );
			DWORD    result = GetModuleFileNameW( NULL, path, 4096 - 1 );

			g_return_val_if_fail( result, NULL );

			result = result * sizeof( wchar_t );

			GError* error    = NULL;
			char*   execpath = g_convert( ( const char* )path, result, "utf-8", "utf-16", NULL, NULL, &error );
			if ( error ) {
				g_error( "启动程序失败，未能获取程序路径: %s", error->message );
			}

			g_string_append( str, execpath );

			g_free( execpath );
		}
#elif defined( G_OS_UNIX )
		{
			char* self_exe = g_file_read_link( "/proc/self/exe", NULL );

			if ( !self_exe ) {
				g_string_set_size( str, 0 );
				g_error( "/proc/self/exe" );
			}
			else {
				g_string_printf( str, "%s", self_exe );
				g_free( self_exe );
			}
		}
#else
#error unsupport
#endif

		g_once_init_leave_pointer( ( gpointer* )&exe_path, str );
	}

	if ( exe_path->len == 0 )
		return NULL;

	return exe_path->str;
}

const char* app_prefix_path( const char* path, ... )
{
	static GString* prefix_path        = NULL;
	static guint    prefix_path_length = 0;
	if ( g_once_init_enter_pointer( ( gpointer* )&prefix_path ) ) {
		GString* str = g_string_new( NULL );

#ifdef DEBUG
		const char* path = g_getenv( "PROJECT_PREFIX" );
		g_string_append( str, path ? path : PROJECT_PREFIX );
#else

#if defined( PREFIX )
		if ( sizeof( PREFIX ) > 0 ) {
			g_string_append( str, PREFIX );
		}
		else
#endif
			G_STMT_START
			{
				const char* exe_path = app_exe_path();
				if ( !exe_path )
					break;
				int exe_path_len = strlen( exe_path );

				char* bin_path = NULL;
				bin_path       = g_utf8_strrchr( exe_path, exe_path_len, '/' );
				bin_path       = bin_path ? bin_path : g_utf8_strrchr( exe_path, exe_path_len, '\\' );

				if ( !bin_path )
					break;

				char* prefix_path = NULL;
				prefix_path       = g_utf8_strrchr( exe_path, ( bin_path - exe_path ) - 1, '/' );
				prefix_path = prefix_path ? prefix_path : g_utf8_strrchr( exe_path, ( bin_path - exe_path ) - 1, '\\' );

				if ( !prefix_path )
					break;

				g_string_append_len( str, exe_path, prefix_path - exe_path );
			}
		G_STMT_END;
#endif

		prefix_path_length = str->len;

		g_once_init_leave_pointer( ( gpointer* )&prefix_path, str );
	}

	g_string_set_size( prefix_path, prefix_path_length );

	if ( prefix_path_length == 0 )
		return NULL;

	va_list args;
	va_start( args, path );
	_va_append( prefix_path, path, args );
	va_end( args );

	return prefix_path->str;
}

const char* app_libexec_path( const char* path, ... )
{
	static GString* libexec_path        = NULL;
	static guint    libexec_path_length = 0;
	if ( g_once_init_enter_pointer( ( gpointer* )&libexec_path ) ) {
		GString* str = g_string_new( NULL );

#ifdef G_OS_WIN32
		const char* prefix_path = app_prefix_path( "bin", NULL );
#else
		const char* prefix_path = app_prefix_path( "libexec" );
#endif
		g_string_append( str, prefix_path );

		libexec_path_length = str->len;

		g_once_init_leave_pointer( ( gpointer* )&libexec_path, str );
	}

	g_string_set_size( libexec_path, libexec_path_length );

	if ( libexec_path_length == 0 )
		return NULL;

	va_list args;
	va_start( args, path );
	_va_append( libexec_path, path, args );
	va_end( args );

	return libexec_path->str;
}

const char* app_share_path( const char* path, ... )
{
	static GString* share_path        = NULL;
	static guint    share_path_length = 0;
	if ( g_once_init_enter_pointer( ( gpointer* )&share_path ) ) {
		GString* str = g_string_new( NULL );

#ifdef DEBUG
		const char* prefix_path = app_prefix_path( NULL );
#else
		const char* prefix_path = app_prefix_path( "share", NULL );
#endif
		if ( prefix_path ) {
			g_string_append( str, prefix_path );

			share_path_length = str->len;
		}
		g_once_init_leave_pointer( ( gpointer* )&share_path, str );
	}

	g_string_set_size( share_path, share_path_length );

	if ( share_path_length == 0 )
		return NULL;

	va_list args;
	va_start( args, path );
	_va_append( share_path, path, args );
	va_end( args );

	return share_path->str;
}

const char* app_data_path( const char* path1, ... )
{
	static GString* path        = NULL;
	static guint    path_length = 0;
	if ( g_once_init_enter_pointer( ( gpointer* )&path ) ) {
		GFile* file = NULL;

		G_STMT_START
		{
			const char* _prefix_path = app_share_path( NULL );
			if ( _prefix_path ) {
				file = g_file_new_build_filename( _prefix_path, APPID, NULL );
				if ( g_file_query_file_type( file, G_FILE_QUERY_INFO_NONE, NULL ) == G_FILE_TYPE_DIRECTORY ) {
					break;
				}
				g_set_object( ( gpointer* )&file, NULL );
			}

			const char* user_data_dir = g_get_user_data_dir();
			file                      = g_file_new_build_filename( user_data_dir, APPID, NULL );
			if ( g_file_query_file_type( file, G_FILE_QUERY_INFO_NONE, NULL ) == G_FILE_TYPE_DIRECTORY ) {
				break;
			}
			g_set_object( ( gpointer* )&file, NULL );

			const char* const* dirs = g_get_system_data_dirs();
			for ( const char* const* dir = dirs; dirs && *dir; dir++ ) {
				file = g_file_new_build_filename( *dir, APPID, NULL );
				if ( g_file_query_file_type( file, G_FILE_QUERY_INFO_NONE, NULL ) == G_FILE_TYPE_DIRECTORY ) {
					break;
				}
				g_set_object( ( gpointer* )&file, NULL );
			}

			// failed, use alternative
			file = g_file_new_build_filename( user_data_dir, APPID, NULL );
			g_file_make_directory_with_parents( file, NULL, NULL );
		}
		G_STMT_END;

		GString* str = NULL;

		if ( file ) {
			str = g_string_new( g_file_peek_path( file ) );

			path_length = str->len;
			g_object_unref( file );
		}

		g_once_init_leave_pointer( ( gpointer* )&path, str );
	}

	g_string_set_size( path, path_length );

	if ( path_length == 0 )
		return NULL;

	va_list args;
	va_start( args, path1 );
	_va_append( path, path1, args );
	va_end( args );

	return path->str;
}

const char* app_user_data_path( const char* path1, ... )
{
	static GString* path        = NULL;
	static guint    path_length = 0;
	if ( g_once_init_enter_pointer( ( gpointer* )&path ) ) {
#ifdef DEBUG

#ifndef APP_USER_DATA_PATH
#define APP_USER_DATA_PATH ""
#endif

		const char* user_data_dir = APP_USER_DATA_PATH;
#else
		const char* user_data_dir = g_get_user_data_dir();
#endif

		GFile* file = g_file_new_build_filename( user_data_dir, APPID, NULL );
		g_file_make_directory( file, NULL, NULL );

		GString* str = g_string_new( g_file_peek_path( file ) );

		path_length = str->len;

		g_object_unref( file );

		g_once_init_leave_pointer( ( gpointer* )&path, str );
	}

	g_string_set_size( path, path_length );

	va_list args;
	va_start( args, path1 );
	_va_append( path, path1, args );
	va_end( args );

	return path->str;
}