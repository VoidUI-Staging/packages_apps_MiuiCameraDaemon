#ifndef _ALHLDC_DEF
#define _ALHLDC_DEF

typedef struct
{
    unsigned short x;
    unsigned short y;
    unsigned short width;
    unsigned short height;
} alhLDC_RECT;

typedef enum {
    ALHLDC_METHOD_DEFAULT = 0, // default kernel method
    ALHLDC_METHOD_MEM_OPT,     // kernel method of memory optimization
} alhLDC_SET_KERNEL_METHOD;

typedef enum { ALHLDC_IMAGE_FORMAT_NV21 = 0, ALHLDC_IMAGE_FORMAT_P010 } alhLDC_IMG_FORMAT;

typedef enum {
    ALHLDC_LEVEL_0 = 0x00, // normal     (Only Version number)
    ALHLDC_LEVEL_1 = 0x01, // api_check  (Print all input parameters of public API)
    ALHLDC_LEVEL_2 = 0x02, // profile    (Print performance log for public API call)
} alhLDC_SET_LOGLV;

#endif // _ALHLDC_DEF
