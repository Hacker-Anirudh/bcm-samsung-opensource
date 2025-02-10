/*
 * Copyright 2011, The Android Open Source Project
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#include "DrawExtra.h"
#include "FindCanvas.h"
#include "GLExtras.h"
#include "IntRect.h"
#include "TilesManager.h"
#include "android_graphics.h"
#include "LayerAndroid.h"
#include <cutils/log.h>
#include <wtf/text/CString.h>

#undef XLOGC
#define XLOGC(...) android_printLog(ANDROID_LOG_DEBUG, "GLExtras", __VA_ARGS__)

#ifdef DEBUG

#undef XLOG
#define XLOG(...) android_printLog(ANDROID_LOG_DEBUG, "GLExtras", __VA_ARGS__)

#else

#undef XLOG
#define XLOG(...)

#endif // DEBUG

// Touch ring border width. This is doubled if the ring is not pressed
#define RING_BORDER_WIDTH 1
// Color of the ring is 0x6633b5e5 (copied from framework's holo_light)
//#define COLOR_HOLO_LIGHT &m_lightRingTexture, 0x33, 0xb5, 0xe5, 0.4f
// SAMSUNG CHANGE : RGB value changed 
#define COLOR_HOLO_LIGHT &m_lightRingTexture, 0xFF, 0x91, 0x29, 0.4f
// Color of the ring is 0x660099cc (copied from framework's holo_dark)
#define COLOR_HOLO_DARK &m_darkRingTexture, 0x00, 0x99, 0xcc, 0.6f
// Put a cap on the number of matches to draw.  If the current page has more
// matches than this, only draw the focused match. This both prevents clutter
// on the page and keeps the performance happy
#define MAX_NUMBER_OF_MATCHES_TO_DRAW 101

//SAMSUNG CHANGE : MPSG100005265 >>
// Find-on-page performance improvement
// In some sites, small portion of page will be keep on updating due to dynamic content
// Limit the triggering of find call in case layer size is less than thresold size
#define LAYER_WIDTH_THRESOLD 100
#define LAYER_HEIGHT_THRESOLD 100
//SAMSUNG CHANGE : MPSG100005265 <<

GLExtras::GLExtras()
    : m_findOnPage(0)
    , m_ring(0)
    , m_drawExtra(0)
    , m_lightRingTexture(-1)
    , m_darkRingTexture(-1)
{
}

GLExtras::~GLExtras()
{
}

void GLExtras::drawRing(SkRect& srcRect, int* texture, int r, int g, int b, float a)
{
    if (*texture == -1)
        *texture = GLUtils::createSampleColorTexture(r, g, b);

    if (srcRect.fRight <= srcRect.fLeft || srcRect.fBottom <= srcRect.fTop) {
        // Invalid rect, reject it
        return;
    }
    XLOG("drawQuad [%fx%f, %f, %f]", srcRect.fLeft, srcRect.fTop,
         srcRect.width(), srcRect.height());
    TilesManager::instance()->shader()->drawQuad(srcRect, *texture, a);
}

void GLExtras::drawRegion(const SkRegion& region, bool fill,
                          bool drawBorder, bool useDark)
{
    if (region.isEmpty())
        return;
    if (fill) {
        SkRegion::Iterator rgnIter(region);
        while (!rgnIter.done()) {
            const SkIRect& ir = rgnIter.rect();
            SkRect r;
            r.set(ir.fLeft, ir.fTop, ir.fRight, ir.fBottom);
            if (useDark)
                drawRing(r, COLOR_HOLO_DARK);
            else
                drawRing(r, COLOR_HOLO_LIGHT);
            rgnIter.next();
        }
    }
    if (fill && !drawBorder)
        return;
    SkPath path;
    if (!region.getBoundaryPath(&path))
        return;
    SkPath::Iter iter(path, true);
    SkPath::Verb verb;
    SkPoint pts[4];
    SkRegion clip;
    SkIRect startRect;
    while ((verb = iter.next(pts)) != SkPath::kDone_Verb) {
        if (verb == SkPath::kLine_Verb) {
            SkRect r;
            r.set(pts, 2);
            SkIRect line;
            int borderWidth = RING_BORDER_WIDTH;
            if (!fill)
                borderWidth *= 2;
            line.fLeft = r.fLeft - borderWidth;
            line.fRight = r.fRight + borderWidth;
            line.fTop = r.fTop - borderWidth;
            line.fBottom = r.fBottom + borderWidth;
            if (clip.intersects(line)) {
                clip.op(line, SkRegion::kReverseDifference_Op);
                if (clip.isEmpty())
                    continue; // Nothing to draw, continue
                line = clip.getBounds();
                if (SkIRect::Intersects(startRect, line)) {
                    clip.op(startRect, SkRegion::kDifference_Op);
                    if (clip.isEmpty())
                        continue; // Nothing to draw, continue
                    line = clip.getBounds();
                }
            } else {
                clip.setRect(line);
            }
            r.set(line.fLeft, line.fTop, line.fRight, line.fBottom);
            if (useDark)
                drawRing(r, COLOR_HOLO_DARK);
            else
                drawRing(r, COLOR_HOLO_LIGHT);
            if (startRect.isEmpty()) {
                startRect.set(line.fLeft, line.fTop, line.fRight, line.fBottom);
            }
        }
        if (verb == SkPath::kMove_Verb) {
            startRect.setEmpty();
        }
    }
}

void GLExtras::drawCursorRings()
{
    SkRegion region;
    for (size_t i = 0; i < m_ring->rings().size(); i++) {
        IntRect rect = m_ring->rings().at(i);
        if (i == 0)
            region.setRect(rect);
        else
            region.op(rect, SkRegion::kUnion_Op);
    }
    drawRegion(region, m_ring->m_isPressed, !m_ring->m_isButton, false);
}

void GLExtras::drawFindOnPage(SkRect& viewport)
{
    WTF::Vector<MatchInfo>* matches = m_findOnPage->matches();
    XLOG("drawFindOnPage, matches: %p", matches);
    if (!matches || !m_findOnPage->isCurrentLocationValid())
        return;
    int count = matches->size();
    int current = m_findOnPage->currentMatchIndex();
    XLOG("match count: %d", count);
    bool findAllNeeded = false; //MPSG100005265	
//SAMSUNG CHANGE: MPSG100005525 >>	
    SkRegion globalLayersClipRegion =  getGlobalVisibleAreaForLayers(m_findOnPage->getRootLayer());   
//SAMSUNG CHANGE <<	
    if (count < MAX_NUMBER_OF_MATCHES_TO_DRAW)
        for (int i = 0; i < count; i++) {
            MatchInfo& info = matches->at(i);
//SAMSUNG CHANGE: MPSG100004860, MPSG100005091, MPSG100005265 - findonpage layer handling >>
            SkRegion mappedRegion(info.getLocation());
            bool isUnclipped = true;
            bool found = true;
            //Change local region bounds for layers to global region bounds
            if(info.isInLayer()) {
                found = getGlobalRegionForLayer(info.layerId(), mappedRegion, isUnclipped,
					info.layerPicture(), info.layerSize());
            }
//SAMSUNG CHANGE: MPSG100005525 >>			
            else {
                SkRegion testRegion(mappedRegion);
                isUnclipped = testRegion.op(globalLayersClipRegion, SkRegion::kDifference_Op);
                if(isUnclipped && testRegion != mappedRegion && 
                    globalLayersClipRegion.getBounds().width() >= LAYER_WIDTH_THRESOLD &&
                    globalLayersClipRegion.getBounds().height() >= LAYER_HEIGHT_THRESOLD) {
                    XLOG("Match [%d] mappedRegion [%dx%d, %d, %d] visible Region [%dx%d, %d, %d] layerClip [%d, %d]", 
                        i, mappedRegion.getBounds().fLeft, mappedRegion.getBounds().fTop,
                        mappedRegion.getBounds().width(), mappedRegion.getBounds().height(),
                        testRegion.getBounds().fLeft, testRegion.getBounds().fTop,
                        testRegion.getBounds().width(), testRegion.getBounds().height(),
                        globalLayersClipRegion.getBounds().width(), globalLayersClipRegion.getBounds().height());			
                    mappedRegion.set(testRegion);
                }
            }
//SAMSUNG CHANGE <<			
            if(!found) {
                findAllNeeded = true; //set flag for triggering new find
            }
            SkIRect rect = mappedRegion.getBounds();	
            if (rect.intersect(viewport.fLeft, viewport.fTop,
                               viewport.fRight, viewport.fBottom) && isUnclipped)
                drawRegion(mappedRegion, i == current, false, true);
//SAMSUNG CHANGE <<
#ifdef DEBUG
            else
                XLOG("Quick rejecting [%dx%d, %d, %d]", rect.fLeft, rect.fTop,
                     rect.width(), rect.height());
#endif // DEBUG
        }
    else {
        MatchInfo& info = matches->at(current);
//SAMSUNG CHANGE: MPSG100004860, MPSG100005091, MPSG100005265 - findonpage layer handling >>
        SkRegion mappedRegion(info.getLocation());
        bool isUnclipped = true;
        //Change local region bounds for layers to global region bounds
        if(info.isInLayer()) {
            findAllNeeded != getGlobalRegionForLayer(info.layerId(), mappedRegion, isUnclipped,
                    info.layerPicture(), info.layerSize());
        }
//SAMSUNG CHANGE <<		
//SAMSUNG CHANGE: MPSG100005525 >>		 
        else {
            SkRegion testRegion(mappedRegion); 
            isUnclipped = testRegion.op(globalLayersClipRegion, SkRegion::kDifference_Op);
            if(isUnclipped && testRegion != mappedRegion && 
                globalLayersClipRegion.getBounds().width() >= LAYER_WIDTH_THRESOLD &&
                globalLayersClipRegion.getBounds().height() >= LAYER_HEIGHT_THRESOLD) {
                XLOG("mappedRegion [%dx%d, %d, %d] visible Region [%dx%d, %d, %d] layerClip [%d, %d]", 
                    mappedRegion.getBounds().fLeft, mappedRegion.getBounds().fTop,
                    mappedRegion.getBounds().width(), mappedRegion.getBounds().height(),
                    testRegion.getBounds().fLeft, testRegion.getBounds().fTop,
                    testRegion.getBounds().width(), testRegion.getBounds().height(),
                    globalLayersClipRegion.getBounds().width(), globalLayersClipRegion.getBounds().height());           
                mappedRegion.set(testRegion);
            }           
        }
        if(isUnclipped)
        drawRegion(mappedRegion, true, false, true);
#ifdef DEBUG
        else
            XLOG("Quick rejecting [%dx%d, %d, %d]", mappedRegion.getBounds().fLeft, mappedRegion.getBounds().fTop,
                mappedRegion.getBounds().width(), mappedRegion.getBounds().height());
#endif // DEBUG		
//SAMSUNG CHANGE <<
    }
#ifdef DEBUG
    if(findAllNeeded) {
        XLOG("Trigger new find operation!!");   
    }
#endif // DEBUG	
    //send back error status if any	
    m_findOnPage->setFindAllNeeded(findAllNeeded); //MPSG100005265
}

//SAMSUNG CHANGE: MPSG100004860, MPSG100005091, MPSG100005265 - findonpage layer handling >>
// Map the local bounds to global bounds using the layer id and return the mapped bounds along with clipping flag
// Returns true if successfully mapped the region, else returns false to trigger new find operation
bool GLExtras::getGlobalRegionForLayer(int layerId, 
                                                                        SkRegion& mappedRegion, 
                                                                        bool& isUnclipped,
                                                                        SkPicture* layerPicture, 
                                                                        SkSize layerSize)
{
    LayerAndroid* root =  m_findOnPage->getRootLayer();
    if(!root) {
        XLOG("getGlobalRegionForLayer(): root = NULL");
        //There has been some change in root layer probably, hence don't draw invalid rects
        isUnclipped = false;		
        return false;
    }
	
    LayerAndroid* layer=root->findById(layerId);
    if(!layer) {
        XLOG("getGlobalRegionForLayer(): root=%p layerId=%d layer = NULL", root, layerId);
        //There has been some change in layers probably, hence don't draw invalid rects
        isUnclipped = false;
        return false;
    }

    if(layerPicture != layer->picture()) {
        XLOG("getGlobalRegionForLayer() : Picture has changed!! [old=%p, new=%p] "
                    "saved layerSize=(w=%g, h=%g) currentLayerSize=(w=%g, h=%g)", 
                    layerPicture, layer->picture(), layerSize.width(), layerSize.height(), 
                    layer->getSize().width(), layer->getSize().height());		
        //There has been some change in layers recording pictures, hence don't draw invalid rects		
        isUnclipped = false;
        if(layerSize == layer->getSize() && 
                (layerSize.width() < LAYER_WIDTH_THRESOLD || layerSize.height() < LAYER_HEIGHT_THRESOLD)) {
            XLOG("getGlobalRegionForLayer() : layerSizes are same, but size [w=%g, h=%g] is smaller than thresold!!",
                layerSize.width(), layerSize.height());
            return true; //skip calling find for small size layers	
        }
        return false;	
    }
	
    SkRect globalRect =layer->bounds();
    SkIRect visibleLayerRect = layer->visibleArea();			
#ifdef DEBUG
    SkIRect iRect = mappedRegion.getBounds();
    XLOG("getGlobalRegionForLayer() : rootLayer=%p layer=%p [%d] iRect=(%d, %d, r=%d, b=%d) "
                "layer-bounds=(%g, %g, r=%g, b=%g) visibleLayerRect=(%d, %d, r=%d, b=%d)", 
                root, layer, layerId, iRect.fLeft, iRect.fTop, iRect.fRight, iRect.fBottom,
                globalRect.fLeft, globalRect.fTop, globalRect.fRight, globalRect.fBottom,        
                visibleLayerRect.fLeft, visibleLayerRect.fTop, visibleLayerRect.fRight, visibleLayerRect.fBottom); 
#endif

//SAMSUNG CHANGE: MPSG100005525 >>
    SkIRect roundedGlobal;
    if(!layer->drawTransform()->isIdentity()) {
        roundedGlobal = layer->drawTransform()->mapRect(layer->unclippedArea());		
    } 
    else {
    globalRect.offset(-layer->getPosition()); // localToGlobal adds in position
    SkMatrix globalMatrix;
    layer->localToGlobal(&globalMatrix);
    globalMatrix.mapRect(&globalRect);
    globalRect.round(&roundedGlobal);   
    }
//SAMSUNG CHANGE <<

    if(mappedRegion.op(visibleLayerRect, SkRegion::kIntersect_Op)) {
        isUnclipped = true;
    } else {
        isUnclipped = false;
    }       
    XLOG("getGlobalRegionForLayer() : global bounds=(%d, %d, r=%d, b=%d) isUnclipped=%d", 
    roundedGlobal.fLeft, roundedGlobal.fTop, roundedGlobal.fRight, roundedGlobal.fBottom, isUnclipped);

    mappedRegion.translate(roundedGlobal.fLeft, roundedGlobal.fTop);

    return true;	
}
//SAMSUNG CHANGE <<

//SAMSUNG CHANGE: MPSG100005525 >>
SkRegion GLExtras::getGlobalVisibleAreaForLayers(LayerAndroid* layer) 
{
    SkRegion visibleGlobalLayerRegion;
    if(!layer) {
        XLOG("%s: layer = NULL", __FUNCTION__);		
        return visibleGlobalLayerRegion;
    }
    XLOG("%s: layer %p [%d] opacity %g needs texture %d children %d", 
        __FUNCTION__, layer, layer->uniqueId(), layer->drawOpacity(), 
        layer->needsTexture(), layer->countChildren());
	
    if(layer->needsTexture() && layer->drawOpacity() >= 0.9) {
        SkIRect clipRect;
        if(!layer->drawTransform()->isIdentity()) {
            XLOG("%s: - use draw transform!!", __FUNCTION__);
            clipRect = layer->drawTransform()->mapRect(layer->unclippedArea());     
        } 
        else {
            SkRect globalRect;
            globalRect.offset(-layer->getPosition()); // localToGlobal adds in position
            SkMatrix globalMatrix;
            layer->localToGlobal(&globalMatrix);
            globalMatrix.mapRect(&globalRect);
            globalRect.round(&clipRect);  
        }
        visibleGlobalLayerRegion.setRect(clipRect);
        XLOG("%s: layer clipRect=(%d, %d, r=%d, b=%d) ", __FUNCTION__,
        clipRect.fLeft, clipRect.fTop, clipRect.fRight, clipRect.fBottom);	   
    }
    int count = layer->countChildren();
    for (int i = 0; i < count; i++) {
        visibleGlobalLayerRegion.op(getGlobalVisibleAreaForLayers(layer->getChild(i)), SkRegion::kUnion_Op);
    }
    return visibleGlobalLayerRegion;	
}
//SAMSUNG CHANGE <<

void GLExtras::drawGL(IntRect& webViewRect, SkRect& viewport, int titleBarHeight)
{
    if (m_drawExtra) {
        if (m_drawExtra == m_ring)
            drawCursorRings();
        else if (m_drawExtra == m_findOnPage)
            drawFindOnPage(viewport);
        else
            XLOGC("m_drawExtra %p is unknown! (cursor: %p, find: %p",
                  m_drawExtra, m_ring, m_findOnPage);
    }
}
