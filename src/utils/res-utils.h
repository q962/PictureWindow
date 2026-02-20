#pragma once

#include "../stdafx.h"

const char* gres_path_translate( const char* path );

const char* app_exe_path();
const char* app_prefix_path( const char* path, ... );
const char* app_share_path( const char* path, ... );
const char* app_libexec_path( const char* path, ... );
const char* app_data_path( const char* path, ... );
const char* app_user_data_path( const char* path, ... );