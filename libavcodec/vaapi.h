/*
 * Video Acceleration API (shared data between FFmpeg and the video player)
 * HW decode acceleration for MPEG-2, MPEG-4, H.264 and VC-1
 *
 * Copyright (C) 2008-2009 Splitted-Desktop Systems
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

#ifndef AVCODEC_VAAPI_H
#define AVCODEC_VAAPI_H

/**
 * @file
 * @ingroup lavc_codec_hwaccel_vaapi
 * Public libavcodec VA API header.
 */

#include <stdint.h>
#include <libavutil/attributes.h>
#include <va/va.h>
#include "version.h"
#include "avcodec.h"

/**
 * @defgroup lavc_codec_hwaccel_vaapi VA API Decoding
 * @ingroup lavc_codec_hwaccel
 * @{
 */

/**
 * This structure is used to share data between the FFmpeg library and
 * the client video application.
 * This shall be zero-allocated and available as
 * AVCodecContext.hwaccel_context. All user members can be set once
 * during initialization or through each AVCodecContext.get_buffer()
 * function call. In any case, they must be valid prior to calling
 * decoding functions.
 *
 * This structure is deprecated. Please refer to config options and
 * associated accessors: av_vaapi_set_display(), av_vaapi_set_config_int(),
 * av_vaapi_set_config().
 */
#if FF_API_VAAPI_CONTEXT
struct vaapi_context {
    /**
     * Window system dependent data
     *
     * - encoding: unused
     * - decoding: Set by user
     */
    attribute_deprecated
    void *display;

    /**
     * Configuration ID
     *
     * - encoding: unused
     * - decoding: Set by user
     */
    attribute_deprecated
    uint32_t config_id;

    /**
     * Context ID (video decode pipeline)
     *
     * - encoding: unused
     * - decoding: Set by user
     */
    attribute_deprecated
    uint32_t context_id;

    /**
     * VAPictureParameterBuffer ID
     *
     * - encoding: unused
     * - decoding: Set by libavcodec
     */
    attribute_deprecated
    uint32_t pic_param_buf_id;

    /**
     * VAIQMatrixBuffer ID
     *
     * - encoding: unused
     * - decoding: Set by libavcodec
     */
    attribute_deprecated
    uint32_t iq_matrix_buf_id;

    /**
     * VABitPlaneBuffer ID (for VC-1 decoding)
     *
     * - encoding: unused
     * - decoding: Set by libavcodec
     */
    attribute_deprecated
    uint32_t bitplane_buf_id;

    /**
     * Slice parameter/data buffer IDs
     *
     * - encoding: unused
     * - decoding: Set by libavcodec
     */
    attribute_deprecated
    uint32_t *slice_buf_ids;

    /**
     * Number of effective slice buffer IDs to send to the HW
     *
     * - encoding: unused
     * - decoding: Set by libavcodec
     */
    attribute_deprecated
    unsigned int n_slice_buf_ids;

    /**
     * Size of pre-allocated slice_buf_ids
     *
     * - encoding: unused
     * - decoding: Set by libavcodec
     */
    attribute_deprecated
    unsigned int slice_buf_ids_alloc;

    /**
     * Pointer to VASliceParameterBuffers
     *
     * - encoding: unused
     * - decoding: Set by libavcodec
     */
    attribute_deprecated
    void *slice_params;

    /**
     * Size of a VASliceParameterBuffer element
     *
     * - encoding: unused
     * - decoding: Set by libavcodec
     */
    attribute_deprecated
    unsigned int slice_param_size;

    /**
     * Size of pre-allocated slice_params
     *
     * - encoding: unused
     * - decoding: Set by libavcodec
     */
    attribute_deprecated
    unsigned int slice_params_alloc;

    /**
     * Number of slices currently filled in
     *
     * - encoding: unused
     * - decoding: Set by libavcodec
     */
    attribute_deprecated
    unsigned int slice_count;

    /**
     * Pointer to slice data buffer base
     * - encoding: unused
     * - decoding: Set by libavcodec
     */
    attribute_deprecated
    const uint8_t *slice_data;

    /**
     * Current size of slice data
     *
     * - encoding: unused
     * - decoding: Set by libavcodec
     */
    attribute_deprecated
    uint32_t slice_data_size;
};
#endif

/** @name VA-API pipeline config options */
/**@{*/
/** @brief VA display (pointer) */
#define AV_VAAPI_CONFIG_OPTION_DISPLAY          "display"
/** @brief VA configuration id (uint32_t) */
#define AV_VAAPI_CONFIG_OPTION_CONFIG           "config"
/** @brief VA context id (uint32_t) */
#define AV_VAAPI_CONFIG_OPTION_CONTEXT          "context"
/**@}*/

/**
 * Binds a user supplied VA display to a codec context
 *
 * This function binds the supplied VA @a display to a codec context
 * @a avctx. The user retains full ownership of the display, and thus
 * shall ensure the VA-API subsystem was initialized with vaInitialize(),
 * make due diligence to keep it live until it is no longer needed,
 * and dispose the associated resources with vaTerminate() whenever
 * appropriate.
 *
 * @note This function has no effect if it is called outside of an
 * AVCodecContext.get_format() hook.
 *
 * @param[in] avctx the codec context being used for decoding the stream
 * @param[in] display the VA display handle to use for decoding
 * @return 0 on success, an AVERROR code on failure.
 */
int av_vaapi_set_display(AVCodecContext *avctx, VADisplay display);

/**
 * Configures the VA-API decoder with a key/value pair, overwriting a
 * previous entry.
 *
 * @note This function has no effect if it is called outside of an
 * AVCodecContext.get_format() hook.
 *
 * @param[in] avctx the codec context being used for decoding the stream
 * @param[in] key config option key (string)
 * @param[in] value config option value (string). Passing a NULL value
 *   will cause an existing entry to be deleted.
 * @return 0 on success, an AVERROR code on failure.
 */
int av_vaapi_set_config(AVCodecContext *avctx, const char *key,
                        const char *value);

/**
 * Configures the VA-API decoder with a key/value pair, overwriting a
 *   previous entry.
 *
 * @note This function has no effect if it is called outside of an
 * AVCodecContext.get_format() hook.
 *
 * @param[in] avctx the codec context being used for decoding the stream
 * @param[in] key config option key (string)
 * @param[in] value config option value (int)
 * @return 0 on success, an AVERROR code on failure.
 */
int av_vaapi_set_config_int(AVCodecContext *avctx, const char *key,
                            int64_t value);

/* @} */

#endif /* AVCODEC_VAAPI_H */
