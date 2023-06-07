#ifndef __DEVICE_H__
#define __DEVICE_H__

#define ISZ_MODE 1
#define HDSR     1

#ifndef ARCSOFT_TYPE_LIST
    #ifndef ARCSOFT_TYPE_LIST_WIDE_0
        #ifndef ARCSOFT_TYPE_LIST_WIDE_0_INDEX
            #define ARCSOFT_TYPE_LIST_WIDE_0_INDEX CameraRoleTypeWide
        #endif

        #ifndef ARCSOFT_TYPE_LIST_WIDE_0_VALUE
            #define ARCSOFT_TYPE_LIST_WIDE_0_VALUE ARC_MFSR_CAMERA_TYPE_REAR_WIDE_0
        #endif

        #define ARCSOFT_TYPE_LIST_WIDE_0 [ARCSOFT_TYPE_LIST_WIDE_0_INDEX] = ARCSOFT_TYPE_LIST_WIDE_0_VALUE
    #endif

    #ifndef ARCSOFT_TYPE_LIST_WIDE_0_RM
        #ifndef ARCSOFT_TYPE_LIST_WIDE_0_RM_INDEX
            #define ARCSOFT_TYPE_LIST_WIDE_0_RM_INDEX 1
        #endif

        #ifndef ARCSOFT_TYPE_LIST_WIDE_0_RM_VALUE
            #define ARCSOFT_TYPE_LIST_WIDE_0_RM_VALUE ARC_MFSR_CAMERA_TYPE_REAR_WIDE_0_RM
        #endif

        #define ARCSOFT_TYPE_LIST_WIDE_0_RM [ARCSOFT_TYPE_LIST_WIDE_0_RM_INDEX] = ARCSOFT_TYPE_LIST_WIDE_0_RM_VALUE
    #endif

    #ifndef ARCSOFT_TYPE_LIST_WIDE_1
        #ifndef ARCSOFT_TYPE_LIST_WIDE_1_INDEX
            #define ARCSOFT_TYPE_LIST_WIDE_1_INDEX 2
        #endif

        #ifndef ARCSOFT_TYPE_LIST_WIDE_1_VALUE
            #define ARCSOFT_TYPE_LIST_WIDE_1_VALUE ARC_MFSR_CAMERA_TYPE_REAR_WIDE_1
        #endif

        #define ARCSOFT_TYPE_LIST_WIDE_1 [ARCSOFT_TYPE_LIST_WIDE_1_INDEX] = ARCSOFT_TYPE_LIST_WIDE_1_VALUE
    #endif

    #ifndef ARCSOFT_TYPE_LIST_WIDE_1_RM
        #ifndef ARCSOFT_TYPE_LIST_WIDE_1_RM_INDEX
            #define ARCSOFT_TYPE_LIST_WIDE_1_RM_INDEX 3
        #endif

        #ifndef ARCSOFT_TYPE_LIST_WIDE_1_RM_VALUE
            #define ARCSOFT_TYPE_LIST_WIDE_1_RM_VALUE ARC_MFSR_CAMERA_TYPE_REAR_WIDE_1_RM
        #endif

        #define ARCSOFT_TYPE_LIST_WIDE_1_RM [ARCSOFT_TYPE_LIST_WIDE_1_RM_INDEX] = ARCSOFT_TYPE_LIST_WIDE_1_RM_VALUE
    #endif

#define ARCSOFT_TYPE_LIST                               \
    {                                                   \
        ARCSOFT_TYPE_LIST_WIDE_0, ARCSOFT_TYPE_LIST_WIDE_0_RM, \
        ARCSOFT_TYPE_LIST_WIDE_1, ARCSOFT_TYPE_LIST_WIDE_1_RM, \
    }
#endif

#endif
