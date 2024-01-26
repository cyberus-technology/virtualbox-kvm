/** @file
 * Stubs for dynamically loading libdbus-1 and the symbols which are needed by
 * VirtualBox.
 */

/*
 * Copyright (C) 2008-2023 Oracle and/or its affiliates.
 *
 * This file is part of VirtualBox base platform packages, as
 * available from https://www.virtualbox.org.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, in version 3 of the
 * License.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <https://www.gnu.org/licenses>.
 *
 * The contents of this file may alternatively be used under the terms
 * of the Common Development and Distribution License Version 1.0
 * (CDDL), a copy of it is provided in the "COPYING.CDDL" file included
 * in the VirtualBox distribution, in which case the provisions of the
 * CDDL are applicable instead of those of the GPL.
 *
 * You may elect to license modified versions of this file under the
 * terms and conditions of either the GPL or the CDDL or both.
 *
 * SPDX-License-Identifier: GPL-3.0-only OR CDDL-1.0
 */

/** The file name of the DBus library */
#define RT_RUNTIME_LOADER_LIB_NAME  "libdbus-1.so.3"

/** The name of the loader function */
#define RT_RUNTIME_LOADER_FUNCTION  RTDBusLoadLib

/** The following are the symbols which we need from the DBus library. */
#define RT_RUNTIME_LOADER_INSERT_SYMBOLS \
 RT_PROXY_STUB(dbus_error_init, void, (DBusError *error), \
                 (error)) \
 RT_PROXY_STUB(dbus_error_is_set, dbus_bool_t, (const DBusError *error), \
                 (error)) \
 RT_PROXY_STUB(dbus_bus_get, DBusConnection *, \
                 (DBusBusType type, DBusError *error), (type, error)) \
 RT_PROXY_STUB(dbus_bus_get_private, DBusConnection *, \
                 (DBusBusType type, DBusError *error), (type, error)) \
 RT_PROXY_STUB(dbus_error_free, void, (DBusError *error), \
                 (error)) \
 RT_PROXY_STUB(dbus_free_string_array, void, (char **str_array), \
                 (str_array)) \
 RT_PROXY_STUB(dbus_connection_ref, DBusConnection *, (DBusConnection *connection), \
                 (connection)) \
 RT_PROXY_STUB(dbus_connection_unref, void, (DBusConnection *connection), \
                 (connection)) \
 RT_PROXY_STUB(dbus_connection_close, void, (DBusConnection *connection), \
                 (connection)) \
 RT_PROXY_STUB(dbus_connection_send, dbus_bool_t, \
                 (DBusConnection *connection, DBusMessage *message, dbus_uint32_t *serial), \
                 (connection, message, serial)) \
 RT_PROXY_STUB(dbus_connection_flush, void, (DBusConnection *connection), \
                 (connection)) \
 RT_PROXY_STUB(dbus_connection_set_exit_on_disconnect, void, \
                 (DBusConnection *connection, dbus_bool_t boolean), \
                 (connection, boolean)) \
 RT_PROXY_STUB(dbus_bus_name_has_owner, dbus_bool_t, \
                 (DBusConnection *connection, const char *string, DBusError *error), \
                 (connection, string, error)) \
 RT_PROXY_STUB(dbus_bus_add_match, void, \
                 (DBusConnection *connection, const char *string, \
                  DBusError *error), \
                 (connection, string, error)) \
 RT_PROXY_STUB(dbus_bus_remove_match, void, \
                 (DBusConnection *connection, const char *string, \
                  DBusError *error), \
                 (connection, string, error)) \
 RT_PROXY_STUB(dbus_message_append_args_valist, dbus_bool_t, \
                 (DBusMessage *message, int first_arg_type, va_list var_args), \
                 (message, first_arg_type, var_args)) \
 RT_PROXY_STUB(dbus_message_get_args_valist, dbus_bool_t, \
                 (DBusMessage *message, DBusError *error, int first_arg_type, va_list var_args), \
                 (message, error, first_arg_type, var_args)) \
 RT_PROXY_STUB(dbus_message_get_type, int, \
                 (DBusMessage *message), \
                 (message)) \
 RT_PROXY_STUB(dbus_message_iter_open_container, dbus_bool_t, \
                 (DBusMessageIter *iter, int type, const char *contained_signature, DBusMessageIter *sub), \
                 (iter, type, contained_signature, sub)) \
 RT_PROXY_STUB(dbus_message_iter_close_container, dbus_bool_t, \
                 (DBusMessageIter *iter, DBusMessageIter *sub), \
                 (iter, sub)) \
 RT_PROXY_STUB(dbus_message_iter_append_fixed_array, dbus_bool_t, \
                 (DBusMessageIter *iter, int element_type, const void *value, int n_elements), \
                 (iter, element_type, value, n_elements)) \
 RT_PROXY_STUB(dbus_message_unref, void, (DBusMessage *message), \
                 (message)) \
 RT_PROXY_STUB(dbus_message_new_method_call, DBusMessage*, \
                 (const char *string1, const char *string2, const char *string3, \
                  const char *string4), \
                 (string1, string2, string3, string4)) \
 RT_PROXY_STUB(dbus_message_iter_init_append, void, \
                 (DBusMessage *message, DBusMessageIter *iter), \
                 (message, iter)) \
 RT_PROXY_STUB(dbus_message_iter_append_basic, dbus_bool_t, \
                 (DBusMessageIter *iter, int val, const void *string), \
                 (iter, val, string)) \
 RT_PROXY_STUB(dbus_connection_send_with_reply_and_block, DBusMessage *, \
                 (DBusConnection *connection, DBusMessage *message, int val, \
                  DBusError *error), \
                 (connection, message, val, error)) \
 RT_PROXY_STUB(dbus_message_iter_init, dbus_bool_t, \
                 (DBusMessage *message, DBusMessageIter *iter), \
                 (message, iter)) \
 RT_PROXY_STUB(dbus_message_get_signature, char *, (DBusMessage *message), \
                 (message)) \
 RT_PROXY_STUB(dbus_message_iter_get_signature, char *, (DBusMessageIter *iter), \
                 (iter)) \
 RT_PROXY_STUB(dbus_message_iter_get_arg_type, int, (DBusMessageIter *iter), \
                 (iter)) \
 RT_PROXY_STUB(dbus_message_iter_get_element_type, int, \
                 (DBusMessageIter *iter), (iter)) \
 RT_PROXY_STUB(dbus_message_iter_recurse, void, \
                 (DBusMessageIter *iter1, DBusMessageIter *iter2), \
                 (iter1, iter2)) \
 RT_PROXY_STUB(dbus_message_iter_get_basic, void, \
                 (DBusMessageIter *iter, void *pvoid), (iter, pvoid)) \
 RT_PROXY_STUB(dbus_message_iter_has_next, dbus_bool_t, \
                 (DBusMessageIter *iter), (iter)) \
 RT_PROXY_STUB(dbus_message_iter_next, dbus_bool_t, (DBusMessageIter *iter), \
                 (iter)) \
 RT_PROXY_STUB(dbus_message_iter_abandon_container_if_open, void, \
                 (DBusMessageIter *iter, DBusMessageIter *sub), (iter, sub)) \
 RT_PROXY_STUB(dbus_connection_add_filter, dbus_bool_t, \
                 (DBusConnection *connection, \
                  DBusHandleMessageFunction function1, void *pvoid, \
                  DBusFreeFunction function2), \
                 (connection, function1, pvoid, function2)) \
 RT_PROXY_STUB(dbus_connection_remove_filter, void, \
                 (DBusConnection *connection, \
                  DBusHandleMessageFunction function, void *pvoid), \
                 (connection, function, pvoid)) \
 RT_PROXY_STUB(dbus_connection_read_write, dbus_bool_t, \
                 (DBusConnection *connection, int val), (connection, val)) \
 RT_PROXY_STUB(dbus_connection_read_write_dispatch, dbus_bool_t, \
                 (DBusConnection *connection, int val), (connection, val)) \
 RT_PROXY_STUB(dbus_message_is_signal, dbus_bool_t, \
                 (DBusMessage *message, const char *string1, \
                  const char *string2), \
                 (message, string1, string2)) \
 RT_PROXY_STUB(dbus_connection_pop_message, DBusMessage *, \
                 (DBusConnection *connection), (connection)) \
 RT_PROXY_STUB(dbus_set_error_from_message, dbus_bool_t, \
                 (DBusError *error, DBusMessage *message), (error, message)) \
 RT_PROXY_STUB(dbus_free, void, \
                 (void *memory), (memory))

#ifdef VBOX_DBUS_GENERATE_HEADER
# define RT_RUNTIME_LOADER_GENERATE_HEADER
# define RT_RUNTIME_LOADER_GENERATE_DECLS
# include <iprt/runtime-loader.h>
# undef RT_RUNTIME_LOADER_GENERATE_HEADER
# undef RT_RUNTIME_LOADER_GENERATE_DECLS

#elif defined(VBOX_DBUS_GENERATE_BODY)
# define RT_RUNTIME_LOADER_GENERATE_BODY_STUBS
# include <iprt/runtime-loader.h>
# undef RT_RUNTIME_LOADER_GENERATE_BODY_STUBS

#else
# error This file should only be included to generate stubs for loading the DBus library at runtime
#endif

#undef RT_RUNTIME_LOADER_LIB_NAME
#undef RT_RUNTIME_LOADER_INSERT_SYMBOLS

