/*
 * Copyright © 2014-2018 Red Hat, Inc
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
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

#ifndef __FLATPAK_CONTEXT_H__
#define __FLATPAK_CONTEXT_H__

#include "libglnx.h"
#include <flatpak-common-types-private.h>
#include "flatpak-exports-private.h"

#include <stdint.h>

typedef enum {
  FLATPAK_POLICY_NONE,
  FLATPAK_POLICY_SEE,
  FLATPAK_POLICY_TALK,
  FLATPAK_POLICY_OWN
} FlatpakPolicy;

typedef struct FlatpakContext FlatpakContext;

typedef enum {
  FLATPAK_CONTEXT_SHARED_NETWORK   = 1 << 0,
  FLATPAK_CONTEXT_SHARED_IPC       = 1 << 1,
} FlatpakContextShares;

typedef enum {
  FLATPAK_CONTEXT_SOCKET_X11         = 1 << 0,
  FLATPAK_CONTEXT_SOCKET_WAYLAND     = 1 << 1,
  FLATPAK_CONTEXT_SOCKET_PULSEAUDIO  = 1 << 2,
  FLATPAK_CONTEXT_SOCKET_SESSION_BUS = 1 << 3,
  FLATPAK_CONTEXT_SOCKET_SYSTEM_BUS  = 1 << 4,
  FLATPAK_CONTEXT_SOCKET_FALLBACK_X11 = 1 << 5, /* For backwards compat, also set SOCKET_X11 */
  FLATPAK_CONTEXT_SOCKET_SSH_AUTH    = 1 << 6,
  FLATPAK_CONTEXT_SOCKET_PCSC        = 1 << 7,
  FLATPAK_CONTEXT_SOCKET_CUPS        = 1 << 8,
  FLATPAK_CONTEXT_SOCKET_GPG_AGENT   = 1 << 9,
} FlatpakContextSockets;

typedef enum {
  FLATPAK_CONTEXT_DEVICE_DRI         = 1 << 0,
  FLATPAK_CONTEXT_DEVICE_ALL         = 1 << 1,
  FLATPAK_CONTEXT_DEVICE_KVM         = 1 << 2,
  FLATPAK_CONTEXT_DEVICE_SHM         = 1 << 3,
  FLATPAK_CONTEXT_DEVICE_INPUT       = 1 << 4,
} FlatpakContextDevices;

typedef enum {
  FLATPAK_CONTEXT_FEATURE_DEVEL        = 1 << 0,
  FLATPAK_CONTEXT_FEATURE_MULTIARCH    = 1 << 1,
  FLATPAK_CONTEXT_FEATURE_BLUETOOTH    = 1 << 2,
  FLATPAK_CONTEXT_FEATURE_CANBUS       = 1 << 3,
  FLATPAK_CONTEXT_FEATURE_PER_APP_DEV_SHM = 1 << 4,
} FlatpakContextFeatures;

struct FlatpakContext
{
  FlatpakContextShares   shares;
  FlatpakContextShares   shares_valid;
  FlatpakContextSockets  sockets;
  FlatpakContextSockets  sockets_valid;
  FlatpakContextDevices  devices;
  FlatpakContextDevices  devices_valid;
  FlatpakContextFeatures features;
  FlatpakContextFeatures features_valid;
  GHashTable            *env_vars;
  GHashTable            *persistent;
  GHashTable            *filesystems;
  GHashTable            *session_bus_policy;
  GHashTable            *system_bus_policy;
  GHashTable            *generic_policy;
  GHashTable            *allowed_usb_devices;
  GHashTable            *blocked_usb_devices;
};

extern const char *flatpak_context_sockets[];
extern const char *flatpak_context_devices[];
extern const char *flatpak_context_features[];
extern const char *flatpak_context_shares[];

gboolean       flatpak_context_parse_filesystem (const char             *filesystem_and_mode,
                                                 gboolean                negated,
                                                 char                  **filesystem_out,
                                                 FlatpakFilesystemMode  *mode_out,
                                                 GError                **error);

FlatpakContext *flatpak_context_new (void);
void           flatpak_context_free (FlatpakContext *context);
void           flatpak_context_merge (FlatpakContext *context,
                                      FlatpakContext *other);
GOptionEntry  *flatpak_context_get_option_entries (void);
GOptionGroup  *flatpak_context_get_options (FlatpakContext *context);
gboolean       flatpak_context_load_metadata (FlatpakContext *context,
                                              GKeyFile       *metakey,
                                              GError        **error);
void           flatpak_context_save_metadata (FlatpakContext *context,
                                              gboolean        flatten,
                                              GKeyFile       *metakey);
void           flatpak_context_allow_host_fs (FlatpakContext *context);
void           flatpak_context_set_session_bus_policy (FlatpakContext *context,
                                                       const char     *name,
                                                       FlatpakPolicy   policy);
GStrv          flatpak_context_get_session_bus_policy_allowed_own_names (FlatpakContext *context);
void           flatpak_context_set_system_bus_policy (FlatpakContext *context,
                                                      const char     *name,
                                                      FlatpakPolicy   policy);
void           flatpak_context_to_args (FlatpakContext *context,
                                        GPtrArray      *args);
FlatpakRunFlags flatpak_context_get_run_flags (FlatpakContext *context);
void           flatpak_context_add_bus_filters (FlatpakContext *context,
                                                const char     *app_id,
                                                gboolean        session_bus,
                                                gboolean        sandboxed,
                                                FlatpakBwrap   *bwrap);

gboolean       flatpak_context_get_needs_session_bus_proxy (FlatpakContext *context);
gboolean       flatpak_context_get_needs_system_bus_proxy (FlatpakContext *context);
gboolean       flatpak_context_adds_permissions (FlatpakContext *old_context,
                                                 FlatpakContext *new_context);

void           flatpak_context_reset_permissions (FlatpakContext *context);
void           flatpak_context_reset_non_permissions (FlatpakContext *context);
void           flatpak_context_make_sandboxed (FlatpakContext *context);

gboolean       flatpak_context_allows_features (FlatpakContext        *context,
                                                FlatpakContextFeatures features);

FlatpakContext *flatpak_context_load_for_deploy (FlatpakDeploy *deploy,
                                                 GError       **error);

FlatpakExports *flatpak_context_get_exports (FlatpakContext *context,
                                             const char     *app_id);
FlatpakExports *flatpak_context_get_exports_full (FlatpakContext *context,
                                                  GFile          *app_id_dir,
                                                  GPtrArray      *extra_app_id_dirs,
                                                  gboolean        do_create,
                                                  gboolean        include_default_dirs,
                                                  gchar         **xdg_dirs_conf,
                                                  gboolean       *home_access_out);

void flatpak_context_append_bwrap_filesystem (FlatpakContext  *context,
                                              FlatpakBwrap    *bwrap,
                                              const char      *app_id,
                                              GFile           *app_id_dir,
                                              FlatpakExports  *exports,
                                              const char      *xdg_dirs_conf,
                                              gboolean         home_access);

gboolean flatpak_context_parse_env_block (FlatpakContext *context,
                                          const char *data,
                                          gsize length,
                                          GError **error);
gboolean flatpak_context_parse_env_fd (FlatpakContext *context,
                                       int fd,
                                       GError **error);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (FlatpakContext, flatpak_context_free)

GFile *flatpak_get_user_base_dir_location (void);
GFile *flatpak_get_data_dir (const char *app_id);

/* USB */

typedef struct
{
  enum {
    FLATPAK_USB_RULE_TYPE_ALL,
    FLATPAK_USB_RULE_TYPE_CLASS,
    FLATPAK_USB_RULE_TYPE_DEVICE,
    FLATPAK_USB_RULE_TYPE_VENDOR,
  } rule_type;

  union {
    struct {
      enum {
        FLATPAK_USB_RULE_CLASS_TYPE_CLASS_ONLY,
        FLATPAK_USB_RULE_CLASS_TYPE_CLASS_SUBCLASS,
      } type;
      uint16_t class;
      uint16_t subclass;
    } device_class;

    struct {
      uint16_t id;
    } product;

    struct {
      uint16_t id;
    } vendor;

  } d;

} FlatpakUsbRule;

gboolean flatpak_context_parse_usb_rule (const char      *data,
                                         FlatpakUsbRule **out_usb_rule,
                                         GError         **error);

void flatpak_usb_rule_print (FlatpakUsbRule *usb_rule,
                             GString        *string);

void flatpak_usb_rule_free (FlatpakUsbRule *usb_rule);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (FlatpakUsbRule, flatpak_usb_rule_free)

typedef struct
{
  GPtrArray *rules;
} FlatpakUsbQuery;

gboolean flatpak_context_parse_usb (const char       *data,
                                    FlatpakUsbQuery **out_usb_query,
                                    GError          **error);

void flatpak_usb_query_print (FlatpakUsbQuery *usb_query,
                              GString         *string);

void flatpak_usb_query_free (FlatpakUsbQuery *usb_query);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (FlatpakUsbQuery, flatpak_usb_query_free)

#endif /* __FLATPAK_CONTEXT_H__ */
