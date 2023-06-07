/*----------------------------------------------------------------------------------------------
 *
 * This file is ArcSoft's property. It contains ArcSoft's trade secret, proprietary and
 * confidential information.
 *
 * The information and code contained in this file is only for authorized ArcSoft employees
 * to design, create, modify, or review.
 *
 * DO NOT DISTRIBUTE, DO NOT DUPLICATE OR TRANSMIT IN ANY FORM WITHOUT PROPER AUTHORIZATION.
 *
 * If you are not an intended recipient of this file, you must not copy, distribute, modify,
 * or take any action in reliance on it.
 *
 * If you have received this file in error, please immediately notify ArcSoft and
 * permanently delete the original and any copy of any file and any printout thereof.
 *
 *-------------------------------------------------------------------------------------------------*/
/*
 * BeautyShot_Video_Algorithm.h
 *
 *
 */

#ifndef ____BEAUTYSHOT_VIDEO_ALGORITHM_H_20150805____
#define ____BEAUTYSHOT_VIDEO_ALGORITHM_H_20150805____

#include "ammem.h"
#include "asvloffscreen.h"
#include "AISFReferenceInter.h"
#include "AISFCommonDef.h"
#include "abstypes.h"

#ifdef ENABLE_DLL_EXPORT
#define ARC_BS_API __declspec(dllexport)
#else
#define ARC_BS_API
#endif

class BeautyShot_Video_Algorithm : public AISF_NS::AISFReferenceInter
{
public:
	/*
	 *The unique ID of the SDK.
	 */
	static const MInt32 CLASSID = ABS_INTERFACE_VERSION;

	/*
	 * Initialize the SDK.
	 * [IN] nInterfaceVersion: ABS_INTERFACE_VERSION.
	 * [IN] nMode: Mode of ABS_MODE.
	 * [IN] ePlatformType: Platform type, like CPU or HVX.
	 * [IN] bUseOneBuffer: Default is MFalse.
	 */
	virtual MRESULT	Init(const MInt32 nInterfaceVersion, ABS_MODE nMode, ABS_PLATFORM_TYPE ePlatformType, MBool bUseOneBuffer)=0;

	/*
	 * Uninitialize the SDK. MUST correspond with Init().
	 */
	virtual MRESULT	UnInit()=0;

	/*
	 * Get the SDK's version.
	 */
	virtual const MPBASE_Version *GetVersion()=0;

	/*
	 * Set the style data which read from file.
	 * [IN] pStyleData: pointer to the style data.
	 * [IN] nDataSize: size of the style data.
	 */
	virtual MRESULT LoadStyle(MVoid *pStyleData, MInt32 nDataSize)=0;

	/*
	 * Set/Get feature level to/from the SDK
	 * [IN] nFeatureKey: feature key
	 * [IN] nLevel: intensity of the feature
	 */
	virtual MVoid SetFeatureLevel(MInt32 nFeatureKey, MInt32 nLevel)=0;
	virtual MInt32 GetFeatureLevel(MInt32 nFeatureKey)=0;

	/*
	 * Reset output. When user close/open beuatyshot quickly, engine need not to uninit/init
	 */
	virtual MRESULT ResetOutput()=0;

	/*
	 * Process the current image with according parameters.
	 * [IN] pImgIn: Pointer of input image data.
	 * [OUT] pImgOut: Pointer of output image data, it can be the same as pImgIn.
	 * [IN] pFacesIn: The detected faces info of input image. If it is MNull, SDK will detect faces inside.
	 * [OUT] pFacesOut: The output faces info. If it is MNull, SDK does not output faces info.
	 * [IN] nFaceOrientation: Face Orientation. [0, 90, 180, 270].
	 */
	virtual MRESULT Process(LPASVLOFFSCREEN pImgIn, LPASVLOFFSCREEN pImgOut, ABS_TFaces *pFacesIn, ABS_TFaces *pFacesOut, MInt32 nFaceOrientation)=0;
};

/*
 * The function exported by the dynamic so, which is used to create the SDK object.
 * Parameters:
 *   [IN] ClassID : The unique class ID, defined as BeautyShot_Video_Algorithm::CLASSID.
 *   [OUT] ppalgorithm: The SDK object will be created in this function with reference count as 1.
 *                and shall be released by calling (*ppalgorithm)->Release() when it is not used anymore.
 * Return:
 *   MOK is succeeded, others are failed.
 */
#ifdef __cplusplus
extern "C" {
#endif

ARC_BS_API MInt32 Create_BeautyShot_Video_Algorithm(const MInt32 ClassID,
											BeautyShot_Video_Algorithm** ppalgorithm);

#ifdef __cplusplus
}
#endif

#endif	//____BEAUTYSHOT_VIDEO_ALGORITHM_H_20150805____
