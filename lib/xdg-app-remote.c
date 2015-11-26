/*
 * Copyright © 2015 Red Hat, Inc
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors:
 *       Alexander Larsson <alexl@redhat.com>
 */

#include "config.h"

#include "xdg-app-remote-private.h"
#include "xdg-app-enum-types.h"
#include "xdg-app-utils.h"

#include <string.h>

typedef struct _XdgAppRemotePrivate XdgAppRemotePrivate;

struct _XdgAppRemotePrivate
{
  char *name;
  char *url;
  char *title;
  OstreeRepo *repo;
};

G_DEFINE_TYPE_WITH_PRIVATE (XdgAppRemote, xdg_app_remote, G_TYPE_OBJECT)

enum {
  PROP_0,

  PROP_NAME,
};

static void
xdg_app_remote_finalize (GObject *object)
{
  XdgAppRemote *self = XDG_APP_REMOTE (object);
  XdgAppRemotePrivate *priv = xdg_app_remote_get_instance_private (self);

  g_free (priv->name);
  g_free (priv->url);
  g_free (priv->title);
  g_object_unref (priv->repo);
  
  G_OBJECT_CLASS (xdg_app_remote_parent_class)->finalize (object);
}

static void
xdg_app_remote_set_property (GObject         *object,
                             guint            prop_id,
                             const GValue    *value,
                             GParamSpec      *pspec)
{
  XdgAppRemote *self = XDG_APP_REMOTE (object);
  XdgAppRemotePrivate *priv = xdg_app_remote_get_instance_private (self);

  switch (prop_id)
    {
    case PROP_NAME:
      g_clear_pointer (&priv->name, g_free);
      priv->name = g_value_dup_string (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
xdg_app_remote_get_property (GObject         *object,
                             guint            prop_id,
                             GValue          *value,
                             GParamSpec      *pspec)
{
  XdgAppRemote *self = XDG_APP_REMOTE (object);
  XdgAppRemotePrivate *priv = xdg_app_remote_get_instance_private (self);

  switch (prop_id)
    {
    case PROP_NAME:
      g_value_set_string (value, priv->name);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
xdg_app_remote_class_init (XdgAppRemoteClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->get_property = xdg_app_remote_get_property;
  object_class->set_property = xdg_app_remote_set_property;
  object_class->finalize = xdg_app_remote_finalize;

  g_object_class_install_property (object_class,
                                   PROP_NAME,
                                   g_param_spec_string ("name",
                                                        "",
                                                        "",
                                                        NULL,
                                                        G_PARAM_READWRITE));
}

static void
xdg_app_remote_init (XdgAppRemote *self)
{
}

const char *
xdg_app_remote_get_name (XdgAppRemote *self)
{
  XdgAppRemotePrivate *priv = xdg_app_remote_get_instance_private (self);

  return priv->name;
}

const char *
xdg_app_remote_get_url (XdgAppRemote *self)
{
  XdgAppRemotePrivate *priv = xdg_app_remote_get_instance_private (self);

  if (priv->url == NULL)
    ostree_repo_remote_get_url (priv->repo, priv->name, &priv->url, NULL);

  return priv->url;
}

const char *
xdg_app_remote_get_title (XdgAppRemote *self)
{
  XdgAppRemotePrivate *priv = xdg_app_remote_get_instance_private (self);
  GKeyFile *config;

  if (priv->title == NULL)
    {
      g_autofree char *group = g_strdup_printf ("remote \"%s\"", priv->name);
      config = ostree_repo_get_config (priv->repo);
      priv->title = g_key_file_get_string (config, group, "xa.title", NULL);
    }

  return priv->title ? priv->title : priv->name;
}

gboolean
xdg_app_remote_get_gpg_verify (XdgAppRemote *self)
{
  XdgAppRemotePrivate *priv = xdg_app_remote_get_instance_private (self);
  gboolean res;

  if (ostree_repo_remote_get_gpg_verify (priv->repo, priv->name, &res, NULL))
    return res;

  return FALSE;
}

static XdgAppRef *
get_ref (XdgAppRemote *self,
         const char *full_ref,
         const char *commit)
{
  g_auto(GStrv) parts = NULL;
  XdgAppRefKind kind = XDG_APP_REF_KIND_APP;

  parts = g_strsplit (full_ref, "/", -1);

  if (strcmp (parts[0], "app") != 0)
    kind = XDG_APP_REF_KIND_RUNTIME;

  return g_object_new (XDG_APP_TYPE_REF,
                       "kind", kind,
                       "name", parts[1],
                       "arch", parts[2],
                       "version", parts[3],
                       "commit", commit,
                       NULL);
}

XdgAppRef **
xdg_app_remote_list_refs (XdgAppRemote *self,
                          GCancellable *cancellable,
                          GError **error)
{
  g_autoptr(GPtrArray) refs = g_ptr_array_new_with_free_func (g_object_unref);
  g_autoptr(GHashTable) ht = NULL;
  const char *url;
  g_autofree char *title = NULL;
  GHashTableIter iter;
  gpointer key;
  gpointer value;

  url = xdg_app_remote_get_url (self);
  g_print ("url: %s\n", url);
  if (url)
    {
      if (!ostree_repo_load_summary (url, &ht, &title, cancellable, error))
        return FALSE;

      g_hash_table_iter_init (&iter, ht);
      while (g_hash_table_iter_next (&iter, &key, &value))
        {
          const char *refspec = key;
          const char *checksum = value;

          g_ptr_array_add (refs,
                           get_ref (self, refspec, checksum));
        }
    }

  g_ptr_array_add (refs, NULL);
  return (XdgAppRef **)g_ptr_array_free (g_steal_pointer (&refs), FALSE);
}


XdgAppRemote *
xdg_app_remote_new (OstreeRepo *repo,
                    const char *name)
{
  XdgAppRemotePrivate *priv;
  XdgAppRemote *self = g_object_new (XDG_APP_TYPE_REMOTE,
                                       "name", name,
                                       NULL);

  priv = xdg_app_remote_get_instance_private (self);
  priv->repo = g_object_ref (repo);

  return self;
}