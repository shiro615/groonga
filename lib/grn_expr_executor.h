/* -*- c-basic-offset: 2 -*- */
/*
  Copyright(C) 2017-2018 Brazil

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

#pragma once

#include "grn_db.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _grn_expr_executor grn_expr_executor;

typedef union {
  struct {
    grn_obj *value;
  } constant;
  struct {
    grn_obj *column;
    grn_obj value_buffer;
  } value;
  struct {
    grn_obj result_buffer;
    void *regex;
    grn_obj value_buffer;
    grn_obj *normalizer;
  } simple_regexp;
  struct {
    grn_proc_ctx proc_ctx;
    int n_args;
  } proc;
  struct {
    grn_obj result_buffer;
  } simple_condition_constant;
  struct {
    grn_obj result_buffer;
    grn_ra *ra;
    grn_ra_cache ra_cache;
    unsigned int ra_element_size;
    grn_obj value_buffer;
    grn_obj constant_buffer;
    grn_operator_exec_func *exec;
  } simple_condition_ra;
  struct {
    grn_bool need_exec;
    grn_obj result_buffer;
    grn_obj value_buffer;
    grn_obj constant_buffer;
    grn_operator_exec_func *exec;
  } simple_condition;
} grn_expr_executor_data;

typedef grn_obj *(*grn_expr_executor_exec_func)(grn_ctx *ctx,
                                                grn_expr_executor *executor,
                                                grn_id id);
typedef void (*grn_expr_executor_fin_func)(grn_ctx *ctx,
                                           grn_expr_executor *executor);

struct _grn_expr_executor {
  grn_obj *expr;
  grn_obj *variable;
  grn_expr_executor_exec_func exec;
  grn_expr_executor_fin_func fin;
  grn_expr_executor_data data;
};

grn_rc
grn_expr_executor_init(grn_ctx *ctx,
                       grn_expr_executor *executor,
                       grn_obj *expr);
grn_rc
grn_expr_executor_fin(grn_ctx *ctx,
                      grn_expr_executor *executor);

grn_expr_executor *
grn_expr_executor_open(grn_ctx *ctx,
                       grn_obj *expr);
grn_rc
grn_expr_executor_close(grn_ctx *ctx,
                        grn_expr_executor *executor);

grn_obj *
grn_expr_executor_exec(grn_ctx *ctx,
                       grn_expr_executor *executor,
                       grn_id id);

#ifdef __cplusplus
}
#endif
