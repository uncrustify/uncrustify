#include "config.h"

#include "nautilus-previewer.h"

#define PREVIEWER2_DBUS_IFACE "org.gnome.NautilusPreviewer2"
#define PREVIEWER_DBUS_PATH "/org/gnome/NautilusPreviewer"

static GDBusProxy * previewer_v2_proxy = NULL;
