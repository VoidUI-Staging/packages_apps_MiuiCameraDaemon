
#ifndef _XMI_HDR_LOCK_
#define _XMI_HDR_LOCK_

#include <iterator>
#include <vector>

#include "amcomdef.h"
#include "arcsoft_high_dynamic_range.h"
#include "xmi_high_dynamic_range.h"

#define MAX_EV_LEN 20
// define length of checker result buffer
#define CHECKER_CACHE_LEN 7

// define threshold of time and magnet diff
#if defined(PRODUCT_K1)
#define THRE_SHOT_TIME 60.0f * 1000
#else
#define THRE_SHOT_TIME 60.0f
#endif
#define THRE_MAGNET_ANGLE 15.0f

#define ABS(x) ((x) > 0 ? (x) : (-(x)))

/************************************************************************
 * This class is used to cache checker results and magnet
 * id_frame      frame ID of each checker result
 * hdr_state     non-hdr / hdr / lhdr (checker type 0/1/2)
 * ev_num        The number of shot image, it must be 1, 2, 3 or 4.
 * ev_arr        The ev value for each image
 * magnet        Magnetic field information for each checker result
 ************************************************************************/
class CheckerCache
{
public:
    MInt32 id_frame;
    MInt32 hdr_state;
    MInt32 ev_num;
    MInt32 ev_arr[MAX_EV_LEN];
    float zoom;
    int hdr_mode;
    int ae_state;

    char flash_mode;
    char is_flash_snapshot;
    char hdr_type;

    MAGNET_INFO magnet;

public:
    CheckerCache();
    CheckerCache(MInt32 _id_frame, MAGNET_INFO _magnet, MInt32 ae_state, MInt32 _hdr_state,
                 MInt32 _ev_num, MInt32 *_ev_arr, float _zoom, int _hdr_mode, char _flash_mode,
                 char _is_flash_snapshot, char _hdr_type);
    CheckerCache(const CheckerCache &obj);
    ~CheckerCache();

public:
    CheckerCache &operator=(const CheckerCache &obj);

    void ClearData();
};

class HDRLock
{
private:
    static CheckerCache last_shot_info;
    static std::vector<CheckerCache> checker_history;
    static bool trigger_lock;
    static bool is_shot;

public:
    static int32_t stable_frame_num;
    static float last_luxindex;

public:
    HDRLock();
    ~HDRLock();

public:
    /*
    Decide whether to change checker output based on magnetic field lock.
    pAEInfo         [in] The AE info of input image
    pi32HdrDetect   [in/out] 0 or 1 or 2.
    pi32ShotNum     [in/out] The number of shot image, it must be 1, 2 or 3.
    pi32EVArray     [in/out] The ev value for each image
    */
    static void LockProcess(LPXMI_LLHDR_AEINFO pAEInfo, HDRMetaData *hdrMetaData,
                            MInt32 *pi32HdrDetect, MInt32 *pi32ShotNum, MInt32 *pi32EVArray);

    /*

    */
    static int SearchCheckerCache();
};

#endif