#pragma once

#include "../build/config.h"

#include <gtk/gtk.h>

#define GETTEXT_PACKAGE APPID
#include <glib/gi18n.h>

#include "utils/gtk-extra.h"
#include "utils/glib-extra.h"
#include "utils/utils.h"

extern GSettings* app_settings;