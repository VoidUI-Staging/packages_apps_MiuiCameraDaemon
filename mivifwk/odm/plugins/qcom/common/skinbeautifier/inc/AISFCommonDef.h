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
* AISFCommonDef.h
*
* Reference:
*
* Description:
*
* Create by ArcSoft
*
*/

#ifndef __AISFCOMMONDEF_H_20141028__
#define __AISFCOMMONDEF_H_20141028__


//namespace macros
#define START_NAMESPACE_(NS) namespace NS {
#define END_NAMESPACE_(NS) };
#define USING_NAMESPACE_(NS) using namespace NS;

//define AISF namespace

#define AISF_NS ARC_AISF
//for aisfframework namespace
#define START_NAMESPACE_AISF START_NAMESPACE_(AISF_NS)

#define END_NAMESPACE_AISF END_NAMESPACE_(AISF_NS)

#define USING_NAMESPACE_AISF USING_NAMESPACE_(AISF_NS)


#endif
