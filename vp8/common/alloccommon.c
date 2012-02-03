/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */


#include "vpx_ports/config.h"
#include "blockd.h"
#include "vpx_mem/vpx_mem.h"
#include "onyxc_int.h"
#include "findnearmv.h"
#include "entropymode.h"
#include "systemdependent.h"


extern  void vp8_init_scan_order_mask();

static void update_mode_info_border( VP8_COMMON *cpi, MODE_INFO *mi_base )
{
    int stride = cpi->mode_info_stride;
    int i;

    // Clear down top border row
    vpx_memset(mi_base, 0, sizeof(MODE_INFO) * cpi->mode_info_stride);

    // Clear left border column
    for (i = 1; i < cpi->mb_rows+1; i++)
    {
        vpx_memset(&mi_base[i*stride], 0, sizeof(MODE_INFO));
    }
}
static void update_mode_info_in_image( VP8_COMMON *cpi, MODE_INFO *mi )
{
    int stride = cpi->mode_info_stride;
    int rows = cpi->mb_rows;
    int cols = cpi->mb_cols;
    int i, j;

    // For each in image mode_info element set the in image flag to 1
    for (i = 0; i < cpi->mb_rows; i++)
    {
        for (j = 0; j < cpi->mb_cols; j++)
        {
            mi->mbmi.mb_in_image = 1;
            mi++;   // Next element in the row
        }

        mi++;       // Step over border element at start of next row
    }
}

void vp8_de_alloc_frame_buffers(VP8_COMMON *oci)
{
    int i;

    for (i = 0; i < NUM_YV12_BUFFERS; i++)
        vp8_yv12_de_alloc_frame_buffer(&oci->yv12_fb[i]);

    vp8_yv12_de_alloc_frame_buffer(&oci->temp_scale_frame);
    vp8_yv12_de_alloc_frame_buffer(&oci->post_proc_buffer);

    vpx_free(oci->above_context);
    vpx_free(oci->mip);
    vpx_free(oci->prev_mip);

    oci->above_context = 0;
    oci->mip = 0;
    oci->prev_mip = 0;

}

int vp8_alloc_frame_buffers(VP8_COMMON *oci, int width, int height)
{
    int i;

    vp8_de_alloc_frame_buffers(oci);

    /* our internal buffers are always multiples of 16 */
    if ((width & 0xf) != 0)
        width += 16 - (width & 0xf);

    if ((height & 0xf) != 0)
        height += 16 - (height & 0xf);


    for (i = 0; i < NUM_YV12_BUFFERS; i++)
    {
        oci->fb_idx_ref_cnt[i] = 0;
        oci->yv12_fb[i].flags = 0;
        if (vp8_yv12_alloc_frame_buffer(&oci->yv12_fb[i], width, height, VP8BORDERINPIXELS) < 0)
        {
            vp8_de_alloc_frame_buffers(oci);
            return 1;
        }
    }

    oci->new_fb_idx = 0;
    oci->lst_fb_idx = 1;
    oci->gld_fb_idx = 2;
    oci->alt_fb_idx = 3;

    oci->fb_idx_ref_cnt[0] = 1;
    oci->fb_idx_ref_cnt[1] = 1;
    oci->fb_idx_ref_cnt[2] = 1;
    oci->fb_idx_ref_cnt[3] = 1;

    if (vp8_yv12_alloc_frame_buffer(&oci->temp_scale_frame,   width, 16, VP8BORDERINPIXELS) < 0)
    {
        vp8_de_alloc_frame_buffers(oci);
        return 1;
    }

    if (vp8_yv12_alloc_frame_buffer(&oci->post_proc_buffer, width, height, VP8BORDERINPIXELS) < 0)
    {
        vp8_de_alloc_frame_buffers(oci);
        return 1;
    }

    oci->mb_rows = height >> 4;
    oci->mb_cols = width >> 4;
    oci->MBs = oci->mb_rows * oci->mb_cols;
    oci->mode_info_stride = oci->mb_cols + 1;
    oci->mip = vpx_calloc((oci->mb_cols + 1) * (oci->mb_rows + 1), sizeof(MODE_INFO));

    if (!oci->mip)
    {
        vp8_de_alloc_frame_buffers(oci);
        return 1;
    }

    oci->mi = oci->mip + oci->mode_info_stride + 1;

    /* allocate memory for last frame MODE_INFO array */

    oci->prev_mip = vpx_calloc((oci->mb_cols + 1) * (oci->mb_rows + 1), sizeof(MODE_INFO));

    if (!oci->prev_mip)
    {
        vp8_de_alloc_frame_buffers(oci);
        return 1;
    }

    oci->prev_mi = oci->prev_mip + oci->mode_info_stride + 1;

    oci->above_context = vpx_calloc(sizeof(ENTROPY_CONTEXT_PLANES) * oci->mb_cols, 1);

    if (!oci->above_context)
    {
        vp8_de_alloc_frame_buffers(oci);
        return 1;
    }

    update_mode_info_border(oci, oci->mip);
    update_mode_info_in_image(oci, oci->mi);
#if CONFIG_ERROR_CONCEALMENT
    update_mode_info_border(oci, oci->prev_mip);
    update_mode_info_in_image(oci, oci->prev_mi);
#endif

    return 0;
}
void vp8_setup_version(VP8_COMMON *cm)
{
    if (cm->version & 0x4)
    {
        if (!CONFIG_EXPERIMENTAL)
            vpx_internal_error(&cm->error, VPX_CODEC_UNSUP_BITSTREAM,
                               "Bitstream was created by an experimental "
                               "encoder");
        cm->experimental = 1;
    }

    switch (cm->version & 0x3)
    {
    case 0:
        cm->no_lpf = 0;
        cm->filter_type = NORMAL_LOOPFILTER;
        cm->use_bilinear_mc_filter = 0;
        cm->full_pixel = 0;
        break;
    case 1:
        cm->no_lpf = 0;
        cm->filter_type = SIMPLE_LOOPFILTER;
        cm->use_bilinear_mc_filter = 1;
        cm->full_pixel = 0;
        break;
    case 2:
        cm->no_lpf = 1;
        cm->filter_type = NORMAL_LOOPFILTER;
        cm->use_bilinear_mc_filter = 1;
        cm->full_pixel = 0;
        break;
    case 3:
        cm->no_lpf = 1;
        cm->filter_type = SIMPLE_LOOPFILTER;
        cm->use_bilinear_mc_filter = 1;
        cm->full_pixel = 1;
        break;
    }
}
void vp8_create_common(VP8_COMMON *oci)
{
    vp8_machine_specific_config(oci);

    vp8_init_mbmode_probs(oci);

    vp8_default_bmode_probs(oci->fc.bmode_prob);

    oci->mb_no_coeff_skip = 1;
#if CONFIG_DUALPRED
    oci->dual_pred_mode = HYBRID_PREDICTION;
#endif /* CONFIG_DUALPRED */
    oci->no_lpf = 0;
    oci->filter_type = NORMAL_LOOPFILTER;
    oci->use_bilinear_mc_filter = 0;
    oci->full_pixel = 0;
    oci->multi_token_partition = ONE_PARTITION;
    oci->clr_type = REG_YUV;
    oci->clamp_type = RECON_CLAMP_REQUIRED;

    /* Initialise reference frame sign bias structure to defaults */
    vpx_memset(oci->ref_frame_sign_bias, 0, sizeof(oci->ref_frame_sign_bias));

    /* Default disable buffer to buffer copying */
    oci->copy_buffer_to_gf = 0;
    oci->copy_buffer_to_arf = 0;
#if CONFIG_QIMODE
    oci->kf_ymode_probs_update = 0;
#endif
}

void vp8_remove_common(VP8_COMMON *oci)
{
    vp8_de_alloc_frame_buffers(oci);
}

void vp8_initialize_common()
{
    vp8_coef_tree_initialize();

    vp8_entropy_mode_init();

    vp8_init_scan_order_mask();

}
