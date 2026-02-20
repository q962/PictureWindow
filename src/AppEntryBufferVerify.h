#pragma once

#include "stdafx.h"

G_BEGIN_DECLS

#if 1  // gobject definition :: AppEntryBufferVerify

G_DECLARE_FINAL_TYPE( AppEntryBufferVerify, app_entry_buffer_verify, APP, ENTRY_BUFFER_VERIFY, GtkEntryBuffer )

AppEntryBufferVerify* app_entry_buffer_verify_new( const char* regex_str );

#endif

G_END_DECLS
