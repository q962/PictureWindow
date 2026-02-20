#pragma once

#include "stdafx.h"

G_BEGIN_DECLS

#if 1  // gobject definition :: AppFolderHistory

G_DECLARE_FINAL_TYPE( AppFolderHistory, app_folder_history, APP, FOLDER_HISTORY, GObject )

AppFolderHistory* app_folder_history_get();

void app_folder_history_append( AppFolderHistory* self, const char* path );

void app_folder_history_clear( AppFolderHistory* self );

const char* app_folder_history_index( AppFolderHistory* self, guint index );

#endif

G_END_DECLS
