#pragma once

#include "stdafx.h"

G_BEGIN_DECLS

#if 1  // gobject definition :: AppPictureList

#define APP_PICTURE_LIST_SIGNAL_N_ITEMS "n-items"  // (guint)

typedef enum {
	PICTURE_LOOP_0,
	PICTURE_LOOP_RANDOM,
	PICTURE_LOOP_SEQUENTIAL,
	PICTURE_LOOP_N
} PictureLoopMode;

GType picture_loop_mode_get_type( void );

G_DECLARE_FINAL_TYPE( AppPictureList, app_picture_list, APP, PICTURE_LIST, GObject )

AppPictureList* app_picture_list_new( PictureLoopMode mode, const char* path );

void app_picture_list_load( AppPictureList* self, const char* path );

void app_picture_list_append( AppPictureList* self, const char* path );
void app_picture_list_appends( AppPictureList* self, const char** paths, guint count );

//! app_picture_list_next() == path
void app_picture_list_append_next( AppPictureList* self, const char* path );
void app_picture_list_append_nexts( AppPictureList* self, const char** paths, int count );

void app_picture_list_remove_index( AppPictureList* self, guint index );
void app_picture_list_remove_all( AppPictureList* self );

void app_picture_list_set_mode( AppPictureList* self, PictureLoopMode mode );

gboolean    app_picture_list_can_prev( AppPictureList* self );
const char* app_picture_list_prev( AppPictureList* self );
const char* app_picture_list_next( AppPictureList* self );
const char* app_picture_list_index( AppPictureList* self );
void        app_picture_list_iter_reset( AppPictureList* self );

void app_picture_list_truncation( AppPictureList* self );

guint app_picture_list_n_items( AppPictureList* self );

#endif

G_END_DECLS
