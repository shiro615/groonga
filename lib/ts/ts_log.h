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

#ifndef GRN_TS_LOG_H
#define GRN_TS_LOG_H

#include "../grn.h"

#ifdef __cplusplus
extern "C" {
#endif

/* GRN_TS_DEBUG() logs a message that is useful for debug. */
#define GRN_TS_DEBUG(...) GRN_LOG(ctx, GRN_LOG_DEBUG, __VA_ARGS__)

/* GRN_TS_WARN() logs a warning. */
#define GRN_TS_WARN(rc, ...) WARN(rc, __VA_ARGS__)

/* GRN_TS_ERR() reports an error. */
#define GRN_TS_ERR(rc, ...) ERR(rc, __VA_ARGS__)

/* GRN_TS_ERR_RETURN() reports an error and returns its error code. */
#define GRN_TS_ERR_RETURN(rc, ...) do {\
  GRN_TS_ERR(rc, __VA_ARGS__);\
  return rc;\
} while (GRN_FALSE)

#ifdef __cplusplus
}
#endif

#endif /* GRN_TS_LOG_H */