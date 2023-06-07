#pragma once

namespace TrueSight {

/**
 * 效果种类枚举
 * ParameterFlag is the image processing effect control parameter
 */
enum EffectIndex {
    ///  all套装1 效果，应用json内包含所有效果(10:ColorTone 20:FaceRetouch 30:BasicRetouch 40:FaceStereo 50:SkinTone 60:Makeup 70:FacialRefine)
    kEffectIndex_packet = 0,                             // 套装类别 Packet category
    kEffectIndex_ColorTone = 10,                         // 全局颜色调整, global color adjustment
    kEffectIndex_FaceRetouch = 20,                       // 美颜磨皮, face retouch
    kEffectIndex_BasicRetouch = 30,                      // 基础美颜, basic retouch
    kEffectIndex_FaceStereo = 40,                        // 五官立体, enhance facial bone structure
    kEffectIndex_SkinTone = 50,                          // 肤色, skin tone
    kEffectIndex_Makeup = 60,                            // 妆容, Makeup
    kEffectIndex_Rule = 65,                              // 特殊效果规则 Special effects rule
    kEffectIndex_FacialRefine = 70,                      // 美型,  body reshaping
    kEffectIndex_Count
};


/**
* AI效果枚举
*/
enum class AIEffectType {
    //拍照AI效果
    kAIEffectType_E2Eacne, /// 祛斑祛痘
    kAIEffectType_E2Ewrinkle, /// 祛纹
    kAIEffectType_E2EskinUnify, /// 匀肤

    //实时预览AI效果
    kAIEffectType_E2EacneLight = 100, /// 祛斑祛痘
};


/**
 * 效果参数枚举
 */
enum ParameterFlag {
    kParameterFlag_Invalid = -1,                         // 非法参数类型  illegal parameter type
    // 美颜相关0-99
    kParameterFlag_Spotless = 0,                         // 祛斑祛痘:          0.0-1.0
    kParameterFlag_FaceRetouch = 1,                      // 面部美颜:          0.0-1.0
    kParameterFlag_HairSmooth = 2,                       // 头发顺滑:          0.0-1.0
    kParameterFlag_BrightEye = 4,                        // 亮眼调整:          0.0-1.0
    kParameterFlag_LightenPouch = 5,                     // 淡化黑眼圈:        0.0-1.0
    kParameterFlag_Rhytidectomy = 6,                     // 祛法令纹:          0.0-1.0
    kParameterFlag_SkinEnhance = 7,                      // 肤质增强:          0.0-1.0
    kParameterFlag_FaceStereoShadow = 8,                 // 五官立体-阴影:     0.0-1.0
    kParameterFlag_FaceStereoLight = 9,                  // 五官立体-高光:     0.0-1.0
    kParameterFlag_SkinToneMapping = 10,                 // 肤色映射:          0.0-1.0
    kParameterFlag_LipColorMapping = 11,                 // 唇色映射:          0.0-1.0
    kParameterFlag_Sharpness = 12,                       // 图像锐化:          0.0-1.0
    kParameterFlag_GlobalHighlight = 13,                 // 全图高光调整:      -1.0-1.0
    kParameterFlag_GlobalShadow = 14,                    // 全图阴影调整:      -1.0-1.0
    /* kParameterFlag_Spotless	                         freckles & acne removal	0.0-1.0
    kParameterFlag_FaceRetouch	                         face retouching	        0.0-1.0
    kParameterFlag_HairSmooth	                         hair smoothing	            0.0-1.0
    kParameterFlag_FacialSmooth	                         face skin smoothing	    0.0-1.0
    kParameterFlag_BrightEye	                         lighten eyes	            0.0-1.0
    kParameterFlag_LightenPouch                          reduce dark circles	    0.0-1.0
    kParameterFlag_Rhytidectomy                          reduce wrinkles	        0.0-1.0
    kParameterFlag_SkinEnhance	                         enhance skin texture	    0.0-1.0
    kParameterFlag_FaceStereoShadow	                     enhance facial bone structure by adding contour	0.0-1.0
    kParameterFlag_FaceStereoLight	                     enhance facial bone structure by adding highlight	0.0-1.0
    kParameterFlag_SkinToneMapping	                     skin tone mapping	        0.0-1.0
    kParameterFlag_LipColorMapping	                     lip color mapping	        0.0-1.0
    kParameterFlag_Sharpness	                         sharpen image	            0.0-1.0
    kParameterFlag_GlobalHighlight	                     adjust the highlight of the whole picture	-1.0-1.0
    kParameterFlag_GlobalShadow	                         adjust the shadow of the whole picture	     -1.0-1.0 */

    // 美妆相关100-199
    kParameterFlag_EyebrowMakeup = 100,          // 眉毛美妆:          0.0-1.0
    kParameterFlag_SilkwormMakeup = 101,         // 卧蚕美妆:          0.0-1.0
    kParameterFlag_DoubleEyelidMakeup = 102,     // 双眼皮美妆:        0.0-1.0
    kParameterFlag_EyelinerMakeup = 103,         // 眼线美妆:          0.0-1.0
    kParameterFlag_EyeShadowMakeup = 104,        // 眼影美妆:          0.0-1.0
    kParameterFlag_EyelashMakeup = 105,          // 眼睫毛美妆:        0.0-1.0
    kParameterFlag_BlusherMakeup = 106,          // 腮红美妆:          0.0-1.0
    kParameterFlag_LipMakeup = 107,              // 唇妆:             0.0-1.0
    kParameterFlag_TeethWhiten = 108,            // 牙齿美白:          0.0关闭, 1.0开启
    kParameterFlag_HairDye = 109,                // 染发:              0.0-1.0
    kParameterFlag_BronzerMakeup = 110,          // 修容:              0.0-1.0
    kParameterFlag_EyePupil = 111,               // 眼瞳:              0.0-1.0
    /* kParameterFlag_EyebrowMakeup	              cute eye bags makeup	0.0-1.0
    kParameterFlag_SilkwormMakeup	              eyeliner makeup	    0.0-1.0
    kParameterFlag_DoubleEyelidMakeup		      eye shadow makeup	    0.0-1.0
    kParameterFlag_EyelinerMakeup		          eyelashes makeup	    0.0-1.0
    kParameterFlag_EyeShadowMakeup		          eye shadow makeup	    0.0-1.0
    kParameterFlag_EyelashMakeup		          eyelashes makeup	    0.0-1.0
    kParameterFlag_BlusherMakeup		          blush makeup	        0.0-1.0
    kParameterFlag_LipMakeup		              lip makeup	        0.0-1.0
    kParameterFlag_TeethWhiten		              Whiten Teeth	        0.0 is off, 1.0 is on
    kParameterFlag_HairDye		                  dye hair	            0.0-1.0
    kParameterFlag_BronzerMakeup		          bronzer	            0.0-1.0
    kParameterFlag_EyePupil		                  eye pupil	            0.0-1.0 */

    // 面部美型200-299
    kParameterFlag_FaceLift = 200,                       // 瘦脸调整:               整个脸部向内瘦，脸颊⾄下巴瘦的程度更强，0.0-0.1表示瘦的程度
    kParameterFlag_FacialRefine_EyeZoom = 201,           // 面部调整-眼部:           眼睛的放大和缩小, 0.0-0.5表示调小, 0.5表示不做, 0.5-1.0表示调大
    kParameterFlag_FacialRefine_HairLine = 202,          // 面部调整-发际线:         发际线调整，0.0-0.5表示上调，0.5表示不做，0.5-1.0表示下调
    kParameterFlag_FacialRefine_Nose = 203,              // 面部调整-鼻子:           鼻子的放大和缩小，0.0-0.5表示调大, 0.5表示不做, 0.5-1.0表示调小
    kParameterFlag_FacialRefine_CheekBone = 204,         // 面部调整-颧骨:           颧⻣调整，0.0-0.5表示内收，0.5表示不做，0.5-1.0表示外扩
    kParameterFlag_FacialRefine_Chin = 205,              // 面部调整-下巴:           下巴的⾼度调整，0.0-0.5表示缩短，0.5表示不做，0.5-1.0表示拉⻓
    kParameterFlag_FacialRefine_EyeHigher = 206,         // 面部调整-眼高:           眼睛的纵向变形, 0.0-0.5表示调小，0.5无效果，0.5-1.0表示调大
    kParameterFlag_FacialRefine_EyeDistance = 207,       // 面部调整-眼距:           两只眼睛的距离远近, 0.0-0.5表示距离调小，0.5无效果，0.5-1.0表示距离调大
    kParameterFlag_FacialRefine_EyeAngle = 208,          // 面部调整-眼角:           眼睛的倾斜⻆度调整, 0.0-0.5眼角下沉，0.5无效果，0.5-1.0眼角上扬
    kParameterFlag_FacialRefine_NoseHigher = 209,        // 面部调整-鼻子高度:        ⿐⼦的纵向变形，0.0-0.5表示缩短，0.5表示不做，0.5-1.0表示拉⻓
    kParameterFlag_FacialRefine_NoseAlar = 210,          // 面部调整-鼻翼调整:        ⿐翼两侧的调整，0.0-0.5表示外扩，0.5表示不做，0.5-1.0表示内收
    kParameterFlag_FacialRefine_NoseBridge = 211,        // 面部调整-鼻梁调整:        ⿐梁的调整，0.0-0.5表示外扩，0.5表示不做，0.5-1.0表示内收
    kParameterFlag_FacialRefine_NoseTip = 212,           // 面部调整-鼻尖调整:        ⿐尖的调整，0.0-0.5表示放⼤，0.5表示不做，0.5-1.0表示缩⼩
    kParameterFlag_FacialRefine_MouthSize = 213,         // 面部调整-嘴巴大小:        嘴巴的放⼤和缩⼩，0.0-0.5表示调⼩，0.5表示不做，0.5-1.0表示调⼤
    kParameterFlag_FacialRefine_MouthHigher = 214,       // 面部调整-嘴巴的上下位置:   嘴巴的上下位置，0.0-0.5表示上调，0.5表示不做，0.5-1.0表示下调
    kParameterFlag_FacialRefine_MouthUpperLip = 215,     // 面部调整-上嘴唇厚度:      上嘴唇厚度调整，0.0-0.5表示薄，0.5表示不做，0.5-1.0表示厚
    kParameterFlag_FacialRefine_MouthLowperLip = 216,    // 面部调整-下嘴唇唇厚度:    下嘴唇厚度调整，0.0-0.5表示薄，0.5表示不做，0.5-1.0表示厚
    kParameterFlag_FacialRefine_EyeBrowThickness = 217,  // 面部调整-眉毛粗细:        眉⽑的粗细程度；0.0-0.5表示变细，0.5表示不做，0.5-1.0表示变粗
    kParameterFlag_FacialRefine_EyeBrowHigher = 218,     // 面部调整-眉毛上下位置:     眉⽑的上下位置；0.0-0.5表示上调，0.5表示不做，0.5-1.0表示下调
    kParameterFlag_FacialRefine_EyeBrowDistance = 219,   // 面部调整-眉毛距离:        两只眉⽑的距离远近，0.0-0.5表示调近，0.5表示不做，0.5-1.0表示调远
    kParameterFlag_FacialRefine_EyeBrowAngle = 220,      // 面部调整-眉毛角度:        眉⽑的倾斜⻆度调整，0.0-0.5表示Λ，0.5表示不做，0.5-1.0表示调V
    kParameterFlag_FacialRefine_EyeBrowShape = 221,      // 面部调整-眉毛形状:        眉⽑的形状调整，0.0-0.5表示变平，0.5表示不做，0.5-1.0表示眉峰变⾼
    kParameterFlag_HeadNarrow = 222,                     // 缩头调整:                整个头部的缩⼩，0.0-0.1表示缩⼩程度
    kParameterFlag_FaceWidth = 223,                      // 脸宽调整:                ⾯部的横向变形，0.0-0.5表示调窄，0.5表示不做，0.5-1.0表示调宽
    kParameterFlag_FacialRefine_MouthWidth = 224,        // 面部调整-嘴巴宽度:        嘴巴的横向变形，0.0-0.5表示调窄，0.5表示不做，0.5-1.0表示调宽
    kParameterFlag_FacialRefine_EyeWidth = 225,          // 面部调整-眼睛宽度:        眼睛的横向变形， 0.0-0.5表示调窄，0.5无效果，0.5-1.0表示调宽
    kParameterFlag_FacialRefine_EyeUpDownAdjust = 226,   // 面部调整-眼睛上下调整:     眼睛的上下位置, 0.0-0.5表示上调，0.5无效果，0.5-1.0表示下调
    kParameterFlag_FacialRefine_Temple = 227,            // 面部调整-太阳穴:          太阳⽳调整，0.0-0.5表示内收，0.5表示不做，0.5-1.0表示外扩
    kParameterFlag_FacialRefine_Jaw = 228,               // 面部调整-下颌:            下颌调整，0.0-0.5表示变宽，0.5表示不做，0.5-1.0表示变窄
    kParameterFlag_FacialRefine_SmallFace = 230,         // 面部调整-小脸:            整个⾯部中下庭向上收，0.0-0.1表示缩⼩程度
    kParameterFlag_FacialRefine_SkullEnhance = 232,      // 面部调整-颅顶增高:         颅顶增高，0.0-0.1表示增高程度
    /* kParameterFlag_FaceLift		                        slim face	                    0.0-1.0
    kParameterFlag_FacialRefine_EyeZoom	                    face sculpt-eyes	            0.0-0.5 is the range that you can control to smaller eyes, 0.5 means no effect, and 0.5-1.0 is the range that you can control to enlarg eyes
    kParameterFlag_FacialRefine_HairLine	                face sculpt-hairline	        0.0-0.5 is the range that you can control to lower hairline, 0.5 means no effect, and 0.5-1.0 is the interval that you can control to shrink hairline
    kParameterFlag_FacialRefine_Nose	                    face sculpt-nose	            0.0-0.5 is the range that you can control to enlarge nose, 0.5 means no effect, and 0.5-1.0 is the range that you can control to smaller nose
    kParameterFlag_FacialRefine_CheekBone	                face sculpt-cheekbone	        0.0 to 0.5 is the range that you can control to flatten the cheekbones, 0.5 indicates no effect, and 0.5 to 1.0 is the range that you can control to make the cheekbones more prominent
    kParameterFlag_FacialRefine_Chin	                    face sculpt-chin	            0.0-0.5 is the range that you can control to shrink chin, 0.5 means no effect, and 0.5-1.0 is the interval you can control to elongate chin
    kParameterFlag_FacialRefine_EyeHigher	                face sculpt-eye height	        0.0-0.5 is the range that you can control to reduce eye height, 0.5 means no effect, and 0.5-1.0 is the interval that you can control to increase eye height
    kParameterFlag_FacialRefine_EyeDistance	                face sculpt-eye distance	    0.0-0.5 is the range that you can control to reduce eyes distance, 0.5 means no effect, and 0.5-1.0 is the interval that you can control to increase eyes distance
    kParameterFlag_FacialRefine_EyeAngle	                face sculpt-tilt eyes	        0.0-0.5 is the range that you can control to tilt down eyes, 0.5 means no effect, and 0.5-1.0 is the interval that you can control to tilt up eyes.
    kParameterFlag_FacialRefine_NoseHigher	                face sculpt-nose height	        0.0-0.5 is the range that you can control to lower nose height, 0.5 means no effect, and 0.5-1.0 is the interval that you can control to higher nose height
    kParameterFlag_FacialRefine_NoseAlar	                face sculpt-nose alar	        0.0-0.5 is the range that you can control to shrink nose alar, 0.5 means no effect, and 0.5-1.0 is the interval that you can control to bigger nose alar
    kParameterFlag_FacialRefine_NoseBridge	                face sculpt-nose bridge	        0.0-0.5 is the range that you can control to narrow nose bridge, 0.5 means no effect, and 0.5-1.0 is the interval that you can control to widen nose bridge
    kParameterFlag_FacialRefine_NoseTip	face                sculpt-nose tip	                0.0-0.5 is the range that you can control to smaller nose tip, 0.5 means no effect, and 0.5-1.0 is the interval that you can control to bigger nose tip
    kParameterFlag_FacialRefine_MouthSize	                face sculpt-mouth size	        0.0-0.5 is the range that you can control to smaller mouth, 0.5 means no effect, and 0.5-1.0 is the range that you can control to bigger eyes
    kParameterFlag_FacialRefine_MouthHigher	ace             sculpt-mouth height	            0.0-0.5 is the range that you can control to reduce mouth height, 0.5 means no effect, and 0.5-1.0 is the range that you can control to increase mouth height
    kParameterFlag_FacialRefine_MouthUpperLip	            face sculpt-upper lip	        0.0-0.5 is the range that you can control to smaller upper lip, 0.5 means no effect, and 0.5-1.0 is the range that you can control to bigger upper lip
    kParameterFlag_FacialRefine_MouthLowperLip	            face sculpt-lower lip	        0.0-0.5 is the range that you can control to smaller lower lip, 0.5 means no effect, and 0.5-1.0 is the range that you can control to bigger lower lip
    kParameterFlag_FacialRefine_EyeBrowThickness	        face sculpt-eyebrow thickness	0.0-0.5 is the range that you can control to thin eyebrows, 0.5 means no effect, and 0.5-1.0 is the range that you can control to thick eyebrows.
    kParameterFlag_FacialRefine_EyeBrowHigher	            face sculpt-eyebrow height	    0.0-0.5 is the range that you can control to reduce eyebrow height, 0.5 means no effect, and 0.5-1.0 is the range that you can control to increase eyebrow height
    kParameterFlag_FacialRefine_EyeBrowDistance	            face sculpt-eyebrow distance	0.0-0.5 is the range that you can control to reduce the distance between eyebrows, 0.5 means no effect, and 0.5-1.0 is the range that you can control to the distance between eyebrows
    kParameterFlag_FacialRefine_EyeBrowAngle	            face sculpt-tilt eyebrow	    0.0-0.5 is the range that you can control to tilt down eyebrow, 0.5 means no effect, and 0.5-1.0 is the range that you can control to tilt up eyebrow
    kParameterFlag_FacialRefine_EyeBrowShape	            face sculpt-eyebrow shape	    0.0-0.5 is the range that you can control to narrow the angle of eyebrow peak, 0.5 means no effect, and 0.5-1.0 is the range that you can control to increase the angle of eyebrow peak.
    kParameterFlag_HeadNarrow	                            scale down head	                0.0-1.0
    kParameterFlag_FaceWidth	                            adjust face width	            0.0-0.5 is the range that you can control to reduce the distance between eyebrows
    kParameterFlag_FacialRefine_MouthWidth	                face sculpt-mouth width	        0.0-0.5 is the range that you can control to reduce the distance between eyebrows, 0.5 means no effect, and 0.5-1.0 is the range that you can control to the distance between eyebrows
    kParameterFlag_FacialRefine_EyeWidth	                face sculpt-eye width	        0.0-0.5 is the range that you can control to reduce the distance between eyebrows, 0.5 means no effect, and 0.5-1.0 is the range that you can control to the distance between eyebrows
    kParameterFlag_FacialRefine_EyeUpDownAdjust	            face sculpt-eye position	    0.0-0.5 is the range that you can control to reduce the distance between eyebrows, 0.5 means no effect, and 0.5-1.0 is the range that you can control to the distance between eyebrows
    kParameterFlag_FacialRefine_Temple	                    face sculpt-temple	            0.0 to 0.5 is the range that you can control to flatten the temple, 0.5 indicates no effect, and 0.5 to 1.0 is the range that you can control to make the temple more prominent
    kParameterFlag_FacialRefine_Jaw	                        face sculpt-jaw	                0.0 to 0.5 is the range that you can control to shrink the jaw, 0.5 indicates no effect, and 0.5 to 1.0 is the range that you can control to elongate the jaw
    kParameterFlag_FacialRefine_SmallFace	                face sculpt-shorten face	    0.0 means off, 1.0 means on   */

    // 滤镜透明度相关300-
    kParameterFlag_FilterAlpha = 300,                    // 滤镜透明度:           0.0-1.0
    kParameterFlag_MakeupFilterAlpha = 301,              // 妆容滤镜-妆容调整:     0.0-1.0
    kParameterFlag_SkinToneAlpha = 302,                  // 肤色透明度调整:        0.0-1.0
    kParameterFlag_ColorToneAlpha = 304,                 // 全局颜色透明度调整:     0.0-1.0
    kParameterFlag_LightAlpha = 305,                     // 全图打光透明度:        0.0-1.0
    /*kParameterFlag_FilterAlpha	                        filter transparency	 0.0-1.0
    kParameterFlag_MakeupFilterAlpha	                    Makeup Filter - Makeup Adjustment	0.0-1.0
    kParameterFlag_SkinToneAlpha	                        Skin Tone Transparency Adjustment	0.0-1.0 */

    // 后处理相关1000-
    kParameterFlag_Geomorphing = 1000,                   // 是否开启几何形变:  0.0关闭, 1.0开启
    //kParameterFlag_Geomorphing	                        whether to enable the portrait distortion correction	0.0 means off, 1.0 means on

    // EnhanceEdit
    kParameterFlag_EnhanceEdit_Brightness = 3001,           // 亮度
    kParameterFlag_EnhanceEdit_Contrast = 3002,             // 对比度
    kParameterFlag_EnhanceEdit_Highlight = 3003,            // 高光
    kParameterFlag_EnhanceEdit_Shadow = 3004,               // 阴影
    kParameterFlag_EnhanceEdit_Fade = 3005,                 // 褪色
    kParameterFlag_EnhanceEdit_Saturation = 3006,           // 饱和度
    kParameterFlag_EnhanceEdit_ColorTemperature = 3007,     // 色温
    kParameterFlag_EnhanceEdit_Hue = 3008,                  // 色调
    kParameterFlag_EnhanceEdit_HueSplit_HightLight = 3009,  // 色调分离 lut 高光
    kParameterFlag_EnhanceEdit_HueSplit_Shadow = 3010,      // 色调分离 lut 阴影
    kParameterFlag_EnhanceEdit_HSL_Hue = 3011,              // HSL 色相
    kParameterFlag_EnhanceEdit_HSL_Saturation = 3012,       // HSL 饱和度
    kParameterFlag_EnhanceEdit_HSL_Lightness = 3013,        // HSL 明度
    kParameterFlag_EnhanceEdit_SharpenEdge = 3014,          // edge锐化
    kParameterFlag_EnhanceEdit_Structure = 3015,            // 结构
    kParameterFlag_EnhanceEdit_GranularNoise = 3016,        // 颗粒
    kParameterFlag_EnhanceEdit_Dispersion = 3017,           // 色散
    kParameterFlag_EnhanceEdit_Vignette = 3018,             // 暗角
    kParameterFlag_EnhanceEdit_Texture = 3019,              // 纹理

    /// 仅针对拍后
    kParameterFlag_FaceRetouch_SkintoneSwitch = 10000,      // 肤色统一开关: 0.0关闭, 1.0开启; unify skin tone: 0.0 means off, 1.0 means on

    kParameterFlag_DeHaze = 10100,                          // 去雾; dehaze: 0.0 means off, 1.0 means on
    kParameterFlag_EnvLight_Makeup = 10101,                 // 环境光(逆光、暗光、小人脸减弱)-妆容; environment light (backlight, hard light, reduce smaller face effect )- makeup: 0.0 means off, 1.0 means on
    kParameterFlag_EnvLight_FaceRetouch = 10102,            // 环境光(暗光减弱)-磨皮; environment light (reduce dim light)- skin smoothing: 0.0 means off, 1.0 means on

    //kParameterFlag_E2E_NevusKeep = 10200,                   // deprecated
    kParameterFlag_E2E_RemoveNevus = 10200,                 // 拍后：祛除痣； Captured image: mole removal: 0.0 means off, 1.0 means on

    kParameterFlag_SkinUnify = 10250,                       // 拍后：匀肤； Captured image: Ai FaceRetouch : 0.0 means off, 1.0 means on
    kParameterFlag_Tooth_Orthodontics = 10251,              // 拍后：牙齿矫正； Captured image: Ai orthodontics : 0.0 means off, 1.0 means on
    kParameterFlag_E2Ewrinkle = 10252,                      // 拍后：ai去纹； Captured image: Ai remove wrinkle: 0.0 means off, 1.0 means on

    //kParameterFlag_Magic_MakeupGender = 10300,              // deprecated，是否区分男女妆容: 0.0关闭男女分妆容(男生开启妆容), 1.0开启男女分妆(男生关闭妆容)
    kParameterFlag_Magic_MakeupGenderMaleDegree = 10301,    // 男生是否开启妆容；whether to enable makeup effect on male: 0.0 means off, 1.0 means on

    kParameterFlag_Count                                    // 参数类型集合
};

} // namespace TrueSight