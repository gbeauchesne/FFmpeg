/*
 * Video Acceleration API (video decoding)
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

#include "libavutil/log.h"
#include "mpegvideo.h"
#include "vaapi_internal.h"

/**
 * @addtogroup VAAPI_Decoding
 *
 * @{
 */

static void destroy_buffers(VADisplay display, VABufferID *buffers, unsigned int n_buffers)
{
    unsigned int i;
    for (i = 0; i < n_buffers; i++) {
        if (buffers[i] != VA_INVALID_ID) {
            vaDestroyBuffer(display, buffers[i]);
            buffers[i] = VA_INVALID_ID;
        }
    }
}

/** @name VA-API pipeline parameters (internal) */
/**@{*/
/** Pipeline configuration flags (AV_HWACCEL_FLAG_*|AV_VAAPI_PIPELINE_FLAG_*) */
#define AV_VAAPI_PIPELINE_PARAM_FLAGS           "flags"
/** User-supplied VA display handle */
#define AV_VAAPI_PIPELINE_PARAM_DISPLAY         "display"
/**@}*/

#define OFFSET(x) offsetof(FFVAContext, x)
static const AVOption FFVAContextOptions[] = {
    { AV_VAAPI_PIPELINE_PARAM_FLAGS, "flags", OFFSET(flags),
      AV_OPT_TYPE_INT, { .i64 = 0 }, 0, UINT32_MAX },
    { AV_VAAPI_PIPELINE_PARAM_DISPLAY, "VA display", OFFSET(user_display),
      AV_OPT_TYPE_INT64, { .i64 = 0 }, 0, UINTPTR_MAX },
    { AV_VAAPI_PIPELINE_PARAM_CONTEXT, "VA context id", OFFSET(user_context_id),
      AV_OPT_TYPE_INT, { .i64 = VA_INVALID_ID }, 0, UINT32_MAX },
    { NULL, }
};
#undef OFFSET

static const AVClass FFVAContextClass = {
    .class_name = "FFVAContext",
    .item_name  = av_default_item_name,
    .option     = FFVAContextOptions,
    .version    = LIBAVUTIL_VERSION_INT,
};

int av_vaapi_set_pipeline_params(AVCodecContext *avctx, VADisplay display,
                                 uint32_t flags, AVDictionary **params)
{
    AVDictionary ** const hwaccel_params = &avctx->internal->hwaccel_config;
    int ret;

    /* av_vaapi_set_pipeline_params() is only allowed to be called
       from within an AVCodecContext.get_format() hook for now. In such
       case, hwaccel is NULL. See ff_get_format() for details */
    if (avctx->hwaccel) {
        av_log(avctx, AV_LOG_ERROR, "Invalid call point.\n");
        return AVERROR(ENOTSUP);
    }

    if (!display) {
        av_log(avctx, AV_LOG_ERROR, "No valid VA display supplied.\n");
        return AVERROR(EINVAL);
    }

    if (params && *params)
        av_dict_copy(hwaccel_params, *params, 0);

    ret = av_dict_set_int(hwaccel_params, AV_VAAPI_PIPELINE_PARAM_FLAGS,
        flags, 0);
    if (ret != 0)
        return ret;

    ret = av_dict_set_int(hwaccel_params, AV_VAAPI_PIPELINE_PARAM_DISPLAY,
        (int64_t)(intptr_t)display, 0);
    if (ret != 0)
        return ret;
    return 0;
}

int ff_vaapi_context_init(AVCodecContext *avctx)
{
    FFVAContext * const vactx = ff_vaapi_get_context(avctx);
    int ret;

    vactx->klass = &FFVAContextClass;
    av_opt_set_defaults(vactx);

#if FF_API_VAAPI_CONTEXT
FF_DISABLE_DEPRECATION_WARNINGS
    if (avctx->hwaccel_context) {
        const struct vaapi_context * const user_vactx = avctx->hwaccel_context;
        vactx->user_display     = (uintptr_t)user_vactx->display;
        vactx->user_context_id  = user_vactx->context_id;
    }
FF_ENABLE_DEPRECATION_WARNINGS
#endif

    vactx->context_id           = VA_INVALID_ID;
    vactx->pic_param_buf_id     = VA_INVALID_ID;
    vactx->iq_matrix_buf_id     = VA_INVALID_ID;
    vactx->bitplane_buf_id      = VA_INVALID_ID;

    ret = av_opt_set_dict(vactx, &avctx->internal->hwaccel_config);
    if (ret != 0)
        return ret;

    vactx->display              = (void *)(uintptr_t)vactx->user_display;
    vactx->context_id           = vactx->user_context_id;

    if (!vactx->display) {
        av_log(avctx, AV_LOG_ERROR, "No valid VA display found.\n");
        return AVERROR(EINVAL);
    }
    return 0;
}

int ff_vaapi_context_fini(AVCodecContext *avctx)
{
    return 0;
}

int ff_vaapi_render_picture(FFVAContext *vactx, VASurfaceID surface)
{
    VABufferID va_buffers[3];
    unsigned int n_va_buffers = 0;

    if (vactx->pic_param_buf_id == VA_INVALID_ID)
        return 0;

    vaUnmapBuffer(vactx->display, vactx->pic_param_buf_id);
    va_buffers[n_va_buffers++] = vactx->pic_param_buf_id;

    if (vactx->iq_matrix_buf_id != VA_INVALID_ID) {
        vaUnmapBuffer(vactx->display, vactx->iq_matrix_buf_id);
        va_buffers[n_va_buffers++] = vactx->iq_matrix_buf_id;
    }

    if (vactx->bitplane_buf_id != VA_INVALID_ID) {
        vaUnmapBuffer(vactx->display, vactx->bitplane_buf_id);
        va_buffers[n_va_buffers++] = vactx->bitplane_buf_id;
    }

    if (vaBeginPicture(vactx->display, vactx->context_id,
                       surface) != VA_STATUS_SUCCESS)
        return -1;

    if (vaRenderPicture(vactx->display, vactx->context_id,
                        va_buffers, n_va_buffers) != VA_STATUS_SUCCESS)
        return -1;

    if (vaRenderPicture(vactx->display, vactx->context_id,
                        vactx->slice_buf_ids,
                        vactx->n_slice_buf_ids) != VA_STATUS_SUCCESS)
        return -1;

    if (vaEndPicture(vactx->display, vactx->context_id) != VA_STATUS_SUCCESS)
        return -1;

    return 0;
}

int ff_vaapi_commit_slices(FFVAContext *vactx)
{
    VABufferID *slice_buf_ids;
    VABufferID slice_param_buf_id, slice_data_buf_id;

    if (vactx->slice_count == 0)
        return 0;

    slice_buf_ids =
        av_fast_realloc(vactx->slice_buf_ids,
                        &vactx->slice_buf_ids_alloc,
                        (vactx->n_slice_buf_ids + 2) * sizeof(slice_buf_ids[0]));
    if (!slice_buf_ids)
        return -1;
    vactx->slice_buf_ids = slice_buf_ids;

    slice_param_buf_id = VA_INVALID_ID;
    if (vaCreateBuffer(vactx->display, vactx->context_id,
                       VASliceParameterBufferType,
                       vactx->slice_param_size,
                       vactx->slice_count, vactx->slice_params,
                       &slice_param_buf_id) != VA_STATUS_SUCCESS)
        return -1;
    vactx->slice_count = 0;

    slice_data_buf_id = VA_INVALID_ID;
    if (vaCreateBuffer(vactx->display, vactx->context_id,
                       VASliceDataBufferType,
                       vactx->slice_data_size,
                       1, (void *)vactx->slice_data,
                       &slice_data_buf_id) != VA_STATUS_SUCCESS)
        return -1;
    vactx->slice_data = NULL;
    vactx->slice_data_size = 0;

    slice_buf_ids[vactx->n_slice_buf_ids++] = slice_param_buf_id;
    slice_buf_ids[vactx->n_slice_buf_ids++] = slice_data_buf_id;
    return 0;
}

static void *alloc_buffer(FFVAContext *vactx, int type, unsigned int size, uint32_t *buf_id)
{
    void *data = NULL;

    *buf_id = VA_INVALID_ID;
    if (vaCreateBuffer(vactx->display, vactx->context_id,
                       type, size, 1, NULL, buf_id) == VA_STATUS_SUCCESS)
        vaMapBuffer(vactx->display, *buf_id, &data);

    return data;
}

void *ff_vaapi_alloc_pic_param(FFVAContext *vactx, unsigned int size)
{
    return alloc_buffer(vactx, VAPictureParameterBufferType, size, &vactx->pic_param_buf_id);
}

void *ff_vaapi_alloc_iq_matrix(FFVAContext *vactx, unsigned int size)
{
    return alloc_buffer(vactx, VAIQMatrixBufferType, size, &vactx->iq_matrix_buf_id);
}

uint8_t *ff_vaapi_alloc_bitplane(FFVAContext *vactx, uint32_t size)
{
    return alloc_buffer(vactx, VABitPlaneBufferType, size, &vactx->bitplane_buf_id);
}

VASliceParameterBufferBase *ff_vaapi_alloc_slice(FFVAContext *vactx, const uint8_t *buffer, uint32_t size)
{
    uint8_t *slice_params;
    VASliceParameterBufferBase *slice_param;

    if (!vactx->slice_data)
        vactx->slice_data = buffer;
    if (vactx->slice_data + vactx->slice_data_size != buffer) {
        if (ff_vaapi_commit_slices(vactx) < 0)
            return NULL;
        vactx->slice_data = buffer;
    }

    slice_params =
        av_fast_realloc(vactx->slice_params,
                        &vactx->slice_params_alloc,
                        (vactx->slice_count + 1) * vactx->slice_param_size);
    if (!slice_params)
        return NULL;
    vactx->slice_params = slice_params;

    slice_param = (VASliceParameterBufferBase *)(slice_params + vactx->slice_count * vactx->slice_param_size);
    slice_param->slice_data_size   = size;
    slice_param->slice_data_offset = vactx->slice_data_size;
    slice_param->slice_data_flag   = VA_SLICE_DATA_FLAG_ALL;

    vactx->slice_count++;
    vactx->slice_data_size += size;
    return slice_param;
}

void ff_vaapi_common_end_frame(AVCodecContext *avctx)
{
    FFVAContext * const vactx = ff_vaapi_get_context(avctx);

    ff_dlog(avctx, "ff_vaapi_common_end_frame()\n");

    destroy_buffers(vactx->display, &vactx->pic_param_buf_id, 1);
    destroy_buffers(vactx->display, &vactx->iq_matrix_buf_id, 1);
    destroy_buffers(vactx->display, &vactx->bitplane_buf_id, 1);
    destroy_buffers(vactx->display, vactx->slice_buf_ids, vactx->n_slice_buf_ids);
    av_freep(&vactx->slice_buf_ids);
    av_freep(&vactx->slice_params);
    vactx->n_slice_buf_ids     = 0;
    vactx->slice_buf_ids_alloc = 0;
    vactx->slice_count         = 0;
    vactx->slice_params_alloc  = 0;
}

#if CONFIG_H263_VAAPI_HWACCEL  || CONFIG_MPEG1_VAAPI_HWACCEL || \
    CONFIG_MPEG2_VAAPI_HWACCEL || CONFIG_MPEG4_VAAPI_HWACCEL || \
    CONFIG_VC1_VAAPI_HWACCEL   || CONFIG_WMV3_VAAPI_HWACCEL
int ff_vaapi_mpeg_end_frame(AVCodecContext *avctx)
{
    FFVAContext * const vactx = ff_vaapi_get_context(avctx);
    MpegEncContext *s = avctx->priv_data;
    int ret;

    ret = ff_vaapi_commit_slices(vactx);
    if (ret < 0)
        goto finish;

    ret = ff_vaapi_render_picture(vactx,
                                  ff_vaapi_get_surface_id(s->current_picture_ptr->f));
    if (ret < 0)
        goto finish;

    ff_mpeg_draw_horiz_band(s, 0, s->avctx->height);

finish:
    ff_vaapi_common_end_frame(avctx);
    return ret;
}
#endif

/* @} */
