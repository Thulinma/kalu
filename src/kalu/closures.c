/**
 * kalu - Copyright (C) 2012-2018 Olivier Brunel
 *
 * closures.c
 * Copyright (C) 2012-2016 Olivier Brunel <jjk@jjacky.com>
 *
 * This file is part of kalu.
 *
 * kalu is free software: you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * kalu is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * kalu. If not, see http://www.gnu.org/licenses/
 */

#include	<glib-object.h>
#include    "closures.h"

#ifdef G_ENABLE_DEBUG
#define g_marshal_value_peek_boolean(v)  g_value_get_boolean (v)
#define g_marshal_value_peek_char(v)     g_value_get_schar (v)
#define g_marshal_value_peek_uchar(v)    g_value_get_uchar (v)
#define g_marshal_value_peek_int(v)      g_value_get_int (v)
#define g_marshal_value_peek_uint(v)     g_value_get_uint (v)
#define g_marshal_value_peek_long(v)     g_value_get_long (v)
#define g_marshal_value_peek_ulong(v)    g_value_get_ulong (v)
#define g_marshal_value_peek_int64(v)    g_value_get_int64 (v)
#define g_marshal_value_peek_uint64(v)   g_value_get_uint64 (v)
#define g_marshal_value_peek_enum(v)     g_value_get_enum (v)
#define g_marshal_value_peek_flags(v)    g_value_get_flags (v)
#define g_marshal_value_peek_float(v)    g_value_get_float (v)
#define g_marshal_value_peek_double(v)   g_value_get_double (v)
#define g_marshal_value_peek_string(v)   (char*) g_value_get_string (v)
#define g_marshal_value_peek_param(v)    g_value_get_param (v)
#define g_marshal_value_peek_boxed(v)    g_value_get_boxed (v)
#define g_marshal_value_peek_pointer(v)  g_value_get_pointer (v)
#define g_marshal_value_peek_object(v)   g_value_get_object (v)
#define g_marshal_value_peek_variant(v)  g_value_get_variant (v)
#else /* !G_ENABLE_DEBUG */
/* WARNING: This code accesses GValues directly, which is UNSUPPORTED API.
 *          Do not access GValues directly in your code. Instead, use the
 *          g_value_get_*() functions
 */
#define g_marshal_value_peek_boolean(v)  (v)->data[0].v_int
#define g_marshal_value_peek_char(v)     (v)->data[0].v_int
#define g_marshal_value_peek_uchar(v)    (v)->data[0].v_uint
#define g_marshal_value_peek_int(v)      (v)->data[0].v_int
#define g_marshal_value_peek_uint(v)     (v)->data[0].v_uint
#define g_marshal_value_peek_long(v)     (v)->data[0].v_long
#define g_marshal_value_peek_ulong(v)    (v)->data[0].v_ulong
#define g_marshal_value_peek_int64(v)    (v)->data[0].v_int64
#define g_marshal_value_peek_uint64(v)   (v)->data[0].v_uint64
#define g_marshal_value_peek_enum(v)     (v)->data[0].v_long
#define g_marshal_value_peek_flags(v)    (v)->data[0].v_ulong
#define g_marshal_value_peek_float(v)    (v)->data[0].v_float
#define g_marshal_value_peek_double(v)   (v)->data[0].v_double
#define g_marshal_value_peek_string(v)   (v)->data[0].v_pointer
#define g_marshal_value_peek_param(v)    (v)->data[0].v_pointer
#define g_marshal_value_peek_boxed(v)    (v)->data[0].v_pointer
#define g_marshal_value_peek_pointer(v)  (v)->data[0].v_pointer
#define g_marshal_value_peek_object(v)   (v)->data[0].v_pointer
#define g_marshal_value_peek_variant(v)  (v)->data[0].v_pointer
#endif /* !G_ENABLE_DEBUG */


/* VOID:STRING,UINT,UINT (closures.def:1) */
void
g_cclosure_user_marshal_VOID__STRING_UINT_UINT (GClosure     *closure,
                                                GValue       *return_value G_GNUC_UNUSED,
                                                guint         n_param_values,
                                                const GValue *param_values,
                                                gpointer      invocation_hint G_GNUC_UNUSED,
                                                gpointer      marshal_data)
{
  typedef void (*GMarshalFunc_VOID__STRING_UINT_UINT) (gpointer     data1,
                                                       gpointer     arg_1,
                                                       guint        arg_2,
                                                       guint        arg_3,
                                                       gpointer     data2);
  GMarshalFunc_VOID__STRING_UINT_UINT callback;
  GCClosure *cc = (GCClosure*) closure;
  gpointer data1, data2;

  g_return_if_fail (n_param_values == 4);

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = g_value_peek_pointer (param_values + 0);
    }
  else
    {
      data1 = g_value_peek_pointer (param_values + 0);
      data2 = closure->data;
    }
  callback = (GMarshalFunc_VOID__STRING_UINT_UINT) (marshal_data ? marshal_data : cc->callback);

  callback (data1,
            g_marshal_value_peek_string (param_values + 1),
            g_marshal_value_peek_uint (param_values + 2),
            g_marshal_value_peek_uint (param_values + 3),
            data2);
}

/* VOID:INT,STRING (closures.def:2) */
void
g_cclosure_user_marshal_VOID__INT_STRING (GClosure     *closure,
                                          GValue       *return_value G_GNUC_UNUSED,
                                          guint         n_param_values,
                                          const GValue *param_values,
                                          gpointer      invocation_hint G_GNUC_UNUSED,
                                          gpointer      marshal_data)
{
  typedef void (*GMarshalFunc_VOID__INT_STRING) (gpointer     data1,
                                                 gint         arg_1,
                                                 gpointer     arg_2,
                                                 gpointer     data2);
  GMarshalFunc_VOID__INT_STRING callback;
  GCClosure *cc = (GCClosure*) closure;
  gpointer data1, data2;

  g_return_if_fail (n_param_values == 3);

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = g_value_peek_pointer (param_values + 0);
    }
  else
    {
      data1 = g_value_peek_pointer (param_values + 0);
      data2 = closure->data;
    }
  callback = (GMarshalFunc_VOID__INT_STRING) (marshal_data ? marshal_data : cc->callback);

  callback (data1,
            g_marshal_value_peek_int (param_values + 1),
            g_marshal_value_peek_string (param_values + 2),
            data2);
}

/* BOOLEAN:STRING (closures.def:3) */
void
g_cclosure_user_marshal_BOOLEAN__STRING (GClosure     *closure,
                                         GValue       *return_value G_GNUC_UNUSED,
                                         guint         n_param_values,
                                         const GValue *param_values,
                                         gpointer      invocation_hint G_GNUC_UNUSED,
                                         gpointer      marshal_data)
{
  typedef gboolean (*GMarshalFunc_BOOLEAN__STRING) (gpointer     data1,
                                                    gpointer     arg_1,
                                                    gpointer     data2);
  GMarshalFunc_BOOLEAN__STRING callback;
  GCClosure *cc = (GCClosure*) closure;
  gpointer data1, data2;
  gboolean v_return;

  g_return_if_fail (return_value != NULL);
  g_return_if_fail (n_param_values == 2);

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = g_value_peek_pointer (param_values + 0);
    }
  else
    {
      data1 = g_value_peek_pointer (param_values + 0);
      data2 = closure->data;
    }
  callback = (GMarshalFunc_BOOLEAN__STRING) (marshal_data ? marshal_data : cc->callback);

  v_return = callback (data1,
                       g_marshal_value_peek_string (param_values + 1),
                       data2);

  g_value_set_boolean (return_value, v_return);
}

/* BOOLEAN:STRING,STRING,STRING,STRING (closures.def:4) */
void
g_cclosure_user_marshal_BOOLEAN__STRING_STRING_STRING_STRING (GClosure     *closure,
                                                              GValue       *return_value G_GNUC_UNUSED,
                                                              guint         n_param_values,
                                                              const GValue *param_values,
                                                              gpointer      invocation_hint G_GNUC_UNUSED,
                                                              gpointer      marshal_data)
{
  typedef gboolean (*GMarshalFunc_BOOLEAN__STRING_STRING_STRING_STRING) (gpointer     data1,
                                                                         gpointer     arg_1,
                                                                         gpointer     arg_2,
                                                                         gpointer     arg_3,
                                                                         gpointer     arg_4,
                                                                         gpointer     data2);
  GMarshalFunc_BOOLEAN__STRING_STRING_STRING_STRING callback;
  GCClosure *cc = (GCClosure*) closure;
  gpointer data1, data2;
  gboolean v_return;

  g_return_if_fail (return_value != NULL);
  g_return_if_fail (n_param_values == 5);

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = g_value_peek_pointer (param_values + 0);
    }
  else
    {
      data1 = g_value_peek_pointer (param_values + 0);
      data2 = closure->data;
    }
  callback = (GMarshalFunc_BOOLEAN__STRING_STRING_STRING_STRING) (marshal_data ? marshal_data : cc->callback);

  v_return = callback (data1,
                       g_marshal_value_peek_string (param_values + 1),
                       g_marshal_value_peek_string (param_values + 2),
                       g_marshal_value_peek_string (param_values + 3),
                       g_marshal_value_peek_string (param_values + 4),
                       data2);

  g_value_set_boolean (return_value, v_return);
}

/* BOOLEAN:STRING,STRING,STRING (closures.def:5) */
void
g_cclosure_user_marshal_BOOLEAN__STRING_STRING_STRING (GClosure     *closure,
                                                       GValue       *return_value G_GNUC_UNUSED,
                                                       guint         n_param_values,
                                                       const GValue *param_values,
                                                       gpointer      invocation_hint G_GNUC_UNUSED,
                                                       gpointer      marshal_data)
{
  typedef gboolean (*GMarshalFunc_BOOLEAN__STRING_STRING_STRING) (gpointer     data1,
                                                                  gpointer     arg_1,
                                                                  gpointer     arg_2,
                                                                  gpointer     arg_3,
                                                                  gpointer     data2);
  GMarshalFunc_BOOLEAN__STRING_STRING_STRING callback;
  GCClosure *cc = (GCClosure*) closure;
  gpointer data1, data2;
  gboolean v_return;

  g_return_if_fail (return_value != NULL);
  g_return_if_fail (n_param_values == 4);

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = g_value_peek_pointer (param_values + 0);
    }
  else
    {
      data1 = g_value_peek_pointer (param_values + 0);
      data2 = closure->data;
    }
  callback = (GMarshalFunc_BOOLEAN__STRING_STRING_STRING) (marshal_data ? marshal_data : cc->callback);

  v_return = callback (data1,
                       g_marshal_value_peek_string (param_values + 1),
                       g_marshal_value_peek_string (param_values + 2),
                       g_marshal_value_peek_string (param_values + 3),
                       data2);

  g_value_set_boolean (return_value, v_return);
}

/* BOOLEAN:POINTER (closures.def:6) */
void
g_cclosure_user_marshal_BOOLEAN__POINTER (GClosure     *closure,
                                          GValue       *return_value G_GNUC_UNUSED,
                                          guint         n_param_values,
                                          const GValue *param_values,
                                          gpointer      invocation_hint G_GNUC_UNUSED,
                                          gpointer      marshal_data)
{
  typedef gboolean (*GMarshalFunc_BOOLEAN__POINTER) (gpointer     data1,
                                                     gpointer     arg_1,
                                                     gpointer     data2);
  GMarshalFunc_BOOLEAN__POINTER callback;
  GCClosure *cc = (GCClosure*) closure;
  gpointer data1, data2;
  gboolean v_return;

  g_return_if_fail (return_value != NULL);
  g_return_if_fail (n_param_values == 2);

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = g_value_peek_pointer (param_values + 0);
    }
  else
    {
      data1 = g_value_peek_pointer (param_values + 0);
      data2 = closure->data;
    }
  callback = (GMarshalFunc_BOOLEAN__POINTER) (marshal_data ? marshal_data : cc->callback);

  v_return = callback (data1,
                       g_marshal_value_peek_pointer (param_values + 1),
                       data2);

  g_value_set_boolean (return_value, v_return);
}

/* INT:STRING,POINTER (closures.def:7) */
void
g_cclosure_user_marshal_INT__STRING_POINTER (GClosure     *closure,
                                             GValue       *return_value G_GNUC_UNUSED,
                                             guint         n_param_values,
                                             const GValue *param_values,
                                             gpointer      invocation_hint G_GNUC_UNUSED,
                                             gpointer      marshal_data)
{
  typedef gint (*GMarshalFunc_INT__STRING_POINTER) (gpointer     data1,
                                                    gpointer     arg_1,
                                                    gpointer     arg_2,
                                                    gpointer     data2);
  GMarshalFunc_INT__STRING_POINTER callback;
  GCClosure *cc = (GCClosure*) closure;
  gpointer data1, data2;
  gint v_return;

  g_return_if_fail (return_value != NULL);
  g_return_if_fail (n_param_values == 3);

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = g_value_peek_pointer (param_values + 0);
    }
  else
    {
      data1 = g_value_peek_pointer (param_values + 0);
      data2 = closure->data;
    }
  callback = (GMarshalFunc_INT__STRING_POINTER) (marshal_data ? marshal_data : cc->callback);

  v_return = callback (data1,
                       g_marshal_value_peek_string (param_values + 1),
                       g_marshal_value_peek_pointer (param_values + 2),
                       data2);

  g_value_set_int (return_value, v_return);
}

/* BOOLEAN:STRING,STRING (closures.def:8) */
void
g_cclosure_user_marshal_BOOLEAN__STRING_STRING (GClosure     *closure,
                                                GValue       *return_value G_GNUC_UNUSED,
                                                guint         n_param_values,
                                                const GValue *param_values,
                                                gpointer      invocation_hint G_GNUC_UNUSED,
                                                gpointer      marshal_data)
{
  typedef gboolean (*GMarshalFunc_BOOLEAN__STRING_STRING) (gpointer     data1,
                                                           gpointer     arg_1,
                                                           gpointer     arg_2,
                                                           gpointer     data2);
  GMarshalFunc_BOOLEAN__STRING_STRING callback;
  GCClosure *cc = (GCClosure*) closure;
  gpointer data1, data2;
  gboolean v_return;

  g_return_if_fail (return_value != NULL);
  g_return_if_fail (n_param_values == 3);

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = g_value_peek_pointer (param_values + 0);
    }
  else
    {
      data1 = g_value_peek_pointer (param_values + 0);
      data2 = closure->data;
    }
  callback = (GMarshalFunc_BOOLEAN__STRING_STRING) (marshal_data ? marshal_data : cc->callback);

  v_return = callback (data1,
                       g_marshal_value_peek_string (param_values + 1),
                       g_marshal_value_peek_string (param_values + 2),
                       data2);

  g_value_set_boolean (return_value, v_return);
}

/* VOID:STRING,STRING,POINTER (closures.def:9) */
void
g_cclosure_user_marshal_VOID__STRING_STRING_POINTER (GClosure     *closure,
                                                     GValue       *return_value G_GNUC_UNUSED,
                                                     guint         n_param_values,
                                                     const GValue *param_values,
                                                     gpointer      invocation_hint G_GNUC_UNUSED,
                                                     gpointer      marshal_data)
{
  typedef void (*GMarshalFunc_VOID__STRING_STRING_POINTER) (gpointer     data1,
                                                            gpointer     arg_1,
                                                            gpointer     arg_2,
                                                            gpointer     arg_3,
                                                            gpointer     data2);
  GMarshalFunc_VOID__STRING_STRING_POINTER callback;
  GCClosure *cc = (GCClosure*) closure;
  gpointer data1, data2;

  g_return_if_fail (n_param_values == 4);

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = g_value_peek_pointer (param_values + 0);
    }
  else
    {
      data1 = g_value_peek_pointer (param_values + 0);
      data2 = closure->data;
    }
  callback = (GMarshalFunc_VOID__STRING_STRING_POINTER) (marshal_data ? marshal_data : cc->callback);

  callback (data1,
            g_marshal_value_peek_string (param_values + 1),
            g_marshal_value_peek_string (param_values + 2),
            g_marshal_value_peek_pointer (param_values + 3),
            data2);
}

/* VOID:STRING,STRING (closures.def:10) */
void
g_cclosure_user_marshal_VOID__STRING_STRING (GClosure     *closure,
                                             GValue       *return_value G_GNUC_UNUSED,
                                             guint         n_param_values,
                                             const GValue *param_values,
                                             gpointer      invocation_hint G_GNUC_UNUSED,
                                             gpointer      marshal_data)
{
  typedef void (*GMarshalFunc_VOID__STRING_STRING) (gpointer     data1,
                                                    gpointer     arg_1,
                                                    gpointer     arg_2,
                                                    gpointer     data2);
  GMarshalFunc_VOID__STRING_STRING callback;
  GCClosure *cc = (GCClosure*) closure;
  gpointer data1, data2;

  g_return_if_fail (n_param_values == 3);

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = g_value_peek_pointer (param_values + 0);
    }
  else
    {
      data1 = g_value_peek_pointer (param_values + 0);
      data2 = closure->data;
    }
  callback = (GMarshalFunc_VOID__STRING_STRING) (marshal_data ? marshal_data : cc->callback);

  callback (data1,
            g_marshal_value_peek_string (param_values + 1),
            g_marshal_value_peek_string (param_values + 2),
            data2);
}

/* VOID:STRING,STRING,STRING,POINTER (closures.def:11) */
void
g_cclosure_user_marshal_VOID__STRING_STRING_STRING_POINTER (GClosure     *closure,
                                                            GValue       *return_value G_GNUC_UNUSED,
                                                            guint         n_param_values,
                                                            const GValue *param_values,
                                                            gpointer      invocation_hint G_GNUC_UNUSED,
                                                            gpointer      marshal_data)
{
  typedef void (*GMarshalFunc_VOID__STRING_STRING_STRING_POINTER) (gpointer     data1,
                                                                   gpointer     arg_1,
                                                                   gpointer     arg_2,
                                                                   gpointer     arg_3,
                                                                   gpointer     arg_4,
                                                                   gpointer     data2);
  GMarshalFunc_VOID__STRING_STRING_STRING_POINTER callback;
  GCClosure *cc = (GCClosure*) closure;
  gpointer data1, data2;

  g_return_if_fail (n_param_values == 5);

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = g_value_peek_pointer (param_values + 0);
    }
  else
    {
      data1 = g_value_peek_pointer (param_values + 0);
      data2 = closure->data;
    }
  callback = (GMarshalFunc_VOID__STRING_STRING_STRING_POINTER) (marshal_data ? marshal_data : cc->callback);

  callback (data1,
            g_marshal_value_peek_string (param_values + 1),
            g_marshal_value_peek_string (param_values + 2),
            g_marshal_value_peek_string (param_values + 3),
            g_marshal_value_peek_pointer (param_values + 4),
            data2);
}

/* VOID:INT,STRING,INT,UINT,UINT (closures.def:12) */
void
g_cclosure_user_marshal_VOID__INT_STRING_INT_UINT_UINT (GClosure     *closure,
                                                        GValue       *return_value G_GNUC_UNUSED,
                                                        guint         n_param_values,
                                                        const GValue *param_values,
                                                        gpointer      invocation_hint G_GNUC_UNUSED,
                                                        gpointer      marshal_data)
{
  typedef void (*GMarshalFunc_VOID__INT_STRING_INT_UINT_UINT) (gpointer     data1,
                                                               gint         arg_1,
                                                               gpointer     arg_2,
                                                               gint         arg_3,
                                                               guint        arg_4,
                                                               guint        arg_5,
                                                               gpointer     data2);
  GMarshalFunc_VOID__INT_STRING_INT_UINT_UINT callback;
  GCClosure *cc = (GCClosure*) closure;
  gpointer data1, data2;

  g_return_if_fail (n_param_values == 6);

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = g_value_peek_pointer (param_values + 0);
    }
  else
    {
      data1 = g_value_peek_pointer (param_values + 0);
      data2 = closure->data;
    }
  callback = (GMarshalFunc_VOID__INT_STRING_INT_UINT_UINT) (marshal_data ? marshal_data : cc->callback);

  callback (data1,
            g_marshal_value_peek_int (param_values + 1),
            g_marshal_value_peek_string (param_values + 2),
            g_marshal_value_peek_int (param_values + 3),
            g_marshal_value_peek_uint (param_values + 4),
            g_marshal_value_peek_uint (param_values + 5),
            data2);
}

/* VOID:INT,STRING,STRING,STRING,STRING (closures.def:13) */
void
g_cclosure_user_marshal_VOID__INT_STRING_STRING_STRING_STRING (GClosure     *closure,
                                                               GValue       *return_value G_GNUC_UNUSED,
                                                               guint         n_param_values,
                                                               const GValue *param_values,
                                                               gpointer      invocation_hint G_GNUC_UNUSED,
                                                               gpointer      marshal_data)
{
  typedef void (*GMarshalFunc_VOID__INT_STRING_STRING_STRING_STRING) (gpointer     data1,
                                                                      gint         arg_1,
                                                                      gpointer     arg_2,
                                                                      gpointer     arg_3,
                                                                      gpointer     arg_4,
                                                                      gpointer     arg_5,
                                                                      gpointer     data2);
  GMarshalFunc_VOID__INT_STRING_STRING_STRING_STRING callback;
  GCClosure *cc = (GCClosure*) closure;
  gpointer data1, data2;

  g_return_if_fail (n_param_values == 6);

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = g_value_peek_pointer (param_values + 0);
    }
  else
    {
      data1 = g_value_peek_pointer (param_values + 0);
      data2 = closure->data;
    }
  callback = (GMarshalFunc_VOID__INT_STRING_STRING_STRING_STRING) (marshal_data ? marshal_data : cc->callback);

  callback (data1,
            g_marshal_value_peek_int (param_values + 1),
            g_marshal_value_peek_string (param_values + 2),
            g_marshal_value_peek_string (param_values + 3),
            g_marshal_value_peek_string (param_values + 4),
            g_marshal_value_peek_string (param_values + 5),
            data2);
}

/* VOID:STRING,STRING,STRING (closures.def:14) */
void
g_cclosure_user_marshal_VOID__STRING_STRING_STRING (GClosure     *closure,
                                                    GValue       *return_value G_GNUC_UNUSED,
                                                    guint         n_param_values,
                                                    const GValue *param_values,
                                                    gpointer      invocation_hint G_GNUC_UNUSED,
                                                    gpointer      marshal_data)
{
  typedef void (*GMarshalFunc_VOID__STRING_STRING_STRING) (gpointer     data1,
                                                           gpointer     arg_1,
                                                           gpointer     arg_2,
                                                           gpointer     arg_3,
                                                           gpointer     data2);
  GMarshalFunc_VOID__STRING_STRING_STRING callback;
  GCClosure *cc = (GCClosure*) closure;
  gpointer data1, data2;

  g_return_if_fail (n_param_values == 4);

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = g_value_peek_pointer (param_values + 0);
    }
  else
    {
      data1 = g_value_peek_pointer (param_values + 0);
      data2 = closure->data;
    }
  callback = (GMarshalFunc_VOID__STRING_STRING_STRING) (marshal_data ? marshal_data : cc->callback);

  callback (data1,
            g_marshal_value_peek_string (param_values + 1),
            g_marshal_value_peek_string (param_values + 2),
            g_marshal_value_peek_string (param_values + 3),
            data2);
}

/* VOID:UINT,INT,INT,STRING,STRING (closures.def:15) */
void
g_cclosure_user_marshal_VOID__UINT_INT_INT_STRING_STRING (GClosure     *closure,
                                                          GValue       *return_value G_GNUC_UNUSED,
                                                          guint         n_param_values,
                                                          const GValue *param_values,
                                                          gpointer      invocation_hint G_GNUC_UNUSED,
                                                          gpointer      marshal_data)
{
  typedef void (*GMarshalFunc_VOID__UINT_INT_INT_STRING_STRING) (gpointer     data1,
                                                                 guint        arg_1,
                                                                 gint         arg_2,
                                                                 gint         arg_3,
                                                                 gpointer     arg_4,
                                                                 gpointer     arg_5,
                                                                 gpointer     data2);
  GMarshalFunc_VOID__UINT_INT_INT_STRING_STRING callback;
  GCClosure *cc = (GCClosure*) closure;
  gpointer data1, data2;

  g_return_if_fail (n_param_values == 6);

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = g_value_peek_pointer (param_values + 0);
    }
  else
    {
      data1 = g_value_peek_pointer (param_values + 0);
      data2 = closure->data;
    }
  callback = (GMarshalFunc_VOID__UINT_INT_INT_STRING_STRING) (marshal_data ? marshal_data : cc->callback);

  callback (data1,
            g_marshal_value_peek_uint (param_values + 1),
            g_marshal_value_peek_int (param_values + 2),
            g_marshal_value_peek_int (param_values + 3),
            g_marshal_value_peek_string (param_values + 4),
            g_marshal_value_peek_string (param_values + 5),
            data2);
}

