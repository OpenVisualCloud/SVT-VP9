/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include <assert.h>

#include <stdint.h>
#include "vp9_tokenize.h"
#include "vp9_entropy.h"
#include "vp9_block.h"
#include "vp9_blockd.h"
#include "vp9_enums.h"
#include "vp9_scan.h"
#include "vp9_encoder.h"

static const TOKENVALUE dct_cat_lt_10_value_tokens[] = {
    {9, 63}, {9, 61}, {9, 59}, {9, 57}, {9, 55}, {9, 53}, {9, 51}, {9, 49}, {9, 47}, {9, 45}, {9, 43}, {9, 41}, {9, 39},
    {9, 37}, {9, 35}, {9, 33}, {9, 31}, {9, 29}, {9, 27}, {9, 25}, {9, 23}, {9, 21}, {9, 19}, {9, 17}, {9, 15}, {9, 13},
    {9, 11}, {9, 9},  {9, 7},  {9, 5},  {9, 3},  {9, 1},  {8, 31}, {8, 29}, {8, 27}, {8, 25}, {8, 23}, {8, 21}, {8, 19},
    {8, 17}, {8, 15}, {8, 13}, {8, 11}, {8, 9},  {8, 7},  {8, 5},  {8, 3},  {8, 1},  {7, 15}, {7, 13}, {7, 11}, {7, 9},
    {7, 7},  {7, 5},  {7, 3},  {7, 1},  {6, 7},  {6, 5},  {6, 3},  {6, 1},  {5, 3},  {5, 1},  {4, 1},  {3, 1},  {2, 1},
    {1, 1},  {0, 0},  {1, 0},  {2, 0},  {3, 0},  {4, 0},  {5, 0},  {5, 2},  {6, 0},  {6, 2},  {6, 4},  {6, 6},  {7, 0},
    {7, 2},  {7, 4},  {7, 6},  {7, 8},  {7, 10}, {7, 12}, {7, 14}, {8, 0},  {8, 2},  {8, 4},  {8, 6},  {8, 8},  {8, 10},
    {8, 12}, {8, 14}, {8, 16}, {8, 18}, {8, 20}, {8, 22}, {8, 24}, {8, 26}, {8, 28}, {8, 30}, {9, 0},  {9, 2},  {9, 4},
    {9, 6},  {9, 8},  {9, 10}, {9, 12}, {9, 14}, {9, 16}, {9, 18}, {9, 20}, {9, 22}, {9, 24}, {9, 26}, {9, 28}, {9, 30},
    {9, 32}, {9, 34}, {9, 36}, {9, 38}, {9, 40}, {9, 42}, {9, 44}, {9, 46}, {9, 48}, {9, 50}, {9, 52}, {9, 54}, {9, 56},
    {9, 58}, {9, 60}, {9, 62}};
const TOKENVALUE *eb_vp9_dct_cat_lt_10_value_tokens = dct_cat_lt_10_value_tokens +
    (sizeof(dct_cat_lt_10_value_tokens) / sizeof(*dct_cat_lt_10_value_tokens)) / 2;
// The corresponding costs of the extra_bits for the tokens in the above table
// are stored in the table below. The values are obtained from looking up the
// entry for the specified extra_bits in the table corresponding to the token
// (as defined in cost element eb_vp9_extra_bits)
// e.g. {9, 63} maps to cat5_cost[63 >> 1], {1, 1} maps to sign_cost[1 >> 1]
static const int dct_cat_lt_10_value_cost[] = {
    3773, 3750, 3704, 3681, 3623, 3600, 3554, 3531, 3432, 3409, 3363, 3340, 3282, 3259, 3213, 3190, 3136, 3113, 3067,
    3044, 2986, 2963, 2917, 2894, 2795, 2772, 2726, 2703, 2645, 2622, 2576, 2553, 3197, 3116, 3058, 2977, 2881, 2800,
    2742, 2661, 2615, 2534, 2476, 2395, 2299, 2218, 2160, 2079, 2566, 2427, 2334, 2195, 2023, 1884, 1791, 1652, 1893,
    1696, 1453, 1256, 1229, 864,  512,  512,  512,  512,  0,    512,  512,  512,  512,  864,  1229, 1256, 1453, 1696,
    1893, 1652, 1791, 1884, 2023, 2195, 2334, 2427, 2566, 2079, 2160, 2218, 2299, 2395, 2476, 2534, 2615, 2661, 2742,
    2800, 2881, 2977, 3058, 3116, 3197, 2553, 2576, 2622, 2645, 2703, 2726, 2772, 2795, 2894, 2917, 2963, 2986, 3044,
    3067, 3113, 3136, 3190, 3213, 3259, 3282, 3340, 3363, 3409, 3432, 3531, 3554, 3600, 3623, 3681, 3704, 3750, 3773,
};
const int *eb_vp9_dct_cat_lt_10_value_cost = dct_cat_lt_10_value_cost +
    (sizeof(dct_cat_lt_10_value_cost) / sizeof(*dct_cat_lt_10_value_cost)) / 2;

// Array indices are identical to previously-existing CONTEXT_NODE indices
/* clang-format off */
const vpx_tree_index eb_vp9_coef_tree[TREE_SIZE(ENTROPY_TOKENS)] = {
  -EOB_TOKEN, 2,                       // 0  = EOB
  -ZERO_TOKEN, 4,                      // 1  = ZERO
  -ONE_TOKEN, 6,                       // 2  = ONE
  8, 12,                               // 3  = LOW_VAL
  -TWO_TOKEN, 10,                      // 4  = TWO
  -THREE_TOKEN, -FOUR_TOKEN,           // 5  = THREE
  14, 16,                              // 6  = HIGH_LOW
  -CATEGORY1_TOKEN, -CATEGORY2_TOKEN,  // 7  = CAT_ONE
  18, 20,                              // 8  = CAT_THREEFOUR
  -CATEGORY3_TOKEN, -CATEGORY4_TOKEN,  // 9  = CAT_THREE
  -CATEGORY5_TOKEN, -CATEGORY6_TOKEN   // 10 = CAT_FIVE
};
/* clang-format on */

static const int16_t zero_cost[]       = {0};
static const int16_t sign_cost[1]      = {512};
static const int16_t cat1_cost[1 << 1] = {864, 1229};
static const int16_t cat2_cost[1 << 2] = {1256, 1453, 1696, 1893};
static const int16_t cat3_cost[1 << 3] = {1652, 1791, 1884, 2023, 2195, 2334, 2427, 2566};
static const int16_t cat4_cost[1 << 4] = {
    2079, 2160, 2218, 2299, 2395, 2476, 2534, 2615, 2661, 2742, 2800, 2881, 2977, 3058, 3116, 3197};
static const int16_t cat5_cost[1 << 5]         = {2553, 2576, 2622, 2645, 2703, 2726, 2772, 2795, 2894, 2917, 2963,
                                                  2986, 3044, 3067, 3113, 3136, 3190, 3213, 3259, 3282, 3340, 3363,
                                                  3409, 3432, 3531, 3554, 3600, 3623, 3681, 3704, 3750, 3773};
const int16_t        eb_vp9_cat6_low_cost[256] = {
    3378, 3390, 3401, 3413, 3435, 3447, 3458, 3470, 3517, 3529, 3540, 3552, 3574, 3586, 3597, 3609, 3671, 3683, 3694,
    3706, 3728, 3740, 3751, 3763, 3810, 3822, 3833, 3845, 3867, 3879, 3890, 3902, 3973, 3985, 3996, 4008, 4030, 4042,
    4053, 4065, 4112, 4124, 4135, 4147, 4169, 4181, 4192, 4204, 4266, 4278, 4289, 4301, 4323, 4335, 4346, 4358, 4405,
    4417, 4428, 4440, 4462, 4474, 4485, 4497, 4253, 4265, 4276, 4288, 4310, 4322, 4333, 4345, 4392, 4404, 4415, 4427,
    4449, 4461, 4472, 4484, 4546, 4558, 4569, 4581, 4603, 4615, 4626, 4638, 4685, 4697, 4708, 4720, 4742, 4754, 4765,
    4777, 4848, 4860, 4871, 4883, 4905, 4917, 4928, 4940, 4987, 4999, 5010, 5022, 5044, 5056, 5067, 5079, 5141, 5153,
    5164, 5176, 5198, 5210, 5221, 5233, 5280, 5292, 5303, 5315, 5337, 5349, 5360, 5372, 4988, 5000, 5011, 5023, 5045,
    5057, 5068, 5080, 5127, 5139, 5150, 5162, 5184, 5196, 5207, 5219, 5281, 5293, 5304, 5316, 5338, 5350, 5361, 5373,
    5420, 5432, 5443, 5455, 5477, 5489, 5500, 5512, 5583, 5595, 5606, 5618, 5640, 5652, 5663, 5675, 5722, 5734, 5745,
    5757, 5779, 5791, 5802, 5814, 5876, 5888, 5899, 5911, 5933, 5945, 5956, 5968, 6015, 6027, 6038, 6050, 6072, 6084,
    6095, 6107, 5863, 5875, 5886, 5898, 5920, 5932, 5943, 5955, 6002, 6014, 6025, 6037, 6059, 6071, 6082, 6094, 6156,
    6168, 6179, 6191, 6213, 6225, 6236, 6248, 6295, 6307, 6318, 6330, 6352, 6364, 6375, 6387, 6458, 6470, 6481, 6493,
    6515, 6527, 6538, 6550, 6597, 6609, 6620, 6632, 6654, 6666, 6677, 6689, 6751, 6763, 6774, 6786, 6808, 6820, 6831,
    6843, 6890, 6902, 6913, 6925, 6947, 6959, 6970, 6982};
const uint16_t eb_vp9_cat6_high_cost[64] = {
    88,   2251, 2727, 4890,  3148,  5311,  5787,  7950,  3666,  5829,  6305,  8468,  6726,  8889,  9365,  11528,
    3666, 5829, 6305, 8468,  6726,  8889,  9365,  11528, 7244,  9407,  9883,  12046, 10304, 12467, 12943, 15106,
    3666, 5829, 6305, 8468,  6726,  8889,  9365,  11528, 7244,  9407,  9883,  12046, 10304, 12467, 12943, 15106,
    7244, 9407, 9883, 12046, 10304, 12467, 12943, 15106, 10822, 12985, 13461, 15624, 13882, 16045, 16521, 18684};

const vp9_extra_bit eb_vp9_extra_bits[ENTROPY_TOKENS] = {
    {0, 0, 0, zero_cost}, // ZERO_TOKEN
    {0, 0, 1, sign_cost}, // ONE_TOKEN
    {0, 0, 2, sign_cost}, // TWO_TOKEN
    {0, 0, 3, sign_cost}, // THREE_TOKEN
    {0, 0, 4, sign_cost}, // FOUR_TOKEN
    {eb_vp9_cat1_prob, 1, CAT1_MIN_VAL, cat1_cost}, // CATEGORY1_TOKEN
    {eb_vp9_cat2_prob, 2, CAT2_MIN_VAL, cat2_cost}, // CATEGORY2_TOKEN
    {eb_vp9_cat3_prob, 3, CAT3_MIN_VAL, cat3_cost}, // CATEGORY3_TOKEN
    {eb_vp9_cat4_prob, 4, CAT4_MIN_VAL, cat4_cost}, // CATEGORY4_TOKEN
    {eb_vp9_cat5_prob, 5, CAT5_MIN_VAL, cat5_cost}, // CATEGORY5_TOKEN
    {eb_vp9_cat6_prob, 14, CAT6_MIN_VAL, 0}, // CATEGORY6_TOKEN
    {0, 0, 0, zero_cost} // EOB_TOKEN
};

const struct vp9_token eb_vp9_coef_encodings[ENTROPY_TOKENS] = {
    {2, 2}, {6, 3}, {28, 5}, {58, 6}, {59, 6}, {60, 6}, {61, 6}, {124, 7}, {125, 7}, {126, 7}, {127, 7}, {0, 1}};

typedef struct tokenize_b_args {
    VP9_COMP    *cpi;
    ThreadData  *td;
    TOKENEXTRA **tp;
} tokenize_b_args;

static void set_entropy_context_b(MACROBLOCKD *const xd, int plane, int block, int row, int col, BLOCK_SIZE plane_bsize,
                                  TX_SIZE tx_size, void *arg) {
    struct tokenize_b_args *const args = (tokenize_b_args *)arg;

    ThreadData *const td = args->td;
    MACROBLOCK *const x  = &td->mb;

    struct macroblock_plane  *p  = &x->plane[plane];
    struct macroblockd_plane *pd = &xd->plane[plane];
    eb_vp9_set_contexts(xd, pd, plane_bsize, tx_size, p->eobs[block] > 0, col, row);
}

static inline void add_token(TOKENEXTRA **t, const vpx_prob *context_tree, int16_t token, EXTRABIT extra,
                             unsigned int *counts) {
    (*t)->context_tree = context_tree;
    (*t)->token        = token;
    (*t)->extra        = extra;
    (*t)++;
    ++counts[token];
}

static inline void add_token_no_extra(TOKENEXTRA **t, const vpx_prob *context_tree, int16_t token,
                                      unsigned int *counts) {
    (*t)->context_tree = context_tree;
    (*t)->token        = token;
    (*t)++;
    ++counts[token];
}

static void tokenize_b(MACROBLOCKD *xd, int plane, int block, int row, int col, BLOCK_SIZE plane_bsize, TX_SIZE tx_size,
                       void *arg) {
    struct tokenize_b_args *const args = (tokenize_b_args *)arg;
    VP9_COMP                     *cpi  = args->cpi;
    ThreadData *const             td   = args->td;
    MACROBLOCK *const             x    = &td->mb;
    TOKENEXTRA                  **tp   = args->tp;
    uint8_t                       token_cache[32 * 32];
    struct macroblock_plane      *p  = &x->plane[plane];
    struct macroblockd_plane     *pd = &xd->plane[plane];
    ModeInfo                     *mi = xd->mi[0];
    int                           pt; /* near block/prev token context index */
    int                           c;
    TOKENEXTRA                   *t      = *tp; /* store tokens starting here */
    int                           eob    = p->eobs[block];
    const PLANE_TYPE              type   = get_plane_type(plane);
    const tran_low_t             *qcoeff = BLOCK_OFFSET(p->qcoeff, block);
    const int16_t                *scan, *nb;
    const scan_order             *so;
    const int                     ref                                = is_inter_block(mi);
    unsigned int (*const counts)[COEFF_CONTEXTS][ENTROPY_TOKENS]     = td->rd_counts.coef_counts[tx_size][type][ref];
    vpx_prob(*const coef_probs)[COEFF_CONTEXTS][UNCONSTRAINED_NODES] = cpi->common.fc->coef_probs[tx_size][type][ref];

    const uint8_t *const band   = get_band_translate(tx_size);
    const int            tx_eob = 16 << (tx_size << 1);
    int16_t              token;
    EXTRABIT             extra;
    pt   = get_entropy_context(tx_size, pd->above_context + col, pd->left_context + row);
    so   = get_scan(xd, tx_size, type, block);
    scan = so->scan;
    nb   = so->neighbors;
    c    = 0;

    while (c < eob) {
        int v = 0;
        v     = qcoeff[scan[c]];
        while (!v) {
            add_token_no_extra(&t, coef_probs[band[c]][pt], ZERO_TOKEN, counts[band[c]][pt]);

            token_cache[scan[c]] = 0;
            ++c;
            pt = get_coef_context(nb, token_cache, c);
            v  = qcoeff[scan[c]];
        }

        vp9_get_token_extra(v, &token, &extra);

        add_token(&t, coef_probs[band[c]][pt], token, extra, counts[band[c]][pt]);

        token_cache[scan[c]] = eb_vp9_pt_energy_class[token];
        ++c;
        pt = get_coef_context(nb, token_cache, c);
    }
    if (c < tx_eob) {
        add_token_no_extra(&t, coef_probs[band[c]][pt], EOB_TOKEN, counts[band[c]][pt]);
    }

    *tp = t;

    eb_vp9_set_contexts(xd, pd, plane_bsize, tx_size, c > 0, col, row);
}

struct is_skippable_args {
    uint16_t *eobs;
    int      *skippable;
};
void eb_vp9_tokenize_sb(struct VP9_COMP *cpi, MACROBLOCKD *const xd, struct ThreadData *td, TOKENEXTRA **t, int dry_run,
                        int seg_skip, BLOCK_SIZE bsize) {
    ModeInfo *const mi = xd->mi[0];

    struct tokenize_b_args arg = {cpi, td, t};

    if (seg_skip) {
        assert(mi->skip);
    }

    if (mi->skip) {
        reset_skip_context(xd, bsize);
        return;
    }

    if (!dry_run) {
        eb_vp9_foreach_transformed_block(xd, bsize, tokenize_b, &arg);
    } else {
        eb_vp9_foreach_transformed_block(xd, bsize, set_entropy_context_b, &arg);
    }
}
