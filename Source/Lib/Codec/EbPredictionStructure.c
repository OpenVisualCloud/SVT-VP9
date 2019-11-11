/*
* Copyright(c) 2019 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include <stdlib.h>
#include <string.h>

#include "EbDefinitions.h"
#include "EbPredictionStructure.h"
#include "EbUtility.h"

/**********************************************************
 * Macros
 **********************************************************/
#define PRED_STRUCT_INDEX(hierarchical_level_count, pred_type, ref_count) (((hierarchical_level_count) * EB_PRED_TOTAL_COUNT + (pred_type))  + (ref_count))

/**********************************************************
 * Instructions for how to create a Predicion Structure
 *
 * Overview:
 *   The prediction structure consists of a collection
 *   of Prediction Structure Entires, which themselves
 *   consist of reference and dependent lists.  The
 *   reference lists are exactly like those found in
 *   the standard and can be clipped in order to reduce
 *   the number of references.
 *
 *   Dependent lists are the corollary to reference lists,
 *   the describe how a particular picture is referenced.
 *   Dependent lists can also be clipped at predefined
 *   junctions (i.e. the list_count array) in order
 *   to reduce the number of references.  Note that the
 *   dependent deltaPOCs must be grouped together in order
 *   of ascending reference_picture in order for the Dependent
 *   List clip to work properly.
 *
 *   All control and RPS information is derived from
 *   these lists.  The lists for a structure are defined
 *   for both P & B-picture variants.  In the case of
 *   P-pictures, only Lists 0 are used.
 *
 *   Negative deltaPOCs are for backward-referencing pictures
 *   in display order and positive deltaPOCs are for
 *   forward-referencing pictures.
 *
 *   Please note that there is no assigned coding order,
 *   the PictureManager will start pictures as soon as
 *   their references become available.
 *
 *   Any prediction structure is possible; however, we are
 *     restricting usage to the following controls:
 *     # Hierarchical Levels
 *     # Number of References
 *     # B-pictures enabled
 *     # Intra Refresh Period
 *
 *  To Get Low Delay P, only use List 0
 *  To Get Low Delay B, replace List 1 with List 0
 *  To Get Random Access, use the preduction structure as is
 **********************************************************/

/************************************************
 * Flat
 *
 *  I-B-B-B-B-B-B-B-B
 *
 * Display & Coding Order:
 *  0 1 2 3 4 5 6 7 8
 *
 ************************************************/
static PredictionStructureConfigEntry flat_pred_struct[] = {

    {
        0,              // GOP Index 0 - Temporal Layer
        0,              // GOP Index 0 - Decode Order
        1,                // GOP Index 0 - Ref List 0
        1                // GOP Index 0 - Ref List 1
    }
};

/************************************************
 * Random Access - Two-Level Hierarchical
 *
 *    b   b   b   b      Temporal Layer 1
 *   / \ / \ / \ / \
 *  I---B---B---B---B    Temporal Layer 0
 *
 * Display Order:
 *  0 1 2 3 4 5 6 7 8
 *
 * Coding Order:
 *  0 2 1 4 3 6 5 8 7
 ************************************************/
static PredictionStructureConfigEntry two_level_hierarchical_pred_struct[] = {

    {
        0,              // GOP Index 0 - Temporal Layer
        0,              // GOP Index 0 - Decode Order
        2,                // GOP Index 0 - Ref List 0
        2                // GOP Index 0 - Ref List 1
    },

    {
        1,              // GOP Index 1 - Temporal Layer
        1,              // GOP Index 1 - Decode Order
        1,                // GOP Index 1 - Ref List 0
       -1                // GOP Index 1 - Ref List 1
    }
};

/************************************************
 * Three-Level Hierarchical
 *
 *      b   b       b   b       b   b        Temporal Layer 2
 *     / \ / \     / \ / \     / \ / \
 *    /   B   \   /   B   \   /   B   \      Temporal Layer 1
 *   /   / \   \ /   / \   \ /   / \   \
 *  I-----------B-----------B-----------B    Temporal Layer 0
 *
 * Display Order:
 *  0   1 2 3   4   5 6 7   8   9 1 1   1
 *                                0 1   2
 *
 * Coding Order:
 *  0   3 2 4   1   7 6 8   5   1 1 1   9
 *                              1 0 2
 ************************************************/
static PredictionStructureConfigEntry three_level_hierarchical_pred_struct[] = {

    {
        0,                  // GOP Index 0 - Temporal Layer
        0,                  // GOP Index 0 - Decode Order
        4,                    // GOP Index 0 - Ref List 0
        4                    // GOP Index 0 - Ref List 1
    },

    {
        2,                  // GOP Index 1 - Temporal Layer
        2,                  // GOP Index 1 - Decode Order
        1,                     // GOP Index 1 - Ref List 0
       -1                    // GOP Index 1 - Ref List 1
    },

    {
        1,                  // GOP Index 2 - Temporal Layer
        1,                  // GOP Index 2 - Decode Order
        2,                    // GOP Index 2 - Ref List 0
       -2                    // GOP Index 2 - Ref List 1
    },

    {
        2,                   // GOP Index 3 - Temporal Layer
        3,                   // GOP Index 3 - Decode Order
        1,                     // GOP Index 3 - Ref List 0
       -1                     // GOP Index 3 - Ref List 1
    }
};

/************************************************************************************************************
 * Four-Level Hierarchical
 *
 *
 *          b     b           b     b               b     b           b     b           Temporal Layer 3
 *         / \   / \         / \   / \             / \   / \         / \   / \
 *        /   \ /   \       /   \ /   \           /   \ /   \       /   \ /   \
 *       /     B     \     /     B     \         /     B     \     /     B     \        Temporal Layer 2
 *      /     / \     \   /     / \     \       /     / \     \   /     / \     \
 *     /     /   \     \ /     /   \     \     /     /   \     \ /     /   \     \
 *    /     /     ------B------     \     \   /     /     ------B------     \     \     Temporal Layer 1
 *   /     /           / \           \     \ /     /           / \           \     \
 *  I---------------------------------------B---------------------------------------B   Temporal Layer 0
 *
 * Display Order:
 *  0       1  2  3     4     5  6  7       8       9  1  1     1     1  1  1       1
 *                                                     0  1     2     3  4  5       6
 *
 * Coding Order:
 *  0       5  3  6     2     7  4  8       1       1  1  1     1     1  1  1       9
 *                                                  3  1  4     0     5  2  6
 *
 ***********************************************************************************************************/
static PredictionStructureConfigEntry four_level_hierarchical_pred_struct[] = {

    {
        0,                  // GOP Index 0 - Temporal Layer
        0,                  // GOP Index 0 - Decode Order
        8,     // GOP Index 0 - Ref List 0
        8     // GOP Index 0 - Ref List 1
    },

    {
        3,                  // GOP Index 1 - Temporal Layer
        3,                  // GOP Index 1 - Decode Order
        1,                    // GOP Index 1 - Ref List 0
       -1                    // GOP Index 1 - Ref List 1
    },

    {
        2,                  // GOP Index 2 - Temporal Layer
        2,                  // GOP Index 2 - Decode Order
        2,                     // GOP Index 2 - Ref List 0
       -2                     // GOP Index 2 - Ref List 1
    },

    {
        3,                  // GOP Index 3 - Temporal Layer
        4,                  // GOP Index 3 - Decode Order
        1,                    // GOP Index 3 - Ref List 0
       -1                    // GOP Index 3 - Ref List 1
    },

    {
        1,                   // GOP Index 4 - Temporal Layer
        1,                   // GOP Index 4 - Decode Order
        4,                     // GOP Index 4 - Ref List 0
       -4                     // GOP Index 4 - Ref List 1
    },

    {
        3,                  // GOP Index 5 - Temporal Layer
        6,                  // GOP Index 5 - Decode Order
        1,                     // GOP Index 5 - Ref List 0
       -1                    // GOP Index 5 - Ref List 1
    },

    {
        2,                  // GOP Index 6 - Temporal Layer
        5,                  // GOP Index 6 - Decode Order
        2,                    // GOP Index 6 - Ref List 0
       -2                    // GOP Index 6 - Ref List 1
    },

    {
        3,                  // GOP Index 7 - Temporal Layer
        7,                  // GOP Index 7 - Decode Order
        1,                    // GOP Index 7 - Ref List 0
       -1                    // GOP Index 7 - Ref List 1
    }
};

/***********************************************************************************************************
 * Five-Level Level Hierarchical
 *
 *           b     b           b     b               b     b           b     b              Temporal Layer 4
 *          / \   / \         / \   / \             / \   / \         / \   / \
 *         /   \ /   \       /   \ /   \           /   \ /   \       /   \ /   \
 *        /     B     \     /     B     \         /     B     \     /     B     \           Temporal Layer 3
 *       /     / \     \   /     / \     \       /     / \     \   /     / \     \
 *      /     /   \     \ /     /   \     \     /     /   \     \ /     /   \     \
 *     /     /     ------B------     \     \   /     /     ------B------     \     \        Temporal Layer 2
 *    /     /           / \           \     \ /     /           / \           \     \
 *   /     /           /   \-----------------B------------------   \           \     \      Temporal Layer 1
 *  /     /           /                     / \                     \           \     \
 * I-----------------------------------------------------------------------------------B    Temporal Layer 0
 *
 * Display Order:
 *  0        1  2  3     4     5  6  7       8       9  1  1     1     1  1  1         1
 *                                                      0  1     2     3  4  5         6
 *
 * Coding Order:
 *  0        9  5  1     3     1  6  1       2       1  7  1     4     1  8  1         1
 *                 0           1     2               3     4           5     6
 *
 ***********************************************************************************************************/
static PredictionStructureConfigEntry five_level_hierarchical_pred_struct[] = {

    {
        0,                  // GOP Index 0 - Temporal Layer
        0,                  // GOP Index 0 - Decode Order
        16,                    // GOP Index 0 - Ref List 0
        16                    // GOP Index 0 - Ref List 1
    },

    {
        4,                  // GOP Index 1 - Temporal Layer
        4,                  // GOP Index 1 - Decode Order
        1,                    // GOP Index 1 - Ref List 0
       -1                     // GOP Index 1 - Ref List 1
    },

    {
        3,                  // GOP Index 2 - Temporal Layer
        3,                  // GOP Index 2 - Decode Order
        2,                    // GOP Index 2 - Ref List 0
       -2                    // GOP Index 2 - Ref List 1
    },

    {
        4,                  // GOP Index 3 - Temporal Layer
        5,                  // GOP Index 3 - Decode Order
        1,                     // GOP Index 3 - Ref List 0
       -1                    // GOP Index 3 - Ref List 1
    },

    {
        2,                  // GOP Index 4 - Temporal Layer
        2,                  // GOP Index 4 - Decode Order
        4,                    // GOP Index 4 - Ref List 0
       -4                    // GOP Index 4 - Ref List 1
    },

    {
        4,                  // GOP Index 5 - Temporal Layer
        7,                  // GOP Index 5 - Decode Order
        1,                    // GOP Index 5 - Ref List 0
       -1                    // GOP Index 5 - Ref List 1
    },

    {
        3,                  // GOP Index 6 - Temporal Layer
        6,                  // GOP Index 6 - Decode Order
        2,                    // GOP Index 6 - Ref List 0
       -2                    // GOP Index 6 - Ref List 1
    },

    {
        4,                  // GOP Index 7 - Temporal Layer
        8,                  // GOP Index 7 - Decode Order
        1,                    // GOP Index 7 - Ref List 0
       -1                     // GOP Index 7 - Ref List 1
    },

    {
        1,                  // GOP Index 8 - Temporal Layer
        1,                  // GOP Index 8 - Decode Order
        8,                    // GOP Index 8 - Ref List 0
       -8                    // GOP Index 8 - Ref List 1
    },

    {
        4,                  // GOP Index 9 - Temporal Layer
        11,                 // GOP Index 9 - Decode Order
        1,                     // GOP Index 9 - Ref List 0
       -1                    // GOP Index 9 - Ref List 1
    },

    {
        3,                  // GOP Index 10 - Temporal Layer
        10,                 // GOP Index 10 - Decode Order
        2,                    // GOP Index 10 - Ref List 0
       -2                    // GOP Index 10 - Ref List 1
    },

    {
        4,                  // GOP Index 11 - Temporal Layer
        12,                 // GOP Index 11 - Decode Order
        1,                    // GOP Index 11 - Ref List 0
       -1                     // GOP Index 11 - Ref List 1
    },

    {
        2,                  // GOP Index 12 - Temporal Layer
        9,                  // GOP Index 12 - Decode Order
        4,                    // GOP Index 12 - Ref List 0
       -4                    // GOP Index 12 - Ref List 1
    },

    {
        4,                  // GOP Index 13 - Temporal Layer
        14,                 // GOP Index 13 - Decode Order
        1,                     // GOP Index 13 - Ref List 0
       -1                     // GOP Index 13 - Ref List 1
    },

    {
        3,                  // GOP Index 14 - Temporal Layer
        13,                 // GOP Index 14 - Decode Order
        2,                     // GOP Index 14 - Ref List 0
       -2                     // GOP Index 14 - Ref List 1
    },

    {
        4,                  // GOP Index 15 - Temporal Layer
        15,                 // GOP Index 15 - Decode Order
        1,                    // GOP Index 15 - Ref List 0
       -1                    // GOP Index 15 - Ref List 1
    }
};

/**********************************************************************************************************************************************************************************************************************
 * Six-Level Level Hierarchical
 *
 *
 *              b     b           b     b               b     b           b     b                   b     b           b     b               b     b           b     b               Temporal Layer 5
 *             / \   / \         / \   / \             / \   / \         / \   / \                 / \   / \         / \   / \             / \   / \         / \   / \
 *            /   \ /   \       /   \ /   \           /   \ /   \       /   \ /   \               /   \ /   \       /   \ /   \           /   \ /   \       /   \ /   \
 *           /     B     \     /     B     \         /     B     \     /     B     \             /     B     \     /     B     \         /     B     \     /     B     \            Temporal Layer 4
 *          /     / \     \   /     / \     \       /     / \     \   /     / \     \           /     / \     \   /     / \     \       /     / \     \   /     / \     \
 *         /     /   \     \ /     /   \     \     /     /   \     \ /     /   \     \         /     /   \     \ /     /   \     \     /     /   \     \ /     /   \     \
 *        /     /     ------B------     \     \   /     /     ------B------     \     \       /     /     ------B------     \     \   /     /     ------B------     \     \         Temporal Layer 3
 *       /     /           / \           \     \ /     /           / \           \     \     /     /           / \           \     \ /     /           / \           \     \
 *      /     /           /   \-----------------B------------------   \           \     \   /     /           /   \-----------------B------------------   \           \     \       Temporal Layer 2
 *     /     /           /                     / \                     \           \     \ /     /           /                     / \                     \           \     \
 *    /     /           /                     /   \---------------------------------------B---------------------------------------/   \                     \           \     \     Temporal Layer 1
 *   /     /           /                     /                                           / \                                           \                     \           \     \
 *  I---------------------------------------------------------------------------------------------------------------------------------------------------------------------------B   Temporal Layer 0
 *
 * Display Order:
 *  0           1  2  3     4     5  6  7       8       9  1  1     1     1  1  1         1         1  1  1     2     2  2  2       2       2  2  2     2     2  3  3           3
 *                                                         0  1     2     3  4  5         6         7  8  9     0     1  2  3       4       5  6  7     8     9  0  1           2
 *
 * Coding Order:
 *  0           1  9  1     5     1  1  2       3       2  1  2     6     2  1  2         2         2  1  2     7     2  1  2       4       2  1  3     8     3  1  3           1
 *              7     8           9  0  0               1  1  2           3  2  4                   5  3  6           7  4  8               9  5  0           1  6  2
 *
 **********************************************************************************************************************************************************************************************************************/
static PredictionStructureConfigEntry six_level_hierarchical_pred_struct[] = {

    {
        0,                  // GOP Index 0 - Temporal Layer
        0,                  // GOP Index 0 - Decode Order
        32,                    // GOP Index 0 - Ref List 0
        32                    // GOP Index 0 - Ref List 1
    },

    {
        5,                  // GOP Index 1 - Temporal Layer
        5,                  // GOP Index 1 - Decode Order
        1,                    // GOP Index 1 - Ref List 0
       -1                    // GOP Index 1 - Ref List 1
    },

    {
        4,                  // GOP Index 2 - Temporal Layer
        4,                  // GOP Index 2 - Decode Order
        2,                    // GOP Index 2 - Ref List 0
       -2                    // GOP Index 2 - Ref List 1
    },

    {
        5,                  // GOP Index 3 - Temporal Layer
        6,                  // GOP Index 3 - Decode Order
        1,                    // GOP Index 3 - Ref List 0
       -1                    // GOP Index 3 - Ref List 1
    },

    {
        3,                  // GOP Index 4 - Temporal Layer
        3,                  // GOP Index 4 - Decode Order
        4,                    // GOP Index 4 - Ref List 0
       -4                    // GOP Index 4 - Ref List 1
    },

    {
        5,                  // GOP Index 5 - Temporal Layer
        8,                  // GOP Index 5 - Decode Order
        1,                    // GOP Index 5 - Ref List 0
       -1                    // GOP Index 5 - Ref List 1
    },

    {
        4,                  // GOP Index 6 - Temporal Layer
        7,                  // GOP Index 6 - Decode Order
        2,                    // GOP Index 6 - Ref List 0
       -2                    // GOP Index 6 - Ref List 1
    },

    {
        5,                  // GOP Index 7 - Temporal Layer
        9,                  // GOP Index 7 - Decode Order
        1,                    // GOP Index 7 - Ref List 0
       -1                    // GOP Index 7 - Ref List 1
    },

    {
        2,                  // GOP Index 8 - Temporal Layer
        2,                  // GOP Index 8 - Decode Order
        8,                    // GOP Index 8 - Ref List 0
       -8                    // GOP Index 8 - Ref List 1
    },

    {
        5,                  // GOP Index 9 - Temporal Layer
        12,                 // GOP Index 9 - Decode Order
        1,                    // GOP Index 9 - Ref List 0
       -1                    // GOP Index 9 - Ref List 1
    },

    {
        4,                  // GOP Index 10 - Temporal Layer
        11,                 // GOP Index 10 - Decode Order
         2,                    // GOP Index 10 - Ref List 0
        -2                    // GOP Index 10 - Ref List 1
    },

    {
        5,                  // GOP Index 11 - Temporal Layer
        13,                 // GOP Index 11 - Decode Order
        1,                    // GOP Index 11 - Ref List 0
       -1                    // GOP Index 11 - Ref List 1
    },

    {
        3,                  // GOP Index 12 - Temporal Layer
        10,                 // GOP Index 12 - Decode Order
        4,                    // GOP Index 12 - Ref List 0
       -4                    // GOP Index 12 - Ref List 1
    },

    {
        5,                  // GOP Index 13 - Temporal Layer
        15,                 // GOP Index 13 - Decode Order
        1,                    // GOP Index 13 - Ref List 0
       -1                    // GOP Index 13 - Ref List 1
    },

    {
        4,                  // GOP Index 14 - Temporal Layer
        14,                 // GOP Index 14 - Decode Order
        2,                    // GOP Index 14 - Ref List 0
       -2                    // GOP Index 14 - Ref List 1
    },

    {
        5,                  // GOP Index 15 - Temporal Layer
        16,                 // GOP Index 15 - Decode Order
        1,                    // GOP Index 15 - Ref List 0
       -1                    // GOP Index 15 - Ref List 1
    },

    {
        1,                  // GOP Index 16 - Temporal Layer
        1,                  // GOP Index 16 - Decode Order
        16,                    // GOP Index 16 - Ref List 0
       -16                    // GOP Index 16 - Ref List 1
    },

    {
        5,                  // GOP Index 17 - Temporal Layer
        20,                 // GOP Index 17 - Decode Order
        1,                    // GOP Index 17 - Ref List 0
       -1                    // GOP Index 17 - Ref List 1
    },

    {
        4,                  // GOP Index 18 - Temporal Layer
        19,                 // GOP Index 18 - Decode Order
        2,                    // GOP Index 18 - Ref List 0
       -2                    // GOP Index 18 - Ref List 1
    },

    {
        5,                  // GOP Index 19 - Temporal Layer
        21,                 // GOP Index 19 - Decode Order
        1,                    // GOP Index 19 - Ref List 0
       -1                    // GOP Index 19 - Ref List 1
    },

    {
        3,                  // GOP Index 20 - Temporal Layer
        18,                 // GOP Index 20 - Decode Order
         4,                    // GOP Index 20 - Ref List 0
        -4                    // GOP Index 20 - Ref List 1
    },

    {
        5,                  // GOP Index 21 - Temporal Layer
        23,                 // GOP Index 21 - Decode Order
        1,                    // GOP Index 21 - Ref List 0
       -1                     // GOP Index 21 - Ref List 1
    },

    {
        4,                  // GOP Index 22 - Temporal Layer
        22,                 // GOP Index 22 - Decode Order
        2,                    // GOP Index 22 - Ref List 0
       -2                    // GOP Index 22 - Ref List 1
    },

    {
        5,                  // GOP Index 23 - Temporal Layer
        24,                 // GOP Index 23 - Decode Order
        1,                    // GOP Index 23 - Ref List 0
       -1                    // GOP Index 23 - Ref List 1
    },

    {
        2,                  // GOP Index 24 - Temporal Layer
        17,                 // GOP Index 24 - Decode Order
        8,                    // GOP Index 24 - Ref List 0
       -8                    // GOP Index 24 - Ref List 1
    },

    {
        5,                  // GOP Index 25 - Temporal Layer
        27,                 // GOP Index 25 - Decode Order
        1,                    // GOP Index 25 - Ref List 0
       -1                    // GOP Index 25 - Ref List 1
    },

    {
        4,                  // GOP Index 26 - Temporal Layer
        26,                 // GOP Index 26 - Decode Order
        2,                    // GOP Index 26 - Ref List 0
       -2                    // GOP Index 26 - Ref List 1
    },

    {
        5,                  // GOP Index 27 - Temporal Layer
        28,                 // GOP Index 27 - Decode Order
        1,                    // GOP Index 27 - Ref List 0
       -1                    // GOP Index 27 - Ref List 1
    },

    {
        3,                  // GOP Index 28 - Temporal Layer
        25,                 // GOP Index 28 - Decode Order
         4,                    // GOP Index 28 - Ref List 0
        -4                    // GOP Index 28 - Ref List 1
    },

    {
        5,                  // GOP Index 29 - Temporal Layer
        30,                 // GOP Index 29 - Decode Order
        1,                    // GOP Index 29 - Ref List 0
       -1                    // GOP Index 29 - Ref List 1
    },

    {
        4,                  // GOP Index 30 - Temporal Layer
        29,                 // GOP Index 30 - Decode Order
        2,                    // GOP Index 30 - Ref List 0
       -2                    // GOP Index 30 - Ref List 1
    },

    {
        5,                  // GOP Index 31 - Temporal Layer
        31,                 // GOP Index 31 - Decode Order
        1,                    // GOP Index 31 - Ref List 0
       -1                    // GOP Index 31 - Ref List 1
    }
};

/************************************************
 * Prediction Structure Config Array
 ************************************************/
static const PredictionStructureConfig prediction_structure_config_array[] = {
    {1,     flat_pred_struct},
    {2,     two_level_hierarchical_pred_struct},
    {4,     three_level_hierarchical_pred_struct},
    {8,     four_level_hierarchical_pred_struct},
    {16,    five_level_hierarchical_pred_struct},
    {32,    six_level_hierarchical_pred_struct},
    {0,     (PredictionStructureConfigEntry*) EB_NULL} // Terminating Code, must always come last!
};

/************************************************
 * Get Prediction Structure
 ************************************************/
PredictionStructure* eb_vp9_get_prediction_structure(
    PredictionStructureGroup *prediction_structure_group_ptr,
    EB_PRED                   pred_structure,
    uint32_t                  number_of_references,
    uint32_t                  levels_of_hierarchy)
{
    PredictionStructure *pred_struct_ptr;
    uint32_t pred_struct_index;

    // Convert number_of_references to an index
    --number_of_references;

    // Determine the Index value
    pred_struct_index  = PRED_STRUCT_INDEX(levels_of_hierarchy, (uint32_t) pred_structure, number_of_references);

    pred_struct_ptr = prediction_structure_group_ptr->prediction_structure_ptr_array[pred_struct_index];

    return pred_struct_ptr;
}

/********************************************************************************************
 * Prediction Structure Ctor
 *
 * GOP Type:
 *   For Low Delay P, eliminate Ref List 0
 *   Fow Low Delay B, copy Ref List 0 into Ref List 1
 *   For Random Access, leave config as is
 *
 * number_of_references:
 *   Clip the Ref Lists
 *
 *  Summary:
 *
 *  The Pred Struct Ctor constructs the Reference Lists, Dependent Lists, and RPS for each
 *    valid prediction structure position. The full prediction structure is composed of four
 *    sections:
 *    a. Leading Pictures
 *    b. Initialization Pictures
 *    c. Steady-state Pictures
 *    d. Trailing Pictures
 *
 *  By definition, the Prediction Structure Config describes the Steady-state Picture
 *    Set. From the PS Config, the other sections are determined by following a simple
 *    set of construction rules. These rules are:
 *    -Leading Pictures use only List 1 of the Steady-state for forward-prediction
 *    -Init Pictures don't violate CRA mechanics
 *    -Steady-state Pictures come directly from the PS Config
 *    -Following pictures use only List 0 of the Steady-state for rear-prediction
 *
 *  In general terms, Leading and Trailing pictures are useful when trying to reduce
 *    the number of base-layer pictures in the presense of scene changes.  Trailing
 *    pictures are also useful for terminating sequences.  Init pictures are needed
 *    when using multiple references that expand outside of a Prediction Structure.
 *    Steady-state pictures are the normal use cases.
 *
 *  Leading and Trailing Pictures are not applicable to Low Delay prediction structures.
 *
 *  Below are a set of example PS diagrams
 *
 *  Low-delay P, Flat, 2 reference:
 *
 *                    I---P---P
 *
 *  Display Order     0   1   2
 *
 *  Sections:
 *    Let PredStructSize = N
 *    Leading Pictures:     [null]  Size: 0
 *    Init Pictures:        [0-1]   Size: Ceil(MaxReference, N) - N + 1
 *    Stead-state Pictures: [2]     Size: N
 *    Trailing Pictures:    [null]  Size: 0
 *    ------------------------------------------
 *      Total Size: Ceil(MaxReference, N) + 1
 *
 *  Low-delay B, 3-level, 2 references:
 *
 *                         b   b     b   b
 *                        /   /     /   /
 *                       /   B     /   B
 *                      /   /     /   /
 *                     I---------B-----------B
 *
 *  Display Order      0   1 2 3 4   5 6 7   8
 *
 *  Sections:
 *    Let PredStructSize = N
 *    Leading Pictures:     [null]  Size: 0
 *    Init Pictures:        [1-4]   Size: Ceil(MaxReference, N) - N + 1
 *    Stead-state Pictures: [5-8]   Size: N
 *    Trailing Pictures:    [null]  Size: 0
 *    ------------------------------------------
 *      Total Size: Ceil(MaxReference, N) + 1
 *
 *  Random Access, 3-level structure with 3 references:
 *
 *                   p   p       b   b       b   b       b   b       p   p
 *                    \   \     / \ / \     / \ / \     / \ / \     /   /
 *                     P   \   /   B   \   /   B   \   /   B   \   /   P
 *                      \   \ /   / \   \ /   / \   \ /   / \   \ /   /
 *                       ----I-----------B-----------B-----------B----
 *  Display Order:   0 1 2   3   4 5 6   7   8 9 1   1   1 1 1   1   1 1 1
 *                                               0   1   2 3 4   5   6 7 8
 *
 *  Decode Order:    2 1 3   0   6 5 7   4   1 9 1   8   1 1 1   1   1 1 1
 *                                           0   1       4 3 5   2   6 7 8
 *
 *  Sections:
 *    Let PredStructSize = N
 *    Leading Pictures:      [0-2]   Size: N - 1
 *    Init Pictures:         [3-11]  Size: Ceil(MaxReference, N) - N + 1
 *    Steady-state Pictures: [12-15] Size: N
 *    Trailing Pictures:     [16-18] Size: N - 1
 *    ------------------------------------------
 *      Total Size: 2*N + Ceil(MaxReference, N) - 1
 *
 *  Encoding Order:
 *                   -------->----------->----------->-----------|------->
 *                                                   |           |
 *                                                   |----<------|
 *
 *
 *  Timeline:
 *
 *  The timeline is a tool that is used to determine for how long a
 *    picture should be preserved for future reference. The concept of
 *    future reference is equivalently defined as dependence.  The RPS
 *    mechanism works by signaling for each picture in the DPB whether
 *    it is used for direct reference, kept for future reference, or
 *    discarded.  The timeline merely provides a means of determing
 *    the reference picture states at each prediction structure position.
 *    Its also important to note that all signaling should be done relative
 *    to decode order, not display order. Display order is irrelevant except
 *    for signaling the POC.
 *
 *  Timeline Example: 3-Level Hierarchical with Leading and Trailing Pictures, 3 References
 *
 *                   p   p       b   b       b   b       b   b       p   p   Temporal Layer 2
 *                    \   \     / \ / \     / \ / \     / \ / \     /   /
 *                     P   \   /   B   \   /   B   \   /   B   \   /   P     Temporal Layer 1
 *                      \   \ /   / \   \ /   / \   \ /   / \   \ /   /
 *                       ----I-----------B-----------B-----------B----       Temporal Layer 0
 *
 *  Decode Order:    2 1 3   0   6 5 7   4   1 9 1   8   1 1 1   1   1 1 1
 *                                           0   1       4 3 5   2   6 7 8
 *
 *  Display Order:   0 1 2   3   4 5 6   7   8 9 1   1   1 1 1   1   1 1 1
 *                                               0   1   2 3 4   5   6 7 8
 *            X --->
 *
 *                                1 1 1 1 1 1 1 1 1   DECODE ORDER
 *            0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8
 *
 *         |----------------------------------------------------------
 *         |
 *  Y D  0 |  \ x---x-x-x-x---x-x-x---x-x-x
 *  | E  1 |    \ x
 *  | C  2 |      \
 *  | O  3 |        \
 *  v D  4 |          \ x---x-x-x-x---x-x-x---x-x-x
 *    E  5 |            \ x-x
 *       6 |              \
 *    O  7 |                \
 *    R  8 |                  \ x---x-x-x-x---x-x-x
 *    D  9 |                    \ x-x
 *    E 10 |                      \
 *    R 11 |                        \
 *      12 |                          \ x---x-x-x-x
 *      13 |                            \ x-x
 *      14 |                              \
 *      15 |                                \
 *      16 |                                  \
 *      17 |                                    \ x
 *      18 |                                      \
 *
 *  Interpreting the timeline:
 *
 *  The most important detail to keep in mind is that all signaling
 *    is done in Decode Order space. The symbols mean the following:
 *    'x' directly referenced picture
 *    '-' picture kept for future reference
 *    ' ' not referenced, inferred discard
 *    '\' eqivalent to ' ', deliminiter that nothing can be to the left of
 *
 *  The basic steps for constructing the timeline are to increment through
 *    each position in the prediction structure (Y-direction on the timeline)
 *    and mark the appropriate state: directly referenced, kept for future reference,
 *    or discarded.  As shown, base-layer pictures are referenced much more
 *    frequently than by the other layers.
 *
 *  The RPS is constructed by looking at each 'x' position in the timeline and
 *    signaling each 'y' reference as depicted in the timeline. DPB analysis is
 *    fairly straigtforward - the total number of directly-referenced and
 *    kept-for-future-reference pictures should not exceed the DPB size.
 *
 *  The RPS Ctor code follows these construction steps.
 ******************************************************************************************/
static EbErrorType prediction_structure_ctor(
    PredictionStructure            **prediction_structure_dbl_ptr,
    const PredictionStructureConfig *prediction_structure_config_ptr,
    EB_PRED                          pred_type,
    uint32_t                         number_of_references)
{
    uint32_t                  entry_index;
    uint32_t                  config_entry_index;
    uint32_t                  ref_index;

    // Section Variables
    uint32_t                  leading_pic_count;
    uint32_t                  init_pic_count;
    uint32_t                  steady_state_pic_count;

    PredictionStructure  *prediction_structure_ptr;
    EB_MALLOC(PredictionStructure*, prediction_structure_ptr, sizeof(PredictionStructure), EB_N_PTR);
    *prediction_structure_dbl_ptr = prediction_structure_ptr;
    EB_MEMSET(prediction_structure_ptr, 0, sizeof(PredictionStructure));

    prediction_structure_ptr->pred_type = pred_type;

    // Set the Pred Struct Period
    prediction_structure_ptr->pred_struct_period = prediction_structure_config_ptr->entry_count;

    //----------------------------------------
    // Find the Pred Struct Entry Count
    //   There are four sections of the pred struct:
    //     -Leading Pictures        Size: N-1
    //     -Init Pictures           Size: Ceil(MaxReference, N) - N + 1
    //     -Steady-state Pictures   Size: N
    //     -Trailing Pictures       Size: N-1
    //----------------------------------------

    //----------------------------------------
    // Determine the Prediction Structure Size
    //   First, start by determining
    //   Ceil(MaxReference, N)
    //----------------------------------------
    {
        int32_t max_ref = MIN_SIGNED_VALUE;
        for(config_entry_index = 0, entry_index = prediction_structure_config_ptr->entry_count - 1; config_entry_index < prediction_structure_config_ptr->entry_count; ++config_entry_index) {

            // Increment through Reference List 0
            ref_index = 0;
            while(ref_index < number_of_references && prediction_structure_config_ptr->entry_array[config_entry_index].ref_list0 != 0) {
                //max_ref = MAX(prediction_structure_config_ptr->entry_array[config_entry_index].ref_list0[ref_index], max_ref);
                max_ref = MAX((int32_t) (prediction_structure_config_ptr->entry_count - entry_index - 1) + prediction_structure_config_ptr->entry_array[config_entry_index].ref_list0, max_ref);
                ++ref_index;
            }

            // Increment through Reference List 1 (Random Access only)
            if(pred_type == EB_PRED_RANDOM_ACCESS) {
                ref_index = 0;
                while(ref_index < number_of_references && prediction_structure_config_ptr->entry_array[config_entry_index].ref_list1 != 0) {
                    //max_ref = MAX(prediction_structure_config_ptr->entry_array[config_entry_index].ref_list1[ref_index], max_ref);
                    max_ref = MAX((int32_t) (prediction_structure_config_ptr->entry_count - entry_index - 1) + prediction_structure_config_ptr->entry_array[config_entry_index].ref_list1, max_ref);
                    ++ref_index;
                }
            }

            // Increment entry_index
            entry_index = (entry_index == prediction_structure_config_ptr->entry_count - 1) ? 0 : entry_index + 1;
        }

        // Perform the Ceil(MaxReference, N) operation
        prediction_structure_ptr->maximum_extent = CEILING(max_ref,prediction_structure_ptr->pred_struct_period);

        // Set the Section Sizes
        leading_pic_count         = (pred_type == EB_PRED_RANDOM_ACCESS) ?     // No leading pictures in low-delay configurations
                                  prediction_structure_ptr->pred_struct_period - 1:
                                  0;
        init_pic_count            = prediction_structure_ptr->maximum_extent - prediction_structure_ptr->pred_struct_period + 1;
        steady_state_pic_count     = prediction_structure_ptr->pred_struct_period;
        //trailingPicCount        = (pred_type == EB_PRED_RANDOM_ACCESS) ?     // No trailing pictures in low-delay configurations
        //    prediction_structure_ptr->pred_struct_period - 1:
        //    0;

        // Set the total Entry Count
        prediction_structure_ptr->pred_struct_entry_count =
            leading_pic_count +
            init_pic_count +
            steady_state_pic_count;

        // Set the Section Indices
        prediction_structure_ptr->leading_pic_index     = 0;
        prediction_structure_ptr->init_pic_index        = prediction_structure_ptr->leading_pic_index + leading_pic_count;
        prediction_structure_ptr->steady_state_index    = prediction_structure_ptr->init_pic_index + init_pic_count;
    }

    // Allocate the entry array
    EB_MALLOC(PredictionStructureEntry**, prediction_structure_ptr->pred_struct_entry_ptr_array, sizeof(PredictionStructureEntry*) * prediction_structure_ptr->pred_struct_entry_count, EB_N_PTR);
    // Allocate the entries
    for(entry_index = 0; entry_index < prediction_structure_ptr->pred_struct_entry_count; ++entry_index) {
        EB_MALLOC(PredictionStructureEntry*, prediction_structure_ptr->pred_struct_entry_ptr_array[entry_index], sizeof(PredictionStructureEntry), EB_N_PTR);
        EB_MEMSET(prediction_structure_ptr->pred_struct_entry_ptr_array[entry_index], 0, sizeof(PredictionStructureEntry));
    }

    // Find the Max Temporal Layer Index
    prediction_structure_ptr->temporal_layer_count = 0;
    for(config_entry_index = 0; config_entry_index < prediction_structure_config_ptr->entry_count; ++config_entry_index) {
        prediction_structure_ptr->temporal_layer_count = MAX(prediction_structure_config_ptr->entry_array[config_entry_index].temporal_layer_index,prediction_structure_ptr->temporal_layer_count);
    }

    // Increment the Zero-indexed temporal layer index to get the total count
    ++prediction_structure_ptr->temporal_layer_count;

    //----------------------------------------
    // Construct Leading Pictures
    //   -Use only Ref List1 from the Config
    //   -Note the Config starts from the 2nd position to construct the leading pictures
    //----------------------------------------
    {
        for(entry_index = 0, config_entry_index = 1; entry_index < leading_pic_count; ++entry_index, ++config_entry_index) {

            // Find the Size of the Config's Reference List 1
            ref_index = 0;
            while(ref_index < number_of_references && prediction_structure_config_ptr->entry_array[config_entry_index].ref_list1 != 0) {
                ++ref_index;
            }

            // Set Leading Picture's Reference List 0 Count {Config List1 => LeadingPic List 0}
            prediction_structure_ptr->pred_struct_entry_ptr_array[entry_index]->ref_list0.reference_list_count = ref_index;

            // Allocate the Leading Picture Reference List 0
             prediction_structure_ptr->pred_struct_entry_ptr_array[entry_index]->ref_list0.reference_list = 0;

            // Copy Config List1 => LeadingPic Reference List 0
            for(ref_index = 0; ref_index < prediction_structure_ptr->pred_struct_entry_ptr_array[entry_index]->ref_list0.reference_list_count; ++ref_index) {
                prediction_structure_ptr->pred_struct_entry_ptr_array[entry_index]->ref_list0.reference_list = prediction_structure_config_ptr->entry_array[config_entry_index].ref_list1;
            }

            // Null out List 1
            prediction_structure_ptr->pred_struct_entry_ptr_array[entry_index]->ref_list1.reference_list_count = 0;
            prediction_structure_ptr->pred_struct_entry_ptr_array[entry_index]->ref_list1.reference_list = 0;

            // Set the Temporal Layer Index
            prediction_structure_ptr->pred_struct_entry_ptr_array[entry_index]->temporal_layer_index = prediction_structure_config_ptr->entry_array[config_entry_index].temporal_layer_index;

            // Set the Decode Order
            prediction_structure_ptr->pred_struct_entry_ptr_array[entry_index]->decode_order = (pred_type == EB_PRED_RANDOM_ACCESS) ?
                    prediction_structure_config_ptr->entry_array[config_entry_index].decode_order :
                    entry_index;

        }
    }

    //----------------------------------------
    // Construct Init Pictures
    //   -Use only references from Ref List0 & Ref List1 from the Config that don't violate CRA mechanics
    //   -The Config Index cycles through continuously
    //----------------------------------------
    {
        uint32_t terminating_entry_index = entry_index + init_pic_count;
        int32_t poc_value;

        for(config_entry_index = 0, poc_value = 0; entry_index < terminating_entry_index; ++entry_index, ++poc_value) {

            // REFERENCE LIST 0

            // Find the Size of the Config's Reference List 0
            ref_index = 0;
            while(
                ref_index < number_of_references &&
                prediction_structure_config_ptr->entry_array[config_entry_index].ref_list0 != 0 &&
                poc_value - prediction_structure_config_ptr->entry_array[config_entry_index].ref_list0 >= 0)  // Stop when we violate the CRA (i.e. reference past it)
            {
                ++ref_index;
            }

            // Set Leading Picture's Reference List 0 Count
            prediction_structure_ptr->pred_struct_entry_ptr_array[entry_index]->ref_list0.reference_list_count = ref_index;

            // Allocate the Leading Picture Reference List 0{
            prediction_structure_ptr->pred_struct_entry_ptr_array[entry_index]->ref_list0.reference_list = 0;

            // Copy Reference List 0
            for(ref_index = 0; ref_index < prediction_structure_ptr->pred_struct_entry_ptr_array[entry_index]->ref_list0.reference_list_count; ++ref_index) {
                prediction_structure_ptr->pred_struct_entry_ptr_array[entry_index]->ref_list0.reference_list = prediction_structure_config_ptr->entry_array[config_entry_index].ref_list0;
            }

            // REFERENCE LIST 1
            switch(pred_type) {

            case EB_PRED_LOW_DELAY_P:

                // Null out List 1
                prediction_structure_ptr->pred_struct_entry_ptr_array[entry_index]->ref_list1.reference_list_count = 0;
                prediction_structure_ptr->pred_struct_entry_ptr_array[entry_index]->ref_list1.reference_list = 0;

                break;

            case EB_PRED_LOW_DELAY_B:

                // Copy List 0 => List 1

                // Set Leading Picture's Reference List 1 Count
                prediction_structure_ptr->pred_struct_entry_ptr_array[entry_index]->ref_list1.reference_list_count = prediction_structure_ptr->pred_struct_entry_ptr_array[entry_index]->ref_list0.reference_list_count;

                // Allocate the Leading Picture Reference List 1
                prediction_structure_ptr->pred_struct_entry_ptr_array[entry_index]->ref_list1.reference_list = 0;

                // Copy Reference List 1
                for(ref_index = 0; ref_index < prediction_structure_ptr->pred_struct_entry_ptr_array[entry_index]->ref_list1.reference_list_count; ++ref_index) {
                    prediction_structure_ptr->pred_struct_entry_ptr_array[entry_index]->ref_list1.reference_list = prediction_structure_ptr->pred_struct_entry_ptr_array[entry_index]->ref_list0.reference_list;
                }

                break;

            case EB_PRED_RANDOM_ACCESS:

                // Find the Size of the Config's Reference List 1
                ref_index = 0;
                while(
                    ref_index < number_of_references &&
                    prediction_structure_config_ptr->entry_array[config_entry_index].ref_list1 != 0 &&
                    poc_value - prediction_structure_config_ptr->entry_array[config_entry_index].ref_list1 >= 0) // Stop when we violate the CRA (i.e. reference past it)
                {
                    ++ref_index;
                }

                // Set Leading Picture's Reference List 1 Count
                prediction_structure_ptr->pred_struct_entry_ptr_array[entry_index]->ref_list1.reference_list_count = ref_index;

                // Allocate the Leading Picture Reference List 1
                prediction_structure_ptr->pred_struct_entry_ptr_array[entry_index]->ref_list1.reference_list = 0;

                // Copy Reference List 1
                for(ref_index = 0; ref_index < prediction_structure_ptr->pred_struct_entry_ptr_array[entry_index]->ref_list1.reference_list_count; ++ref_index) {
                    prediction_structure_ptr->pred_struct_entry_ptr_array[entry_index]->ref_list1.reference_list = prediction_structure_config_ptr->entry_array[config_entry_index].ref_list1;
                }

                break;

            default:
                break;

            }

            // Set the Temporal Layer Index
            prediction_structure_ptr->pred_struct_entry_ptr_array[entry_index]->temporal_layer_index = prediction_structure_config_ptr->entry_array[config_entry_index].temporal_layer_index;

            // Set the Decode Order
            prediction_structure_ptr->pred_struct_entry_ptr_array[entry_index]->decode_order = (pred_type == EB_PRED_RANDOM_ACCESS) ?
                    prediction_structure_config_ptr->entry_array[config_entry_index].decode_order :
                    entry_index;

            // Rollover the Config Index
            config_entry_index = (config_entry_index == prediction_structure_config_ptr->entry_count - 1) ?
                               0:
                               config_entry_index + 1;
        }
    }

    //----------------------------------------
    // Construct Steady-state Pictures
    //   -Copy directly from the Config
    //----------------------------------------
    {
        uint32_t terminating_entry_index = entry_index + steady_state_pic_count;

        for(/*config_entry_index = 0*/; entry_index < terminating_entry_index; ++entry_index/*, ++config_entry_index*/) {

            // Find the Size of Reference List 0
            ref_index = 0;
            while(ref_index < number_of_references && prediction_structure_config_ptr->entry_array[config_entry_index].ref_list0 != 0) {
                ++ref_index;
            }

            // Set Reference List 0 Count
            prediction_structure_ptr->pred_struct_entry_ptr_array[entry_index]->ref_list0.reference_list_count = ref_index;

            // Allocate Reference List 0
            //EB_MALLOC(int32_t*, prediction_structure_ptr->pred_struct_entry_ptr_array[entry_index]->ref_list0.reference_list, sizeof(int32_t) * prediction_structure_ptr->pred_struct_entry_ptr_array[entry_index]->ref_list0.reference_list_count, EB_N_PTR);
            // Copy Reference List 0
            for(ref_index = 0; ref_index < prediction_structure_ptr->pred_struct_entry_ptr_array[entry_index]->ref_list0.reference_list_count; ++ref_index) {
                prediction_structure_ptr->pred_struct_entry_ptr_array[entry_index]->ref_list0.reference_list = prediction_structure_config_ptr->entry_array[config_entry_index].ref_list0;
            }

            // REFERENCE LIST 1
            switch(pred_type) {

            case EB_PRED_LOW_DELAY_P:

                // Null out List 1
                prediction_structure_ptr->pred_struct_entry_ptr_array[entry_index]->ref_list1.reference_list_count = 0;
                prediction_structure_ptr->pred_struct_entry_ptr_array[entry_index]->ref_list1.reference_list = 0;

                break;

            case EB_PRED_LOW_DELAY_B:

                // Copy List 0 => List 1

                // Set Leading Picture's Reference List 1 Count
                prediction_structure_ptr->pred_struct_entry_ptr_array[entry_index]->ref_list1.reference_list_count = prediction_structure_ptr->pred_struct_entry_ptr_array[entry_index]->ref_list0.reference_list_count;

                // Allocate the Leading Picture Reference List 1
                prediction_structure_ptr->pred_struct_entry_ptr_array[entry_index]->ref_list1.reference_list = 0;

                // Copy Reference List 1
                for(ref_index = 0; ref_index < prediction_structure_ptr->pred_struct_entry_ptr_array[entry_index]->ref_list1.reference_list_count; ++ref_index) {
                    prediction_structure_ptr->pred_struct_entry_ptr_array[entry_index]->ref_list1.reference_list = prediction_structure_ptr->pred_struct_entry_ptr_array[entry_index]->ref_list0.reference_list;
                }

                break;

            case EB_PRED_RANDOM_ACCESS:

                // Find the Size of the Config's Reference List 1
                ref_index = 0;
                while(ref_index < number_of_references && prediction_structure_config_ptr->entry_array[config_entry_index].ref_list1 != 0) {
                    ++ref_index;
                }

                // Set Leading Picture's Reference List 1 Count
                prediction_structure_ptr->pred_struct_entry_ptr_array[entry_index]->ref_list1.reference_list_count = ref_index;

                // Allocate the Leading Picture Reference List 1
                prediction_structure_ptr->pred_struct_entry_ptr_array[entry_index]->ref_list1.reference_list = 0;

                // Copy Reference List 1
                for(ref_index = 0; ref_index < prediction_structure_ptr->pred_struct_entry_ptr_array[entry_index]->ref_list1.reference_list_count; ++ref_index) {
                    prediction_structure_ptr->pred_struct_entry_ptr_array[entry_index]->ref_list1.reference_list = prediction_structure_config_ptr->entry_array[config_entry_index].ref_list1;
                }

                break;

            default:
                break;

            }

            // Set the Temporal Layer Index
            prediction_structure_ptr->pred_struct_entry_ptr_array[entry_index]->temporal_layer_index = prediction_structure_config_ptr->entry_array[config_entry_index].temporal_layer_index;

            // Set the Decode Order
            prediction_structure_ptr->pred_struct_entry_ptr_array[entry_index]->decode_order = (pred_type == EB_PRED_RANDOM_ACCESS) ?
                    prediction_structure_config_ptr->entry_array[config_entry_index].decode_order :
                    entry_index;

            // Rollover the Config Index
            config_entry_index = (config_entry_index == prediction_structure_config_ptr->entry_count - 1) ?
                               0:
                               config_entry_index + 1;
        }
    }

    //----------------------------------------
    // Construct Trailing Pictures
    //   -Use only Ref List0 from the Config
    //----------------------------------------
    //{
    //    uint32_t terminating_entry_index = entry_index + trailingPicCount;
    //
    //    for(config_entry_index = 0; entry_index < terminating_entry_index; ++entry_index, ++config_entry_index) {
    //
    //        // Set Reference List 0 Count
    //        // *Note - only 1 reference is used for trailing pictures.  If you have frequent CRAs and the Pred Struct
    //        //   has many references, you can run into edge conditions.
    //        prediction_structure_ptr->pred_struct_entry_ptr_array[entry_index]->ref_list0.reference_list_count = 1;
    //
    //        // Allocate Reference List 0
    //        prediction_structure_ptr->pred_struct_entry_ptr_array[entry_index]->ref_list0.reference_list = (int32_t*) malloc(sizeof(int32_t) * prediction_structure_ptr->pred_struct_entry_ptr_array[entry_index]->ref_list0.reference_list_count);
    //
    //        // Copy Reference List 0
    //        for(ref_index = 0; ref_index < prediction_structure_ptr->pred_struct_entry_ptr_array[entry_index]->ref_list0.reference_list_count; ++ref_index) {
    //            prediction_structure_ptr->pred_struct_entry_ptr_array[entry_index]->ref_list0.reference_list[ref_index] = prediction_structure_config_ptr->entry_array[config_entry_index].ref_list0[ref_index];
    //        }
    //
    //        // Null out List 1
    //        prediction_structure_ptr->pred_struct_entry_ptr_array[entry_index]->ref_list1.reference_list_count = 0;
    //        prediction_structure_ptr->pred_struct_entry_ptr_array[entry_index]->ref_list1.reference_list = (int32_t*) EB_NULL;
    //
    //        // Set the Temporal Layer Index
    //        prediction_structure_ptr->pred_struct_entry_ptr_array[entry_index]->temporal_layer_index = prediction_structure_config_ptr->entry_array[config_entry_index].temporal_layer_index;
    //
    //        // Set the Decode Order
    //        prediction_structure_ptr->pred_struct_entry_ptr_array[entry_index]->decode_order = (pred_type == EB_PRED_RANDOM_ACCESS) ?
    //            prediction_structure_config_ptr->entry_array[config_entry_index].decode_order :
    //            entry_index;
    //    }
    //}

    //----------------------------------------
    // CONSTRUCT DEPENDENT LIST 0
    //----------------------------------------

    {
        int64_t   dep_index;
        uint64_t  picture_number;

        // First, determine the Dependent List Size for each Entry by incrementing the dependent list length
        {

            // Go through a single pass of the Leading Pictures and Init pictures
            for(picture_number = 0, entry_index = 0; picture_number < prediction_structure_ptr->steady_state_index; ++picture_number) {

                // Go through each Reference picture and accumulate counts
                for(ref_index = 0; ref_index < prediction_structure_ptr->pred_struct_entry_ptr_array[entry_index]->ref_list0.reference_list_count; ++ref_index) {

                    dep_index = picture_number - prediction_structure_ptr->pred_struct_entry_ptr_array[entry_index]->ref_list0.reference_list;

                    if(dep_index >= 0 && dep_index < (int32_t) (prediction_structure_ptr->steady_state_index + prediction_structure_ptr->pred_struct_period)) {
                        ++prediction_structure_ptr->pred_struct_entry_ptr_array[dep_index]->dep_list0.list_count;
                    }
                }

                // Increment the entry_index
                ++entry_index;
            }

            // Go through an entire maximum extent pass for the Steady-state pictures
            for(entry_index = prediction_structure_ptr->steady_state_index; picture_number <= prediction_structure_ptr->steady_state_index + 2*prediction_structure_ptr->maximum_extent; ++picture_number) {

                // Go through each Reference picture and accumulate counts
                for(ref_index = 0; ref_index < prediction_structure_ptr->pred_struct_entry_ptr_array[entry_index]->ref_list0.reference_list_count; ++ref_index) {

                    dep_index = picture_number - prediction_structure_ptr->pred_struct_entry_ptr_array[entry_index]->ref_list0.reference_list;

                    if(dep_index >= 0 && dep_index < (int32_t) (prediction_structure_ptr->steady_state_index + prediction_structure_ptr->pred_struct_period)) {
                        ++prediction_structure_ptr->pred_struct_entry_ptr_array[dep_index]->dep_list0.list_count;
                    }
                }

                // Rollover the entry_index each time it reaches the end of the steady state index
                entry_index = (entry_index == prediction_structure_ptr->pred_struct_entry_count - 1) ?
                             prediction_structure_ptr->steady_state_index :
                             entry_index + 1;
            }
        }

        // Second, allocate memory for each dependent list of each Entry
        for(entry_index = 0; entry_index < prediction_structure_ptr->pred_struct_entry_count; ++entry_index) {

            // If the dependent list count is non-zero, allocate the list, else the list is NULL.
            if(prediction_structure_ptr->pred_struct_entry_ptr_array[entry_index]->dep_list0.list_count > 0) {
                EB_MALLOC(int32_t*, prediction_structure_ptr->pred_struct_entry_ptr_array[entry_index]->dep_list0.list, sizeof(int32_t) * prediction_structure_ptr->pred_struct_entry_ptr_array[entry_index]->dep_list0.list_count, EB_N_PTR);
            }
            else {
                prediction_structure_ptr->pred_struct_entry_ptr_array[entry_index]->dep_list0.list = (int32_t*) EB_NULL;
            }
        }

        // Third, reset the Dependent List Length (they are re-derived)
        for(entry_index = 0; entry_index < prediction_structure_ptr->pred_struct_entry_count; ++entry_index) {
            prediction_structure_ptr->pred_struct_entry_ptr_array[entry_index]->dep_list0.list_count = 0;
        }

        // Fourth, run through each Reference List entry again and populate the Dependent Lists and Dep List Counts
        {
            // Go through a single pass of the Leading Pictures and Init pictures
            for(picture_number = 0, entry_index = 0; picture_number < prediction_structure_ptr->steady_state_index; ++picture_number) {

                // Go through each Reference picture and accumulate counts
                for(ref_index = 0; ref_index < prediction_structure_ptr->pred_struct_entry_ptr_array[entry_index]->ref_list0.reference_list_count; ++ref_index) {

                    dep_index = picture_number - prediction_structure_ptr->pred_struct_entry_ptr_array[entry_index]->ref_list0.reference_list;

                    if(dep_index >= 0 && dep_index < (int32_t) (prediction_structure_ptr->steady_state_index + prediction_structure_ptr->pred_struct_period)) {
                        prediction_structure_ptr->pred_struct_entry_ptr_array[dep_index]->dep_list0.list[prediction_structure_ptr->pred_struct_entry_ptr_array[dep_index]->dep_list0.list_count++] =
                            prediction_structure_ptr->pred_struct_entry_ptr_array[entry_index]->ref_list0.reference_list;
                    }
                }

                // Increment the entry_index
                ++entry_index;
            }

            // Go through an entire maximum extent pass for the Steady-state pictures
            for(entry_index = prediction_structure_ptr->steady_state_index; picture_number <= prediction_structure_ptr->steady_state_index + 2*prediction_structure_ptr->maximum_extent; ++picture_number) {

                // Go through each Reference picture and accumulate counts
                for(ref_index = 0; ref_index < prediction_structure_ptr->pred_struct_entry_ptr_array[entry_index]->ref_list0.reference_list_count; ++ref_index) {

                    dep_index = picture_number - prediction_structure_ptr->pred_struct_entry_ptr_array[entry_index]->ref_list0.reference_list;

                    // Assign the Reference to the Dep List and Increment the Dep List Count
                    if(dep_index >= 0 && dep_index < (int32_t) (prediction_structure_ptr->steady_state_index + prediction_structure_ptr->pred_struct_period)) {
                        prediction_structure_ptr->pred_struct_entry_ptr_array[dep_index]->dep_list0.list[prediction_structure_ptr->pred_struct_entry_ptr_array[dep_index]->dep_list0.list_count++] =
                            prediction_structure_ptr->pred_struct_entry_ptr_array[entry_index]->ref_list0.reference_list;
                    }
                }

                // Rollover the entry_index each time it reaches the end of the steady state index
                entry_index = (entry_index == prediction_structure_ptr->pred_struct_entry_count - 1) ?
                             prediction_structure_ptr->steady_state_index :
                             entry_index + 1;
            }
        }
    }

    //----------------------------------------
    // CONSTRUCT DEPENDENT LIST 1
    //----------------------------------------

    {
        int32_t   dep_index;
        uint32_t  picture_number;

        // First, determine the Dependent List Size for each Entry by incrementing the dependent list length
        {

            // Go through a single pass of the Leading Pictures and Init pictures
            for(picture_number = 0, entry_index = 0; picture_number < prediction_structure_ptr->steady_state_index; ++picture_number) {

                // Go through each Reference picture and accumulate counts
                for(ref_index = 0; ref_index < prediction_structure_ptr->pred_struct_entry_ptr_array[entry_index]->ref_list1.reference_list_count; ++ref_index) {

                    dep_index = picture_number - prediction_structure_ptr->pred_struct_entry_ptr_array[entry_index]->ref_list1.reference_list;

                    if(dep_index >= 0 && dep_index < (int32_t) (prediction_structure_ptr->steady_state_index + prediction_structure_ptr->pred_struct_period)) {
                        ++prediction_structure_ptr->pred_struct_entry_ptr_array[dep_index]->dep_list1.list_count;
                    }
                }

                // Increment the entry_index
                ++entry_index;
            }

            // Go through an entire maximum extent pass for the Steady-state pictures
            for(entry_index = prediction_structure_ptr->steady_state_index; picture_number <= prediction_structure_ptr->steady_state_index + 2*prediction_structure_ptr->maximum_extent; ++picture_number) {

                // Go through each Reference picture and accumulate counts
                for(ref_index = 0; ref_index < prediction_structure_ptr->pred_struct_entry_ptr_array[entry_index]->ref_list1.reference_list_count; ++ref_index) {

                    dep_index = picture_number - prediction_structure_ptr->pred_struct_entry_ptr_array[entry_index]->ref_list1.reference_list;

                    if(dep_index >= 0 && dep_index < (int32_t) (prediction_structure_ptr->steady_state_index + prediction_structure_ptr->pred_struct_period)) {
                        ++prediction_structure_ptr->pred_struct_entry_ptr_array[dep_index]->dep_list1.list_count;
                    }
                }

                // Rollover the entry_index each time it reaches the end of the steady state index
                entry_index = (entry_index == prediction_structure_ptr->pred_struct_entry_count - 1) ?
                             prediction_structure_ptr->steady_state_index :
                             entry_index + 1;
            }
        }

        // Second, allocate memory for each dependent list of each Entry
        for(entry_index = 0; entry_index < prediction_structure_ptr->pred_struct_entry_count; ++entry_index) {

            // If the dependent list count is non-zero, allocate the list, else the list is NULL.
            if(prediction_structure_ptr->pred_struct_entry_ptr_array[entry_index]->dep_list1.list_count > 0) {
                EB_MALLOC(int32_t*, prediction_structure_ptr->pred_struct_entry_ptr_array[entry_index]->dep_list1.list, sizeof(int32_t) * prediction_structure_ptr->pred_struct_entry_ptr_array[entry_index]->dep_list1.list_count, EB_N_PTR);
            }
            else {
                prediction_structure_ptr->pred_struct_entry_ptr_array[entry_index]->dep_list1.list = (int32_t*) EB_NULL;
            }
        }

        // Third, reset the Dependent List Length (they are re-derived)
        for(entry_index = 0; entry_index < prediction_structure_ptr->pred_struct_entry_count; ++entry_index) {
            prediction_structure_ptr->pred_struct_entry_ptr_array[entry_index]->dep_list1.list_count = 0;
        }

        // Fourth, run through each Reference List entry again and populate the Dependent Lists and Dep List Counts
        {
            // Go through a single pass of the Leading Pictures and Init pictures
            for(picture_number = 0, entry_index = 0; picture_number < prediction_structure_ptr->steady_state_index; ++picture_number) {

                // Go through each Reference picture and accumulate counts
                for(ref_index = 0; ref_index < prediction_structure_ptr->pred_struct_entry_ptr_array[entry_index]->ref_list1.reference_list_count; ++ref_index) {

                    dep_index = picture_number - prediction_structure_ptr->pred_struct_entry_ptr_array[entry_index]->ref_list1.reference_list;

                    if(dep_index >= 0 && dep_index < (int32_t) (prediction_structure_ptr->steady_state_index + prediction_structure_ptr->pred_struct_period)) {
                        prediction_structure_ptr->pred_struct_entry_ptr_array[dep_index]->dep_list1.list[prediction_structure_ptr->pred_struct_entry_ptr_array[dep_index]->dep_list1.list_count++] =
                            prediction_structure_ptr->pred_struct_entry_ptr_array[entry_index]->ref_list1.reference_list;
                    }
                }

                // Increment the entry_index
                ++entry_index;
            }

            // Go through an entire maximum extent pass for the Steady-state pictures
            for(entry_index = prediction_structure_ptr->steady_state_index; picture_number <= prediction_structure_ptr->steady_state_index + 2*prediction_structure_ptr->maximum_extent; ++picture_number) {

                // Go through each Reference picture and accumulate counts
                for(ref_index = 0; ref_index < prediction_structure_ptr->pred_struct_entry_ptr_array[entry_index]->ref_list1.reference_list_count; ++ref_index) {

                    dep_index = picture_number - prediction_structure_ptr->pred_struct_entry_ptr_array[entry_index]->ref_list1.reference_list;

                    // Assign the Reference to the Dep List and Increment the Dep List Count
                    if(dep_index >= 0 && dep_index < (int32_t) (prediction_structure_ptr->steady_state_index + prediction_structure_ptr->pred_struct_period)) {
                        prediction_structure_ptr->pred_struct_entry_ptr_array[dep_index]->dep_list1.list[prediction_structure_ptr->pred_struct_entry_ptr_array[dep_index]->dep_list1.list_count++] =
                            prediction_structure_ptr->pred_struct_entry_ptr_array[entry_index]->ref_list1.reference_list;
                    }
                }

                // Rollover the entry_index each time it reaches the end of the steady state index
                entry_index = (entry_index == prediction_structure_ptr->pred_struct_entry_count - 1) ?
                             prediction_structure_ptr->steady_state_index :
                             entry_index + 1;
            }
        }
    }

    // Set is_referenced for each entry
    for(entry_index = 0; entry_index < prediction_structure_ptr->pred_struct_entry_count; ++entry_index) {
        prediction_structure_ptr->pred_struct_entry_ptr_array[entry_index]->is_referenced =
            ((prediction_structure_ptr->pred_struct_entry_ptr_array[entry_index]->dep_list0.list_count > 0) ||
             (prediction_structure_ptr->pred_struct_entry_ptr_array[entry_index]->dep_list1.list_count > 0)) ?
            EB_TRUE :
            EB_FALSE;
    }

    //----------------------------------------
    // CONSTRUCT THE RPSes
    //----------------------------------------
    {
        // Counts & Indicies
        uint32_t  ref_index;
        uint32_t  dep_index;
        uint32_t  entry_index;
        uint32_t  current_poc_index;
        uint32_t  ref_poc_index;

        uint32_t  decode_order_table_size;
        int32_t  *decode_order_table;
        uint32_t *display_order_table;
        uint32_t  gop_number;
        uint32_t  base_number;

        // Timeline Map Variables
        EB_BOOL *timeline_map;
        uint32_t timeline_size;

        int32_t  dep_list_max;
        int32_t  dep_list_min;

        int32_t  lifetime_start;
        int32_t  lifetime_span;

        int32_t  delta_poc;
        int32_t  prev_delta_poc;
        EB_BOOL  poc_in_reference_list0;
        EB_BOOL  poc_in_reference_list1;
        EB_BOOL  poc_in_timeline;

        int32_t  adjusted_dep_index;

        // Allocate & Initialize the Timeline map
        timeline_size = prediction_structure_ptr->pred_struct_entry_count;
        decode_order_table_size = CEILING(prediction_structure_ptr->pred_struct_entry_count + prediction_structure_ptr->maximum_extent, prediction_structure_ptr->pred_struct_entry_count);
        EB_MALLOC(EB_BOOL*, timeline_map, sizeof(EB_BOOL) * SQR(timeline_size), EB_N_PTR);
        EB_MEMSET(timeline_map, 0, sizeof(EB_BOOL) * SQR(timeline_size));

        // Construct the Decode & Display Order
        EB_MALLOC(int32_t*, decode_order_table, sizeof(int32_t) * decode_order_table_size, EB_N_PTR);

        EB_MALLOC(uint32_t*, display_order_table, sizeof(uint32_t) * decode_order_table_size, EB_N_PTR);

        for(current_poc_index = 0, entry_index=0; current_poc_index < decode_order_table_size; ++current_poc_index) {

            // Set the Decode Order
            gop_number = (current_poc_index / prediction_structure_ptr->pred_struct_period);
            base_number = gop_number * prediction_structure_ptr->pred_struct_period;

            if(pred_type == EB_PRED_RANDOM_ACCESS) {
                decode_order_table[current_poc_index] = base_number + prediction_structure_ptr->pred_struct_entry_ptr_array[entry_index]->decode_order;
            }
            else {
                decode_order_table[current_poc_index] = current_poc_index;
            }
            display_order_table[decode_order_table[current_poc_index]] = current_poc_index;

            // Increment the entry_index
            entry_index = (entry_index == prediction_structure_ptr->pred_struct_entry_count - 1) ?
                         prediction_structure_ptr->pred_struct_entry_count - prediction_structure_ptr->pred_struct_period :
                         entry_index + 1;
        }

        // Construct the timeline map from the dependency lists
        for(ref_poc_index=0, entry_index=0; ref_poc_index < timeline_size; ++ref_poc_index) {

            // Initialize Max to most negative signed value and Min to most positive signed value
            dep_list_max = MIN_SIGNED_VALUE;
            dep_list_min = MAX_SIGNED_VALUE;

            // Find dep_list_max and dep_list_min for the entry_index in the prediction structure for dep_list0
            for(dep_index=0; dep_index < prediction_structure_ptr->pred_struct_entry_ptr_array[entry_index]->dep_list0.list_count; ++dep_index) {

                adjusted_dep_index = prediction_structure_ptr->pred_struct_entry_ptr_array[entry_index]->dep_list0.list[dep_index] + (int32_t) ref_poc_index;

                //if(adjusted_dep_index >= 0 && adjusted_dep_index < (int32_t) timeline_size) {
                if(adjusted_dep_index >= 0) {

                    // Update Max
                    dep_list_max = MAX(decode_order_table[adjusted_dep_index], dep_list_max);

                    // Update Min
                    dep_list_min = MIN(decode_order_table[adjusted_dep_index], dep_list_min);
                }
            }

            // Continue search for dep_list_max and dep_list_min for the entry_index in the prediction structure for dep_list1,
            //   the lists are combined in the RPS logic
            for(dep_index=0; dep_index < prediction_structure_ptr->pred_struct_entry_ptr_array[entry_index]->dep_list1.list_count; ++dep_index) {

                adjusted_dep_index = prediction_structure_ptr->pred_struct_entry_ptr_array[entry_index]->dep_list1.list[dep_index] + (int32_t) ref_poc_index;

                //if(adjusted_dep_index >= 0 && adjusted_dep_index < (int32_t) timeline_size)  {
                if(adjusted_dep_index >= 0)  {

                    // Update Max
                    dep_list_max = MAX(decode_order_table[adjusted_dep_index], dep_list_max);

                    // Update Min
                    dep_list_min = MIN(decode_order_table[adjusted_dep_index], dep_list_min);
                }

            }

            // If the Dependent Lists are empty, ensure that no RPS signaling is set
            if((prediction_structure_ptr->pred_struct_entry_ptr_array[entry_index]->dep_list0.list_count > 0) ||
                    (prediction_structure_ptr->pred_struct_entry_ptr_array[entry_index]->dep_list1.list_count > 0)) {

                // Determine lifetime_start and lifetime_span - its important to note that out-of-range references are
                //   clipped/eliminated to not violate IDR/CRA referencing rules
                lifetime_start = dep_list_min;

                if(lifetime_start < (int32_t) timeline_size) {
                    lifetime_start = CLIP3(0, (int32_t) (timeline_size - 1), lifetime_start);

                    lifetime_span = dep_list_max - dep_list_min + 1;
                    lifetime_span = CLIP3(0, (int32_t) timeline_size - lifetime_start, lifetime_span);

                    // Set the timeline_map
                    for(current_poc_index=(uint32_t) lifetime_start; current_poc_index < (uint32_t) (lifetime_start + lifetime_span); ++current_poc_index) {
                        timeline_map[ref_poc_index*timeline_size + display_order_table[current_poc_index]] = EB_TRUE;
                    }
                }
            }

            // Increment the entry_index
            entry_index = (entry_index == prediction_structure_ptr->pred_struct_entry_count - 1) ?
                         prediction_structure_ptr->pred_struct_entry_count - prediction_structure_ptr->pred_struct_period :
                         entry_index + 1;
        }

        //--------------------------------------------------------
        // Create the RPS for Prediction Structure Entry
        //--------------------------------------------------------

        // *Note- many of the below Syntax Elements are signaled
        //    in the Slice Header and not in the RPS.  These syntax
        //    elements can be configured during runtime to manipulate
        //    existing RPS structures.  E.g. a reference list could
        //    be shortened...

        // Initialize the RPS Group
        prediction_structure_ptr->restricted_ref_pic_lists_enable_flag       = EB_TRUE;
        prediction_structure_ptr->lists_modification_enable_flag           = EB_FALSE;
        prediction_structure_ptr->long_term_enable_flag                    = EB_FALSE;
        prediction_structure_ptr->default_ref_pics_list0_total_count_minus1   = 0;
        prediction_structure_ptr->default_ref_pics_list1_total_count_minus1   = 0;

        // For each RPS Index
        for(entry_index = 0; entry_index < prediction_structure_ptr->pred_struct_entry_count; ++entry_index) {

            // Determine the Current POC Index
            current_poc_index = entry_index;

            // Initialize the RPS
            prediction_structure_ptr->pred_struct_entry_ptr_array[entry_index]->short_term_rps_in_sps_flag                = EB_TRUE;
            prediction_structure_ptr->pred_struct_entry_ptr_array[entry_index]->short_term_rps_in_sps_index               = entry_index;
            prediction_structure_ptr->pred_struct_entry_ptr_array[entry_index]->inter_rps_prediction_flag               = EB_FALSE;
            prediction_structure_ptr->pred_struct_entry_ptr_array[entry_index]->long_term_rps_present_flag               = EB_FALSE;
            prediction_structure_ptr->pred_struct_entry_ptr_array[entry_index]->list0_modification_flag                = EB_FALSE;
            prediction_structure_ptr->pred_struct_entry_ptr_array[entry_index]->negative_ref_pics_total_count            = 0;
            prediction_structure_ptr->pred_struct_entry_ptr_array[entry_index]->positive_ref_pics_total_count            = 0;
            prediction_structure_ptr->pred_struct_entry_ptr_array[entry_index]->ref_pics_list0_total_count_minus1         = ~0;
            prediction_structure_ptr->pred_struct_entry_ptr_array[entry_index]->ref_pics_list1_total_count_minus1         = ~0;

            // Create the Negative List
            prev_delta_poc = 0;
            for(ref_poc_index=current_poc_index-1; (int32_t) ref_poc_index >= 0; --ref_poc_index) {

                // Find the delta_poc value
                delta_poc = (int32_t) current_poc_index - (int32_t) ref_poc_index;

                // Check to see if the delta_poc is in Reference List 0
                poc_in_reference_list0 = EB_FALSE;
                for(ref_index=0; (ref_index < prediction_structure_ptr->pred_struct_entry_ptr_array[entry_index]->ref_list0.reference_list_count) && (poc_in_reference_list0 == EB_FALSE); ++ref_index) {

                    // Reference List 0
                    if ((prediction_structure_ptr->pred_struct_entry_ptr_array[entry_index]->ref_list0.reference_list != 0) &&
                            (prediction_structure_ptr->pred_struct_entry_ptr_array[entry_index]->ref_list0.reference_list == delta_poc))
                    {
                        poc_in_reference_list0 = EB_TRUE;
                        ++prediction_structure_ptr->pred_struct_entry_ptr_array[entry_index]->ref_pics_list0_total_count_minus1;
                    }
                }

                // Check to see if the delta_poc is in Reference List 1
                poc_in_reference_list1 = EB_FALSE;
                for(ref_index=0; (ref_index < prediction_structure_ptr->pred_struct_entry_ptr_array[entry_index]->ref_list1.reference_list_count) && (poc_in_reference_list1 == EB_FALSE); ++ref_index) {

                    // Reference List 1
                    if ((prediction_structure_ptr->pred_struct_entry_ptr_array[entry_index]->ref_list1.reference_list != 0) &&
                            (prediction_structure_ptr->pred_struct_entry_ptr_array[entry_index]->ref_list1.reference_list == delta_poc))
                    {
                        poc_in_reference_list1 = EB_TRUE;
                        ++prediction_structure_ptr->pred_struct_entry_ptr_array[entry_index]->ref_pics_list1_total_count_minus1;
                    }
                }

                // Check to see if the ref_poc_index is in the timeline
                poc_in_timeline = timeline_map[ref_poc_index*timeline_size + current_poc_index];

                // If the delta_poc is in the timeline
                if(poc_in_timeline == EB_TRUE) {
                    prediction_structure_ptr->pred_struct_entry_ptr_array[entry_index]->used_by_negative_curr_pic_flag[prediction_structure_ptr->pred_struct_entry_ptr_array[entry_index]->negative_ref_pics_total_count]   = (poc_in_reference_list0 == EB_TRUE || poc_in_reference_list1 == EB_TRUE) ? EB_TRUE : EB_FALSE;
                    prediction_structure_ptr->pred_struct_entry_ptr_array[entry_index]->delta_negative_gop_pos_minus1[prediction_structure_ptr->pred_struct_entry_ptr_array[entry_index]->negative_ref_pics_total_count++] = delta_poc - 1 - prev_delta_poc;
                    prev_delta_poc = delta_poc;
                }
            }

            // Create the Positive List
            prev_delta_poc = 0;
            for(ref_poc_index=current_poc_index+1; ref_poc_index < timeline_size; ++ref_poc_index) {

                // Find the delta_poc value
                delta_poc = (int32_t) current_poc_index - (int32_t) ref_poc_index;

                // Check to see if the delta_poc is in Ref List 0
                poc_in_reference_list0 = EB_FALSE;
                for(ref_index=0; (ref_index < prediction_structure_ptr->pred_struct_entry_ptr_array[entry_index]->ref_list0.reference_list_count) && (poc_in_reference_list0 == EB_FALSE); ++ref_index) {

                    // Reference List 0
                    if ((prediction_structure_ptr->pred_struct_entry_ptr_array[entry_index]->ref_list0.reference_list != 0) &&
                            (prediction_structure_ptr->pred_struct_entry_ptr_array[entry_index]->ref_list0.reference_list == delta_poc))
                    {
                        poc_in_reference_list0 = EB_TRUE;
                        ++prediction_structure_ptr->pred_struct_entry_ptr_array[entry_index]->ref_pics_list0_total_count_minus1;
                    }
                }

                // Check to see if the delta_poc is in Ref List 1
                poc_in_reference_list1 = EB_FALSE;
                for(ref_index=0; (ref_index < prediction_structure_ptr->pred_struct_entry_ptr_array[entry_index]->ref_list1.reference_list_count) && (poc_in_reference_list1 == EB_FALSE); ++ref_index) {
                    if ((prediction_structure_ptr->pred_struct_entry_ptr_array[entry_index]->ref_list1.reference_list != 0) &&
                            (prediction_structure_ptr->pred_struct_entry_ptr_array[entry_index]->ref_list1.reference_list == delta_poc))
                    {
                        poc_in_reference_list1 = EB_TRUE;
                        ++prediction_structure_ptr->pred_struct_entry_ptr_array[entry_index]->ref_pics_list1_total_count_minus1;
                    }
                }

                // Check to see if the Y-position is in the timeline
                poc_in_timeline = timeline_map[ref_poc_index*timeline_size + current_poc_index];

                // If the Y-position is in the time lime
                if(poc_in_timeline == EB_TRUE) {
                    prediction_structure_ptr->pred_struct_entry_ptr_array[entry_index]->used_by_positive_curr_pic_flag[prediction_structure_ptr->pred_struct_entry_ptr_array[entry_index]->positive_ref_pics_total_count]   = (poc_in_reference_list0 == EB_TRUE || poc_in_reference_list1 == EB_TRUE) ? EB_TRUE : EB_FALSE;
                    prediction_structure_ptr->pred_struct_entry_ptr_array[entry_index]->delta_positive_gop_pos_minus1[prediction_structure_ptr->pred_struct_entry_ptr_array[entry_index]->positive_ref_pics_total_count++] = -delta_poc - 1 - prev_delta_poc;
                    prev_delta_poc = -delta_poc;
                }
            }

            // Adjust Reference Counts if list is empty
            prediction_structure_ptr->pred_struct_entry_ptr_array[entry_index]->ref_pics_list0_total_count_minus1  =
                (prediction_structure_ptr->pred_struct_entry_ptr_array[entry_index]->ref_pics_list0_total_count_minus1 == ~0) ?
                0:
                prediction_structure_ptr->pred_struct_entry_ptr_array[entry_index]->ref_pics_list0_total_count_minus1;

            prediction_structure_ptr->pred_struct_entry_ptr_array[entry_index]->ref_pics_list1_total_count_minus1  =
                (prediction_structure_ptr->pred_struct_entry_ptr_array[entry_index]->ref_pics_list1_total_count_minus1 == ~0) ?
                0:
                prediction_structure_ptr->pred_struct_entry_ptr_array[entry_index]->ref_pics_list1_total_count_minus1;

            // Set ref_pics_override_total_count_flag to TRUE if RefListCount is different than the default
            prediction_structure_ptr->pred_struct_entry_ptr_array[entry_index]->ref_pics_override_total_count_flag =
                ((prediction_structure_ptr->pred_struct_entry_ptr_array[entry_index]->ref_pics_list0_total_count_minus1 != (int32_t) prediction_structure_ptr->default_ref_pics_list0_total_count_minus1) ||
                 (prediction_structure_ptr->pred_struct_entry_ptr_array[entry_index]->ref_pics_list1_total_count_minus1 != (int32_t) prediction_structure_ptr->default_ref_pics_list1_total_count_minus1)) ?
                EB_TRUE :
                EB_FALSE;

        }

        // Free the decode order table
        //free(decode_order_table);

        // Free the display order table
        //free(display_order_table);

        // Free the timeline map
        //free(timeline_map);
    }

    return EB_ErrorNone;
}

/*************************************************
 * Prediction Structure Group Ctor
 *
 * Summary: Converts the Prediction Structure Config
 *   into the usable Prediction Structure with RPS and
 *   Dependent List control.
 *
 * From each config, several prediction structures
 *   are created. These include:
 *   -Variable Number of References
 *      # [1 - 4]
 *   -Temporal Layers
 *      # [1 - 6]
 *   -GOP Type
 *      # Low Delay P
 *      # Low Delay B
 *      # Random Access
 *
 *************************************************/
EbErrorType eb_vp9_prediction_structure_group_ctor(
    PredictionStructureGroup **prediction_structure_group_dbl_ptr,
    uint32_t                   base_layer_switch_mode)
{
    uint32_t          pred_struct_index = 0;
    uint32_t          ref_idx           = 0;
    uint32_t          hierarchical_level_idx;
    uint32_t          pred_type_idx;
    uint32_t          number_of_references;
    EbErrorType       return_error      = EB_ErrorNone;

    PredictionStructureGroup *prediction_structure_group_ptr;
    EB_MALLOC(PredictionStructureGroup*, prediction_structure_group_ptr, sizeof(PredictionStructureGroup), EB_N_PTR);
    *prediction_structure_group_dbl_ptr = prediction_structure_group_ptr;

    // Count the number of Prediction Structures
    while((prediction_structure_config_array[pred_struct_index].entry_array != 0) && (prediction_structure_config_array[pred_struct_index].entry_count != 0)) {
        // Get Random Access + P for temporal ID 0
        if(prediction_structure_config_array[pred_struct_index].entry_array->temporal_layer_index == 0 && base_layer_switch_mode) {
                prediction_structure_config_array[pred_struct_index].entry_array->ref_list1 = 0;
        }
        ++pred_struct_index;
    }

    prediction_structure_group_ptr->prediction_structure_count = MAX_TEMPORAL_LAYERS * EB_PRED_TOTAL_COUNT;
    EB_MALLOC(PredictionStructure**, prediction_structure_group_ptr->prediction_structure_ptr_array, sizeof(PredictionStructure*) * prediction_structure_group_ptr->prediction_structure_count, EB_N_PTR);
    for(hierarchical_level_idx = 0; hierarchical_level_idx < MAX_TEMPORAL_LAYERS; ++hierarchical_level_idx) {
        for(pred_type_idx = 0; pred_type_idx < EB_PRED_TOTAL_COUNT; ++pred_type_idx) {
                pred_struct_index = PRED_STRUCT_INDEX(hierarchical_level_idx, pred_type_idx, ref_idx);
                number_of_references = ref_idx + 1;

                return_error = prediction_structure_ctor(
                    &(prediction_structure_group_ptr->prediction_structure_ptr_array[pred_struct_index]),
                    &(prediction_structure_config_array[hierarchical_level_idx]),
                    (EB_PRED) pred_type_idx,
                    number_of_references);
                if (return_error == EB_ErrorInsufficientResources){
                    return EB_ErrorInsufficientResources;
                }
        }
    }

    return EB_ErrorNone;
}
