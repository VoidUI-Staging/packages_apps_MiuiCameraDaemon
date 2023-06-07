/*******************************************************************************
Copyright(c) ArcSoft, All right reserved.

This file is ArcSoft's property. It contains ArcSoft's trade secret, proprietary
and confidential information.

The information and code contained in this file is only for authorized ArcSoft
employees to design, create, modify, or review.

DO NOT DISTRIBUTE, DO NOT DUPLICATE OR TRANSMIT IN ANY FORM WITHOUT PROPER
AUTHORIZATION.

If you are not an intended recipient of this file, you must not copy,
distribute, modify, or take any action in reliance on it.

If you have received this file in error, please immediately notify ArcSoft and
permanently delete the original and any copy of any file and any printout
thereof.
*******************************************************************************/

#ifndef UTILS_H
#define UTILS_H

#include "VendorMetadataParser.h"
#include "asvloffscreen.h"
#include "amcomdef.h"
#include "chivendortag.h"

static void __attribute__((unused)) DrawPoint(ASVLOFFSCREEN* pOffscreen, MPOINT xPoint, MInt32 nPointWidth, MInt32 iType)
{
    if(MNull == pOffscreen)
    {
        return;
    }
    MInt32 nPoints = 10;
    nPoints = (nPoints < nPointWidth)? nPointWidth : nPoints;

    int nStartX = xPoint.x - (nPoints/2);
    int nStartY = xPoint.y - (nPoints/2);
    nStartX = nStartX < 0? 0 : nStartX;
    nStartY = nStartY < 0? 0 : nStartY;
    if(nStartX>pOffscreen->i32Width)
    {
        nStartX = pOffscreen->i32Width - nPoints;
    }
    if(nStartY>pOffscreen->i32Height)
    {
        nStartY = pOffscreen->i32Height - nPoints;

    }

    if(pOffscreen->u32PixelArrayFormat == ASVL_PAF_NV21 ||ASVL_PAF_NV12==pOffscreen->u32PixelArrayFormat)
    {
        MUInt8* pByteYTop =pOffscreen->ppu8Plane[0] + nStartY*pOffscreen->pi32Pitch[0] + nStartX;
        MUInt8* pByteVTop =pOffscreen->ppu8Plane[1] + (nStartY/2)*(pOffscreen->pi32Pitch[1]) + nStartX;
        for(int i=0;i<nPoints;i++)
        {
            if(0 == iType) {
                memset(pByteYTop,255,nPoints);
                memset(pByteVTop,0,nPoints);
            } else {
                memset(pByteYTop,81,nPoints);
                memset(pByteVTop,238,nPoints);
            }
            pByteYTop+=pOffscreen->pi32Pitch[0];
            if(i%2 == 1)
                pByteVTop +=(pOffscreen->pi32Pitch[1]);
        }
    }
}

#define MATHL_MAX(a,b) ((a) > (b) ? (a) : (b))
#define MATH_MIN(a,b) ((a) < (b) ? (a) : (b))

static MRECT __attribute__((unused))  intersection(const MRECT& a, const MRECT& d)
{
    int l = MATHL_MAX(a.left, d.left);
    int t = MATHL_MAX(a.top, d.top);
    int r = MATH_MIN(a.right, d.right);
    int b = MATH_MIN(a.bottom, d.bottom);
    if (l >= r || t >= b)
    {
             l = 0;
             t = 0;
             r = 0;
             b = 0;
      }
    MRECT c={l,t,r,b};
    return c;
}

static void __attribute__((unused)) DrawRect(LPASVLOFFSCREEN pOffscreen, MRECT RectDraw, MInt32 nLineWidth, MInt32 iType)
{
    if(MNull == pOffscreen)
    {
        return;
    }
	nLineWidth = (nLineWidth+1)/2*2;
    MInt32 nPoints = nLineWidth<=0 ? 1: nLineWidth;

    MRECT RectImage={0,0,pOffscreen->i32Width,pOffscreen->i32Height};
    MRECT rectIn = intersection(RectDraw,RectImage);

    MInt32 nLeft = rectIn.left;
    MInt32 nTop = rectIn.top;
    MInt32 nRight = rectIn.right;
    MInt32 nBottom = rectIn.bottom;
    //MUInt8* pByteY = pOffscreen->ppu8Plane[0];
    int nRectW = nRight - nLeft;
    int nRecrH = nBottom - nTop;
    if(nTop+nPoints+nRecrH>pOffscreen->i32Height)
    {
        nTop = pOffscreen->i32Height - nPoints - nRecrH;
    }
    if(nTop<nPoints/2)
    {
        nTop=nPoints/2;
    }
    if(nBottom-nPoints<0)
    {
        nBottom = nPoints;
    }
    if(nBottom+nPoints/2>pOffscreen->i32Height)
    {
        nBottom = pOffscreen->i32Height-nPoints/2;
    }
    if (pOffscreen->u32PixelArrayFormat == ASVL_PAF_YUYV)
    {
        nLeft=(nLeft+1)/2*2;
        nRight = (nRight+1)/2*2;
    }
    if(nLeft+nPoints+nRectW>pOffscreen->i32Width)
    {
        nLeft = pOffscreen->i32Width - nPoints - nRectW;
    }
    if(nLeft<nPoints/2)
    {
        nLeft=nPoints/2;
    }

    if(nRight-nPoints<0)
    {
        nRight = nPoints;
    }
    if(nRight+nPoints/2>pOffscreen->i32Width)
    {
        nRight = pOffscreen->i32Width-nPoints/2;
    }


    nRectW = nRight - nLeft;
    nRecrH = nBottom - nTop;

    if(pOffscreen->u32PixelArrayFormat == ASVL_PAF_NV21 ||ASVL_PAF_NV12==pOffscreen->u32PixelArrayFormat)
    {
        //draw the top line
        MUInt8* pByteYTop =pOffscreen->ppu8Plane[0] + (nTop-nPoints/2)*pOffscreen->pi32Pitch[0] + nLeft -nPoints/2;
        MUInt8* pByteVTop =pOffscreen->ppu8Plane[1] + ((nTop-nPoints/2)/2)*pOffscreen->pi32Pitch[1] + nLeft-nPoints/2;
        MUInt8* pByteYBottom =pOffscreen->ppu8Plane[0] + (nTop+nRecrH-nPoints/2)*pOffscreen->pi32Pitch[0]+nLeft-nPoints/2;
        MUInt8* pByteVBottom =pOffscreen->ppu8Plane[1] + ((nTop+nRecrH-nPoints/2)/2)*pOffscreen->pi32Pitch[1]+nLeft-nPoints/2;
        for(int i=0;i<nPoints;i++)
        {
            pByteVTop = pOffscreen->ppu8Plane[1] + ((nTop+i-nPoints/2)/2)*pOffscreen->pi32Pitch[1] + nLeft -nPoints/2;
            pByteVBottom = pOffscreen->ppu8Plane[1] + ((nTop+i+nRecrH-nPoints/2)/2)*pOffscreen->pi32Pitch[1] + nLeft -nPoints/2;
            memset(pByteYTop,255,nRectW+nPoints);
            memset(pByteYBottom,255,nRectW+nPoints);
            pByteYTop+=pOffscreen->pi32Pitch[0];
            pByteYBottom+=pOffscreen->pi32Pitch[0];
            memset(pByteVTop,60*iType,nRectW+nPoints);
            memset(pByteVBottom, 60*iType,nRectW+nPoints);
        }
        for(int i = 0 ;i<nRecrH;i++)
        {
            MUInt8* pByteYLeft = pOffscreen->ppu8Plane[0] + (nTop+i)*pOffscreen->pi32Pitch[0]+nLeft-nPoints/2;
            MUInt8* pByteYRight = pOffscreen->ppu8Plane[0] + (nTop+i)*pOffscreen->pi32Pitch[0]+nLeft+nRectW-nPoints/2;
            MUInt8* pByteVLeft = pOffscreen->ppu8Plane[1]+ (((nTop+i)/2)*pOffscreen->pi32Pitch[1])   +nLeft-nPoints/2;
            MUInt8* pByteVRight = pOffscreen->ppu8Plane[1]+ (((nTop+i)/2)*pOffscreen->pi32Pitch[1])   +nLeft+nRectW-nPoints/2;
            memset(pByteYLeft,255,nPoints);
            memset(pByteYRight,255,nPoints);
            memset(pByteVLeft,60*iType,nPoints);
            memset(pByteVRight,60*iType,nPoints);
        }

    }
}

static void __attribute__((unused))  DrawRectROI(LPASVLOFFSCREEN pOffscreen, CHIRECT rt, MInt32 nLineWidth, MInt32 iType)
{
    int l = rt.left;
    int t = rt.top;
    int r = rt.left + rt.width;
    int b = rt.top + rt.height;
    MRECT rect = {l, t, r, b};
    DrawRect(pOffscreen, rect, nLineWidth, iType);
}

#endif // UTILS_H
