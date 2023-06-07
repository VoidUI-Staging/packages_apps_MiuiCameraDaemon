/*
 * Copyright (c) 2019 AnC Technology Co., Ltd. All rights reserved.
 */
#ifndef COMMON_INCLUDE_COMMON_WBOKEH_COMMON_H_
#define COMMON_INCLUDE_COMMON_WBOKEH_COMMON_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief spot value type.
 *
 * - WRF_SPOT_CIRCLE: default spot type.
 * - WRF_SPOT_SHARPCIRCLE: the shape of the spot is sharp circle.
 * - WRF_SPOT_DONUTCIRCLE: the shape of the spot is donut circle.
 * - WRF_SPOT_CONCENTRICCIRCLE: he shape of the spot is concentric circle.
 * - WRF_SPOT_HEXAGON: the shape of the spot is hexagon.
 * - WRF_SPOT_HEART: the shape of the spot is heart.
 * - WRF_SPOT_STAR: the shape of the spot is star.
 * - WRF_SPOT_DIAMOND: the shape of the spot is diamond.
 * - WRF_SPOT_BUTTERFLY: the shape of the spot is butterfly.
 * - WRF_SPOT_WATERDROP: the shape of the spot is waterdrop.
 * - WRF_SPOT_FLAKE: the shape of the spot is flake.
 * - WRF_SPOT_ZIESS: the ziess spot.
 */
typedef enum _WBokehSpotType {
    WRF_SPOT_CIRCLE = 0,
    WRF_SPOT_SHARPCIRCLE,
    WRF_SPOT_DONUTCIRCLE,
    WRF_SPOT_CONCENTRICCIRCLE,
    WRF_SPOT_HEXAGON,
    WRF_SPOT_HEART,
    WRF_SPOT_STAR,
    WRF_SPOT_DIAMOND,
    WRF_SPOT_BUTTERFLY,
    WRF_SPOT_WATERDROP,
    WRF_SPOT_FLAKE,
    WRF_SPOT_ZIESS,
} WBokehSpotType;

/**
 * @brief bokeh style.
 *
 * - WRF_STYLE_DSLR: default bokeh style, similar DSLR.
 * - WRF_STYLE_ROTATE: the rotate style of bokeh.
 * - WRF_STYLE_ZOOM: the zoom style of bokeh.
 * - WRF_STYLE_COLOR: the color style of bokeh.
 * - WRF_STYLE_APERTURE: the aperture style of bokeh.
 */
typedef enum _WBokehStyle {
    WRF_STYLE_DSLR = 0,
    WRF_STYLE_ROTATE,
    WRF_STYLE_ZOOM,
    WRF_STYLE_COLOR,
    WRF_STYLE_APERTURE,
} WBokehStyle;

#ifdef __cplusplus
} // extern "C"
#endif

#endif // COMMON_INCLUDE_COMMON_WBOKEH_COMMON_H_
