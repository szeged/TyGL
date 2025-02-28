/*
 * Copyright (C) 2009 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

// This all-in-one cpp file cuts down on template bloat to allow us to build our Windows release build.

#include "ColorDistance.cpp"
#include "SVGAElement.cpp"
#include "SVGAltGlyphDefElement.cpp"
#include "SVGAltGlyphElement.cpp"
#include "SVGAltGlyphItemElement.cpp"
#include "SVGAngle.cpp"
#include "SVGAnimateColorElement.cpp"
#include "SVGAnimateElement.cpp"
#include "SVGAnimateElementBase.cpp"
#include "SVGAnimateMotionElement.cpp"
#include "SVGAnimateTransformElement.cpp"
#include "SVGAnimatedAngle.cpp"
#include "SVGAnimatedBoolean.cpp"
#include "SVGAnimatedColor.cpp"
#include "SVGAnimatedEnumeration.cpp"
#include "SVGAnimatedInteger.cpp"
#include "SVGAnimatedIntegerOptionalInteger.cpp"
#include "SVGAnimatedLength.cpp"
#include "SVGAnimatedLengthList.cpp"
#include "SVGAnimatedNumber.cpp"
#include "SVGAnimatedNumberList.cpp"
#include "SVGAnimatedNumberOptionalNumber.cpp"
#include "SVGAnimatedPath.cpp"
#include "SVGAnimatedPointList.cpp"
#include "SVGAnimatedPreserveAspectRatio.cpp"
#include "SVGAnimatedRect.cpp"
#include "SVGAnimatedString.cpp"
#include "SVGAnimatedTransformList.cpp"
#include "SVGAnimatedType.cpp"
#include "SVGAnimatedTypeAnimator.cpp"
#include "SVGAnimationElement.cpp"
#include "SVGCircleElement.cpp"
#include "SVGClipPathElement.cpp"
#include "SVGColor.cpp"
#include "SVGComponentTransferFunctionElement.cpp"
#include "SVGCursorElement.cpp"
#include "SVGDefsElement.cpp"
#include "SVGDescElement.cpp"
#include "SVGDocument.cpp"
#include "SVGDocumentExtensions.cpp"
#include "SVGElement.cpp"
#include "SVGElementInstance.cpp"
#include "SVGEllipseElement.cpp"
#include "SVGException.cpp"
#include "SVGExternalResourcesRequired.cpp"
#include "SVGFEBlendElement.cpp"
#include "SVGFEColorMatrixElement.cpp"
#include "SVGFEComponentTransferElement.cpp"
#include "SVGFECompositeElement.cpp"
#include "SVGFEConvolveMatrixElement.cpp"
#include "SVGFEDiffuseLightingElement.cpp"
#include "SVGFEDisplacementMapElement.cpp"
#include "SVGFEDistantLightElement.cpp"
#include "SVGFEDropShadowElement.cpp"
#include "SVGFEFloodElement.cpp"
#include "SVGFEFuncAElement.cpp"
#include "SVGFEFuncBElement.cpp"
#include "SVGFEFuncGElement.cpp"
#include "SVGFEFuncRElement.cpp"
#include "SVGFEGaussianBlurElement.cpp"
#include "SVGFEImageElement.cpp"
#include "SVGFELightElement.cpp"
#include "SVGFEMergeElement.cpp"
#include "SVGFEMergeNodeElement.cpp"
#include "SVGFEMorphologyElement.cpp"
#include "SVGFEOffsetElement.cpp"
#include "SVGFEPointLightElement.cpp"
#include "SVGFESpecularLightingElement.cpp"
#include "SVGFESpotLightElement.cpp"
#include "SVGFETileElement.cpp"
#include "SVGFETurbulenceElement.cpp"
#include "SVGFilterElement.cpp"
#include "SVGFilterPrimitiveStandardAttributes.cpp"
#include "SVGFitToViewBox.cpp"
#include "SVGFontData.cpp"
#include "SVGFontElement.cpp"
#include "SVGFontFaceElement.cpp"
#include "SVGFontFaceFormatElement.cpp"
#include "SVGFontFaceNameElement.cpp"
#include "SVGFontFaceSrcElement.cpp"
#include "SVGFontFaceUriElement.cpp"
#include "SVGForeignObjectElement.cpp"
#include "SVGGElement.cpp"
#include "SVGGlyphElement.cpp"
#include "SVGGlyphRefElement.cpp"
#include "SVGGradientElement.cpp"
#include "SVGGraphicsElement.cpp"
#include "SVGHKernElement.cpp"
#include "SVGImageElement.cpp"
#include "SVGImageLoader.cpp"
#include "SVGLangSpace.cpp"
#include "SVGLength.cpp"
#include "SVGLengthContext.cpp"
#include "SVGLengthList.cpp"
#include "SVGLineElement.cpp"
#include "SVGLinearGradientElement.cpp"
#include "SVGLocatable.cpp"
#include "SVGMPathElement.cpp"
#include "SVGMarkerElement.cpp"
#include "SVGMaskElement.cpp"
#include "SVGMetadataElement.cpp"
#include "SVGMissingGlyphElement.cpp"
#include "SVGNumberList.cpp"
#include "SVGPaint.cpp"
#include "SVGParserUtilities.cpp"
#include "SVGPathBlender.cpp"
#include "SVGPathBuilder.cpp"
#include "SVGPathByteStreamBuilder.cpp"
#include "SVGPathByteStreamSource.cpp"
#include "SVGPathElement.cpp"
#include "SVGPathParser.cpp"
#include "SVGPathSegList.cpp"
#include "SVGPathSegListBuilder.cpp"
#include "SVGPathSegListSource.cpp"
#include "SVGPathStringBuilder.cpp"
#include "SVGPathStringSource.cpp"
#include "SVGPathTraversalStateBuilder.cpp"
#include "SVGPathUtilities.cpp"
#include "SVGPatternElement.cpp"
#include "SVGPointList.cpp"
#include "SVGPolyElement.cpp"
#include "SVGPolygonElement.cpp"
#include "SVGPolylineElement.cpp"
#include "SVGPreserveAspectRatio.cpp"
#include "SVGRadialGradientElement.cpp"
#include "SVGRectElement.cpp"
#include "SVGSVGElement.cpp"
#include "SVGScriptElement.cpp"
#include "SVGSetElement.cpp"
#include "SVGStopElement.cpp"
#include "SVGStringList.cpp"
#include "SVGStyleElement.cpp"
#include "SVGSwitchElement.cpp"
#include "SVGSymbolElement.cpp"
#include "SVGTRefElement.cpp"
#include "SVGTSpanElement.cpp"
#include "SVGTests.cpp"
#include "SVGTextContentElement.cpp"
#include "SVGTextElement.cpp"
#include "SVGTextPathElement.cpp"
#include "SVGTextPositioningElement.cpp"
#include "SVGTitleElement.cpp"
#include "SVGTransform.cpp"
#include "SVGTransformDistance.cpp"
#include "SVGTransformList.cpp"
#include "SVGTransformable.cpp"
#include "SVGURIReference.cpp"
#include "SVGUseElement.cpp"
#include "SVGVKernElement.cpp"
#include "SVGViewElement.cpp"
#include "SVGViewSpec.cpp"
#include "SVGZoomAndPan.cpp"
#include "SVGZoomEvent.cpp"
