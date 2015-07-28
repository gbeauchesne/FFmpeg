/*
 * vaapi_utils.h - Video Acceleration API (VA-API) utilities
 *
 * Copyright (C) 2013-2015 Intel Corporation
 *   Author: Gwenole Beauchesne <gwenole.beauchesne@intel.com>
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef AVCODEC_VAAPI_UTILS_H
#define AVCODEC_VAAPI_UTILS_H

#include <va/va.h>
#include "avcodec.h"
#include "libavutil/pixfmt.h"

/** Converts VA status to an FFmpeg error code */
int
ff_vaapi_get_error(VAStatus status);

/** Retrieves all supported profiles */
int
ff_vaapi_get_profiles(VADisplay display, VAProfile **profiles_ptr,
    unsigned int *n_profiles_ptr);

/** Retrieves all supported entrypoints for the supplied profile */
int
ff_vaapi_get_entrypoints(VADisplay display, VAProfile profile,
    VAEntrypoint **entrypoints_ptr, unsigned int *n_entrypoints_ptr);

/** Converts FFmpeg pixel format to VA chroma format */
int
ff_vaapi_get_chroma_format(enum AVPixelFormat pix_fmt, unsigned int *format_ptr);

/** Converts FFmpeg pixel format to VA fourcc */
int
ff_vaapi_get_pixel_format(enum AVPixelFormat pix_fmt, uint32_t *fourcc_ptr);

#endif /* AVCODEC_VAAPI_UTILS_H */
