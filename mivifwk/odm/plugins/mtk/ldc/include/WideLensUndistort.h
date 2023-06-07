/*
 * Copyright (c) 2019 AnC Technology Co., Ltd. All rights reserved.
 */
// NOLINT
#ifndef WIDELENSUNDISTORT_INCLUDE_SILCLIENT_WIDELENSUNDISTORT_H_
#define WIDELENSUNDISTORT_INCLUDE_SILCLIENT_WIDELENSUNDISTORT_H_

#include "common/wcommon.h"
#include "common/wundistort_common.h"
#include "image/Image.h"
#include "utils/base_export.h"

namespace wa {

//! @addtogroup WideLensUndistort2
//! @{

/** @example example_undistort_v2.cc
 *
 * An example using ldc SDK
 */

class BASE_EXPORT WideLensUndistort
{
public:
    enum RunMode { MODE_PREVIEW, MODE_CAPTURE };

    WideLensUndistort();
    ~WideLensUndistort();

    WideLensUndistort(const WideLensUndistort &rf);
    WideLensUndistort &operator=(const WideLensUndistort &rf);

    /** @brief init algo with static config.
     *
     * @param config static config data include mecp bin, etc.
     *
     * @return true if success, otherwise failure.
     */
    bool init(const WUndistortConfig &config);

    /** @brief set run mode, capture or preview
     *
     *  @param mode: MODE_PREVIEW or MODE_CAPTURE
     */
    bool setMode(RunMode mode);

    /** @brief Set model file path or data of model file. unused.
     *
     * @param path model file path. if set NULL, need set right model and size,
     * else set the right path
     * @param model model file data. if set right path, can set NULL, else need
     * the data of model file
     * @param size model file data size. if set right path, can set 0, else need
     * the right length of model file data
     *
     * @return true if success, otherwise failure.
     */
    bool setModel(const char *path, const void *model, int size);

    /** @brief Set human face info.
     *
     * @param faces the faces roi rect.
     * @param faceCounts the face numbers, match the face rect counts.
     *
     * @return true if success, otherwise failure.
     */
    bool setFaces(const WFace *faces, int faceCounts);

    /** @brief Set the crop rect in the origin image.
     *
     * @param originSize the origin image size.
     * @param cropRect   the crop rect.
     *
     * @return true if success, otherwise failure.
     */
    bool setCropRect(const WSize &originSize, const WRect &cropRect);

    /** @brief Run wide lens undistort
     *
     * @param inImg input image
     * @param outImg output Image
     * @return true if success, otherwise failure
     */
    bool run(wa::Image &inImg, wa::Image &outImg);

    /** @brief Get version number
     *
     * @return version number
     */
    const char *getVersion() const;

    /** @brief Get internal api version
     *
     * @return version
     */
    const char *getApiVersion() const;

    /* @brief Defined api version
     *
     * Change the value when the api have changed
     *
     * The format is major.minor.revision
     * - major: main version, fixed to 4, do not modify
     * - minor: modifiable, add or remove api
     * - revision: modifiable, add or remove api
     *
     * Base: 2.0.0
     */
    const char *mApiVersion = "2.0.0";

private:
    class Head;
    Head *h;

    class Impl;
    Impl *p;
}; // class WideLensUndistort

//! @} WideLensUndistort2

} // namespace wa

#endif //  WIDELENSUNDISTORT_INCLUDE_SILCLIENT_WIDELENSUNDISTORT_H_
