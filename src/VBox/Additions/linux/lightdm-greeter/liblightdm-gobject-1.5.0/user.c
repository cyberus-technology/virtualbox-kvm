/* -*- Mode: C; indent-tabs-mode:nil; tab-width:4 -*-
 *
 * Copyright (C) 2010 Robert Ancell.
 * Author: Robert Ancell <robert.ancell@canonical.com>
 * 
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

/*
 * Oracle LGPL Disclaimer: For the avoidance of doubt, except that if any license choice
 * other than GPL or LGPL is available it will apply instead, Oracle elects to use only
 * the Lesser General Public License version 2.1 (LGPLv2) at this time for any software where
 * a choice of LGPL license versions is made available with the language indicating
 * that LGPLv2 or any later version may be used, or where a choice of which version
 * of the LGPL is applied is otherwise unspecified.
 */

#include "config.h"

#include <errno.h>
#include <string.h>
#include <sys/utsname.h>
#include <pwd.h>
#include <gio/gio.h>

#include "lightdm/user.h"

enum
{
    LIST_PROP_0,
    LIST_PROP_NUM_USERS,
    LIST_PROP_USERS,
};

enum
{
    USER_PROP_0,
    USER_PROP_NAME,
    USER_PROP_REAL_NAME,
    USER_PROP_DISPLAY_NAME,
    USER_PROP_HOME_DIRECTORY,
    USER_PROP_IMAGE,
    USER_PROP_BACKGROUND,
    USER_PROP_LANGUAGE,
    USER_PROP_LAYOUT,
    USER_PROP_LAYOUTS,
    USER_PROP_SESSION,
    USER_PROP_LOGGED_IN,
    USER_PROP_HAS_MESSAGES
};

enum
{
    USER_ADDED,
    USER_CHANGED,
    USER_REMOVED,
    LAST_LIST_SIGNAL
};
static guint list_signals[LAST_LIST_SIGNAL] = { 0 };

enum
{
    CHANGED,
    LAST_USER_SIGNAL
};
static guint user_signals[LAST_USER_SIGNAL] = { 0 };

typedef struct
{
    /* Connection to AccountsService */
    GDBusProxy *accounts_service_proxy;
    GList *user_account_objects;

    /* Connection to DisplayManager */
    GDBusProxy *display_manager_proxy;

    /* File monitor for password file */
    GFileMonitor *passwd_monitor;
  
    /* TRUE if have scanned users */
    gboolean have_users;

    /* List of users */
    GList *users;

    /* List of sessions */
    GList *sessions;
} LightDMUserListPrivate;

typedef struct
{
    GDBusProxy *proxy;
    LightDMUser *user;
} UserAccountObject;

typedef struct
{
    LightDMUserList *user_list;

    gchar *name;
    gchar *real_name;
    gchar *home_directory;
    gchar *image;
    gchar *background;
    gboolean has_messages;

    GKeyFile *dmrc_file;
    gchar *language;
    gchar **layouts;
    gchar *session;
} LightDMUserPrivate;

typedef struct
{
    GObject parent_instance;
    gchar *path;
    gchar *username;
} Session;

typedef struct
{
    GObjectClass parent_class;
} SessionClass;

G_DEFINE_TYPE (LightDMUserList, lightdm_user_list, G_TYPE_OBJECT);
G_DEFINE_TYPE (LightDMUser, lightdm_user, G_TYPE_OBJECT);
#define SESSION(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), session_get_type (), Session))
GType session_get_type (void);
G_DEFINE_TYPE (Session, session, G_TYPE_OBJECT);

#define GET_LIST_PRIVATE(obj) G_TYPE_INSTANCE_GET_PRIVATE ((obj), LIGHTDM_TYPE_USER_LIST, LightDMUserListPrivate)
#define GET_USER_PRIVATE(obj) G_TYPE_INSTANCE_GET_PRIVATE ((obj), LIGHTDM_TYPE_USER, LightDMUserPrivate)

#define PASSWD_FILE      "/etc/passwd"
#define USER_CONFIG_FILE "/etc/lightdm/users.conf"

static LightDMUserList *singleton = NULL;

/**
 * lightdm_user_list_get_instance:
 *
 * Get the user list.
 *
 * Return value: (transfer none): the #LightDMUserList
 **/
LightDMUserList *
lightdm_user_list_get_instance (void)
{
    if (!singleton)
        singleton = g_object_new (LIGHTDM_TYPE_USER_LIST, NULL);
    return singleton;
}

static LightDMUser *
get_user_by_name (LightDMUserList *user_list, const gchar *username)
{
    LightDMUserListPrivate *priv = GET_LIST_PRIVATE (user_list);
    GList *link;
  
    for (link = priv->users; link; link = link->next)
    {
        LightDMUser *user = link->data;
        if (strcmp (lightdm_user_get_name (user), username) == 0)
            return user;
    }

    return NULL;
}
  
static gint
compare_user (gconstpointer a, gconstpointer b)
{
    LightDMUser *user_a = (LightDMUser *) a, *user_b = (LightDMUser *) b;
    return strcmp (lightdm_user_get_display_name (user_a), lightdm_user_get_display_name (user_b));
}

static gboolean
update_passwd_user (LightDMUser *user, const gchar *real_name, const gchar *home_directory, const gchar *image)
{
    LightDMUserPrivate *priv = GET_USER_PRIVATE (user);

    if (g_strcmp0 (lightdm_user_get_real_name (user), real_name) == 0 &&
        g_strcmp0 (lightdm_user_get_home_directory (user), home_directory) == 0 &&
        g_strcmp0 (lightdm_user_get_image (user), image) == 0)
        return FALSE;

    g_free (priv->real_name);
    priv->real_name = g_strdup (real_name);
    g_free (priv->home_directory);
    priv->home_directory = g_strdup (home_directory);
    g_free (priv->image);
    priv->image = g_strdup (image);

    return TRUE;
}

static void
user_changed_cb (LightDMUser *user, LightDMUserList *user_list)
{
    g_signal_emit (user_list, list_signals[USER_CHANGED], 0, user);
}

static void
load_passwd_file (LightDMUserList *user_list, gboolean emit_add_signal)
{
    LightDMUserListPrivate *priv = GET_LIST_PRIVATE (user_list);
    GKeyFile *config;
    gchar *value;
    gint minimum_uid;
    gchar **hidden_users, **hidden_shells;
    GList *users = NULL, *old_users, *new_users = NULL, *changed_users = NULL, *link;
    GError *error = NULL;

    g_debug ("Loading user config from %s", USER_CONFIG_FILE);

    config = g_key_file_new ();
    g_key_file_load_from_file (config, USER_CONFIG_FILE, G_KEY_FILE_NONE, &error);
    if (error && !g_error_matches (error, G_FILE_ERROR, G_FILE_ERROR_NOENT))
        g_warning ("Failed to load configuration from %s: %s", USER_CONFIG_FILE, error->message); // FIXME: Don't make warning on no file, just info
    g_clear_error (&error);

    if (g_key_file_has_key (config, "UserList", "minimum-uid", NULL))
        minimum_uid = g_key_file_get_integer (config, "UserList", "minimum-uid", NULL);
    else
        minimum_uid = 500;

    value = g_key_file_get_string (config, "UserList", "hidden-users", NULL);
    if (!value)
        value = g_strdup ("nobody nobody4 noaccess");
    hidden_users = g_strsplit (value, " ", -1);
    g_free (value);

    value = g_key_file_get_string (config, "UserList", "hidden-shells", NULL);
    if (!value)
        value = g_strdup ("/bin/false /usr/sbin/nologin");
    hidden_shells = g_strsplit (value, " ", -1);
    g_free (value);

    g_key_file_free (config);

    setpwent ();

    while (TRUE)
    {
        struct passwd *entry;
        LightDMUser *user;
        LightDMUserPrivate *user_priv;
        char **tokens;
        gchar *real_name, *image;
        int i;

        errno = 0;
        entry = getpwent ();
        if (!entry)
            break;

        /* Ignore system users */
        if (entry->pw_uid < minimum_uid)
            continue;

        /* Ignore users disabled by shell */
        if (entry->pw_shell)
        {
            for (i = 0; hidden_shells[i] && strcmp (entry->pw_shell, hidden_shells[i]) != 0; i++);
            if (hidden_shells[i])
                continue;
        }

        /* Ignore certain users */
        for (i = 0; hidden_users[i] && strcmp (entry->pw_name, hidden_users[i]) != 0; i++);
        if (hidden_users[i])
            continue;

        tokens = g_strsplit (entry->pw_gecos, ",", -1);
        if (tokens[0] != NULL && tokens[0][0] != '\0')
            real_name = g_strdup (tokens[0]);
        else
            real_name = g_strdup ("");
        g_strfreev (tokens);

        image = g_build_filename (entry->pw_dir, ".face", NULL);
        if (!g_file_test (image, G_FILE_TEST_EXISTS))
        {
            g_free (image);
            image = g_build_filename (entry->pw_dir, ".face.icon", NULL);
            if (!g_file_test (image, G_FILE_TEST_EXISTS))
            {
                g_free (image);
                image = NULL;
            }
        }

        user = g_object_new (LIGHTDM_TYPE_USER, NULL);
        user_priv = GET_USER_PRIVATE (user);
        user_priv->user_list = user_list;
        g_free (user_priv->name);
        user_priv->name = g_strdup (entry->pw_name);
        g_free (user_priv->real_name);
        user_priv->real_name = real_name;
        g_free (user_priv->home_directory);
        user_priv->home_directory = g_strdup (entry->pw_dir);
        g_free (user_priv->image);
        user_priv->image = image;

        /* Update existing users if have them */
        for (link = priv->users; link; link = link->next)
        {
            LightDMUser *info = link->data;
            if (strcmp (lightdm_user_get_name (info), lightdm_user_get_name (user)) == 0)
            {
                if (update_passwd_user (info, lightdm_user_get_real_name (user), lightdm_user_get_home_directory (user), lightdm_user_get_image (user)))
                    changed_users = g_list_insert_sorted (changed_users, info, compare_user);
                g_object_unref (user);
                user = info;
                break;
            }
        }
        if (!link)
        {
            /* Only notify once we have loaded the user list */
            if (priv->have_users)
                new_users = g_list_insert_sorted (new_users, user, compare_user);
        }
        users = g_list_insert_sorted (users, user, compare_user);
    }
    g_strfreev (hidden_users);
    g_strfreev (hidden_shells);

    if (errno != 0)
        g_warning ("Failed to read password database: %s", strerror (errno));

    endpwent ();

    /* Use new user list */
    old_users = priv->users;
    priv->users = users;
  
    /* Notify of changes */
    for (link = new_users; link; link = link->next)
    {
        LightDMUser *info = link->data;
        g_debug ("User %s added", lightdm_user_get_name (info));
        g_signal_connect (info, "changed", G_CALLBACK (user_changed_cb), user_list);
        if (emit_add_signal)
            g_signal_emit (user_list, list_signals[USER_ADDED], 0, info);
    }
    g_list_free (new_users);
    for (link = changed_users; link; link = link->next)
    {
        LightDMUser *info = link->data;
        g_debug ("User %s changed", lightdm_user_get_name (info));
        g_signal_emit (info, user_signals[CHANGED], 0);
    }
    g_list_free (changed_users);
    for (link = old_users; link; link = link->next)
    {
        GList *new_link;

        /* See if this user is in the current list */
        for (new_link = priv->users; new_link; new_link = new_link->next)
        {
            if (new_link->data == link->data)
                break;
        }

        if (!new_link)
        {
            LightDMUser *info = link->data;
            g_debug ("User %s removed", lightdm_user_get_name (info));
            g_signal_emit (user_list, list_signals[USER_REMOVED], 0, info);
            g_object_unref (info);
        }
    }
    g_list_free (old_users);
}

static void
passwd_changed_cb (GFileMonitor *monitor, GFile *file, GFile *other_file, GFileMonitorEvent event_type, LightDMUserList *user_list)
{
    if (event_type == G_FILE_MONITOR_EVENT_CHANGES_DONE_HINT)
    {
        g_debug ("%s changed, reloading user list", g_file_get_path (file));
        load_passwd_file (user_list, TRUE);
    }
}

static gboolean
update_user (UserAccountObject *object)
{
    LightDMUserPrivate *priv = GET_USER_PRIVATE (object->user);
    GVariant *result, *value;
    GVariantIter *iter;
    gchar *name;
    GError *error = NULL;

    result = g_dbus_connection_call_sync (g_dbus_proxy_get_connection (object->proxy),
                                          "org.freedesktop.Accounts",
                                          g_dbus_proxy_get_object_path (object->proxy),
                                          "org.freedesktop.DBus.Properties",
                                          "GetAll",
                                          g_variant_new ("(s)", "org.freedesktop.Accounts.User"),
                                          G_VARIANT_TYPE ("(a{sv})"),
                                          G_DBUS_CALL_FLAGS_NONE,
                                          -1,
                                          NULL,
                                          &error);
    if (error)
        g_warning ("Error updating user %s: %s", g_dbus_proxy_get_object_path (object->proxy), error->message);
    g_clear_error (&error);
    if (!result)
        return FALSE;

    g_variant_get (result, "(a{sv})", &iter);
    while (g_variant_iter_loop (iter, "{&sv}", &name, &value))
    {
        if (strcmp (name, "UserName") == 0 && g_variant_is_of_type (value, G_VARIANT_TYPE_STRING))
        {
            gchar *user_name;
            g_variant_get (value, "&s", &user_name);
            g_free (priv->name);
            priv->name = g_strdup (user_name);
        }
        else if (strcmp (name, "RealName") == 0 && g_variant_is_of_type (value, G_VARIANT_TYPE_STRING))
        {
            gchar *real_name;
            g_variant_get (value, "&s", &real_name);
            g_free (priv->real_name);
            priv->real_name = g_strdup (real_name);
        }
        else if (strcmp (name, "HomeDirectory") == 0 && g_variant_is_of_type (value, G_VARIANT_TYPE_STRING))
        {
            gchar *home_directory;
            g_variant_get (value, "&s", &home_directory);
            g_free (priv->home_directory);
            priv->home_directory = g_strdup (home_directory);
        }
        else if (strcmp (name, "IconFile") == 0 && g_variant_is_of_type (value, G_VARIANT_TYPE_STRING))
        {
            gchar *icon_file;
            g_variant_get (value, "&s", &icon_file);
            g_free (priv->image);
            if (strcmp (icon_file, "") == 0)
                priv->image = NULL;
            else
                priv->image = g_strdup (icon_file);
        }
        else if (strcmp (name, "BackgroundFile") == 0 && g_variant_is_of_type (value, G_VARIANT_TYPE_STRING))
        {
            gchar *background_file;
            g_variant_get (value, "&s", &background_file);
            g_free (priv->background);
            if (strcmp (background_file, "") == 0)
                priv->background = NULL;
            else
                priv->background = g_strdup (background_file);
        }
    }
    g_variant_iter_free (iter);

    g_variant_unref (result);

    return TRUE;
}

static void
user_signal_cb (GDBusProxy *proxy, gchar *sender_name, gchar *signal_name, GVariant *parameters, UserAccountObject *object)
{
    if (strcmp (signal_name, "Changed") == 0)
    {
        if (g_variant_is_of_type (parameters, G_VARIANT_TYPE ("()")))
        {
            g_debug ("User %s changed", g_dbus_proxy_get_object_path (object->proxy));
            update_user (object);
            g_signal_emit (object->user, user_signals[CHANGED], 0);
        }
        else
            g_warning ("Got org.freedesktop.Accounts.User signal Changed with unknown parameters %s", g_variant_get_type_string (parameters));
    }
}

static UserAccountObject *
user_account_object_new (LightDMUserList *user_list, const gchar *path)
{
    GDBusProxy *proxy;
    UserAccountObject *object;
    GError *error = NULL;

    proxy = g_dbus_proxy_new_for_bus_sync (G_BUS_TYPE_SYSTEM,
                                           G_DBUS_PROXY_FLAGS_NONE,
                                           NULL,
                                           "org.freedesktop.Accounts",
                                           path,
                                           "org.freedesktop.Accounts.User",
                                           NULL,
                                           &error);
    if (error)
        g_warning ("Error getting user %s: %s", path, error->message);
    g_clear_error (&error);
    if (!proxy)
        return NULL;

    object = g_malloc0 (sizeof (UserAccountObject));  
    object->user = g_object_new (LIGHTDM_TYPE_USER, NULL);
    GET_USER_PRIVATE (object->user)->user_list = user_list;
    object->proxy = proxy;
    g_signal_connect (proxy, "g-signal", G_CALLBACK (user_signal_cb), object);
  
    return object;
}

static void
user_account_object_free (UserAccountObject *object)
{
    if (!object)
        return;
    g_object_unref (object->user);
    g_object_unref (object->proxy);
    g_free (object);
}

static UserAccountObject *
find_user_account_object (LightDMUserList *user_list, const gchar *path)
{
    LightDMUserListPrivate *priv = GET_LIST_PRIVATE (user_list);
    GList *link;

    for (link = priv->user_account_objects; link; link = link->next)
    {
        UserAccountObject *object = link->data;
        if (strcmp (g_dbus_proxy_get_object_path (object->proxy), path) == 0)
            return object;
    }

    return NULL;
}

static void
user_accounts_signal_cb (GDBusProxy *proxy, gchar *sender_name, gchar *signal_name, GVariant *parameters, LightDMUserList *user_list)
{
    LightDMUserListPrivate *priv = GET_LIST_PRIVATE (user_list);
  
    if (strcmp (signal_name, "UserAdded") == 0)
    {
        if (g_variant_is_of_type (parameters, G_VARIANT_TYPE ("(o)")))
        {
            gchar *path;
            UserAccountObject *object;

            g_variant_get (parameters, "(&o)", &path);

            /* Ignore duplicate requests */
            object = find_user_account_object (user_list, path);
            if (object)
                return;

            object = user_account_object_new (user_list, path);
            if (object && update_user (object))
            {
                g_debug ("User %s added", path);
                priv->user_account_objects = g_list_append (priv->user_account_objects, object);
                priv->users = g_list_insert_sorted (priv->users, g_object_ref (object->user), compare_user);
                g_signal_connect (object->user, "changed", G_CALLBACK (user_changed_cb), user_list);
                g_signal_emit (user_list, list_signals[USER_ADDED], 0, object->user);
            }
            else
                user_account_object_free (object);
        }
        else
            g_warning ("Got UserAccounts signal UserAdded with unknown parameters %s", g_variant_get_type_string (parameters));
    }
    else if (strcmp (signal_name, "UserDeleted") == 0)
    {
        if (g_variant_is_of_type (parameters, G_VARIANT_TYPE ("(o)")))
        {
            gchar *path;
            UserAccountObject *object;

            g_variant_get (parameters, "(&o)", &path);

            object = find_user_account_object (user_list, path);
            if (!object)
                return;

            g_debug ("User %s deleted", path);
            priv->users = g_list_remove (priv->users, object->user);
            g_object_unref (object->user);

            g_signal_emit (user_list, list_signals[USER_REMOVED], 0, object->user);

            priv->user_account_objects = g_list_remove (priv->user_account_objects, object);
            user_account_object_free (object);
        }
        else
            g_warning ("Got UserAccounts signal UserDeleted with unknown parameters %s", g_variant_get_type_string (parameters));
    }
}

static Session *
load_session (LightDMUserList *user_list, const gchar *path)
{
    LightDMUserListPrivate *priv = GET_LIST_PRIVATE (user_list);
    Session *session = NULL;
    GVariant *result, *username;
    GError *error = NULL;

    result = g_dbus_connection_call_sync (g_dbus_proxy_get_connection (priv->display_manager_proxy),
                                          "org.freedesktop.DisplayManager",
                                          path,
                                          "org.freedesktop.DBus.Properties",
                                          "Get",
                                          g_variant_new ("(ss)", "org.freedesktop.DisplayManager.Session", "UserName"),
                                          G_VARIANT_TYPE ("(v)"),
                                          G_DBUS_CALL_FLAGS_NONE,
                                          -1,
                                          NULL,
                                          &error);
    if (error)
        g_warning ("Error getting UserName from org.freedesktop.DisplayManager.Session: %s", error->message);
    g_clear_error (&error);
    if (!result)
        return NULL;

    g_variant_get (result, "(v)", &username);
    if (g_variant_is_of_type (username, G_VARIANT_TYPE_STRING))
    {
        gchar *name;

        g_variant_get (username, "&s", &name);

        g_debug ("Loaded session %s (%s)", path, name);
        session = g_object_new (session_get_type (), NULL);
        session->username = g_strdup (name);
        session->path = g_strdup (path);
        priv->sessions = g_list_append (priv->sessions, session);
    }
    g_variant_unref (username);
    g_variant_unref (result);

    return session;
}

static void
display_manager_signal_cb (GDBusProxy *proxy, gchar *sender_name, gchar *signal_name, GVariant *parameters, LightDMUserList *user_list)
{
    LightDMUserListPrivate *priv = GET_LIST_PRIVATE (user_list);

    if (strcmp (signal_name, "SessionAdded") == 0)
    {
        if (g_variant_is_of_type (parameters, G_VARIANT_TYPE ("(o)")))
        {
            gchar *path;
            Session *session;
            LightDMUser *user = NULL;

            g_variant_get (parameters, "(&o)", &path);
            session = load_session (user_list, path);
            if (session)
                user = get_user_by_name (user_list, session->username);
            if (user)
                g_signal_emit (user, user_signals[CHANGED], 0);
        }
    }
    else if (strcmp (signal_name, "SessionRemoved") == 0)
    {
        if (g_variant_is_of_type (parameters, G_VARIANT_TYPE ("(o)")))
        {
            gchar *path;
            GList *link;

            g_variant_get (parameters, "(&o)", &path);

            for (link = priv->sessions; link; link = link->next)
            {
                Session *session = link->data;
                if (strcmp (session->path, path) == 0)
                {
                    LightDMUser *user;

                    g_debug ("Session %s removed", path);
                    priv->sessions = g_list_remove_link (priv->sessions, link);
                    user = get_user_by_name (user_list, session->username);
                    if (user)
                        g_signal_emit (user, user_signals[CHANGED], 0);
                    g_object_unref (session);
                    break;
                }
            }
        }
    }
}

static void
update_users (LightDMUserList *user_list)
{
    LightDMUserListPrivate *priv = GET_LIST_PRIVATE (user_list);
    GError *error = NULL;

    if (priv->have_users)
        return;
    priv->have_users = TRUE;

    priv->accounts_service_proxy = g_dbus_proxy_new_for_bus_sync (G_BUS_TYPE_SYSTEM,
                                                                  G_DBUS_PROXY_FLAGS_NONE,
                                                                  NULL,
                                                                  "org.freedesktop.Accounts",
                                                                  "/org/freedesktop/Accounts",
                                                                  "org.freedesktop.Accounts",
                                                                  NULL,
                                                                  &error);
    if (error)
        g_warning ("Error contacting org.freedesktop.Accounts: %s", error->message);
    g_clear_error (&error);

    /* Check if the service exists */
    if (priv->accounts_service_proxy)
    {
        gchar *name;

        name = g_dbus_proxy_get_name_owner (priv->accounts_service_proxy);
        if (!name)
        {
            g_debug ("org.freedesktop.Accounts does not exist, falling back to passwd file");
            g_object_unref (priv->accounts_service_proxy);
            priv->accounts_service_proxy = NULL;
        }
        g_free (name);
    }

    if (priv->accounts_service_proxy)
    {
        GVariant *result;

        g_signal_connect (priv->accounts_service_proxy, "g-signal", G_CALLBACK (user_accounts_signal_cb), user_list);

        result = g_dbus_proxy_call_sync (priv->accounts_service_proxy,
                                         "ListCachedUsers",
                                         g_variant_new ("()"),
                                         G_DBUS_CALL_FLAGS_NONE,
                                         -1,
                                         NULL,
                                         &error);
        if (error)
            g_warning ("Error getting user list from org.freedesktop.Accounts: %s", error->message);
        g_clear_error (&error);
        if (!result)
            return;

        if (g_variant_is_of_type (result, G_VARIANT_TYPE ("(ao)")))
        {
            GVariantIter *iter;
            const gchar *path;

            g_debug ("Loading users from org.freedesktop.Accounts");
            g_variant_get (result, "(ao)", &iter);
            while (g_variant_iter_loop (iter, "&o", &path))
            {
                UserAccountObject *object;

                g_debug ("Loading user %s", path);

                object = user_account_object_new (user_list, path);
                if (object && update_user (object))
                {
                    priv->user_account_objects = g_list_append (priv->user_account_objects, object);
                    priv->users = g_list_insert_sorted (priv->users, g_object_ref (object->user), compare_user);
                    g_signal_connect (object->user, "changed", G_CALLBACK (user_changed_cb), user_list);
                }
                else
                    user_account_object_free (object);
            }
            g_variant_iter_free (iter);
        }
        else
            g_warning ("Unexpected type from ListCachedUsers: %s", g_variant_get_type_string (result));

        g_variant_unref (result);
    }
    else
    {
        GFile *passwd_file;

        load_passwd_file (user_list, FALSE);

        /* Watch for changes to user list */

        passwd_file = g_file_new_for_path (PASSWD_FILE);
        priv->passwd_monitor = g_file_monitor (passwd_file, G_FILE_MONITOR_NONE, NULL, &error);
        g_object_unref (passwd_file);
        if (error)
            g_warning ("Error monitoring %s: %s", PASSWD_FILE, error->message);
        else
            g_signal_connect (priv->passwd_monitor, "changed", G_CALLBACK (passwd_changed_cb), user_list);
        g_clear_error (&error);
    }

    priv->display_manager_proxy = g_dbus_proxy_new_for_bus_sync (G_BUS_TYPE_SYSTEM,
                                                                 G_DBUS_PROXY_FLAGS_NONE,
                                                                 NULL,
                                                                 "org.freedesktop.DisplayManager",
                                                                 "/org/freedesktop/DisplayManager",
                                                                 "org.freedesktop.DisplayManager",
                                                                 NULL,
                                                                 &error);
    if (error)
        g_warning ("Error contacting org.freedesktop.DisplayManager: %s", error->message);
    g_clear_error (&error);

    if (priv->display_manager_proxy)
    {
        GVariant *result;

        g_signal_connect (priv->display_manager_proxy, "g-signal", G_CALLBACK (display_manager_signal_cb), user_list);

        result = g_dbus_connection_call_sync (g_dbus_proxy_get_connection (priv->display_manager_proxy),
                                              "org.freedesktop.DisplayManager",
                                              "/org/freedesktop/DisplayManager",
                                              "org.freedesktop.DBus.Properties",
                                              "Get",
                                              g_variant_new ("(ss)", "org.freedesktop.DisplayManager", "Sessions"),
                                              G_VARIANT_TYPE ("(v)"),
                                              G_DBUS_CALL_FLAGS_NONE,
                                              -1,
                                              NULL,
                                              &error);
        if (error)
            g_warning ("Error getting session list from org.freedesktop.DisplayManager: %s", error->message);
        g_clear_error (&error);
        if (!result)
            return;

        if (g_variant_is_of_type (result, G_VARIANT_TYPE ("(v)")))
        {
            GVariant *value;
            GVariantIter *iter;
            const gchar *path;

            g_variant_get (result, "(v)", &value);

            g_debug ("Loading sessions from org.freedesktop.DisplayManager");
            g_variant_get (value, "ao", &iter);
            while (g_variant_iter_loop (iter, "&o", &path))
                load_session (user_list, path);
            g_variant_iter_free (iter);

            g_variant_unref (value);
        }
        else
            g_warning ("Unexpected type from org.freedesktop.DisplayManager.Sessions: %s", g_variant_get_type_string (result));

        g_variant_unref (result);
    }
}

/**
 * lightdm_user_list_get_length:
 * @user_list: a #LightDMUserList
 *
 * Return value: The number of users able to log in
 **/
gint
lightdm_user_list_get_length (LightDMUserList *user_list)
{
    g_return_val_if_fail (LIGHTDM_IS_USER_LIST (user_list), 0);
    update_users (user_list);
    return g_list_length (GET_LIST_PRIVATE (user_list)->users);
}

/**
 * lightdm_user_list_get_users:
 * @user_list: A #LightDMUserList
 *
 * Get a list of users to present to the user.  This list may be a subset of the
 * available users and may be empty depending on the server configuration.
 *
 * Return value: (element-type LightDMUser) (transfer none): A list of #LightDMUser that should be presented to the user.
 **/
GList *
lightdm_user_list_get_users (LightDMUserList *user_list)
{
    g_return_val_if_fail (LIGHTDM_IS_USER_LIST (user_list), NULL);
    update_users (user_list);
    return GET_LIST_PRIVATE (user_list)->users;
}

/**
 * lightdm_user_list_get_user_by_name:
 * @user_list: A #LightDMUserList
 * @username: Name of user to get.
 *
 * Get infomation about a given user or #NULL if this user doesn't exist.
 *
 * Return value: (transfer none): A #LightDMUser entry for the given user.
 **/
LightDMUser *
lightdm_user_list_get_user_by_name (LightDMUserList *user_list, const gchar *username)
{
    g_return_val_if_fail (LIGHTDM_IS_USER_LIST (user_list), NULL);
    g_return_val_if_fail (username != NULL, NULL);

    update_users (user_list);

    return get_user_by_name (user_list, username);
}

static void
lightdm_user_list_init (LightDMUserList *user_list)
{
}

static void
lightdm_user_list_set_property (GObject    *object,
                                guint       prop_id,
                                const GValue *value,
                                GParamSpec *pspec)
{
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
}

static void
lightdm_user_list_get_property (GObject    *object,
                                guint       prop_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
    LightDMUserList *self;

    self = LIGHTDM_USER_LIST (object);

    switch (prop_id)
    {
    case LIST_PROP_NUM_USERS:
        g_value_set_int (value, lightdm_user_list_get_length (self));
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
lightdm_user_list_finalize (GObject *object)
{
    LightDMUserList *self = LIGHTDM_USER_LIST (object);
    LightDMUserListPrivate *priv = GET_LIST_PRIVATE (self);

    if (priv->accounts_service_proxy)
        g_object_unref (priv->accounts_service_proxy);
    g_list_free_full (priv->user_account_objects, (GDestroyNotify) user_account_object_free);
    if (priv->passwd_monitor)
        g_object_unref (priv->passwd_monitor);
    g_list_free_full (priv->users, g_object_unref);
    g_list_free_full (priv->sessions, g_object_unref);

    G_OBJECT_CLASS (lightdm_user_list_parent_class)->finalize (object);
}

static void
lightdm_user_list_class_init (LightDMUserListClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    g_type_class_add_private (klass, sizeof (LightDMUserListPrivate));

    object_class->set_property = lightdm_user_list_set_property;
    object_class->get_property = lightdm_user_list_get_property;
    object_class->finalize = lightdm_user_list_finalize;

    g_object_class_install_property (object_class,
                                     LIST_PROP_NUM_USERS,
                                     g_param_spec_int ("num-users",
                                                       "num-users",
                                                       "Number of login users",
                                                       0, G_MAXINT, 0,
                                                       G_PARAM_READABLE));
    /**
     * LightDMUserList::user-added:
     * @user_list: A #LightDMUserList
     * @user: The #LightDM user that has been added.
     *
     * The ::user-added signal gets emitted when a user account is created.
     **/
    list_signals[USER_ADDED] =
        g_signal_new ("user-added",
                      G_TYPE_FROM_CLASS (klass),
                      G_SIGNAL_RUN_LAST,
                      G_STRUCT_OFFSET (LightDMUserListClass, user_added),
                      NULL, NULL,
                      g_cclosure_marshal_VOID__OBJECT,
                      G_TYPE_NONE, 1, LIGHTDM_TYPE_USER);

    /**
     * LightDMUserList::user-changed:
     * @user_list: A #LightDMUserList
     * @user: The #LightDM user that has been changed.
     *
     * The ::user-changed signal gets emitted when a user account is modified.
     **/
    list_signals[USER_CHANGED] =
        g_signal_new ("user-changed",
                      G_TYPE_FROM_CLASS (klass),
                      G_SIGNAL_RUN_LAST,
                      G_STRUCT_OFFSET (LightDMUserListClass, user_changed),
                      NULL, NULL,
                      g_cclosure_marshal_VOID__OBJECT,
                      G_TYPE_NONE, 1, LIGHTDM_TYPE_USER);

    /**
     * LightDMUserList::user-removed:
     * @user_list: A #LightDMUserList
     * @user: The #LightDM user that has been removed.
     *
     * The ::user-removed signal gets emitted when a user account is removed.
     **/
    list_signals[USER_REMOVED] =
        g_signal_new ("user-removed",
                      G_TYPE_FROM_CLASS (klass),
                      G_SIGNAL_RUN_LAST,
                      G_STRUCT_OFFSET (LightDMUserListClass, user_removed),
                      NULL, NULL,
                      g_cclosure_marshal_VOID__OBJECT,
                      G_TYPE_NONE, 1, LIGHTDM_TYPE_USER);
}

/**
 * lightdm_user_get_name:
 * @user: A #LightDMUser
 * 
 * Get the name of a user.
 * 
 * Return value: The name of the given user
 **/
const gchar *
lightdm_user_get_name (LightDMUser *user)
{
    g_return_val_if_fail (LIGHTDM_IS_USER (user), NULL);
    return GET_USER_PRIVATE (user)->name;
}

/**
 * lightdm_user_get_real_name:
 * @user: A #LightDMUser
 * 
 * Get the real name of a user.
 *
 * Return value: The real name of the given user
 **/
const gchar *
lightdm_user_get_real_name (LightDMUser *user)
{
    g_return_val_if_fail (LIGHTDM_IS_USER (user), NULL);
    return GET_USER_PRIVATE (user)->real_name;
}

/**
 * lightdm_user_get_display_name:
 * @user: A #LightDMUser
 * 
 * Get the display name of a user.
 * 
 * Return value: The display name of the given user
 **/
const gchar *
lightdm_user_get_display_name (LightDMUser *user)
{
    LightDMUserPrivate *priv;

    g_return_val_if_fail (LIGHTDM_IS_USER (user), NULL);

    priv = GET_USER_PRIVATE (user);
    if (strcmp (priv->real_name, ""))
        return priv->real_name;
    else
        return priv->name;
}

/**
 * lightdm_user_get_home_directory:
 * @user: A #LightDMUser
 * 
 * Get the home directory for a user.
 * 
 * Return value: The users home directory
 */
const gchar *
lightdm_user_get_home_directory (LightDMUser *user)
{
    g_return_val_if_fail (LIGHTDM_IS_USER (user), NULL);
    return GET_USER_PRIVATE (user)->home_directory;
}

/**
 * lightdm_user_get_image:
 * @user: A #LightDMUser
 * 
 * Get the image URI for a user.
 * 
 * Return value: The image URI for the given user or #NULL if no URI
 **/
const gchar *
lightdm_user_get_image (LightDMUser *user)
{
    g_return_val_if_fail (LIGHTDM_IS_USER (user), NULL);
    return GET_USER_PRIVATE (user)->image;
}

/**
 * lightdm_user_get_background:
 * @user: A #LightDMUser
 * 
 * Get the background file path for a user.
 * 
 * Return value: The background file path for the given user or #NULL if no path
 **/
const gchar *
lightdm_user_get_background (LightDMUser *user)
{
    g_return_val_if_fail (LIGHTDM_IS_USER (user), NULL);
    return GET_USER_PRIVATE (user)->background;
}

static void
load_dmrc (LightDMUser *user)
{
    LightDMUserPrivate *priv = GET_USER_PRIVATE (user);
    gchar *path;
    //gboolean have_dmrc;

    if (!priv->dmrc_file)
        priv->dmrc_file = g_key_file_new ();

    /* Load from the user directory */  
    path = g_build_filename (priv->home_directory, ".dmrc", NULL);
    /*have_dmrc = */g_key_file_load_from_file (priv->dmrc_file, path, G_KEY_FILE_KEEP_COMMENTS, NULL);
    g_free (path);

    /* If no ~/.dmrc, then load from the cache */
    // FIXME

    // FIXME: Watch for changes

    /* The Language field is actually a locale, strip the codeset off it to get the language */
    if (priv->language)
        g_free (priv->language);
    priv->language = g_key_file_get_string (priv->dmrc_file, "Desktop", "Language", NULL);
    if (priv->language)
    {
        gchar *codeset = strchr (priv->language, '.');
        if (codeset)
            *codeset = '\0';
    }

    if (priv->layouts)
    {
        g_strfreev (priv->layouts);
        priv->layouts = NULL;
    }
    if (g_key_file_has_key (priv->dmrc_file, "Desktop", "Layout", NULL))
    {
        priv->layouts = g_malloc (sizeof (gchar *) * 2);
        priv->layouts[0] = g_key_file_get_string (priv->dmrc_file, "Desktop", "Layout", NULL);
        priv->layouts[1] = NULL;
    }

    if (priv->session)
        g_free (priv->session);
    priv->session = g_key_file_get_string (priv->dmrc_file, "Desktop", "Session", NULL);
}

static GVariant *
get_property (GDBusProxy *proxy, const gchar *property)
{
    GVariant *answer;

    if (!proxy)
        return NULL;

    answer = g_dbus_proxy_get_cached_property (proxy, property);

    if (!answer)
    {
        g_warning ("Could not get accounts property %s", property);
        return NULL;
    }

    return answer;
}

static gboolean
get_boolean_property (GDBusProxy *proxy, const gchar *property)
{
    GVariant *answer;
    gboolean rv;

    answer = get_property (proxy, property);
    if (!g_variant_is_of_type (answer, G_VARIANT_TYPE_BOOLEAN))
    {
        g_warning ("Unexpected accounts property type for %s: %s",
                   property, g_variant_get_type_string (answer));
        g_variant_unref (answer);
        return FALSE;
    }

    rv = g_variant_get_boolean (answer);
    g_variant_unref (answer);

    return rv;
}

static gchar *
get_string_property (GDBusProxy *proxy, const gchar *property)
{
    GVariant *answer;
    gchar *rv;
  
    answer = get_property (proxy, property);
    if (!g_variant_is_of_type (answer, G_VARIANT_TYPE_STRING))
    {
        g_warning ("Unexpected accounts property type for %s: %s",
                   property, g_variant_get_type_string (answer));
        g_variant_unref (answer);
        return NULL;
    }

    rv = g_strdup (g_variant_get_string (answer, NULL));
    if (strcmp (rv, "") == 0)
    {
        g_free (rv);
        rv = NULL;
    }
    g_variant_unref (answer);

    return rv;
}

static gchar **
get_string_array_property (GDBusProxy *proxy, const gchar *property)
{
    GVariant *answer;
    gchar **rv;

    if (!proxy)
        return NULL;

    answer = g_dbus_proxy_get_cached_property (proxy, property);

    if (!answer)
    {
        g_warning ("Could not get accounts property %s", property);
        return NULL;
    }

    if (!g_variant_is_of_type (answer, G_VARIANT_TYPE ("as")))
    {
        g_warning ("Unexpected accounts property type for %s: %s",
                   property, g_variant_get_type_string (answer));
        g_variant_unref (answer);
        return NULL;
    }

    rv = g_variant_dup_strv (answer, NULL);

    g_variant_unref (answer);
    return rv;
}

static gboolean
load_accounts_service (LightDMUser *user)
{
    LightDMUserPrivate *priv = GET_USER_PRIVATE (user);
    LightDMUserListPrivate *list_priv = GET_LIST_PRIVATE (priv->user_list);
    UserAccountObject *account = NULL;
    GList *iter;
    gchar **value;

    /* First, find AccountObject proxy */
    for (iter = list_priv->user_account_objects; iter; iter = iter->next)
    {
        UserAccountObject *a = iter->data;
        if (a->user == user)
        {
            account = a;
            break;
        }
    }
    if (!account)
        return FALSE;

    /* We have proxy, let's grab some properties */
    if (priv->language)
        g_free (priv->language);
    priv->language = get_string_property (account->proxy, "Language");
    if (priv->session)
        g_free (priv->session);
    priv->session = get_string_property (account->proxy, "XSession");

    value = get_string_array_property (account->proxy, "XKeyboardLayouts");
    if (value)
    {
        if (value[0])
        {
            g_strfreev (priv->layouts);
            priv->layouts = value;
        }
        else
            g_strfreev (value);
    }

    priv->has_messages = get_boolean_property (account->proxy, "XHasMessages");

    return TRUE;
}

/* Loads language/layout/session info for user */
static void
load_user_values (LightDMUser *user)
{
    LightDMUserPrivate *priv = GET_USER_PRIVATE (user);

    load_dmrc (user);
    load_accounts_service (user); // overrides dmrc values

    /* Ensure a few guarantees */
    if (priv->layouts == NULL)
    {
        priv->layouts = g_malloc (sizeof (gchar *) * 1);
        priv->layouts[0] = NULL;
    }
}

/**
 * lightdm_user_get_language:
 * @user: A #LightDMUser
 * 
 * Get the language for a user.
 * 
 * Return value: The language for the given user or #NULL if using system defaults.
 **/
const gchar *
lightdm_user_get_language (LightDMUser *user)
{
    g_return_val_if_fail (LIGHTDM_IS_USER (user), NULL);
    load_user_values (user);
    return GET_USER_PRIVATE (user)->language;
}

/**
 * lightdm_user_get_layout:
 * @user: A #LightDMUser
 * 
 * Get the keyboard layout for a user.
 * 
 * Return value: The keyboard layout for the given user or #NULL if using system defaults.  Copy the value if you want to use it long term.
 **/
const gchar *
lightdm_user_get_layout (LightDMUser *user)
{
    g_return_val_if_fail (LIGHTDM_IS_USER (user), NULL);
    load_user_values (user);
    return GET_USER_PRIVATE (user)->layouts[0];
}

/**
 * lightdm_user_get_layouts:
 * @user: A #LightDMUser
 * 
 * Get the configured keyboard layouts for a user.
 * 
 * Return value: (transfer none): A NULL-terminated array of keyboard layouts for the given user.  Copy the values if you want to use them long term.
 **/
const gchar * const *
lightdm_user_get_layouts (LightDMUser *user)
{
    g_return_val_if_fail (LIGHTDM_IS_USER (user), NULL);
    load_user_values (user);
    return (const gchar * const *) GET_USER_PRIVATE (user)->layouts;
}

/**
 * lightdm_user_get_session:
 * @user: A #LightDMUser
 * 
 * Get the session for a user.
 * 
 * Return value: The session for the given user or #NULL if using system defaults.
 **/
const gchar *
lightdm_user_get_session (LightDMUser *user)
{
    g_return_val_if_fail (LIGHTDM_IS_USER (user), NULL);
    load_user_values (user);
    return GET_USER_PRIVATE (user)->session; 
}

/**
 * lightdm_user_get_logged_in:
 * @user: A #LightDMUser
 * 
 * Check if a user is logged in.
 * 
 * Return value: #TRUE if the user is currently logged in.
 **/
gboolean
lightdm_user_get_logged_in (LightDMUser *user)
{
    LightDMUserPrivate *priv = GET_USER_PRIVATE (user);
    LightDMUserListPrivate *list_priv = GET_LIST_PRIVATE (priv->user_list);
    GList *link;

    g_return_val_if_fail (LIGHTDM_IS_USER (user), FALSE);

    for (link = list_priv->sessions; link; link = link->next)
    {
        Session *session = link->data;
        if (strcmp (session->username, priv->name) == 0)
            return TRUE;
    }

    return FALSE;
}

/**
 * lightdm_user_get_has_messages:
 * @user: A #LightDMUser
 * 
 * Check if a user has waiting messages.
 * 
 * Return value: #TRUE if the user has waiting messages.
 **/
gboolean
lightdm_user_get_has_messages (LightDMUser *user)
{
    g_return_val_if_fail (LIGHTDM_IS_USER (user), FALSE);
    load_user_values (user);
    return GET_USER_PRIVATE (user)->has_messages;
}

static void
lightdm_user_init (LightDMUser *user)
{
}

static void
lightdm_user_set_property (GObject    *object,
                           guint       prop_id,
                           const GValue *value,
                           GParamSpec *pspec)
{
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
}

static void
lightdm_user_get_property (GObject    *object,
                           guint       prop_id,
                           GValue     *value,
                           GParamSpec *pspec)
{
    LightDMUser *self;

    self = LIGHTDM_USER (object);

    switch (prop_id)
    {
    case USER_PROP_NAME:
        g_value_set_string (value, lightdm_user_get_name (self));
        break;
    case USER_PROP_REAL_NAME:
        g_value_set_string (value, lightdm_user_get_real_name (self));
        break;
    case USER_PROP_DISPLAY_NAME:
        g_value_set_string (value, lightdm_user_get_display_name (self));
        break;
    case USER_PROP_HOME_DIRECTORY:
        g_value_set_string (value, lightdm_user_get_home_directory (self));
        break;
    case USER_PROP_IMAGE:
        g_value_set_string (value, lightdm_user_get_image (self));
        break;
    case USER_PROP_BACKGROUND:
        g_value_set_string (value, lightdm_user_get_background (self));
        break;
    case USER_PROP_LANGUAGE:
        g_value_set_string (value, lightdm_user_get_language (self));
        break;
    case USER_PROP_LAYOUT:
        g_value_set_string (value, lightdm_user_get_layout (self));
        break;
    case USER_PROP_LAYOUTS:
        g_value_set_boxed (value, g_strdupv ((gchar **) lightdm_user_get_layouts (self)));
        break;
    case USER_PROP_SESSION:
        g_value_set_string (value, lightdm_user_get_session (self));
        break;
    case USER_PROP_LOGGED_IN:
        g_value_set_boolean (value, lightdm_user_get_logged_in (self));
        break;
    case USER_PROP_HAS_MESSAGES:
        g_value_set_boolean (value, lightdm_user_get_has_messages (self));
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
lightdm_user_finalize (GObject *object)
{
    LightDMUser *self = LIGHTDM_USER (object);
    LightDMUserPrivate *priv = GET_USER_PRIVATE (self);

    g_free (priv->name);
    g_free (priv->real_name);
    g_free (priv->home_directory);
    g_free (priv->image);
    g_free (priv->background);
    g_strfreev (priv->layouts);
    if (priv->dmrc_file)
        g_key_file_free (priv->dmrc_file);
}

static void
lightdm_user_class_init (LightDMUserClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);
  
    g_type_class_add_private (klass, sizeof (LightDMUserPrivate));

    object_class->set_property = lightdm_user_set_property;
    object_class->get_property = lightdm_user_get_property;
    object_class->finalize = lightdm_user_finalize;

    g_object_class_install_property (object_class,
                                     USER_PROP_NAME,
                                     g_param_spec_string ("name",
                                                          "name",
                                                          "Username",
                                                          NULL,
                                                          G_PARAM_READWRITE));
    g_object_class_install_property (object_class,
                                     USER_PROP_REAL_NAME,
                                     g_param_spec_string ("real-name",
                                                          "real-name",
                                                          "Users real name",
                                                          NULL,
                                                          G_PARAM_READWRITE));
    g_object_class_install_property (object_class,
                                     USER_PROP_DISPLAY_NAME,
                                     g_param_spec_string ("display-name",
                                                          "display-name",
                                                          "Users display name",
                                                          NULL,
                                                          G_PARAM_READABLE));
    g_object_class_install_property (object_class,
                                     USER_PROP_HOME_DIRECTORY,
                                     g_param_spec_string ("home-directory",
                                                          "home-directory",
                                                          "Home directory",
                                                          NULL,
                                                          G_PARAM_READWRITE));
    g_object_class_install_property (object_class,
                                     USER_PROP_IMAGE,
                                     g_param_spec_string ("image",
                                                          "image",
                                                          "Avatar image",
                                                          NULL,
                                                          G_PARAM_READWRITE));
    g_object_class_install_property (object_class,
                                     USER_PROP_BACKGROUND,
                                     g_param_spec_string ("background",
                                                          "background",
                                                          "User background",
                                                          NULL,
                                                          G_PARAM_READWRITE));
    g_object_class_install_property (object_class,
                                     USER_PROP_LANGUAGE,
                                     g_param_spec_string ("language",
                                                         "language",
                                                         "Language used by this user",
                                                         NULL,
                                                         G_PARAM_READABLE));
    g_object_class_install_property (object_class,
                                     USER_PROP_LAYOUT,
                                     g_param_spec_string ("layout",
                                                          "layout",
                                                          "Keyboard layout used by this user",
                                                          NULL,
                                                          G_PARAM_READABLE));
    g_object_class_install_property (object_class,
                                     USER_PROP_LAYOUTS,
                                     g_param_spec_boxed ("layouts",
                                                         "layouts",
                                                         "Keyboard layouts used by this user",
                                                         G_TYPE_STRV,
                                                         G_PARAM_READABLE));
    g_object_class_install_property (object_class,
                                     USER_PROP_SESSION,
                                     g_param_spec_string ("session",
                                                          "session",
                                                          "Session used by this user",
                                                          NULL,
                                                          G_PARAM_READABLE));
    g_object_class_install_property (object_class,
                                     USER_PROP_LOGGED_IN,
                                     g_param_spec_boolean ("logged-in",
                                                           "logged-in",
                                                           "TRUE if the user is currently in a session",
                                                           FALSE,
                                                           G_PARAM_READWRITE));
    g_object_class_install_property (object_class,
                                     USER_PROP_LOGGED_IN,
                                     g_param_spec_boolean ("has-messages",
                                                           "has-messages",
                                                           "TRUE if the user is has waiting messages",
                                                           FALSE,
                                                           G_PARAM_READWRITE));

    /**
     * LightDMUser::changed:
     * @user: A #LightDMUser
     *
     * The ::changed signal gets emitted this user account is modified.
     **/
    user_signals[CHANGED] =
        g_signal_new ("changed",
                      G_TYPE_FROM_CLASS (klass),
                      G_SIGNAL_RUN_LAST,
                      G_STRUCT_OFFSET (LightDMUserClass, changed),
                      NULL, NULL,
                      g_cclosure_marshal_VOID__VOID,
                      G_TYPE_NONE, 0);
}

static void
session_init (Session *session)
{
}

static void
session_finalize (GObject *object)
{
    Session *self = SESSION (object);

    g_free (self->path);
    g_free (self->username);
}

static void
session_class_init (SessionClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);
    object_class->finalize = session_finalize;
}
