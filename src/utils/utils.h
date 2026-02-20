#pragma once

#define TIMESTAMP( X ) GDateTime* _TIMESTAMP_##X = g_date_time_new_now_utc();

#define TIMESTAMP_PRINT( X1, X2 ) \
	g_message( "%dms", g_date_time_difference( _TIMESTAMP_##X2, _TIMESTAMP_##X1 ) / G_TIME_SPAN_MILLISECOND );

#define TIMESTAMP_PRINT_TIP( X1, X2, TIP ) \
	g_message(                             \
	  "%s: %dms", ( TIP ), g_date_time_difference( _TIMESTAMP_##X2, _TIMESTAMP_##X1 ) / G_TIME_SPAN_MILLISECOND );

#define _G_ERROR_IS( X, Y ) ( error->domain == ( X ) && error->code == ( X##_##Y ) )
