/*
 * TODO: check for errors return values where its possible
 * TODO: power saving features (timers to dim screen, etc...). Only do this if no alternatives. (check acpi)
 * 
 * can add more interfaces to monitor more things in the future
 */
#include "mpdbus.h"
#include "mpdctl.h"
#include <assert.h>
#include <getopt.h>
#include <gio/gio.h>
#include <glib-unix.h>
#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/* ---------------------------------------------------------------------------------------------------- */

#define TIMEOUT 1

int port = 6600;
char *host = "localhost";

static GDBusNodeInfo *introspection_data = NULL;

// TEMP = virer ça de là
static GDBusConnection *conn;

/* Introspection data for the service we are exporting */
static const gchar introspection_xml[] =
    "<node>"
    "<interface name='org.mpris.MediaPlayer2'>"
    "    <method name='Raise' />"
    "    <method name='Quit' />"
    "    <property name='CanQuit' type='b' access='read' />"
    "    <property name='CanRaise' type='b' access='read' />"
    "    <property name='HasTrackList' type='b' access='read' />"
    "    <property name='Identity' type='s' access='read' />"
    "    <property name='SupportedUriSchemes' type='as' access='read' />"
    "    <property name='SupportedMimeTypes' type='as' access='read' />"
    "</interface>"
    "<interface name='org.mpris.MediaPlayer2.Player'>"
    "    <method name='Next' />"
    "    <method name='Previous' />"
    "    <method name='Pause' />"
    "    <method name='PlayPause' />"
    "    <method name='Stop' />"
    "    <method name='Play' />"
    "    <method name='Seek'> "
    "        <arg name='Offset' direction='in' type='x'/>"
    "    </method>"
    "    <method name='SetPosition'>"
    "        <arg name='TrackId' direction='in' type='o'/>"
    "        <arg name='Position' direction='in' type='x'/>"
    "    </method>"
    "    <!-- <method name='OpenUri'>"
    "        <arg name='Uri' direction='in' type='s'/>"
    "    </method> -->"
    "    <signal name='Seeked'>"
    "        <arg name='Position' type='x'/>"
    "    </signal>"
    "    <property name='PlaybackStatus' type='s' access='read' />"
    "    <property name='LoopStatus' type='s' access='readwrite' />"
    "    <property name='Rate' type='d' access='readwrite' />"
    "    <property name='Shuffle' type='b' access='readwrite' />"
    "    <property name='Metadata' type='a{sv}' access='read' />"
    "    <property name='Volume' type='d' access='readwrite' />"
    "    <property name='Position' type='x' access='read' />"
    "    <property name='MinimumRate' type='d' access='read' />"
    "    <property name='MaximumRate' type='d' access='read' />"
    "    <property name='CanGoNext' type='b' access='read' />"
    "    <property name='CanGoPrevious' type='b' access='read' />"
    "    <property name='CanPlay' type='b' access='read' />"
    "    <property name='CanPause' type='b' access='read' />"
    "    <property name='CanSeek' type='b' access='read' />"
    "    <property name='CanControl' type='b' access='read' />"
    "</interface>"
    "</node>";

/* ---------------------------------------------------------------------------------------------------- */

void send_properties_changed_signal(const gchar *interface_name, GVariantBuilder *changed, GVariantBuilder *invalidated) {
    g_dbus_connection_emit_signal(conn,
                                  NULL,
                                  OBJECT_PATH,
                                  "org.freedesktop.DBus.Properties",
                                  "PropertiesChanged",
                                  g_variant_new("(sa{sv}as)",
                                                interface_name,
                                                changed,
                                                invalidated),
                                  NULL);
}

static void handle_mpris_interface_method(const gchar *method_name, GVariant *parameters, GDBusMethodInvocation *invocation) {
    if (g_strcmp0(method_name, "Quit") == 0) {
        g_dbus_method_invocation_return_value(invocation, NULL);
    } else if (g_strcmp0(method_name, "Raise") == 0) {
        g_dbus_method_invocation_return_value(invocation, NULL);
    }
}

static void handle_player_interface_method(const gchar *method_name, GVariant *parameters, GDBusMethodInvocation *invocation) {
    if (g_strcmp0(method_name, "Next") == 0) {
        if (mpdctl_get_player()->can_go_next)
            mpdctl_next();
        g_dbus_method_invocation_return_value(invocation, NULL);
    // } else if (g_strcmp0(method_name, "OpenUri") == 0) {
    //     // TODO

    //     g_dbus_method_invocation_return_value(invocation, NULL);
    } else if (g_strcmp0(method_name, "Pause") == 0) {
        if (mpdctl_get_player()->can_pause)
            mpdctl_pause();
        g_dbus_method_invocation_return_value(invocation, NULL);
    } else if (g_strcmp0(method_name, "Play") == 0) {
        if (mpdctl_get_player()->can_play)
            mpdctl_play();
        g_dbus_method_invocation_return_value(invocation, NULL);
    } else if (g_strcmp0(method_name, "PlayPause") == 0) {
        if (mpdctl_get_player()->can_pause) {
            mpdctl_toggle_pause();
            g_dbus_method_invocation_return_value(invocation, NULL);
        } else {
            g_dbus_method_invocation_return_error_literal(invocation, g_quark_from_string("mpris-error"), G_ERR_UNKNOWN, "CanPause is FALSE\n");
        }
    } else if (g_strcmp0(method_name, "Previous") == 0) {
        if (mpdctl_get_player()->can_go_previous)
            mpdctl_previous();
        g_dbus_method_invocation_return_value(invocation, NULL);
    } else if (g_strcmp0(method_name, "Seek") == 0) {
        gint64 offset;
        g_variant_get(parameters, "(x)", &offset);
        if (mpdctl_get_player()->can_seek) {
            mpdctl_seek(offset / 1000000, true);
        }
        g_dbus_method_invocation_return_value(invocation, NULL);
    } else if (g_strcmp0(method_name, "SetPosition") == 0) {
        gchar *trackid;
        gint64 position;
        g_variant_get(parameters, "(ox)", &trackid, &position);

        char buf[128];
        int ret = snprintf(buf, 128, TRACK_ID_PATH, mpdctl_get_metadata()->id);

        if (mpdctl_get_player()->can_seek && g_strcmp0(trackid, buf) == 0) {
            if (position >= 0 && position <= mpdctl_get_player()->length) {
                mpdctl_seek(position / 1000000, false);
            }
        }

        free(trackid);
        g_dbus_method_invocation_return_value(invocation, NULL);
    } else if (g_strcmp0(method_name, "Stop") == 0) {
        if (mpdctl_get_player()->can_control) {
            mpdctl_stop();
            g_dbus_method_invocation_return_value(invocation, NULL);
        } else {
            g_dbus_method_invocation_return_error_literal(invocation, g_quark_from_string("mpris-error"), G_ERR_UNKNOWN, "CanPause is FALSE\n");
        }
    }
}

static void handle_method_call(GDBusConnection *connection,
                               const gchar *sender,
                               const gchar *object_path,
                               const gchar *interface_name,
                               const gchar *method_name,
                               GVariant *parameters,
                               GDBusMethodInvocation *invocation,
                               gpointer user_data) {
    if (g_strcmp0(interface_name, INTERFACE_MPRIS) == 0) {
        handle_mpris_interface_method(method_name, parameters, invocation);
    } else if (g_strcmp0(interface_name, INTERFACE_PLAYER) == 0) {
        handle_player_interface_method(method_name, parameters, invocation);
    }
}

static GVariant *handle_mpris_interface_get(const gchar *property_name) {
    GVariant *ret = NULL;

    if (g_strcmp0(property_name, "Identity") == 0) {
        ret = g_variant_new_string(mpdctl_get_mpris()->identity);
    } else if (g_strcmp0(property_name, "CanQuit") == 0) {
        ret = g_variant_new_boolean(mpdctl_get_mpris()->can_quit);
    } else if (g_strcmp0(property_name, "CanRaise") == 0) {
        ret = g_variant_new_boolean(mpdctl_get_mpris()->can_raise);
    } else if (g_strcmp0(property_name, "HasTrackList") == 0) {
        ret = g_variant_new_boolean(mpdctl_get_mpris()->has_track_list);
    } else if (g_strcmp0(property_name, "SupportedMimeTypes") == 0) {
        // for now empty
        GVariantBuilder *builder = g_variant_builder_new(G_VARIANT_TYPE("as"));
        ret = g_variant_builder_end(builder);
        g_variant_builder_unref(builder);
    } else if (g_strcmp0(property_name, "SupportedUriSchemes") == 0) {
        // for now empty
        GVariantBuilder *builder = g_variant_builder_new(G_VARIANT_TYPE("as"));
        ret = g_variant_builder_end(builder);
        g_variant_builder_unref(builder);
    }

    return ret;
}

static GVariant *handle_player_interface_get(const gchar *property_name) {
    GVariant *ret = NULL;

    if (g_strcmp0(property_name, "CanControl") == 0) {
        ret = g_variant_new_boolean(mpdctl_get_player()->can_control);
    } else if (g_strcmp0(property_name, "CanGoNext") == 0) {
        ret = g_variant_new_boolean(mpdctl_get_player()->can_go_next);
    } else if (g_strcmp0(property_name, "CanGoPrevious") == 0) {
        ret = g_variant_new_boolean(mpdctl_get_player()->can_go_previous);
    } else if (g_strcmp0(property_name, "CanPause") == 0) {
        ret = g_variant_new_boolean(mpdctl_get_player()->can_pause);
    } else if (g_strcmp0(property_name, "CanPlay") == 0) {
        ret = g_variant_new_boolean(mpdctl_get_player()->can_play);
    } else if (g_strcmp0(property_name, "CanSeek") == 0) {
        ret = g_variant_new_boolean(mpdctl_get_player()->can_seek);
    } else if (g_strcmp0(property_name, "Shuffle") == 0) {
        ret = g_variant_new_boolean(mpdctl_get_player()->shuffle);
    } else if (g_strcmp0(property_name, "Metadata") == 0) {
        GVariantBuilder *builder = g_variant_builder_new(G_VARIANT_TYPE("a{sv}"));
        char buf[128];
        int r = snprintf(buf, 128, TRACK_ID_PATH, mpdctl_get_metadata()->id);
        g_variant_builder_add(builder, "{sv}", "mpris:trackid", g_variant_new_object_path(buf));
        g_variant_builder_add(builder, "{sv}", "mpris:length", g_variant_new_int64(mpdctl_get_metadata()->length));
        g_variant_builder_add(builder, "{sv}", "xesam:album", g_variant_new_string(mpdctl_get_metadata()->album));
        g_variant_builder_add(builder, "{sv}", "xesam:title", g_variant_new_string(mpdctl_get_metadata()->title));
        g_variant_builder_add(builder, "{sv}", "xesam:artist", g_variant_new_string(mpdctl_get_metadata()->artist));
        // TODO: more metadata
        ret = g_variant_builder_end(builder);
        g_variant_builder_unref(builder);
    } else if (g_strcmp0(property_name, "MaximumRate") == 0) {
        ret = g_variant_new_double(mpdctl_get_player()->maximum_rate);
    } else if (g_strcmp0(property_name, "MinimumRate") == 0) {
        ret = g_variant_new_double(mpdctl_get_player()->minimum_rate);
    } else if (g_strcmp0(property_name, "Rate") == 0) {
        ret = g_variant_new_double(mpdctl_get_player()->rate);
    } else if (g_strcmp0(property_name, "Volume") == 0) {
        ret = g_variant_new_double(mpdctl_get_player()->volume);
    } else if (g_strcmp0(property_name, "Position") == 0) {
        ret = g_variant_new_int64(mpdctl_get_player()->position);
    } else if (g_strcmp0(property_name, "LoopStatus") == 0) {
        ret = g_variant_new_string(mpdctl_get_player()->loop_status);
    } else if (g_strcmp0(property_name, "PlaybackStatus") == 0) {
        ret = g_variant_new_string(mpdctl_get_player()->playback_status);
    }

    return ret;
}

static GVariant *handle_get_property(GDBusConnection *connection,
                                     const gchar *sender,
                                     const gchar *object_path,
                                     const gchar *interface_name,
                                     const gchar *property_name,
                                     GError **error,
                                     gpointer user_data) {
    GVariant *ret = NULL;

    if (g_strcmp0(interface_name, INTERFACE_MPRIS) == 0) {
        ret = handle_mpris_interface_get(property_name);
    } else if (g_strcmp0(interface_name, INTERFACE_PLAYER) == 0) {
        ret = handle_player_interface_get(property_name);
    }

    return ret;
}

static GError **handle_player_interface_set(const gchar *property_name, GVariant *value, GError **error) {
    if (g_strcmp0(property_name, "Shuffle") == 0) {
        if (!(mpdctl_get_player()->can_control)) {
            g_set_error(error, G_IO_ERROR, G_IO_ERROR_FAILED, "CanControl is FALSE");
        } else {
            mpdctl_set_random(g_variant_get_boolean(value));
        }
    } else if (g_strcmp0(property_name, "Rate") == 0) {
        // Not supported
    } else if (g_strcmp0(property_name, "Volume") == 0) {
        if (!(mpdctl_get_player()->can_control)) {
            g_set_error(error, G_IO_ERROR, G_IO_ERROR_FAILED, "CanControl is FALSE");
        } else {
            gdouble vol = g_variant_get_double(value);
            if (vol < 0)
                vol = 0;
            else if (vol > 1)
                vol = 1;
            mpdctl_set_volume(vol * 100);
        }
    } else if (g_strcmp0(property_name, "LoopStatus") == 0) {
        const gchar *str = g_variant_get_string(value, NULL);

        if (g_strcmp0(str, LOOP_STATUS_NONE) == 0) {
            // change between none <-> playlist -> bug: no event intercepted
            // don't understand the cause but an easy fix may be to force update from here
            // maybe its because we set repeat and single at once
            mpdctl_set_repeat(false);
            mpdctl_set_single(false);
        } else if (g_strcmp0(str, LOOP_STATUS_PLAYLIST) == 0) {
            mpdctl_set_repeat(true);
            mpdctl_set_single(false);
        } else if (g_strcmp0(str, LOOP_STATUS_TRACK) == 0) {
            mpdctl_set_repeat(true);
            mpdctl_set_single(true);
        }
    }
    return error;
}

static gboolean handle_set_property(GDBusConnection *connection,
                                    const gchar *sender,
                                    const gchar *object_path,
                                    const gchar *interface_name,
                                    const gchar *property_name,
                                    GVariant *value,
                                    GError **error,
                                    gpointer user_data) {
    // there is no writeable properties in INTERFACE_MPRIS, so we ignore it
    if (g_strcmp0(interface_name, INTERFACE_PLAYER) == 0) {
        handle_player_interface_set(property_name, value, error);
    }

    return *error == NULL;
}

static const GDBusInterfaceVTable interface_vtable =
    {
        handle_method_call,
        handle_get_property,
        handle_set_property};

static gboolean on_timeout_cb(gpointer user_data) {
    if (g_strcmp0(mpdctl_get_player()->playback_status, PLAYBACK_STATUS_PLAYING) == 0) {
        mpdctl_get_player()->position += TIMEOUT * 1000000 * mpdctl_get_player()->rate;
    }
}

static void on_bus_acquired(GDBusConnection *connection, const gchar *name, gpointer user_data) {}

static void on_name_acquired(GDBusConnection *connection,
                             const gchar *name,
                             gpointer user_data) {
    g_print("D-Bus \"%s\" name acquired\n", name);

    guint id = g_dbus_connection_register_object(connection,
                                                 OBJECT_PATH,
                                                 introspection_data->interfaces[0],
                                                 &interface_vtable,
                                                 NULL,  /* user_data */
                                                 NULL,  /* user_data_free_func */
                                                 NULL); /* GError** */
    g_assert(id > 0);

    id = g_dbus_connection_register_object(connection,
                                           OBJECT_PATH,
                                           introspection_data->interfaces[1],
                                           &interface_vtable,
                                           NULL,  /* user_data */
                                           NULL,  /* user_data_free_func */
                                           NULL); /* GError** */
    g_assert(id > 0);

    conn = connection;

    mpdctl_start(host, port);

    g_timeout_add_seconds(TIMEOUT, on_timeout_cb, NULL);
}

static void on_name_lost(GDBusConnection *connection,
                         const gchar *name,
                         gpointer user_data) {
    fprintf(stderr, "DBus name lost\n");
    exit(EXIT_FAILURE);
}

static gboolean quit(gpointer user_data) {
    GMainLoop *loop = (GMainLoop *) user_data;
    g_main_loop_quit(loop);
    return FALSE;
}

static void start_monitoring() {
    GMainLoop *loop = g_main_loop_new(NULL, FALSE);
    assert(loop);

    g_unix_signal_add(SIGINT, quit, loop);
    // g_unix_signal_add(SIGHUP, quit, loop); // should reload config file but there is none yet
    g_unix_signal_add(SIGTERM, quit, loop);

    introspection_data = g_dbus_node_info_new_for_xml(introspection_xml, NULL);
    g_assert(introspection_data != NULL);

    guint id = g_bus_own_name(G_BUS_TYPE_SESSION,
                              DBUS_NAME,
                              G_BUS_NAME_OWNER_FLAGS_NONE,
                              on_bus_acquired,
                              on_name_acquired,
                              on_name_lost,
                              NULL,
                              NULL);

    g_main_loop_run(loop);

    // free and close everything here
    g_print("exiting...\n");
    g_bus_unown_name(id); // maybe useless
    g_dbus_node_info_unref(introspection_data);
    g_main_loop_unref(loop);
    mpdctl_close();
}

static void usage() {
    g_print("Usage: mpdbus [OPTIONS]\n\
  -d, --daemon\t\tLaunch as a daemon.\n\
  -h, --help\t\tShow this message.\n\
  -H, --host\t\tmpd server host (default: localhost).\n\
  -P, --port\t\tmpd server port (default: 6600).\n");
}

int main(int argc, char **argv) {
    int opt, opt_index;
    struct option long_options[] = {
        {"daemon", no_argument, NULL, 'd'},
        {"help", no_argument, NULL, 'h'},
        {"host", required_argument, NULL, 'H'},
        {"port", required_argument, NULL, 'P'},
        {NULL, 0, NULL, 0}};

    int daemonize = 0;
    while ((opt = getopt_long(argc, argv, "hdP:H:", long_options, &opt_index)) != -1) {
        switch (opt) {
        case 0:
            break;
        case 'd':
            daemonize = 1;
            break;
        case 'H':
            host = optarg;
            break;
        case 'P':
            port = atoi(optarg);
            break;
        case 'h':
            usage();
            return EXIT_SUCCESS;
        default:
            usage();
            return EXIT_FAILURE;
        }
    }

    if (daemonize) {
        if (daemon(0, 0) == -1) {
            fprintf(stderr, "failed to daemonize\n");
            return EXIT_FAILURE;
        }
    }

    start_monitoring();

    return EXIT_SUCCESS;
}
