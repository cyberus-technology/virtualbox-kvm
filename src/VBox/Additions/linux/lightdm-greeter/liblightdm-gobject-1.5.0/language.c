/*
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

#include <string.h>
#include <locale.h>
#include <langinfo.h>
#include <stdio.h>
#include <glib/gi18n.h>

#include "lightdm/language.h"

enum {
    PROP_0,
    PROP_CODE,
    PROP_NAME,
    PROP_TERRITORY
};

typedef struct
{
    gchar *code;
    gchar *name;
    gchar *territory;
} LightDMLanguagePrivate;

G_DEFINE_TYPE (LightDMLanguage, lightdm_language, G_TYPE_OBJECT);

#define GET_PRIVATE(obj) G_TYPE_INSTANCE_GET_PRIVATE ((obj), LIGHTDM_TYPE_LANGUAGE, LightDMLanguagePrivate)

static gboolean have_languages = FALSE;
static GList *languages = NULL;

static void
update_languages (void)
{
    gchar *command = "locale -a";
    gchar *stdout_text = NULL, *stderr_text = NULL;
    gint exit_status;
    gboolean result;
    GError *error = NULL;

    if (have_languages)
        return;

    result = g_spawn_command_line_sync (command, &stdout_text, &stderr_text, &exit_status, &error);
    if (error)
    {
        g_warning ("Failed to run '%s': %s", command, error->message);
        g_clear_error (&error);
    }
    else if (exit_status != 0)
        g_warning ("Failed to get languages, '%s' returned %d", command, exit_status);
    else if (result)
    {
        gchar **tokens;
        int i;

        tokens = g_strsplit_set (stdout_text, "\n\r", -1);
        for (i = 0; tokens[i]; i++)
        {
            LightDMLanguage *language;
            gchar *code;

            code = g_strchug (tokens[i]);
            if (code[0] == '\0')
                continue;

            /* Ignore the non-interesting languages */
            if (strcmp (command, "locale -a") == 0 && !g_strrstr (code, ".utf8"))
                continue;

            language = g_object_new (LIGHTDM_TYPE_LANGUAGE, "code", code, NULL);
            languages = g_list_append (languages, language);
        }

        g_strfreev (tokens);
    }

    g_free (stdout_text);
    g_free (stderr_text);

    have_languages = TRUE;
}

static gboolean
is_utf8 (const gchar *code)
{
    return g_strrstr (code, ".utf8") || g_strrstr (code, ".UTF-8");
}

/* Get a valid locale name that can be passed to setlocale(), so we always can use nl_langinfo() to get language and country names. */
static gchar *
get_locale_name (const gchar *code)
{
    gchar *locale = NULL, *language;
    char *at;
    static gchar **avail_locales;
    gint i;

    if (is_utf8 (code))
        return (gchar *) code;

    if ((at = strchr (code, '@')))
        language = g_strndup (code, at - code);
    else
        language = g_strdup (code);

    if (!avail_locales)
    {
        gchar *locales;
        GError *error = NULL;

        if (g_spawn_command_line_sync ("locale -a", &locales, NULL, NULL, &error))
        {
            avail_locales = g_strsplit (g_strchomp (locales), "\n", -1);
            g_free (locales);
        }
        else
        {
            g_warning ("Failed to run 'locale -a': %s", error->message);
            g_clear_error (&error);
        }
    }

    if (avail_locales)
    {
        for (i = 0; avail_locales[i]; i++)
        {
            gchar *loc = avail_locales[i];
            if (!g_strrstr (loc, ".utf8"))
                continue;
            if (g_str_has_prefix (loc, language))
            {
                locale = g_strdup (loc);
                break;
            }
        }
    }

    g_free (language);

    return locale;
}

/**
 * lightdm_get_language:
 *
 * Get the current language.
 *
 * Return value: (transfer none): The current language or #NULL if no language.
 **/
LightDMLanguage *
lightdm_get_language (void)
{
    const gchar *lang;
    GList *link;

    lang = g_getenv ("LANG");
    if (!lang)
        return NULL;

    for (link = lightdm_get_languages (); link; link = link->next)
    {
        LightDMLanguage *language = link->data;
        if (lightdm_language_matches (language, lang))
            return language;
    }

    return NULL;
}

/**
 * lightdm_get_languages:
 *
 * Get a list of languages to present to the user.
 *
 * Return value: (element-type LightDMLanguage) (transfer none): A list of #LightDMLanguage that should be presented to the user.
 **/
GList *
lightdm_get_languages (void)
{
    update_languages ();
    return languages;
}

/**
 * lightdm_language_get_code:
 * @language: A #LightDMLanguage
 * 
 * Get the code of a language.
 * 
 * Return value: The code of the language
 **/
const gchar *
lightdm_language_get_code (LightDMLanguage *language)
{
    g_return_val_if_fail (LIGHTDM_IS_LANGUAGE (language), NULL);
    return GET_PRIVATE (language)->code;
}

/**
 * lightdm_language_get_name:
 * @language: A #LightDMLanguage
 * 
 * Get the name of a language.
 *
 * Return value: The name of the language
 **/
const gchar *
lightdm_language_get_name (LightDMLanguage *language)
{
    LightDMLanguagePrivate *priv;

    g_return_val_if_fail (LIGHTDM_IS_LANGUAGE (language), NULL);

    priv = GET_PRIVATE (language);

    if (!priv->name)
    {
        gchar *locale = get_locale_name (priv->code);
        if (locale)
        {
            gchar *current = setlocale (LC_ALL, NULL);
            setlocale (LC_IDENTIFICATION, locale);
            setlocale (LC_MESSAGES, "");

            gchar *language_en = nl_langinfo (_NL_IDENTIFICATION_LANGUAGE);
            if (language_en && strlen (language_en) > 0)
                priv->name = g_strdup (dgettext ("iso_639_3", language_en));

            setlocale (LC_ALL, current);
        }
        if (!priv->name)
        {
            gchar **tokens = g_strsplit_set (priv->code, "_.@", 2);
            priv->name = g_strdup (tokens[0]);
            g_strfreev (tokens);
        }
    }

    return priv->name;
}

/**
 * lightdm_language_get_territory:
 * @language: A #LightDMLanguage
 * 
 * Get the territory the language is used in.
 * 
 * Return value: The territory the language is used in.
 **/
const gchar *
lightdm_language_get_territory (LightDMLanguage *language)
{
    LightDMLanguagePrivate *priv;

    g_return_val_if_fail (LIGHTDM_IS_LANGUAGE (language), NULL);

    priv = GET_PRIVATE (language);

    if (!priv->territory && strchr (priv->code, '_'))
    {
        gchar *locale = get_locale_name (priv->code);
        if (locale)
        {
            gchar *current = setlocale (LC_ALL, NULL);
            setlocale (LC_IDENTIFICATION, locale);
            setlocale (LC_MESSAGES, "");

            gchar *country_en = nl_langinfo (_NL_IDENTIFICATION_TERRITORY);
            if (country_en && strlen (country_en) > 0 && g_strcmp0 (country_en, "ISO") != 0)
                priv->territory = g_strdup (dgettext ("iso_3166", country_en));

            setlocale (LC_ALL, current);
        }
        if (!priv->territory)
        {
            gchar **tokens = g_strsplit_set (priv->code, "_.@", 3);
            priv->territory = g_strdup (tokens[1]);
            g_strfreev (tokens);
        }
    }

    return priv->territory;
}

/**
 * lightdm_language_matches:
 * @language: A #LightDMLanguage
 * @code: A language code
 * 
 * Check if a language code matches this language.
 * 
 * Return value: #TRUE if the code matches this language.
 **/
gboolean
lightdm_language_matches (LightDMLanguage *language, const gchar *code)
{
    LightDMLanguagePrivate *priv;

    g_return_val_if_fail (LIGHTDM_IS_LANGUAGE (language), FALSE);
    g_return_val_if_fail (code != NULL, FALSE);

    priv = GET_PRIVATE (language);

    /* Handle the fact the UTF-8 is specified both as '.utf8' and '.UTF-8' */
    if (is_utf8 (priv->code) && is_utf8 (code))
    {
        /* Match the characters before the '.' */
        int i;
        for (i = 0; priv->code[i] && code[i] && priv->code[i] == code[i] && code[i] != '.' ; i++);
        return priv->code[i] == '.' && code[i] == '.';
    }

    return g_str_equal (priv->code, code);
}

static void
lightdm_language_init (LightDMLanguage *language)
{
}

static void
lightdm_language_set_property (GObject      *object,
                           guint         prop_id,
                           const GValue *value,
                           GParamSpec   *pspec)
{
    LightDMLanguage *self = LIGHTDM_LANGUAGE (object);
    LightDMLanguagePrivate *priv = GET_PRIVATE (self);

    switch (prop_id) {
    case PROP_CODE:
        g_free (priv->name);
        priv->code = g_strdup (g_value_get_string (value));
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
lightdm_language_get_property (GObject    *object,
                           guint       prop_id,
                           GValue     *value,
                           GParamSpec *pspec)
{
    LightDMLanguage *self;

    self = LIGHTDM_LANGUAGE (object);

    switch (prop_id) {
    case PROP_CODE:
        g_value_set_string (value, lightdm_language_get_code (self));
        break;
    case PROP_NAME:
        g_value_set_string (value, lightdm_language_get_name (self));
        break;
    case PROP_TERRITORY:
        g_value_set_string (value, lightdm_language_get_territory (self));
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
lightdm_language_class_init (LightDMLanguageClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);
  
    g_type_class_add_private (klass, sizeof (LightDMLanguagePrivate));

    object_class->set_property = lightdm_language_set_property;
    object_class->get_property = lightdm_language_get_property;

    g_object_class_install_property (object_class,
                                     PROP_CODE,
                                     g_param_spec_string ("code",
                                                          "code",
                                                          "Language code",
                                                          NULL,
                                                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property (object_class,
                                     PROP_NAME,
                                     g_param_spec_string ("name",
                                                          "name",
                                                          "Name of the language",
                                                          NULL,
                                                          G_PARAM_READABLE));
    g_object_class_install_property (object_class,
                                     PROP_TERRITORY,
                                     g_param_spec_string ("territory",
                                                          "territory",
                                                          "Territory the language is from",
                                                          NULL,
                                                          G_PARAM_READABLE));
}
