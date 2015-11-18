/* -*- c-basic-offset: 2 -*- */
/*
  Copyright(C) 2015 Brazil

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License version 2.1 as published by the Free Software Foundation.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "grn.h"
#include "grn_db.h"
#include "grn_index_column.h"

grn_bool
grn_obj_is_builtin(grn_ctx *ctx, grn_obj *obj)
{
  grn_id id;

  if (!obj) { return GRN_FALSE; }

  id = grn_obj_id(ctx, obj);
  if (id == GRN_ID_NIL) {
    return GRN_FALSE;
  } else {
    return id < GRN_N_RESERVED_TYPES;
  }
}

grn_bool
grn_obj_is_table(grn_ctx *ctx, grn_obj *obj)
{
  grn_bool is_table = GRN_FALSE;

  if (!obj) {
    return GRN_FALSE;
  }

  switch (obj->header.type) {
  case GRN_TABLE_NO_KEY :
  case GRN_TABLE_HASH_KEY :
  case GRN_TABLE_PAT_KEY :
  case GRN_TABLE_DAT_KEY :
    is_table = GRN_TRUE;
  default :
    break;
  }

  return is_table;
}

grn_bool
grn_obj_is_accessor(grn_ctx *ctx, grn_obj *obj)
{
  if (!obj) {
    return GRN_FALSE;
  }

  return obj->header.type == GRN_ACCESSOR;
}

grn_bool
grn_obj_is_key_accessor(grn_ctx *ctx, grn_obj *obj)
{
  grn_accessor *accessor;

  if (!grn_obj_is_accessor(ctx, obj)) {
    return GRN_FALSE;
  }

  accessor = (grn_accessor *)obj;
  if (accessor->next) {
    return GRN_FALSE;
  }

  return accessor->action == GRN_ACCESSOR_GET_KEY;
}

grn_bool
grn_obj_is_type(grn_ctx *ctx, grn_obj *obj)
{
  if (!obj) {
    return GRN_FALSE;
  }

  return obj->header.type == GRN_TYPE;
}

grn_bool
grn_obj_is_proc(grn_ctx *ctx, grn_obj *obj)
{
  if (!obj) {
    return GRN_FALSE;
  }

  return obj->header.type == GRN_PROC;
}

grn_bool
grn_obj_is_tokenizer_proc(grn_ctx *ctx, grn_obj *obj)
{
  grn_proc *proc;

  if (!grn_obj_is_proc(ctx, obj)) {
    return GRN_FALSE;
  }

  proc = (grn_proc *)obj;
  return proc->type == GRN_PROC_TOKENIZER;
}

grn_bool
grn_obj_is_function_proc(grn_ctx *ctx, grn_obj *obj)
{
  grn_proc *proc;

  if (!grn_obj_is_proc(ctx, obj)) {
    return GRN_FALSE;
  }

  proc = (grn_proc *)obj;
  return proc->type == GRN_PROC_FUNCTION;
}

grn_bool
grn_obj_is_selector_proc(grn_ctx *ctx, grn_obj *obj)
{
  grn_proc *proc;

  if (!grn_obj_is_function_proc(ctx, obj)) {
    return GRN_FALSE;
  }

  proc = (grn_proc *)obj;
  return proc->selector != NULL;
}

grn_bool
grn_obj_is_selector_only_proc(grn_ctx *ctx, grn_obj *obj)
{
  grn_proc *proc;

  if (!grn_obj_is_selector_proc(ctx, obj)) {
    return GRN_FALSE;
  }

  proc = (grn_proc *)obj;
  return proc->funcs[PROC_INIT] == NULL;
}

grn_bool
grn_obj_is_normalizer_proc(grn_ctx *ctx, grn_obj *obj)
{
  grn_proc *proc;

  if (!grn_obj_is_proc(ctx, obj)) {
    return GRN_FALSE;
  }

  proc = (grn_proc *)obj;
  return proc->type == GRN_PROC_NORMALIZER;
}

grn_bool
grn_obj_is_token_filter_proc(grn_ctx *ctx, grn_obj *obj)
{
  grn_proc *proc;

  if (!grn_obj_is_proc(ctx, obj)) {
    return GRN_FALSE;
  }

  proc = (grn_proc *)obj;
  return proc->type == GRN_PROC_TOKEN_FILTER;
}

grn_bool
grn_obj_is_scorer_proc(grn_ctx *ctx, grn_obj *obj)
{
  grn_proc *proc;

  if (!grn_obj_is_proc(ctx, obj)) {
    return GRN_FALSE;
  }

  proc = (grn_proc *)obj;
  return proc->type == GRN_PROC_SCORER;
}

static void
grn_db_reindex(grn_ctx *ctx, grn_obj *db)
{
  grn_table_cursor *cursor;
  grn_id id;

  cursor = grn_table_cursor_open(ctx, db,
                                 NULL, 0, NULL, 0,
                                 0, -1,
                                 GRN_CURSOR_BY_ID);
  if (!cursor) {
    return;
  }

  while ((id = grn_table_cursor_next(ctx, cursor)) != GRN_ID_NIL) {
    grn_obj *object;

    object = grn_ctx_at(ctx, id);
    if (!object) {
      ERRCLR(ctx);
      continue;
    }

    switch (object->header.type) {
    case GRN_TABLE_HASH_KEY :
    case GRN_TABLE_PAT_KEY :
    case GRN_TABLE_DAT_KEY :
      grn_obj_reindex(ctx, object);
      break;
    default:
      break;
    }

    grn_obj_unlink(ctx, object);

    if (ctx->rc != GRN_SUCCESS) {
      break;
    }
  }
  grn_table_cursor_close(ctx, cursor);
}

static void
grn_table_reindex(grn_ctx *ctx, grn_obj *table)
{
  grn_hash *columns;

  columns = grn_hash_create(ctx, NULL, sizeof(grn_id), 0,
                            GRN_OBJ_TABLE_HASH_KEY | GRN_HASH_TINY);
  if (!columns) {
    ERR(GRN_NO_MEMORY_AVAILABLE,
        "[table][reindex] failed to create a table to store columns");
    return;
  }

  if (grn_table_columns(ctx, table, "", 0, (grn_obj *)columns) > 0) {
    grn_bool have_data_column = GRN_FALSE;
    grn_id *key;
    GRN_HASH_EACH(ctx, columns, id, &key, NULL, NULL, {
      grn_obj *column = grn_ctx_at(ctx, *key);
      if (column && column->header.type != GRN_COLUMN_INDEX) {
        have_data_column = GRN_TRUE;
        break;
      }
    });
    if (!have_data_column) {
      grn_table_truncate(ctx, table);
    }
    GRN_HASH_EACH(ctx, columns, id, &key, NULL, NULL, {
      grn_obj *column = grn_ctx_at(ctx, *key);
      if (column && column->header.type == GRN_COLUMN_INDEX) {
        grn_obj_reindex(ctx, column);
      }
    });
  }
  grn_hash_close(ctx, columns);
}

static void
grn_data_column_reindex(grn_ctx *ctx, grn_obj *data_column)
{
  grn_hook *hooks;

  for (hooks = DB_OBJ(data_column)->hooks[GRN_HOOK_SET];
       hooks;
       hooks = hooks->next) {
    grn_obj_default_set_value_hook_data *data = (void *)GRN_NEXT_ADDR(hooks);
    grn_obj *target = grn_ctx_at(ctx, data->target);
    if (target->header.type != GRN_COLUMN_INDEX) {
      continue;
    }
    grn_obj_reindex(ctx, target);
    if (ctx->rc != GRN_SUCCESS) {
      break;
    }
  }
}

grn_rc
grn_obj_reindex(grn_ctx *ctx, grn_obj *obj)
{
  GRN_API_ENTER;

  if (!obj) {
    ERR(GRN_INVALID_ARGUMENT, "[object][reindex] object must not be NULL");
    GRN_API_RETURN(ctx->rc);
  }

  switch (obj->header.type) {
  case GRN_DB :
    grn_db_reindex(ctx, obj);
    break;
  case GRN_TABLE_HASH_KEY :
  case GRN_TABLE_PAT_KEY :
  case GRN_TABLE_DAT_KEY :
    grn_table_reindex(ctx, obj);
    break;
  case GRN_COLUMN_FIX_SIZE :
  case GRN_COLUMN_VAR_SIZE :
    grn_data_column_reindex(ctx, obj);
    break;
  case GRN_COLUMN_INDEX :
    grn_index_column_rebuild(ctx, obj);
    break;
  default :
    {
      grn_obj type_name;
      GRN_TEXT_INIT(&type_name, 0);
      grn_inspect_type(ctx, &type_name, obj->header.type);
      ERR(GRN_INVALID_ARGUMENT,
          "[object][reindex] object must be TABLE_HASH_KEY, "
          "TABLE_PAT_KEY, TABLE_DAT_KEY or COLUMN_INDEX: <%.*s>",
          (int)GRN_TEXT_LEN(&type_name),
          GRN_TEXT_VALUE(&type_name));
      GRN_OBJ_FIN(ctx, &type_name);
      GRN_API_RETURN(ctx->rc);
    }
    break;
  }

  GRN_API_RETURN(ctx->rc);
}
