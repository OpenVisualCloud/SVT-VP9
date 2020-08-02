/*
* Copyright(c) 2019 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include "EbUtility.h"
#include "EbDefinitions.h"
#include "vp9_common_data.h"

#ifdef _WIN32
//#if  (WIN_ENCODER_TIMING || WIN_DECODER_TIMING)
#include <time.h>
#include <stdio.h>

//#endif
#elif __linux__
#include <stdio.h>
#include <stdlib.h>
#include "EbDefinitions.h"
//#if   (LINUX_ENCODER_TIMING || LINUX_DECODER_TIMING)
#include <sys/time.h>
#include <time.h>
//#endif

#else
#error OS/Platform not supported.
#endif

static PaBlockStats pa_get_block_stats_array[] = {

    //   Depth       Size      size_log2     origin_x    origin_y   cu_num_in_depth   Index
        {0,           64,         6,           0,         0,        0     ,   0    },  // 0
        {1,           32,         5,           0,         0,        0     ,   1    },  // 1
        {2,           16,         4,           0,         0,        0     ,   1    },  // 2
        {3,            8,         3,           0,         0,        0     ,   1    },  // 3
        {3,            8,         3,           8,         0,        1     ,   1    },  // 4
        {3,            8,         3,           0,         8,        8     ,   1    },  // 5
        {3,            8,         3,           8,         8,        9     ,   1    },  // 6
        {2,           16,         4,          16,         0,        1     ,   1    },  // 7
        {3,            8,         3,          16,         0,        2     ,   1    },  // 8
        {3,            8,         3,          24,         0,        3     ,   1    },  // 9
        {3,            8,         3,          16,         8,        10    ,   1    },  // 10
        {3,            8,         3,          24,         8,        11    ,   1    },  // 11
        {2,           16,         4,           0,        16,        4     ,   1    },  // 12
        {3,            8,         3,           0,        16,        16    ,   1    },  // 13
        {3,            8,         3,           8,        16,        17    ,   1    },  // 14
        {3,            8,         3,           0,        24,        24    ,   1    },  // 15
        {3,            8,         3,           8,        24,        25    ,   1    },  // 16
        {2,           16,         4,          16,        16,        5     ,   1    },  // 17
        {3,            8,         3,          16,        16,        18    ,   1    },  // 18
        {3,            8,         3,          24,        16,        19    ,   1    },  // 19
        {3,            8,         3,          16,        24,        26    ,   1    },  // 20
        {3,            8,         3,          24,        24,        27    ,   1    },  // 21
        {1,           32,         5,          32,         0,        1     ,   2    },  // 22
        {2,           16,         4,          32,         0,        2     ,   2    },  // 23
        {3,            8,         3,          32,         0,        4     ,   2    },  // 24
        {3,            8,         3,          40,         0,        5     ,   2    },  // 25
        {3,            8,         3,          32,         8,        12    ,   2    },  // 26
        {3,            8,         3,          40,         8,        13    ,   2    },  // 27
        {2,           16,         4,          48,         0,        3     ,   2    },  // 28
        {3,            8,         3,          48,         0,        6     ,   2    },  // 29
        {3,            8,         3,          56,         0,        7     ,   2    },  // 30
        {3,            8,         3,          48,         8,        14    ,   2    },  // 31
        {3,            8,         3,          56,         8,        15    ,   2    },  // 32
        {2,           16,         4,          32,        16,        6     ,   2    },  // 33
        {3,            8,         3,          32,        16,        20    ,   2    },  // 34
        {3,            8,         3,          40,        16,        21    ,   2    },  // 35
        {3,            8,         3,          32,        24,        28    ,   2    },  // 36
        {3,            8,         3,          40,        24,        29    ,   2    },  // 37
        {2,           16,         4,          48,        16,        7     ,   2    },  // 38
        {3,            8,         3,          48,        16,        22    ,   2    },  // 39
        {3,            8,         3,          56,        16,        23    ,   2    },  // 40
        {3,            8,         3,          48,        24,        30    ,   2    },  // 41
        {3,            8,         3,          56,        24,        31    ,   2    },  // 42
        {1,           32,         5,           0,        32,        2     ,   3    },  // 43
        {2,           16,         4,           0,        32,        8     ,   3    },  // 44
        {3,            8,         3,           0,        32,        32    ,   3    },  // 45
        {3,            8,         3,           8,        32,        33    ,   3    },  // 46
        {3,            8,         3,           0,        40,        40    ,   3    },  // 47
        {3,            8,         3,           8,        40,        41    ,   3    },  // 48
        {2,           16,         4,          16,        32,        9     ,   3    },  // 49
        {3,            8,         3,          16,        32,        34    ,   3    },  // 50
        {3,            8,         3,          24,        32,        35    ,   3    },  // 51
        {3,            8,         3,          16,        40,        42    ,   3    },  // 52
        {3,            8,         3,          24,        40,        43    ,   3    },  // 53
        {2,           16,         4,           0,        48,        12    ,   3    },  // 54
        {3,            8,         3,           0,        48,        48    ,   3    },  // 55
        {3,            8,         3,           8,        48,        49    ,   3    },  // 56
        {3,            8,         3,           0,        56,        56    ,   3    },  // 57
        {3,            8,         3,           8,        56,        57    ,   3    },  // 58
        {2,           16,         4,          16,        48,        13    ,   3    },  // 59
        {3,            8,         3,          16,        48,        50    ,   3    },  // 60
        {3,            8,         3,          24,        48,        51    ,   3    },  // 61
        {3,            8,         3,          16,        56,        58    ,   3    },  // 62
        {3,            8,         3,          24,        56,        59    ,   3    },  // 63
        {1,           32,         5,          32,        32,        3     ,   4    },  // 64
        {2,           16,         4,          32,        32,        10    ,   4    },  // 65
        {3,            8,         3,          32,        32,        36    ,   4    },  // 66
        {3,            8,         3,          40,        32,        37    ,   4    },  // 67
        {3,            8,         3,          32,        40,        44    ,   4    },  // 68
        {3,            8,         3,          40,        40,        45    ,   4    },  // 69
        {2,           16,         4,          48,        32,        11    ,   4    },  // 70
        {3,            8,         3,          48,        32,        38    ,   4    },  // 71
        {3,            8,         3,          56,        32,        39    ,   4    },  // 72
        {3,            8,         3,          48,        40,        46    ,   4    },  // 73
        {3,            8,         3,          56,        40,        47    ,   4    },  // 74
        {2,           16,         4,          32,        48,        14    ,   4    },  // 75
        {3,            8,         3,          32,        48,        52    ,   4    },  // 76
        {3,            8,         3,          40,        48,        53    ,   4    },  // 77
        {3,            8,         3,          32,        56,        60    ,   4    },  // 78
        {3,            8,         3,          40,        56,        61    ,   4    },  // 79
        {2,           16,         4,          48,        48,        15    ,   4    },  // 80
        {3,            8,         3,          48,        48,        54    ,   4    },  // 81
        {3,            8,         3,          56,        48,        55    ,   4    },  // 82
        {3,            8,         3,          48,        56,        62    ,   4    },  // 83
        {3,            8,         3,          56,        56,        63    ,   4    }   // 84
};

PaBlockStats  * pa_get_block_stats(int block_index)
{
    return &pa_get_block_stats_array[block_index];
}

/*****************************************
 * Long Log 2
 *  This is a quick adaptation of a Number
 *  Leading Zeros (NLZ) algorithm to get
 *  the log2f of a 64-bit number
 *****************************************/
inline uint64_t log2f64(uint64_t x)
{
    uint64_t y;
    int64_t n = 64, c = 32;

    do {
        y = x >> c;
        if (y > 0) {
            n -= c;
            x = y;
        }
        c >>= 1;
    } while (c > 0);

    return 64 - n;
}

/*****************************************
 * Endian Swap
 *****************************************/
uint32_t eb_vp9_endian_swap(uint32_t ui)
{
    unsigned int ul2;

    ul2  = ui>>24;
    ul2 |= (ui>>8) & 0x0000ff00;
    ul2 |= (ui<<8) & 0x00ff0000;
    ul2 |= ui<<24;

    return ul2;

}

uint64_t eb_vp9_log2f_high_precision(uint64_t x, uint8_t precision)
{

    uint64_t sig_bit_location = log2f64(x);
    uint64_t remainder = x - ((uint64_t)1 << (uint8_t) sig_bit_location);
    uint64_t result;

    result = (sig_bit_location<<precision) + ((remainder<<precision) / ((uint64_t)1<< (uint8_t) sig_bit_location));

    return result;

}

static const MiniGopStats mini_gop_stats_array[] = {

    //    hierarchical_levels    start_index    end_index    Lenght    mini_gop_index
    { 5,  0, 31, 32 },    // 0
    { 4,  0, 15, 16 },    // 1
    { 3,  0,  7,  8 },    // 2
    { 2,  0,  3,  4 },    // 3
    { 2,  4,  7,  4 },    // 4
    { 3,  8, 15,  8 },    // 5
    { 2,  8, 11,  4 },    // 6
    { 2, 12, 15,  4 },    // 7
    { 4, 16, 31, 16 },    // 8
    { 3, 16, 23,  8 },    // 9
    { 2, 16, 19,  4 },    // 10
    { 2, 20, 23,  4 },    // 11
    { 3, 24, 31,  8 },    // 12
    { 2, 24, 27,  4 },    // 13
    { 2, 28, 31,  4 }    // 14
};

/**************************************************************
* Get Mini GOP Statistics
**************************************************************/
const MiniGopStats* eb_vp9_get_mini_gop_stats(const uint32_t mini_gop_index)
{
    return &mini_gop_stats_array[mini_gop_index];
}

uint8_t eb_vp9_ns_quarter_off_mult[9/*Up to 9 part*/][2/*x+y*/][4/*Up to 4 ns blocks per part*/] =
{
    //9 means not used.

    //          |   x   |     |   y   |

    /*P=0*/{ { 0,9,9,9 }  ,{ 0,9,9,9 } },
    /*P=1*/{ { 0,0,9,9 }  ,{ 0,2,9,9 } },
    /*P=2*/{ { 0,2,9,9 }  ,{ 0,0,9,9 } },
    /*P=3*/{ { 0,2,0,9 }  ,{ 0,0,2,9 } },
    /*P=4*/{ { 0,0,2,9 }  ,{ 0,2,2,9 } },
    /*P=5*/{ { 0,0,2,9 }  ,{ 0,2,0,9 } },
    /*P=6*/{ { 0,2,2,9 }  ,{ 0,0,2,9 } },
    /*P=7*/{ { 0,0,0,0 }  ,{ 0,1,2,3 } },
    /*P=8*/{ { 0,1,2,3 }  ,{ 0,0,0,0 } }

};

uint8_t eb_vp9_ns_quarter_size_mult[9/*Up to 9 part*/][2/*h+v*/][4/*Up to 4 ns blocks per part*/] =
{
    //9 means not used.

    //          |   h   |     |   v   |

    /*P=0*/{ { 4,9,9,9 }  ,{ 4,9,9,9 } },
    /*P=1*/{ { 4,4,9,9 }  ,{ 2,2,9,9 } },
    /*P=2*/{ { 2,2,9,9 }  ,{ 4,4,9,9 } },
    /*P=3*/{ { 2,2,4,9 }  ,{ 2,2,2,9 } },
    /*P=4*/{ { 4,2,2,9 }  ,{ 2,2,2,9 } },
    /*P=5*/{ { 2,2,2,9 }  ,{ 2,2,4,9 } },
    /*P=6*/{ { 2,2,2,9 }  ,{ 4,2,2,9 } },
    /*P=7*/{ { 4,4,4,4 }  ,{ 1,1,1,1 } },
    /*P=8*/{ { 1,1,1,1 }  ,{ 4,4,4,4 } }

};

BLOCK_SIZE eb_vp9_hvsize_to_bsize[/*H*/5][/*V*/5] =
{
{ BLOCK_4X4,        BLOCK_4X8,      BLOCK_INVALID,      BLOCK_INVALID,      BLOCK_INVALID,  },
{ BLOCK_8X4,        BLOCK_8X8,      BLOCK_8X16,         BLOCK_INVALID,      BLOCK_INVALID,  },
{ BLOCK_INVALID,    BLOCK_16X8,     BLOCK_16X16,        BLOCK_16X32,        BLOCK_INVALID,  },
{ BLOCK_INVALID,    BLOCK_INVALID,  BLOCK_32X16,        BLOCK_32X32,        BLOCK_32X64,    },
{ BLOCK_INVALID,    BLOCK_INVALID,  BLOCK_INVALID,      BLOCK_64X32,        BLOCK_64X64,    }

};

uint8_t   eb_vp9_max_sb = 64;

uint32_t  eb_vp9_max_depth = 5;

uint32_t  eb_vp9_max_part = 3;
uint32_t  eb_vp9_max_num_active_blocks;

//data could be  organized in 2 forms: depth scan (dps) or MD scan (mds):
//dps: all depth0 - all depth1 - all depth2 - all depth3.
//     within a depth: square blk0 in raster scan (followed by all its ns blcoks),
//     square blk1 in raster scan (followed by all its ns blcoks), etc
//mds: top-down and Z scan.
EpBlockStats ep_block_stats_ptr_dps[EP_BLOCK_MAX_COUNT];  //to access geom info of a particular block : use this table if you have the block index in depth scan
EpBlockStats ep_block_stats_ptr_mds[EP_BLOCK_MAX_COUNT];  //to access geom info of a particular block : use this table if you have the block index in md    scan

uint32_t eb_vp9_search_matching_from_dps(
    uint8_t depth,
    Part    part,
    uint8_t x,
    uint8_t y)
{
    uint32_t found = 0;
    uint32_t it;
    uint32_t matched = 0xFFFF;
    for (it = 0; it < eb_vp9_max_num_active_blocks; it++)
    {
        if (ep_block_stats_ptr_dps[it].depth == depth && ep_block_stats_ptr_dps[it].shape == part && ep_block_stats_ptr_dps[it].origin_x == x && ep_block_stats_ptr_dps[it].origin_y == y)
        {
            if (found == 0)
            {
                matched = it;
                found = 1;
            }
            else {
                matched = 0xFFFF;
                break;
            }

        }
    }

    if (matched == 0xFFFF)
        SVT_LOG(" \n\n PROBLEM\n\n ");

    return matched;

}
uint32_t eb_vp9_search_matching_from_mds(
    uint8_t depth,
    Part    part,
    uint8_t x,
    uint8_t y)
{
    uint32_t found = 0;
    uint32_t it;
    uint32_t matched = 0xFFFF;
    for (it = 0; it < eb_vp9_max_num_active_blocks; it++)
    {
        if (ep_block_stats_ptr_mds[it].depth == depth && ep_block_stats_ptr_mds[it].shape == part && ep_block_stats_ptr_mds[it].origin_x == x && ep_block_stats_ptr_mds[it].origin_y == y)
        {
            if (found == 0)
            {
                matched = it;
                found = 1;
            }
            else {
                matched = 0xFFFF;
                break;
            }

        }
    }

    if (matched == 0xFFFF)
        SVT_LOG(" \n\n PROBLEM\n\n ");

    return matched;

}

static   BLOCK_SIZE get_plane_block_size(BLOCK_SIZE bsize,
    int subsampling_x,
    int subsampling_y) {
    if (bsize == BLOCK_INVALID) return BLOCK_INVALID;
    return eb_vp9_ss_size_lookup[bsize][subsampling_x][subsampling_y];
}

void eb_vp9_md_scan_all_blks(uint16_t *idx_mds, uint8_t sq_size, uint8_t x, uint8_t y, int is_last_quadrant)
{
    // the input block is the parent square block of size sq_size located at pos (x,y)
    uint32_t part_it;
    uint16_t sqi_mds;
    uint8_t d1_it, nsq_it;

    uint8_t halfsize = sq_size / 2;
    uint8_t quartsize = sq_size / 4;

    uint32_t max_part_updated = sq_size == 128 ? MIN(eb_vp9_max_part, 7) :

        sq_size == 8 ? MIN(eb_vp9_max_part, 3) :

        sq_size == 4 ? 1 : eb_vp9_max_part;

    d1_it = 0;
    sqi_mds = *idx_mds;

    ep_block_stats_ptr_mds[*idx_mds].sq_size = sq_size;
    ep_block_stats_ptr_mds[*idx_mds].sq_size_uv = MAX((sq_size >> 1), 4);
    ep_block_stats_ptr_mds[*idx_mds].is_last_quadrant = is_last_quadrant;

    for (part_it = 0; part_it < max_part_updated; part_it++)
    {
        uint8_t tot_num_ns_per_part =
            part_it < 1 ? 1 :
            part_it < 3 ? 2 :
            part_it < 7 ? 3 : 4;

        for (nsq_it = 0; nsq_it < tot_num_ns_per_part; nsq_it++)
        {
            ep_block_stats_ptr_mds[*idx_mds].depth = sq_size == eb_vp9_max_sb / 1 ? 0 :
                sq_size == eb_vp9_max_sb / 2 ? 1 :
                sq_size == eb_vp9_max_sb / 4 ? 2 :
                sq_size == eb_vp9_max_sb / 8 ? 3 :
                sq_size == eb_vp9_max_sb / 16 ? 4 : 5;

            ep_block_stats_ptr_mds[*idx_mds].shape = (Part)part_it;
            ep_block_stats_ptr_mds[*idx_mds].origin_x = x + quartsize * eb_vp9_ns_quarter_off_mult[part_it][0][nsq_it];
            ep_block_stats_ptr_mds[*idx_mds].origin_y = y + quartsize * eb_vp9_ns_quarter_off_mult[part_it][1][nsq_it];

            ep_block_stats_ptr_mds[*idx_mds].d1i = d1_it++;
            ep_block_stats_ptr_mds[*idx_mds].sqi_mds = sqi_mds;
            ep_block_stats_ptr_mds[*idx_mds].totns = tot_num_ns_per_part;
            ep_block_stats_ptr_mds[*idx_mds].nsi = nsq_it;

            uint32_t matched = eb_vp9_search_matching_from_dps(
                ep_block_stats_ptr_mds[*idx_mds].depth,
                ep_block_stats_ptr_mds[*idx_mds].shape,
                ep_block_stats_ptr_mds[*idx_mds].origin_x,
                ep_block_stats_ptr_mds[*idx_mds].origin_y);

            ep_block_stats_ptr_mds[*idx_mds].blkidx_dps = ep_block_stats_ptr_dps[matched].blkidx_dps;

            ep_block_stats_ptr_mds[*idx_mds].bwidth = quartsize * eb_vp9_ns_quarter_size_mult[part_it][0][nsq_it];
            ep_block_stats_ptr_mds[*idx_mds].bheight = quartsize * eb_vp9_ns_quarter_size_mult[part_it][1][nsq_it];
            ep_block_stats_ptr_mds[*idx_mds].bwidth_log2 = (uint8_t) Log2f(ep_block_stats_ptr_mds[*idx_mds].bwidth);
            ep_block_stats_ptr_mds[*idx_mds].bheight_log2 = (uint8_t) Log2f(ep_block_stats_ptr_mds[*idx_mds].bheight);
            ep_block_stats_ptr_mds[*idx_mds].bsize = eb_vp9_hvsize_to_bsize[ep_block_stats_ptr_mds[*idx_mds].bwidth_log2 - 2][ep_block_stats_ptr_mds[*idx_mds].bheight_log2 - 2];

            ep_block_stats_ptr_mds[*idx_mds].bwidth_uv = MAX(4, ep_block_stats_ptr_mds[*idx_mds].bwidth >> 1);
            ep_block_stats_ptr_mds[*idx_mds].bheight_uv = MAX(4, ep_block_stats_ptr_mds[*idx_mds].bheight >> 1);
            ep_block_stats_ptr_mds[*idx_mds].has_uv = 1;

            if (ep_block_stats_ptr_mds[*idx_mds].bwidth == 4 && ep_block_stats_ptr_mds[*idx_mds].bheight == 4)
                ep_block_stats_ptr_mds[*idx_mds].has_uv = is_last_quadrant ? 1 : 0;
            else

                if ((ep_block_stats_ptr_mds[*idx_mds].bwidth >> 1) < ep_block_stats_ptr_mds[*idx_mds].bwidth_uv || (ep_block_stats_ptr_mds[*idx_mds].bheight >> 1) < ep_block_stats_ptr_mds[*idx_mds].bheight_uv) {
                    int num_blk_same_uv = 1;
                    if (ep_block_stats_ptr_mds[*idx_mds].bwidth >> 1 < 4)
                        num_blk_same_uv *= 2;
                    if (ep_block_stats_ptr_mds[*idx_mds].bheight >> 1 < 4)
                        num_blk_same_uv *= 2;
                    if (ep_block_stats_ptr_mds[*idx_mds].nsi != (num_blk_same_uv - 1) && ep_block_stats_ptr_mds[*idx_mds].nsi != (2 * num_blk_same_uv - 1))
                        ep_block_stats_ptr_mds[*idx_mds].has_uv = 0;
                }

            ep_block_stats_ptr_mds[*idx_mds].bsize_uv = get_plane_block_size(ep_block_stats_ptr_mds[*idx_mds].bsize, 1, 1);
            ep_block_stats_ptr_mds[*idx_mds].tx_size = blocksize_to_txsize[ep_block_stats_ptr_mds[*idx_mds].bsize];
            ep_block_stats_ptr_mds[*idx_mds].tx_size_uv = blocksize_to_txsize[ep_block_stats_ptr_mds[*idx_mds].bsize_uv];

            ep_block_stats_ptr_mds[*idx_mds].blkidx_mds = (*idx_mds);
            (*idx_mds) = (*idx_mds) + 1;

        }
    }

    uint32_t min_size = eb_vp9_max_sb >> (eb_vp9_max_depth - 1);
    if (halfsize >= min_size)
    {
        eb_vp9_md_scan_all_blks(idx_mds, halfsize, x, y, 0);
        eb_vp9_md_scan_all_blks(idx_mds, halfsize, x + halfsize, y, 0);
        eb_vp9_md_scan_all_blks(idx_mds, halfsize, x, y + halfsize, 0);
        eb_vp9_md_scan_all_blks(idx_mds, halfsize, x + halfsize, y + halfsize, 1);
    }
}

void eb_vp9_depth_scan_all_blks()
{
    uint8_t depth_it, sq_it_y, sq_it_x, part_it, nsq_it;
    uint8_t sq_orgx, sq_orgy;
    uint16_t depth_scan_idx = 0;

    for (depth_it = 0; depth_it < eb_vp9_max_depth; depth_it++)
    {
        uint32_t tot_num_sq = 1 << depth_it;
        uint8_t sq_size = depth_it == 0 ? eb_vp9_max_sb :
            depth_it == 1 ? eb_vp9_max_sb / 2 :
            depth_it == 2 ? eb_vp9_max_sb / 4 :
            depth_it == 3 ? eb_vp9_max_sb / 8 :
            depth_it == 4 ? eb_vp9_max_sb / 16 : eb_vp9_max_sb / 32;

        uint32_t max_part_updated = sq_size == 128 ? MIN(eb_vp9_max_part, 7) :

            sq_size == 8 ? MIN(eb_vp9_max_part, 3) :

            sq_size == 4 ? 1 : eb_vp9_max_part;

        for (sq_it_y = 0; sq_it_y < tot_num_sq; sq_it_y++)
        {
            sq_orgy = sq_it_y * sq_size;

            for (sq_it_x = 0; sq_it_x < tot_num_sq; sq_it_x++)
            {
                sq_orgx = sq_it_x * sq_size;

                for (part_it = 0; part_it < max_part_updated; part_it++)
                {
                    uint32_t tot_num_ns_per_part = part_it < 1 ? 1 :
                        part_it < 3 ? 2 :
                        part_it < 7 ? 3 : 4;

                    for (nsq_it = 0; nsq_it < tot_num_ns_per_part; nsq_it++)
                    {
                        ep_block_stats_ptr_dps[depth_scan_idx].blkidx_dps = depth_scan_idx;
                        ep_block_stats_ptr_dps[depth_scan_idx].depth = depth_it;
                        ep_block_stats_ptr_dps[depth_scan_idx].shape = (Part) part_it;
                        ep_block_stats_ptr_dps[depth_scan_idx].origin_x = sq_orgx + (sq_size / 4) *eb_vp9_ns_quarter_off_mult[part_it][0][nsq_it];
                        ep_block_stats_ptr_dps[depth_scan_idx].origin_y = sq_orgy + (sq_size / 4) *eb_vp9_ns_quarter_off_mult[part_it][1][nsq_it];

                        depth_scan_idx++;
                    }
                }
            }
        }
    }
}

void eb_vp9_finish_depth_scan_all_blks()
{
    uint32_t do_print = 0;
    //uint32_t min_size = eb_vp9_max_sb >> (eb_vp9_max_depth - 1);
    //FILE * fp = NULL;
    //if (do_print)
    //    FOPEN(fp, "e:\\test\\data.csv", "w");

    uint32_t depth_it, sq_it_y, sq_it_x, part_it, nsq_it;

    uint32_t  depth_scan_idx = 0;

    for (depth_it = 0; depth_it < eb_vp9_max_depth; depth_it++)
    {
        uint32_t  tot_num_sq = 1 << depth_it;
        uint32_t  sq_size = depth_it == 0 ? eb_vp9_max_sb :
            depth_it == 1 ? eb_vp9_max_sb / 2 :
            depth_it == 2 ? eb_vp9_max_sb / 4 :
            depth_it == 3 ? eb_vp9_max_sb / 8 :
            depth_it == 4 ? eb_vp9_max_sb / 16 : eb_vp9_max_sb / 32;

        uint32_t max_part_updated = sq_size == 128 ? MIN(eb_vp9_max_part, 7) :

            sq_size == 8 ? MIN(eb_vp9_max_part, 3) :

            sq_size == 4 ? 1 : eb_vp9_max_part;

        if (do_print)
        {
            SVT_LOG("\n\n\n");
            SVT_LOG("\n\n\n");
        }

        for (sq_it_y = 0; sq_it_y < tot_num_sq; sq_it_y++)
        {
            if (do_print)
            {
                //for (uint32_t i = 0; i < sq_size / min_size; i++)
                //{
                    // SVT_LOG( "\n ");
                SVT_LOG("\n \n\n \n");
                // }
            }

            for (sq_it_x = 0; sq_it_x < tot_num_sq; sq_it_x++)
            {
                for (part_it = 0; part_it < max_part_updated; part_it++)
                {
                    uint32_t tot_num_ns_per_part = part_it < 1 ? 1 :
                        part_it < 3 ? 2 :
                        part_it < 7 ? 3 : 4;

                    for (nsq_it = 0; nsq_it < tot_num_ns_per_part; nsq_it++)
                    {
                        uint32_t matched = eb_vp9_search_matching_from_mds(
                            ep_block_stats_ptr_dps[depth_scan_idx].depth,
                            ep_block_stats_ptr_dps[depth_scan_idx].shape,
                            ep_block_stats_ptr_dps[depth_scan_idx].origin_x,
                            ep_block_stats_ptr_dps[depth_scan_idx].origin_y);

                        ep_block_stats_ptr_dps[depth_scan_idx].blkidx_mds = ep_block_stats_ptr_mds[matched].blkidx_mds;

                        if (do_print && part_it == 0)
                        {
                            SVT_LOG("%i", ep_block_stats_ptr_dps[depth_scan_idx].blkidx_mds);
                            //SVT_LOG("%i", ep_block_stats_ptr_dps[depth_scan_idx].blkidx_mds);

                            //for (uint32_t i = 0; i < sq_size / min_size; i++)
                            //{
                            SVT_LOG("\t\t\t\t");
                            //   SVT_LOG("\t");
                          // }
                        }
                        depth_scan_idx++;
                    }
                }
            }
        }
    }

    SVT_LOG("\n\n");
    //if (do_print)
    //    fclose(fp);
}

uint32_t eb_vp9_count_total_num_of_active_blks()
{
    uint32_t depth_it, sq_it_y, sq_it_x, part_it, nsq_it;

    uint32_t depth_scan_idx = 0;

    for (depth_it = 0; depth_it < eb_vp9_max_depth; depth_it++)
    {
        uint32_t  tot_num_sq = 1 << depth_it;
        uint32_t  sq_size = depth_it == 0 ? eb_vp9_max_sb :
            depth_it == 1 ? eb_vp9_max_sb / 2 :
            depth_it == 2 ? eb_vp9_max_sb / 4 :
            depth_it == 3 ? eb_vp9_max_sb / 8 :
            depth_it == 4 ? eb_vp9_max_sb / 16 : eb_vp9_max_sb / 32;

        uint32_t max_part_updated = sq_size == 128 ? MIN(eb_vp9_max_part, 7) :

            sq_size == 8 ? MIN(eb_vp9_max_part, 3) :

            sq_size == 4 ? 1 : eb_vp9_max_part;

        for (sq_it_y = 0; sq_it_y < tot_num_sq; sq_it_y++)
        {
            for (sq_it_x = 0; sq_it_x < tot_num_sq; sq_it_x++)
            {
                for (part_it = 0; part_it < max_part_updated; part_it++)
                {
                    uint32_t tot_num_ns_per_part = part_it < 1 ? 1 :
                        part_it < 3 ? 2 :
                        part_it < 7 ? 3 : 4;

                    for (nsq_it = 0; nsq_it < tot_num_ns_per_part; nsq_it++)
                    {
                        depth_scan_idx++;
                    }
                }
            }
        }
    }

    return depth_scan_idx;

}

void build_ep_block_stats()
{
    //(0)compute total number of blocks using the information provided
    eb_vp9_max_num_active_blocks = eb_vp9_count_total_num_of_active_blks();
    if (eb_vp9_max_num_active_blocks != EP_BLOCK_MAX_COUNT)
        SVT_LOG(" \n\n Error %i blocks\n\n ", eb_vp9_max_num_active_blocks);

    //(1) Construct depth scan ep_block_stats_ptr_dps
    eb_vp9_depth_scan_all_blks();

    //(2) Construct md scan ep_block_stats_ptr_mds:  use info from dps
    uint16_t idx_mds = 0;
    eb_vp9_md_scan_all_blks(&idx_mds, eb_vp9_max_sb, 0, 0, 0);

    //(3) Fill more info from mds to dps - print using dps
    eb_vp9_finish_depth_scan_all_blks();
}

const EpBlockStats *ep_get_block_stats(uint32_t bidx_mds)
{
    return &ep_block_stats_ptr_mds[bidx_mds];
}
