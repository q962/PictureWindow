#pragma once

#include "stdafx.h"

G_BEGIN_DECLS

#if 1  // gobject definition :: AppBinBox

G_DECLARE_FINAL_TYPE( AppBinBox, app_bin_box, APP, BIN_BOX, GtkWidget )

AppBinBox* app_bin_box_new();

#endif

G_END_DECLS
