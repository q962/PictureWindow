#include "utils.h"

// https://gnome.pages.gitlab.gnome.org/glycin/#supported-image-formats
static GRegex* test = NULL;

gboolean _is_glycin_format( const char* path )
{
	if ( !test )
		test = g_regex_new( "\\.("
		                    "avif"
		                    "|bmp"
		                    "|cr2"
		                    "|dds"
		                    "|dng"
		                    "|erf"
		                    "|exr"
		                    "|gif"
		                    "|heic"
		                    "|ico"
		                    "|jpe?g"
		                    "|jxl"
		                    "|mrw"
		                    "|orf"
		                    "|pbm"
		                    "|pef"
		                    "|pgm"
		                    "|png"
		                    "|ppm"
		                    "|qoi"
		                    "|rw2?"
		                    "|srf"
		                    "|svgz?"
		                    "|tga"
		                    "|tiff?"
		                    "|webp"
		                    "|xbm"
		                    "|xpm"
		                    ")$",
		                    G_REGEX_CASELESS | G_REGEX_OPTIMIZE,
		                    0,
		                    NULL );

	return g_regex_match( test, path, 0, NULL );
}
