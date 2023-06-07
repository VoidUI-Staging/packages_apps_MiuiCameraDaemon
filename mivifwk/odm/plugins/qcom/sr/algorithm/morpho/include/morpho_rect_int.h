/**
 * @file     morpho_rect_int.h
 * @brief    Structure definition for rectangle data
 * @version  1.0.0
 * @date     2008-06-09
 *
 * Copyright (C) 2006 Morpho, Inc.
 */

#ifndef MORPHO_RECT_INT_H
#define MORPHO_RECT_INT_H

#ifdef __cplusplus
extern "C" {
#endif

/** @struct morpho_RectInt
 *  @brief  Rectangle data.
 */
typedef struct
{
    int sx; /**< left */
    int sy; /**< top */
    int ex; /**< right */
    int ey; /**< bottom */
} morpho_RectInt;

/*! @def    morpho_RectInt_setRect(rect,l,t,r,b)
 *  @brief  Set top-left coordinate(l,t) and bottom-right coordinate(r,b) of the rectangle area,
 * rect.
 */
#define morpho_RectInt_setRect(rect, l, t, r, b) \
    do {                                         \
        (rect)->sx = (l);                        \
        (rect)->sy = (t);                        \
        (rect)->ex = (r);                        \
        (rect)->ey = (b);                        \
    } while (0)

#ifdef __cplusplus
}
#endif

#endif /* #ifndef MORPHO_RECT_INT_H */
