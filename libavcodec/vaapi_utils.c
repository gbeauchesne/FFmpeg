/*
 * vaapi_utils.c - Video Acceleration API (VA-API) utilities
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

#include "vaapi_utils.h"

/* Converts VA status to an FFmpeg error code */
int
ff_vaapi_get_error(VAStatus status)
{
    int ret;

    switch (status) {
    case VA_STATUS_ERROR_OPERATION_FAILED:
        ret = AVERROR(ENOTSUP);
        break;
    case VA_STATUS_ERROR_INVALID_DISPLAY:
    case VA_STATUS_ERROR_INVALID_CONFIG:
    case VA_STATUS_ERROR_INVALID_CONTEXT:
    case VA_STATUS_ERROR_INVALID_SURFACE:
    case VA_STATUS_ERROR_INVALID_BUFFER:
    case VA_STATUS_ERROR_INVALID_IMAGE:
    case VA_STATUS_ERROR_INVALID_SUBPICTURE:
        ret = AVERROR(EINVAL);
        break;
    case VA_STATUS_ERROR_INVALID_PARAMETER:
    case VA_STATUS_ERROR_INVALID_VALUE:
        ret = AVERROR(EINVAL);
        break;
    case VA_STATUS_ERROR_ALLOCATION_FAILED:
        ret = AVERROR(ENOMEM);
        break;
    case VA_STATUS_ERROR_UNIMPLEMENTED:
        ret = AVERROR(ENOSYS);
        break;
    case VA_STATUS_ERROR_SURFACE_BUSY:
        ret = AVERROR(EBUSY);
        break;
    default:
        ret = AVERROR_UNKNOWN;
        break;
    }
    return ret;
}

/* Retrieves all supported profiles */
int
ff_vaapi_get_profiles(VADisplay display, VAProfile **profiles_ptr,
    unsigned int *n_profiles_ptr)
{
    VAStatus status;
    int n_profiles, ret;

    n_profiles = vaMaxNumProfiles(display);
    ret = av_reallocp_array(profiles_ptr, n_profiles, sizeof(VAProfile));
    if (ret != 0)
        return ret;

    status = vaQueryConfigProfiles(display, *profiles_ptr, &n_profiles);
    if (status != VA_STATUS_SUCCESS)
        return ff_vaapi_get_error(status);

    if (n_profiles_ptr)
        *n_profiles_ptr = n_profiles;
    return 0;
}

/* Retrieves all supported entrypoints for the supplied profile */
int
ff_vaapi_get_entrypoints(VADisplay display, VAProfile profile,
    VAEntrypoint **entrypoints_ptr, unsigned int *n_entrypoints_ptr)
{
    VAStatus status;
    int n_entrypoints, ret;

    n_entrypoints = vaMaxNumEntrypoints(display);
    ret = av_reallocp_array(entrypoints_ptr, n_entrypoints, sizeof(VAEntrypoint));
    if (ret != 0)
        return ret;

    status = vaQueryConfigEntrypoints(display, profile, *entrypoints_ptr,
        &n_entrypoints);
    if (status != VA_STATUS_SUCCESS)
        return ff_vaapi_get_error(status);

    if (n_entrypoints_ptr)
        *n_entrypoints_ptr = n_entrypoints;
    return 0;
}

/* Converts FFmpeg pixel format to VA chroma format */
int
ff_vaapi_get_chroma_format(enum AVPixelFormat pix_fmt, unsigned int *format_ptr)
{
    uint32_t format;

    switch (pix_fmt) {
    case AV_PIX_FMT_GRAY8:
        format = VA_RT_FORMAT_YUV400;
        break;
    case AV_PIX_FMT_YUV420P:
    case AV_PIX_FMT_NV12:
        format = VA_RT_FORMAT_YUV420;
        break;
    case AV_PIX_FMT_YUV422P:
    case AV_PIX_FMT_YUYV422:
    case AV_PIX_FMT_UYVY422:
        format = VA_RT_FORMAT_YUV422;
        break;
    case AV_PIX_FMT_YUV444P:
        format = VA_RT_FORMAT_YUV444;
        break;
    default:
        /* FIXME: fill out missing entries */
        return AVERROR(ENOSYS);
    }

    if (format_ptr)
        *format_ptr = format;
    return 0;
}

/* Converts FFmpeg pixel format to VA fourcc */
int
ff_vaapi_get_pixel_format(enum AVPixelFormat pix_fmt, uint32_t *fourcc_ptr)
{
    uint32_t fourcc;

    switch (pix_fmt) {
    case AV_PIX_FMT_GRAY8:
        fourcc = VA_FOURCC('Y','8','0','0');
        break;
    case AV_PIX_FMT_YUV420P:
        fourcc = VA_FOURCC('I','4','2','0');
        break;
    case AV_PIX_FMT_NV12:
        fourcc = VA_FOURCC('N','V','1','2');
        break;
    case AV_PIX_FMT_YUV422P:
        fourcc = VA_FOURCC('4','2','2','H');
        break;
    case AV_PIX_FMT_YUYV422:
        fourcc = VA_FOURCC('Y','U','Y','V');
        break;
    case AV_PIX_FMT_UYVY422:
        fourcc = VA_FOURCC('U','Y','V','Y');
        break;
    case AV_PIX_FMT_YUV444P:
        fourcc = VA_FOURCC('4','4','4','P');
        break;
    default:
        /* FIXME: fill out missing entries */
        return AVERROR(ENOSYS);
    }

    if (fourcc_ptr)
        *fourcc_ptr = fourcc;
    return 0;
}
