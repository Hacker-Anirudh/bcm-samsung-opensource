/*
 * Copyright (C) 2000 Lars Knoll (knoll@kde.org)
 * Copyright (C) 2003, 2004, 2006, 2007, 2008, 2009, 2010, 2011 Apple Inc. All right reserved.
 * Copyright (C) 2010 Google Inc. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#include "config.h"

#include "BidiResolver.h"
#include "Hyphenation.h"
#include "InlineIterator.h"
#include "InlineTextBox.h"
#include "Logging.h"
#include "RenderArena.h"
#include "RenderCombineText.h"
#include "RenderInline.h"
#include "RenderLayer.h"
#include "RenderListMarker.h"
#include "RenderRubyRun.h"
#include "RenderView.h"
#include "Settings.h"
#include "TextBreakIterator.h"
#include "TextRun.h"
#include "TrailingFloatsRootInlineBox.h"
#include "VerticalPositionCache.h"
#include "break_lines.h"
#include <wtf/AlwaysInline.h>
#include <wtf/RefCountedLeakCounter.h>
#include <wtf/StdLibExtras.h>
#include <wtf/Vector.h>
#include <wtf/unicode/CharacterNames.h>

#if ENABLE(SVG)
#include "RenderSVGInlineText.h"
#include "SVGRootInlineBox.h"
#endif
// SAMSUNG CHANGE >>
#include "WidthIterator.h"
// SAMSUNG CHANGE <<


#ifdef ANDROID_LAYOUT
#include "Frame.h"
#include "FrameTree.h"
#include "Settings.h"
#include "Text.h"
#include "HTMLNames.h"
#endif // ANDROID_LAYOUT

using namespace std;
using namespace WTF;
using namespace Unicode;

namespace WebCore {

// We don't let our line box tree for a single line get any deeper than this.
const unsigned cMaxLineDepth = 200;

static inline int borderPaddingMarginStart(RenderInline* child)
{
    return child->marginStart() + child->paddingStart() + child->borderStart();
}

static inline int borderPaddingMarginEnd(RenderInline* child)
{
    return child->marginEnd() + child->paddingEnd() + child->borderEnd();
}

static int inlineLogicalWidth(RenderObject* child, bool start = true, bool end = true)
{
    unsigned lineDepth = 1;
    int extraWidth = 0;
    RenderObject* parent = child->parent();
    while (parent->isRenderInline() && lineDepth++ < cMaxLineDepth) {
        RenderInline* parentAsRenderInline = toRenderInline(parent);
        if (start && !child->previousSibling())
            extraWidth += borderPaddingMarginStart(parentAsRenderInline);
        if (end && !child->nextSibling())
            extraWidth += borderPaddingMarginEnd(parentAsRenderInline);
        child = parent;
        parent = child->parent();
    }
    return extraWidth;
}

static void checkMidpoints(LineMidpointState& lineMidpointState, InlineIterator& lBreak)
{
    // Check to see if our last midpoint is a start point beyond the line break.  If so,
    // shave it off the list, and shave off a trailing space if the previous end point doesn't
    // preserve whitespace.
    if (lBreak.m_obj && lineMidpointState.numMidpoints && !(lineMidpointState.numMidpoints % 2)) {
        InlineIterator* midpoints = lineMidpointState.midpoints.data();
        InlineIterator& endpoint = midpoints[lineMidpointState.numMidpoints - 2];
        const InlineIterator& startpoint = midpoints[lineMidpointState.numMidpoints - 1];
        InlineIterator currpoint = endpoint;
        while (!currpoint.atEnd() && currpoint != startpoint && currpoint != lBreak)
            currpoint.increment();
        if (currpoint == lBreak) {
            // We hit the line break before the start point.  Shave off the start point.
            lineMidpointState.numMidpoints--;
            if (endpoint.m_obj->style()->collapseWhiteSpace())
                endpoint.m_pos--;
        }
    }    
}

static void addMidpoint(LineMidpointState& lineMidpointState, const InlineIterator& midpoint)
{
    if (lineMidpointState.midpoints.size() <= lineMidpointState.numMidpoints)
        lineMidpointState.midpoints.grow(lineMidpointState.numMidpoints + 10);

    InlineIterator* midpoints = lineMidpointState.midpoints.data();
    midpoints[lineMidpointState.numMidpoints++] = midpoint;
}

static inline BidiRun* createRun(int start, int end, RenderObject* obj, InlineBidiResolver& resolver)
{
    return new (obj->renderArena()) BidiRun(start, end, obj, resolver.context(), resolver.dir());
}

void RenderBlock::appendRunsForObject(BidiRunList<BidiRun>& runs, int start, int end, RenderObject* obj, InlineBidiResolver& resolver)
{
    if (start > end || obj->isFloating() ||
        (obj->isPositioned() && !obj->style()->isOriginalDisplayInlineType() && !obj->container()->isRenderInline()))
        return;

    LineMidpointState& lineMidpointState = resolver.midpointState();
    bool haveNextMidpoint = (lineMidpointState.currentMidpoint < lineMidpointState.numMidpoints);
    InlineIterator nextMidpoint;
    if (haveNextMidpoint)
        nextMidpoint = lineMidpointState.midpoints[lineMidpointState.currentMidpoint];
    if (lineMidpointState.betweenMidpoints) {
        if (!(haveNextMidpoint && nextMidpoint.m_obj == obj))
            return;
        // This is a new start point. Stop ignoring objects and 
        // adjust our start.
        lineMidpointState.betweenMidpoints = false;
        start = nextMidpoint.m_pos;
        lineMidpointState.currentMidpoint++;
        if (start < end)
            return appendRunsForObject(runs, start, end, obj, resolver);
    } else {
        if (!haveNextMidpoint || (obj != nextMidpoint.m_obj)) {
            runs.addRun(createRun(start, end, obj, resolver));
            return;
        }

        // An end midpoint has been encountered within our object.  We
        // need to go ahead and append a run with our endpoint.
        if (static_cast<int>(nextMidpoint.m_pos + 1) <= end) {
            lineMidpointState.betweenMidpoints = true;
            lineMidpointState.currentMidpoint++;
            if (nextMidpoint.m_pos != UINT_MAX) { // UINT_MAX means stop at the object and don't include any of it.
                if (static_cast<int>(nextMidpoint.m_pos + 1) > start)
                    runs.addRun(createRun(start, nextMidpoint.m_pos + 1, obj, resolver));
                return appendRunsForObject(runs, nextMidpoint.m_pos + 1, end, obj, resolver);
            }
        } else
           runs.addRun(createRun(start, end, obj, resolver));
    }
}

static inline InlineBox* createInlineBoxForRenderer(RenderObject* obj, bool isRootLineBox, bool isOnlyRun = false)
{
    if (isRootLineBox)
        return toRenderBlock(obj)->createAndAppendRootInlineBox();
    
    if (obj->isText()) {
        InlineTextBox* textBox = toRenderText(obj)->createInlineTextBox();
        // We only treat a box as text for a <br> if we are on a line by ourself or in strict mode
        // (Note the use of strict mode.  In "almost strict" mode, we don't treat the box for <br> as text.)
        if (obj->isBR())
            textBox->setIsText(isOnlyRun || obj->document()->inNoQuirksMode());
        return textBox;
    }
    
    if (obj->isBox())
        return toRenderBox(obj)->createInlineBox();
    
    return toRenderInline(obj)->createAndAppendInlineFlowBox();
}

static inline void dirtyLineBoxesForRenderer(RenderObject* o, bool fullLayout)
{
    if (o->isText()) {
        if (o->preferredLogicalWidthsDirty() && (o->isCounter() || o->isQuote()))
            toRenderText(o)->computePreferredLogicalWidths(0); // FIXME: Counters depend on this hack. No clue why. Should be investigated and removed.
        toRenderText(o)->dirtyLineBoxes(fullLayout);
    } else
        toRenderInline(o)->dirtyLineBoxes(fullLayout);
}

static bool parentIsConstructedOrHaveNext(InlineFlowBox* parentBox)
{
    do {
        if (parentBox->isConstructed() || parentBox->nextOnLine())
            return true;
        parentBox = parentBox->parent();
    } while (parentBox);
    return false;
}

InlineFlowBox* RenderBlock::createLineBoxes(RenderObject* obj, bool firstLine, InlineBox* childBox)
{
    // See if we have an unconstructed line box for this object that is also
    // the last item on the line.
    unsigned lineDepth = 1;
    InlineFlowBox* parentBox = 0;
    InlineFlowBox* result = 0;
    bool hasDefaultLineBoxContain = style()->lineBoxContain() == RenderStyle::initialLineBoxContain();
    do {
        ASSERT(obj->isRenderInline() || obj == this);
        
        RenderInline* inlineFlow = (obj != this) ? toRenderInline(obj) : 0;

        // Get the last box we made for this render object.
        parentBox = inlineFlow ? inlineFlow->lastLineBox() : toRenderBlock(obj)->lastLineBox();

        // If this box or its ancestor is constructed then it is from a previous line, and we need
        // to make a new box for our line.  If this box or its ancestor is unconstructed but it has
        // something following it on the line, then we know we have to make a new box
        // as well.  In this situation our inline has actually been split in two on
        // the same line (this can happen with very fancy language mixtures).
        bool constructedNewBox = false;
        bool allowedToConstructNewBox = !hasDefaultLineBoxContain || !inlineFlow || inlineFlow->alwaysCreateLineBoxes();
        bool canUseExistingParentBox = parentBox && !parentIsConstructedOrHaveNext(parentBox);
        if (allowedToConstructNewBox && !canUseExistingParentBox) {
            // We need to make a new box for this render object.  Once
            // made, we need to place it at the end of the current line.
            InlineBox* newBox = createInlineBoxForRenderer(obj, obj == this);
            ASSERT(newBox->isInlineFlowBox());
            parentBox = static_cast<InlineFlowBox*>(newBox);
            parentBox->setFirstLineStyleBit(firstLine);
            parentBox->setIsHorizontal(isHorizontalWritingMode());
            if (!hasDefaultLineBoxContain)
                parentBox->clearDescendantsHaveSameLineHeightAndBaseline();
            constructedNewBox = true;
        }

        if (constructedNewBox || canUseExistingParentBox) {
            if (!result)
                result = parentBox;

            // If we have hit the block itself, then |box| represents the root
            // inline box for the line, and it doesn't have to be appended to any parent
            // inline.
            if (childBox)
                parentBox->addToLine(childBox);

            if (!constructedNewBox || obj == this)
                break;

            childBox = parentBox;        
        }

        // If we've exceeded our line depth, then jump straight to the root and skip all the remaining
        // intermediate inline flows.
        obj = (++lineDepth >= cMaxLineDepth) ? this : obj->parent();

    } while (true);

    return result;
}

static bool reachedEndOfTextRenderer(const BidiRunList<BidiRun>& bidiRuns)
{
    BidiRun* run = bidiRuns.logicallyLastRun();
    if (!run)
        return true;
    unsigned int pos = run->stop();
    RenderObject* r = run->m_object;
    if (!r->isText() || r->isBR())
        return false;
    RenderText* renderText = toRenderText(r);
    if (pos >= renderText->textLength())
        return true;

    while (isASCIISpace(renderText->characters()[pos])) {
        pos++;
        if (pos >= renderText->textLength())
            return true;
    }
    return false;
}

RootInlineBox* RenderBlock::constructLine(BidiRunList<BidiRun>& bidiRuns, bool firstLine, bool lastLine)
{
    ASSERT(bidiRuns.firstRun());

    bool rootHasSelectedChildren = false;
    InlineFlowBox* parentBox = 0;
    for (BidiRun* r = bidiRuns.firstRun(); r; r = r->next()) {
        // Create a box for our object.
        bool isOnlyRun = (bidiRuns.runCount() == 1);
        if (bidiRuns.runCount() == 2 && !r->m_object->isListMarker())
            isOnlyRun = (!style()->isLeftToRightDirection() ? bidiRuns.lastRun() : bidiRuns.firstRun())->m_object->isListMarker();

        InlineBox* box = createInlineBoxForRenderer(r->m_object, false, isOnlyRun);
        r->m_box = box;

        ASSERT(box);
        if (!box)
            continue;

        if (!rootHasSelectedChildren && box->renderer()->selectionState() != RenderObject::SelectionNone)
            rootHasSelectedChildren = true;

        // If we have no parent box yet, or if the run is not simply a sibling,
        // then we need to construct inline boxes as necessary to properly enclose the
        // run's inline box.
        if (!parentBox || parentBox->renderer() != r->m_object->parent())
            // Create new inline boxes all the way back to the appropriate insertion point.
            parentBox = createLineBoxes(r->m_object->parent(), firstLine, box);
        else {
            // Append the inline box to this line.
            parentBox->addToLine(box);
        }

        bool visuallyOrdered = r->m_object->style()->visuallyOrdered();
        box->setBidiLevel(r->level());

        if (box->isInlineTextBox()) {
            InlineTextBox* text = static_cast<InlineTextBox*>(box);
            text->setStart(r->m_start);
            text->setLen(r->m_stop - r->m_start);
            text->m_dirOverride = r->dirOverride(visuallyOrdered);
            if (r->m_hasHyphen)
                text->setHasHyphen(true);
        }
    }

    // We should have a root inline box.  It should be unconstructed and
    // be the last continuation of our line list.
    ASSERT(lastLineBox() && !lastLineBox()->isConstructed());

    // Set the m_selectedChildren flag on the root inline box if one of the leaf inline box
    // from the bidi runs walk above has a selection state.
    if (rootHasSelectedChildren)
        lastLineBox()->root()->setHasSelectedChildren(true);

    // Set bits on our inline flow boxes that indicate which sides should
    // paint borders/margins/padding.  This knowledge will ultimately be used when
    // we determine the horizontal positions and widths of all the inline boxes on
    // the line.
    bool isLogicallyLastRunWrapped = bidiRuns.logicallyLastRun()->m_object && bidiRuns.logicallyLastRun()->m_object->isText() ? !reachedEndOfTextRenderer(bidiRuns) : true;
    lastLineBox()->determineSpacingForFlowBoxes(lastLine, isLogicallyLastRunWrapped, bidiRuns.logicallyLastRun()->m_object);

    // Now mark the line boxes as being constructed.
    lastLineBox()->setConstructed();

    // Return the last line.
    return lastRootBox();
}

ETextAlign RenderBlock::textAlignmentForLine(bool endsWithSoftBreak) const
{
    ETextAlign alignment = style()->textAlign();
    if (!endsWithSoftBreak && alignment == JUSTIFY)
        alignment = TAAUTO;

    return alignment;
}

static void updateLogicalWidthForLeftAlignedBlock(bool isLeftToRightDirection, BidiRun* trailingSpaceRun, float& logicalLeft, float& totalLogicalWidth, float availableLogicalWidth)
{
    // The direction of the block should determine what happens with wide lines.
    // In particular with RTL blocks, wide lines should still spill out to the left.
    if (isLeftToRightDirection) {
        if (totalLogicalWidth > availableLogicalWidth && trailingSpaceRun)
            trailingSpaceRun->m_box->setLogicalWidth(max<float>(0, trailingSpaceRun->m_box->logicalWidth() - totalLogicalWidth + availableLogicalWidth));
        return;
    }

    if (trailingSpaceRun)
        trailingSpaceRun->m_box->setLogicalWidth(0);
    else if (totalLogicalWidth > availableLogicalWidth)
        logicalLeft -= (totalLogicalWidth - availableLogicalWidth);
}

static void updateLogicalWidthForRightAlignedBlock(bool isLeftToRightDirection, BidiRun* trailingSpaceRun, float& logicalLeft, float& totalLogicalWidth, float availableLogicalWidth)
{
    // Wide lines spill out of the block based off direction.
    // So even if text-align is right, if direction is LTR, wide lines should overflow out of the right
    // side of the block.
    if (isLeftToRightDirection) {
        if (trailingSpaceRun) {
            totalLogicalWidth -= trailingSpaceRun->m_box->logicalWidth();
            trailingSpaceRun->m_box->setLogicalWidth(0);
        }
        if (totalLogicalWidth < availableLogicalWidth)
            logicalLeft += availableLogicalWidth - totalLogicalWidth;
        return;
    }

    if (totalLogicalWidth > availableLogicalWidth && trailingSpaceRun) {
        trailingSpaceRun->m_box->setLogicalWidth(max<float>(0, trailingSpaceRun->m_box->logicalWidth() - totalLogicalWidth + availableLogicalWidth));
        totalLogicalWidth -= trailingSpaceRun->m_box->logicalWidth();
    } else
        logicalLeft += availableLogicalWidth - totalLogicalWidth;
}

static void updateLogicalWidthForCenterAlignedBlock(bool isLeftToRightDirection, BidiRun* trailingSpaceRun, float& logicalLeft, float& totalLogicalWidth, float availableLogicalWidth)
{
    float trailingSpaceWidth = 0;
    if (trailingSpaceRun) {
        totalLogicalWidth -= trailingSpaceRun->m_box->logicalWidth();
        trailingSpaceWidth = min(trailingSpaceRun->m_box->logicalWidth(), (availableLogicalWidth - totalLogicalWidth + 1) / 2);
        trailingSpaceRun->m_box->setLogicalWidth(max<float>(0, trailingSpaceWidth));
    }
    if (isLeftToRightDirection)
        logicalLeft += max<float>((availableLogicalWidth - totalLogicalWidth) / 2, 0);
    else
        logicalLeft += totalLogicalWidth > availableLogicalWidth ? (availableLogicalWidth - totalLogicalWidth) : (availableLogicalWidth - totalLogicalWidth) / 2 - trailingSpaceWidth;
}

void RenderBlock::computeInlineDirectionPositionsForLine(RootInlineBox* lineBox, bool firstLine, BidiRun* firstRun, BidiRun* trailingSpaceRun, bool reachedEnd,
                                                         GlyphOverflowAndFallbackFontsMap& textBoxDataMap, VerticalPositionCache& verticalPositionCache)
{
    ETextAlign textAlign = textAlignmentForLine(!reachedEnd && !lineBox->endsWithBreak());
    float logicalLeft = logicalLeftOffsetForLine(logicalHeight(), firstLine);
    float availableLogicalWidth = logicalRightOffsetForLine(logicalHeight(), firstLine) - logicalLeft;

    bool needsWordSpacing = false;
    float totalLogicalWidth = lineBox->getFlowSpacingLogicalWidth();
    unsigned expansionOpportunityCount = 0;
    bool isAfterExpansion = true;
    Vector<unsigned, 16> expansionOpportunities;
    RenderObject* previousObject = 0;

    for (BidiRun* r = firstRun; r; r = r->next()) {
        if (!r->m_box || r->m_object->isPositioned() || r->m_box->isLineBreak())
            continue; // Positioned objects are only participating to figure out their
                      // correct static x position.  They have no effect on the width.
                      // Similarly, line break boxes have no effect on the width.
        if (r->m_object->isText()) {
            RenderText* rt = toRenderText(r->m_object);

            if (textAlign == JUSTIFY && r != trailingSpaceRun) {
                if (!isAfterExpansion)
                    static_cast<InlineTextBox*>(r->m_box)->setCanHaveLeadingExpansion(true);
                unsigned opportunitiesInRun = Font::expansionOpportunityCount(rt->characters() + r->m_start, r->m_stop - r->m_start, r->m_box->direction(), isAfterExpansion);
                expansionOpportunities.append(opportunitiesInRun);
                expansionOpportunityCount += opportunitiesInRun;
            }

            if (int length = rt->textLength()) {
                if (!r->m_start && needsWordSpacing && isSpaceOrNewline(rt->characters()[r->m_start]))
                    totalLogicalWidth += rt->style(firstLine)->font().wordSpacing();
                needsWordSpacing = !isSpaceOrNewline(rt->characters()[r->m_stop - 1]) && r->m_stop == length;          
            }
            HashSet<const SimpleFontData*> fallbackFonts;
            GlyphOverflow glyphOverflow;
            
            // Always compute glyph overflow if the block's line-box-contain value is "glyphs".
            if (lineBox->fitsToGlyphs()) {
                // If we don't stick out of the root line's font box, then don't bother computing our glyph overflow. This optimization
                // will keep us from computing glyph bounds in nearly all cases.
                bool includeRootLine = lineBox->includesRootLineBoxFontOrLeading();
                int baselineShift = lineBox->verticalPositionForBox(r->m_box, verticalPositionCache);
                int rootDescent = includeRootLine ? lineBox->renderer()->style(firstLine)->font().fontMetrics().descent() : 0;
                int rootAscent = includeRootLine ? lineBox->renderer()->style(firstLine)->font().fontMetrics().ascent() : 0;
                int boxAscent = rt->style(firstLine)->font().fontMetrics().ascent() - baselineShift;
                int boxDescent = rt->style(firstLine)->font().fontMetrics().descent() + baselineShift;
                if (boxAscent > rootDescent ||  boxDescent > rootAscent)
                    glyphOverflow.computeBounds = true; 
            }

            int hyphenWidth = 0;
            if (static_cast<InlineTextBox*>(r->m_box)->hasHyphen()) {
                const AtomicString& hyphenString = rt->style()->hyphenString();
                hyphenWidth = rt->style(firstLine)->font().width(TextRun(hyphenString.characters(), hyphenString.length()));
            }
            r->m_box->setLogicalWidth(rt->width(r->m_start, r->m_stop - r->m_start, totalLogicalWidth, firstLine, &fallbackFonts, &glyphOverflow) + hyphenWidth);
            if (!fallbackFonts.isEmpty()) {
                ASSERT(r->m_box->isText());
                GlyphOverflowAndFallbackFontsMap::iterator it = textBoxDataMap.add(static_cast<InlineTextBox*>(r->m_box), make_pair(Vector<const SimpleFontData*>(), GlyphOverflow())).first;
                ASSERT(it->second.first.isEmpty());
                copyToVector(fallbackFonts, it->second.first);
                r->m_box->parent()->clearDescendantsHaveSameLineHeightAndBaseline();
            }
            if ((glyphOverflow.top || glyphOverflow.bottom || glyphOverflow.left || glyphOverflow.right)) {
                ASSERT(r->m_box->isText());
                GlyphOverflowAndFallbackFontsMap::iterator it = textBoxDataMap.add(static_cast<InlineTextBox*>(r->m_box), make_pair(Vector<const SimpleFontData*>(), GlyphOverflow())).first;
                it->second.second = glyphOverflow;
                r->m_box->clearKnownToHaveNoOverflow();
            }
        } else {
            isAfterExpansion = false;
            if (!r->m_object->isRenderInline()) {
                RenderBox* renderBox = toRenderBox(r->m_object);
                if (renderBox->isRubyRun()) {
                    int startOverhang;
                    int endOverhang;
                    RenderObject* nextObject = 0;
                    for (BidiRun* runWithNextObject = r->next(); runWithNextObject; runWithNextObject = runWithNextObject->next()) {
                        if (!runWithNextObject->m_object->isPositioned() && !runWithNextObject->m_box->isLineBreak()) {
                            nextObject = runWithNextObject->m_object;
                            break;
                        }
                    }
                    toRenderRubyRun(renderBox)->getOverhang(firstLine, renderBox->style()->isLeftToRightDirection() ? previousObject : nextObject, renderBox->style()->isLeftToRightDirection() ? nextObject : previousObject, startOverhang, endOverhang);
                    setMarginStartForChild(renderBox, -startOverhang);
                    setMarginEndForChild(renderBox, -endOverhang);
                }
                r->m_box->setLogicalWidth(logicalWidthForChild(renderBox));
                totalLogicalWidth += marginStartForChild(renderBox) + marginEndForChild(renderBox);
            }
        }

        totalLogicalWidth += r->m_box->logicalWidth();
        previousObject = r->m_object;
    }

    if (isAfterExpansion && !expansionOpportunities.isEmpty()) {
        expansionOpportunities.last()--;
        expansionOpportunityCount--;
    }

    // Armed with the total width of the line (without justification),
    // we now examine our text-align property in order to determine where to position the
    // objects horizontally.  The total width of the line can be increased if we end up
    // justifying text.
    switch (textAlign) {
        case LEFT:
        case WEBKIT_LEFT:
            updateLogicalWidthForLeftAlignedBlock(style()->isLeftToRightDirection(), trailingSpaceRun, logicalLeft, totalLogicalWidth, availableLogicalWidth);
            break;
        case JUSTIFY:
            adjustInlineDirectionLineBounds(expansionOpportunityCount, logicalLeft, availableLogicalWidth);
            if (expansionOpportunityCount) {
                if (trailingSpaceRun) {
                    totalLogicalWidth -= trailingSpaceRun->m_box->logicalWidth();
                    trailingSpaceRun->m_box->setLogicalWidth(0);
                }
                break;
            }
            // fall through
        case TAAUTO:
            // for right to left fall through to right aligned
            if (style()->isLeftToRightDirection()) {
                if (totalLogicalWidth > availableLogicalWidth && trailingSpaceRun)
                    trailingSpaceRun->m_box->setLogicalWidth(max<float>(0, trailingSpaceRun->m_box->logicalWidth() - totalLogicalWidth + availableLogicalWidth));
                break;
            }
        case RIGHT:
        case WEBKIT_RIGHT:
            updateLogicalWidthForRightAlignedBlock(style()->isLeftToRightDirection(), trailingSpaceRun, logicalLeft, totalLogicalWidth, availableLogicalWidth);
            break;
        case CENTER:
        case WEBKIT_CENTER:
            updateLogicalWidthForCenterAlignedBlock(style()->isLeftToRightDirection(), trailingSpaceRun, logicalLeft, totalLogicalWidth, availableLogicalWidth);
            break;
        case TASTART:
            if (style()->isLeftToRightDirection())
                updateLogicalWidthForLeftAlignedBlock(style()->isLeftToRightDirection(), trailingSpaceRun, logicalLeft, totalLogicalWidth, availableLogicalWidth);
            else
                updateLogicalWidthForRightAlignedBlock(style()->isLeftToRightDirection(), trailingSpaceRun, logicalLeft, totalLogicalWidth, availableLogicalWidth);
            break;
        case TAEND:
            if (style()->isLeftToRightDirection())
                updateLogicalWidthForRightAlignedBlock(style()->isLeftToRightDirection(), trailingSpaceRun, logicalLeft, totalLogicalWidth, availableLogicalWidth);
            else
                updateLogicalWidthForLeftAlignedBlock(style()->isLeftToRightDirection(), trailingSpaceRun, logicalLeft, totalLogicalWidth, availableLogicalWidth);
            break;
    }

    if (expansionOpportunityCount && availableLogicalWidth > totalLogicalWidth) {
        size_t i = 0;
        for (BidiRun* r = firstRun; r; r = r->next()) {
            if (!r->m_box || r == trailingSpaceRun)
                continue;

            if (r->m_object->isText()) {
                unsigned opportunitiesInRun = expansionOpportunities[i++];

                ASSERT(opportunitiesInRun <= expansionOpportunityCount);

                // Only justify text if whitespace is collapsed.
                if (r->m_object->style()->collapseWhiteSpace()) {
                    InlineTextBox* textBox = static_cast<InlineTextBox*>(r->m_box);
                    float expansion = (availableLogicalWidth - totalLogicalWidth) * opportunitiesInRun / expansionOpportunityCount;
                    textBox->setExpansion(expansion);
                    totalLogicalWidth += expansion;
                }
                expansionOpportunityCount -= opportunitiesInRun;
                if (!expansionOpportunityCount)
                    break;
            }
        }
    }

    // The widths of all runs are now known.  We can now place every inline box (and
    // compute accurate widths for the inline flow boxes).
    needsWordSpacing = false;
    lineBox->placeBoxesInInlineDirection(logicalLeft, needsWordSpacing, textBoxDataMap);
}

void RenderBlock::computeBlockDirectionPositionsForLine(RootInlineBox* lineBox, BidiRun* firstRun, GlyphOverflowAndFallbackFontsMap& textBoxDataMap,
                                                        VerticalPositionCache& verticalPositionCache)
{
    setLogicalHeight(lineBox->alignBoxesInBlockDirection(logicalHeight(), textBoxDataMap, verticalPositionCache));
    lineBox->setBlockLogicalHeight(logicalHeight());

    // Now make sure we place replaced render objects correctly.
    for (BidiRun* r = firstRun; r; r = r->next()) {
        ASSERT(r->m_box);
        if (!r->m_box)
            continue; // Skip runs with no line boxes.

        // Align positioned boxes with the top of the line box.  This is
        // a reasonable approximation of an appropriate y position.
        if (r->m_object->isPositioned())
            r->m_box->setLogicalTop(logicalHeight());

        // Position is used to properly position both replaced elements and
        // to update the static normal flow x/y of positioned elements.
        if (r->m_object->isText())
            toRenderText(r->m_object)->positionLineBox(r->m_box);
        else if (r->m_object->isBox())
            toRenderBox(r->m_object)->positionLineBox(r->m_box);
    }
    // Positioned objects and zero-length text nodes destroy their boxes in
    // position(), which unnecessarily dirties the line.
    lineBox->markDirty(false);
}

static inline bool isCollapsibleSpace(UChar character, RenderText* renderer)
{
    if (character == ' ' || character == '\t' || character == softHyphen)
        return true;
    if (character == '\n')
        return !renderer->style()->preserveNewline();
    if (character == noBreakSpace)
        return renderer->style()->nbspMode() == SPACE;
    return false;
}


static void setStaticPositions(RenderBlock* block, RenderBox* child)
{
    // FIXME: The math here is actually not really right. It's a best-guess approximation that
    // will work for the common cases
    RenderObject* containerBlock = child->container();
    int blockHeight = block->logicalHeight();
    if (containerBlock->isRenderInline()) {
        // A relative positioned inline encloses us. In this case, we also have to determine our
        // position as though we were an inline. Set |staticInlinePosition| and |staticBlockPosition| on the relative positioned
        // inline so that we can obtain the value later.
        toRenderInline(containerBlock)->layer()->setStaticInlinePosition(block->startOffsetForLine(blockHeight, false));
        toRenderInline(containerBlock)->layer()->setStaticBlockPosition(blockHeight);
    }

    if (child->style()->isOriginalDisplayInlineType())
        child->layer()->setStaticInlinePosition(block->startOffsetForLine(blockHeight, false));
    else
        child->layer()->setStaticInlinePosition(block->borderAndPaddingStart());
    child->layer()->setStaticBlockPosition(blockHeight);
}

inline BidiRun* RenderBlock::handleTrailingSpaces(BidiRunList<BidiRun>& bidiRuns, BidiContext* currentContext)
{
    if (!bidiRuns.runCount()
        || !bidiRuns.logicallyLastRun()->m_object->style()->breakOnlyAfterWhiteSpace()
        || !bidiRuns.logicallyLastRun()->m_object->style()->autoWrap())
        return 0;

    BidiRun* trailingSpaceRun = bidiRuns.logicallyLastRun();
    RenderObject* lastObject = trailingSpaceRun->m_object;
    if (!lastObject->isText())
        return 0;

    RenderText* lastText = toRenderText(lastObject);
    const UChar* characters = lastText->characters();
    int firstSpace = trailingSpaceRun->stop();
    while (firstSpace > trailingSpaceRun->start()) {
        UChar current = characters[firstSpace - 1];
        if (!isCollapsibleSpace(current, lastText))
            break;
        firstSpace--;
    }
    if (firstSpace == trailingSpaceRun->stop())
        return 0;

    TextDirection direction = style()->direction();
    bool shouldReorder = trailingSpaceRun != (direction == LTR ? bidiRuns.lastRun() : bidiRuns.firstRun());
    if (firstSpace != trailingSpaceRun->start()) {
        BidiContext* baseContext = currentContext;
        while (BidiContext* parent = baseContext->parent())
            baseContext = parent;

        BidiRun* newTrailingRun = new (renderArena()) BidiRun(firstSpace, trailingSpaceRun->m_stop, trailingSpaceRun->m_object, baseContext, OtherNeutral);
        trailingSpaceRun->m_stop = firstSpace;
        if (direction == LTR)
            bidiRuns.addRun(newTrailingRun);
        else
            bidiRuns.prependRun(newTrailingRun);
        trailingSpaceRun = newTrailingRun;
        return trailingSpaceRun;
    }
    if (!shouldReorder)
        return trailingSpaceRun;

    if (direction == LTR) {
        bidiRuns.moveRunToEnd(trailingSpaceRun);
        trailingSpaceRun->m_level = 0;
    } else {
        bidiRuns.moveRunToBeginning(trailingSpaceRun);
        trailingSpaceRun->m_level = 1;
    }
    return trailingSpaceRun;
}

void RenderBlock::appendFloatingObjectToLastLine(FloatingObject* floatingObject)
{
    ASSERT(!floatingObject->m_originatingLine);
    floatingObject->m_originatingLine = lastRootBox();
    lastRootBox()->appendFloat(floatingObject->renderer());
}

void RenderBlock::layoutInlineChildren(bool relayoutChildren, int& repaintLogicalTop, int& repaintLogicalBottom)
{
    bool useRepaintBounds = false;
    
    m_overflow.clear();
        
    setLogicalHeight(borderBefore() + paddingBefore());

    // Figure out if we should clear out our line boxes.
    // FIXME: Handle resize eventually!
    bool fullLayout = !firstLineBox() || selfNeedsLayout() || relayoutChildren;
    if (fullLayout)
        lineBoxes()->deleteLineBoxes(renderArena());

    // Text truncation only kicks in if your overflow isn't visible and your text-overflow-mode isn't
    // clip.
    // FIXME: CSS3 says that descendants that are clipped must also know how to truncate.  This is insanely
    // difficult to figure out (especially in the middle of doing layout), and is really an esoteric pile of nonsense
    // anyway, so we won't worry about following the draft here.
    bool hasTextOverflow = style()->textOverflow() && hasOverflowClip();
    bool firstRTL = false; //STRI direction of first text node
    // Walk all the lines and delete our ellipsis line boxes if they exist.
    if (hasTextOverflow)
         deleteEllipsisLineBoxes();

    if (firstChild()) {
		 //SISO: for RTL text alignment - start
		 RenderObject* o = bidiFirst(this, 0, false);
		//SISO: for RTL text alignment - end		
#ifdef ANDROID_LAYOUT
        // if we are in fitColumnToScreen mode
        // and the current object is not float:right in LTR or not float:left in RTL,
        // and text align is auto, or justify or left in LTR, or right in RTL, we
        // will wrap text around screen width so that it doesn't need to scroll
        // horizontally when reading a paragraph.
        // In case the line height is less than the font size, we skip
        // the text wrapping since this will cause text overlapping.
        // If a text has background image, we ignore text wrapping,
        // otherwise the background will be potentially messed up.
        const Settings* settings = document()->settings();
        bool doTextWrap = settings && settings->layoutAlgorithm() == Settings::kLayoutFitColumnToScreen;
        if (doTextWrap) {
            int ta = style()->textAlign();
            int dir = style()->direction();

			//SISO_MEA_JP - start
			//BIDI in HTML composer
			if(!settings->editableSupportEnabled()) {
				if(dir != RTL){
					bool isRTL = false;
					if(o->isText() || toRenderText(o)->isText()){
						isRTL = toRenderText(o)->checkIsRTL();			
					}
					style()->setDirection(isRTL? RTL: LTR); 	
					dir = style()->direction();
				}	
			}
			//SISO_MEA_JP - end
			
            bool autowrap = style()->autoWrap();
            // if the RenderBlock is positioned, don't wrap text around screen
            // width as it may cause text to overlap.
            bool positioned = isPositioned();
            EFloat cssfloat = style()->floating();
            const int lineHeight = style()->computedLineHeight();
            const int fontSize = style()->fontSize();
            doTextWrap = autowrap && !positioned &&
                    (fontSize <= lineHeight) && !style()->hasBackground() &&
                    (((dir == LTR && cssfloat != FRIGHT) ||
                    (dir == RTL && cssfloat != FNONE)) &&
                    ((ta == TAAUTO) || (ta == JUSTIFY) ||
                    ((ta == LEFT || ta == WEBKIT_LEFT) && (dir == LTR)) ||
                    ((ta == RIGHT || ta == WEBKIT_RIGHT) && (dir == RTL))));
        }
        bool hasTextToWrap = false;
#endif
        // layout replaced elements
        bool endOfInline = false;
		//	 RenderObject* o = bidiFirst(this, 0, false); //SISO: commented for RTL alignment; moved this statement to top.
        Vector<FloatWithRect> floats;
        bool hasInlineChild = false;
		bool firstText = false; //STRI first text
        while (o) {
            if (!hasInlineChild && o->isInline())
                hasInlineChild = true;
			if(document() && document()->page() && document()->page()->settings() 
			&& document()->page()->settings()->editableSupportEnabled()) {
				if(!firstText && (o->isText()/*|| toRenderText(o)->isText()*/)) { 
				//STRI check direction only for first text node. (for set leading direction)
					firstRTL = toRenderText(o)->checkIsRTL();			
					firstText = true;
				}
				style()->setDirection(firstRTL? RTL: LTR); //STRI apply leading direction for all children
			}
            if (o->isReplaced() || o->isFloating() || o->isPositioned()) {
                RenderBox* box = toRenderBox(o);
                
                if (relayoutChildren || o->style()->width().isPercent() || o->style()->height().isPercent())
                    o->setChildNeedsLayout(true, false);
                    
                // If relayoutChildren is set and we have percentage padding, we also need to invalidate the child's pref widths.
                if (relayoutChildren && (o->style()->paddingStart().isPercent() || o->style()->paddingEnd().isPercent()))
                    o->setPreferredLogicalWidthsDirty(true, false);
            
                if (o->isPositioned())
                    o->containingBlock()->insertPositionedObject(box);
#if PLATFORM(ANDROID)
                else {
#ifdef ANDROID_LAYOUT
                    // ignore text wrap for textField or menuList
                    if (doTextWrap && (o->isTextField() || o->isMenuList()))
                        doTextWrap = false;
#endif
                    if (o->isFloating())
                        floats.append(FloatWithRect(box));
                    else if (fullLayout || o->needsLayout()) {
                        // Replaced elements
                        toRenderBox(o)->dirtyLineBoxes(fullLayout);
                        o->layoutIfNeeded();
                    }
                }
#else
                else if (o->isFloating())
                    floats.append(FloatWithRect(box));
                else if (fullLayout || o->needsLayout()) {
                    // Replaced elements
                    toRenderBox(o)->dirtyLineBoxes(fullLayout);
                    o->layoutIfNeeded();
                }
#endif // PLATFORM(ANDROID)
            } else if (o->isText() || (o->isRenderInline() && !endOfInline)) {
                if (!o->isText())
                    toRenderInline(o)->updateAlwaysCreateLineBoxes();
                if (fullLayout || o->selfNeedsLayout())
                    dirtyLineBoxesForRenderer(o, fullLayout);
                o->setNeedsLayout(false);
#ifdef ANDROID_LAYOUT
                if (doTextWrap && !hasTextToWrap && o->isText()) {
                    Node* node = o->node();
                    // as it is very common for sites to use a serial of <a> or
                    // <li> as tabs, we don't force text to wrap if all the text
                    // are short and within an <a> or <li> tag, and only separated
                    // by short word like "|" or ";".
                    if (node && node->isTextNode() &&
                            !static_cast<Text*>(node)->containsOnlyWhitespace()) {
                        int length = static_cast<Text*>(node)->length();
                        // FIXME, need a magic number to decide it is too long to
                        // be a tab. Pick 25 for now as it covers around 160px
                        // (half of 320px) with the default font.
                        if (length > 25 || (length > 3 &&
                                (!node->parentOrHostNode()->hasTagName(HTMLNames::aTag) &&
                                !node->parentOrHostNode()->hasTagName(HTMLNames::liTag))))
                            hasTextToWrap = true;
                    }
                }
#endif
            }
            o = bidiNext(this, o, 0, false, &endOfInline);
        }

#ifdef ANDROID_LAYOUT
        // try to make sure that inline text will not span wider than the
        // screen size unless the container has a fixed height,
        if (doTextWrap && hasTextToWrap) {
            // check all the nested containing blocks, unless it is table or
            // table-cell, to make sure there is no fixed height as it implies
            // fixed layout. If we constrain the text to fit screen, we may
            // cause text overlap with the block after.
            bool isConstrained = false;
            RenderObject* obj = this;
            while (obj) {
                if (obj->style()->height().isFixed() && (!obj->isTable() && !obj->isTableCell())) {
                    isConstrained = true;
                    break;
                }
                if (obj->isFloating() || obj->isPositioned()) {
                    // floating and absolute or fixed positioning are done out
                    // of normal flow. Don't need to worry about height any more.
                    break;
                }
                obj = obj->container();
            }
            if (!isConstrained) {
                int textWrapWidth = view()->frameView()->textWrapWidth();
                int padding = paddingLeft() + paddingRight();
                if (textWrapWidth > 0 && width() > (textWrapWidth + padding)) {
                    // limit the content width (width excluding padding) to be
                    // (textWrapWidth - 2 * ANDROID_FCTS_MARGIN_PADDING)
                    int maxWidth = textWrapWidth - 2 * ANDROID_FCTS_MARGIN_PADDING + padding;
                    setWidth(min(width(), maxWidth));
                    m_minPreferredLogicalWidth = min(m_minPreferredLogicalWidth, maxWidth);
                    m_maxPreferredLogicalWidth = min(m_maxPreferredLogicalWidth, maxWidth);

                    IntRect overflow = layoutOverflowRect();
                    if (overflow.width() > maxWidth) {
                        overflow.setWidth(maxWidth);
                        clearLayoutOverflow();
                        addLayoutOverflow(overflow);
                    }
                }
            }
        }
#endif
        // We want to skip ahead to the first dirty line
        InlineBidiResolver resolver;
        unsigned floatIndex;
        bool firstLine = true;
        bool previousLineBrokeCleanly = true;
        RootInlineBox* startLine = determineStartPosition(firstLine, fullLayout, previousLineBrokeCleanly, resolver, floats, floatIndex,
                                                          useRepaintBounds, repaintLogicalTop, repaintLogicalBottom);

        if (fullLayout && hasInlineChild && !selfNeedsLayout()) {
            setNeedsLayout(true, false);  // Mark ourselves as needing a full layout. This way we'll repaint like
                                          // we're supposed to.
            RenderView* v = view();
            if (v && !v->doingFullRepaint() && hasLayer()) {
                // Because we waited until we were already inside layout to discover
                // that the block really needed a full layout, we missed our chance to repaint the layer
                // before layout started.  Luckily the layer has cached the repaint rect for its original
                // position and size, and so we can use that to make a repaint happen now.
                repaintUsingContainer(containerForRepaint(), layer()->repaintRect());
            }
        }

        FloatingObject* lastFloat = (m_floatingObjects && !m_floatingObjects->set().isEmpty()) ? m_floatingObjects->set().last() : 0;

        LineMidpointState& lineMidpointState = resolver.midpointState();

        // We also find the first clean line and extract these lines.  We will add them back
        // if we determine that we're able to synchronize after handling all our dirty lines.
        InlineIterator cleanLineStart;
        BidiStatus cleanLineBidiStatus;
        int endLineLogicalTop = 0;
        RootInlineBox* endLine = (fullLayout || !startLine) ? 
                                 0 : determineEndPosition(startLine, floats, floatIndex, cleanLineStart, cleanLineBidiStatus, endLineLogicalTop);

        if (startLine) {
            if (!useRepaintBounds) {
                useRepaintBounds = true;
                repaintLogicalTop = logicalHeight();
                repaintLogicalBottom = logicalHeight();
            }
            RenderArena* arena = renderArena();
            RootInlineBox* box = startLine;
            while (box) {
                repaintLogicalTop = min(repaintLogicalTop, box->logicalTopVisualOverflow());
                repaintLogicalBottom = max(repaintLogicalBottom, box->logicalBottomVisualOverflow());
                RootInlineBox* next = box->nextRootBox();
                box->deleteLine(arena);
                box = next;
            }
        }

        InlineIterator end = resolver.position();

        if (!fullLayout && lastRootBox() && lastRootBox()->endsWithBreak()) {
            // If the last line before the start line ends with a line break that clear floats,
            // adjust the height accordingly.
            // A line break can be either the first or the last object on a line, depending on its direction.
            if (InlineBox* lastLeafChild = lastRootBox()->lastLeafChild()) {
                RenderObject* lastObject = lastLeafChild->renderer();
                if (!lastObject->isBR())
                    lastObject = lastRootBox()->firstLeafChild()->renderer();
                if (lastObject->isBR()) {
                    EClear clear = lastObject->style()->clear();
                    if (clear != CNONE)
                        newLine(clear);
                }
            }
        }

        bool endLineMatched = false;
        bool checkForEndLineMatch = endLine;
        bool checkForFloatsFromLastLine = false;

        bool isLineEmpty = true;
        bool paginated = view()->layoutState() && view()->layoutState()->isPaginated();

        LineBreakIteratorInfo lineBreakIteratorInfo;
        VerticalPositionCache verticalPositionCache;

        while (!end.atEnd()) {
            // FIXME: Is this check necessary before the first iteration or can it be moved to the end?
            if (checkForEndLineMatch && (endLineMatched = matchedEndLine(resolver, cleanLineStart, cleanLineBidiStatus, endLine, endLineLogicalTop, repaintLogicalBottom, repaintLogicalTop)))
                break;

            lineMidpointState.reset();
            
            isLineEmpty = true;
            
            EClear clear = CNONE;
            bool hyphenated;
            Vector<RenderBox*> positionedObjects;
            
            InlineIterator oldEnd = end;
            FloatingObject* lastFloatFromPreviousLine = (m_floatingObjects && !m_floatingObjects->set().isEmpty()) ? m_floatingObjects->set().last() : 0;
            end = findNextLineBreak(resolver, firstLine, isLineEmpty, lineBreakIteratorInfo, previousLineBrokeCleanly, hyphenated, &clear, lastFloatFromPreviousLine, positionedObjects);
            if (resolver.position().atEnd()) {
                // FIXME: We shouldn't be creating any runs in findNextLineBreak to begin with!
                // Once BidiRunList is separated from BidiResolver this will not be needed.
                resolver.runs().deleteRuns();
                resolver.markCurrentRunEmpty(); // FIXME: This can probably be replaced by an ASSERT (or just removed).
                checkForFloatsFromLastLine = true;
                break;
            }
            ASSERT(end != resolver.position());

            if (isLineEmpty) {
                if (lastRootBox())
                    lastRootBox()->setLineBreakInfo(end.m_obj, end.m_pos, resolver.status());
            } else {
                VisualDirectionOverride override = (style()->visuallyOrdered() ? (style()->direction() == LTR ? VisualLeftToRightOverride : VisualRightToLeftOverride) : NoVisualOverride);
                // FIXME: This ownership is reversed. We should own the BidiRunList and pass it to createBidiRunsForLine.
                BidiRunList<BidiRun>& bidiRuns = resolver.runs();
                resolver.createBidiRunsForLine(end, override, previousLineBrokeCleanly);
                ASSERT(resolver.position() == end);

                BidiRun* trailingSpaceRun = !previousLineBrokeCleanly ? handleTrailingSpaces(bidiRuns, resolver.context()) : 0;

                // Now that the runs have been ordered, we create the line boxes.
                // At the same time we figure out where border/padding/margin should be applied for
                // inline flow boxes.

                RootInlineBox* lineBox = 0;
                int oldLogicalHeight = logicalHeight();
                if (bidiRuns.runCount()) {
                    if (hyphenated)
                        bidiRuns.logicallyLastRun()->m_hasHyphen = true;
                    lineBox = constructLine(bidiRuns, firstLine, !end.m_obj);
                    if (lineBox) {
                        lineBox->setEndsWithBreak(previousLineBrokeCleanly);

#if ENABLE(SVG)
                        bool isSVGRootInlineBox = lineBox->isSVGRootInlineBox();
#else
                        bool isSVGRootInlineBox = false;
#endif

                        GlyphOverflowAndFallbackFontsMap textBoxDataMap;
                    
                        // Now we position all of our text runs horizontally.
                        if (!isSVGRootInlineBox)
                            computeInlineDirectionPositionsForLine(lineBox, firstLine, bidiRuns.firstRun(), trailingSpaceRun, end.atEnd(), textBoxDataMap, verticalPositionCache);

                        // Now position our text runs vertically.
                        computeBlockDirectionPositionsForLine(lineBox, bidiRuns.firstRun(), textBoxDataMap, verticalPositionCache);

#if ENABLE(SVG)
                        // SVG text layout code computes vertical & horizontal positions on its own.
                        // Note that we still need to execute computeVerticalPositionsForLine() as
                        // it calls InlineTextBox::positionLineBox(), which tracks whether the box
                        // contains reversed text or not. If we wouldn't do that editing and thus
                        // text selection in RTL boxes would not work as expected.
                        if (isSVGRootInlineBox) {
                            ASSERT(isSVGText());
                            static_cast<SVGRootInlineBox*>(lineBox)->computePerCharacterLayoutInformation();
                        }
#endif

                        // Compute our overflow now.
                        lineBox->computeOverflow(lineBox->lineTop(), lineBox->lineBottom(), textBoxDataMap);
    
#if PLATFORM(MAC)
                        // Highlight acts as an overflow inflation.
                        if (style()->highlight() != nullAtom)
                            lineBox->addHighlightOverflow();
#endif
                    }
                }

                bidiRuns.deleteRuns();
                resolver.markCurrentRunEmpty(); // FIXME: This can probably be replaced by an ASSERT (or just removed).

                if (lineBox) {
                    lineBox->setLineBreakInfo(end.m_obj, end.m_pos, resolver.status());
                    if (useRepaintBounds) {
                        repaintLogicalTop = min(repaintLogicalTop, lineBox->logicalTopVisualOverflow());
                        repaintLogicalBottom = max(repaintLogicalBottom, lineBox->logicalBottomVisualOverflow());
                    }
                    
                    if (paginated) {
                        int adjustment = 0;
                        adjustLinePositionForPagination(lineBox, adjustment);
                        if (adjustment) {
                            int oldLineWidth = availableLogicalWidthForLine(oldLogicalHeight, firstLine);
                            lineBox->adjustBlockDirectionPosition(adjustment);
                            if (useRepaintBounds) // This can only be a positive adjustment, so no need to update repaintTop.
                                repaintLogicalBottom = max(repaintLogicalBottom, lineBox->logicalBottomVisualOverflow());
                                
                            if (availableLogicalWidthForLine(oldLogicalHeight + adjustment, firstLine) != oldLineWidth) {
                                // We have to delete this line, remove all floats that got added, and let line layout re-run.
                                lineBox->deleteLine(renderArena());
                                removeFloatingObjectsBelow(lastFloatFromPreviousLine, oldLogicalHeight);
                                setLogicalHeight(oldLogicalHeight + adjustment);
                                resolver.setPosition(oldEnd);
                                end = oldEnd;
                                continue;
                            }

                            setLogicalHeight(lineBox->blockLogicalHeight());
                        }
                    }
                }

                for (size_t i = 0; i < positionedObjects.size(); ++i)
                    setStaticPositions(this, positionedObjects[i]);

                firstLine = false;
                newLine(clear);
            }

            if (m_floatingObjects && lastRootBox()) {
                FloatingObjectSet& floatingObjectSet = m_floatingObjects->set();
                FloatingObjectSetIterator it = floatingObjectSet.begin();
                FloatingObjectSetIterator end = floatingObjectSet.end();
                if (lastFloat) {
                    FloatingObjectSetIterator lastFloatIterator = floatingObjectSet.find(lastFloat);
                    ASSERT(lastFloatIterator != end);
                    ++lastFloatIterator;
                    it = lastFloatIterator;
                }
                for (; it != end; ++it) {
                    FloatingObject* f = *it;
                    appendFloatingObjectToLastLine(f);
                    ASSERT(f->m_renderer == floats[floatIndex].object);
                    // If a float's geometry has changed, give up on syncing with clean lines.
                    if (floats[floatIndex].rect != f->frameRect())
                        checkForEndLineMatch = false;
                    floatIndex++;
                }
                lastFloat = !floatingObjectSet.isEmpty() ? floatingObjectSet.last() : 0;
            }

            lineMidpointState.reset();
            resolver.setPosition(end);

            // Exit loop if we have already filled the container
            // when overflow mode is 'hidden'
            if (style()->height().value()) {
                EOverflow ov = style()->overflowY();
                if (ov == OHIDDEN && logicalHeight() > style()->height().value())
                    break;
            }
        }

        if (endLine) {
            if (endLineMatched) {
                // Attach all the remaining lines, and then adjust their y-positions as needed.
                int delta = logicalHeight() - endLineLogicalTop;
                for (RootInlineBox* line = endLine; line; line = line->nextRootBox()) {
                    line->attachLine();
                    if (paginated) {
                        delta -= line->paginationStrut();
                        adjustLinePositionForPagination(line, delta);
                    }
                    if (delta) {
                        repaintLogicalTop = min(repaintLogicalTop, line->logicalTopVisualOverflow() + min(delta, 0));
                        repaintLogicalBottom = max(repaintLogicalBottom, line->logicalBottomVisualOverflow() + max(delta, 0));
                        line->adjustBlockDirectionPosition(delta);
                    }
                    if (Vector<RenderBox*>* cleanLineFloats = line->floatsPtr()) {
                        Vector<RenderBox*>::iterator end = cleanLineFloats->end();
                        for (Vector<RenderBox*>::iterator f = cleanLineFloats->begin(); f != end; ++f) {
                            FloatingObject* floatingObject = insertFloatingObject(*f);
                            ASSERT(!floatingObject->m_originatingLine);
                            floatingObject->m_originatingLine = line;
                            setLogicalHeight(logicalTopForChild(*f) - marginBeforeForChild(*f) + delta);
                            positionNewFloats();
                        }
                    }
                }
                setLogicalHeight(lastRootBox()->blockLogicalHeight());
            } else {
                // Delete all the remaining lines.
                RootInlineBox* line = endLine;
                RenderArena* arena = renderArena();
                while (line) {
                    repaintLogicalTop = min(repaintLogicalTop, line->logicalTopVisualOverflow());
                    repaintLogicalBottom = max(repaintLogicalBottom, line->logicalBottomVisualOverflow());
                    RootInlineBox* next = line->nextRootBox();
                    line->deleteLine(arena);
                    line = next;
                }
            }
        }
        if (m_floatingObjects && (checkForFloatsFromLastLine || positionNewFloats()) && lastRootBox()) {
            // In case we have a float on the last line, it might not be positioned up to now.
            // This has to be done before adding in the bottom border/padding, or the float will
            // include the padding incorrectly. -dwh
            if (checkForFloatsFromLastLine) {
                int bottomVisualOverflow = lastRootBox()->logicalBottomVisualOverflow();
                int bottomLayoutOverflow = lastRootBox()->logicalBottomLayoutOverflow();
                TrailingFloatsRootInlineBox* trailingFloatsLineBox = new (renderArena()) TrailingFloatsRootInlineBox(this);
                m_lineBoxes.appendLineBox(trailingFloatsLineBox);
                trailingFloatsLineBox->setConstructed();
                GlyphOverflowAndFallbackFontsMap textBoxDataMap;
                VerticalPositionCache verticalPositionCache;
                trailingFloatsLineBox->alignBoxesInBlockDirection(logicalHeight(), textBoxDataMap, verticalPositionCache);
                int blockLogicalHeight = logicalHeight();
                IntRect logicalLayoutOverflow(0, blockLogicalHeight, 1, bottomLayoutOverflow - blockLogicalHeight);
                IntRect logicalVisualOverflow(0, blockLogicalHeight, 1, bottomVisualOverflow - blockLogicalHeight);
                trailingFloatsLineBox->setOverflowFromLogicalRects(logicalLayoutOverflow, logicalVisualOverflow, trailingFloatsLineBox->lineTop(), trailingFloatsLineBox->lineBottom());
                trailingFloatsLineBox->setBlockLogicalHeight(logicalHeight());
            }

            FloatingObjectSet& floatingObjectSet = m_floatingObjects->set();
            FloatingObjectSetIterator it = floatingObjectSet.begin();
            FloatingObjectSetIterator end = floatingObjectSet.end();
            if (lastFloat) {
                FloatingObjectSetIterator lastFloatIterator = floatingObjectSet.find(lastFloat);
                ASSERT(lastFloatIterator != end);
                ++lastFloatIterator;
                it = lastFloatIterator;
            }
            for (; it != end; ++it)
                appendFloatingObjectToLastLine(*it);
            lastFloat = !floatingObjectSet.isEmpty() ? floatingObjectSet.last() : 0;
        }
        size_t floatCount = floats.size();
        // Floats that did not have layout did not repaint when we laid them out. They would have
        // painted by now if they had moved, but if they stayed at (0, 0), they still need to be
        // painted.
        for (size_t i = 0; i < floatCount; ++i) {
            if (!floats[i].everHadLayout) {
                RenderBox* f = floats[i].object;
                if (!f->x() && !f->y() && f->checkForRepaintDuringLayout())
                    f->repaint();
            }
        }
    }

    // Expand the last line to accommodate Ruby and emphasis marks.
    int lastLineAnnotationsAdjustment = 0;
    if (lastRootBox()) {
        int lowestAllowedPosition = max(lastRootBox()->lineBottom(), logicalHeight() + paddingAfter());
        if (!style()->isFlippedLinesWritingMode())
            lastLineAnnotationsAdjustment = lastRootBox()->computeUnderAnnotationAdjustment(lowestAllowedPosition);
        else
            lastLineAnnotationsAdjustment = lastRootBox()->computeOverAnnotationAdjustment(lowestAllowedPosition);
    }

    // Now add in the bottom border/padding.
    setLogicalHeight(logicalHeight() + lastLineAnnotationsAdjustment + borderAfter() + paddingAfter() + scrollbarLogicalHeight());

    if (!firstLineBox() && hasLineIfEmpty())
        setLogicalHeight(logicalHeight() + lineHeight(true, isHorizontalWritingMode() ? HorizontalLine : VerticalLine, PositionOfInteriorLineBoxes));

    // See if we have any lines that spill out of our block.  If we do, then we will possibly need to
    // truncate text.
    if (hasTextOverflow)
        checkLinesForTextOverflow();
}

void RenderBlock::checkFloatsInCleanLine(RootInlineBox* line, Vector<FloatWithRect>& floats, size_t& floatIndex, bool& encounteredNewFloat, bool& dirtiedByFloat)
{
    Vector<RenderBox*>* cleanLineFloats = line->floatsPtr();
    if (!cleanLineFloats)
        return;

    Vector<RenderBox*>::iterator end = cleanLineFloats->end();
    for (Vector<RenderBox*>::iterator it = cleanLineFloats->begin(); it != end; ++it) {
        RenderBox* floatingBox = *it;
        floatingBox->layoutIfNeeded();
        IntSize newSize(floatingBox->width() + floatingBox->marginLeft() + floatingBox->marginRight(), floatingBox->height() + floatingBox->marginTop() + floatingBox->marginBottom());
        ASSERT(floatIndex < floats.size());
        if (floats[floatIndex].object != floatingBox) {
            encounteredNewFloat = true;
            return;
        }
        if (floats[floatIndex].rect.size() != newSize) {
            int floatTop = isHorizontalWritingMode() ? floats[floatIndex].rect.y() : floats[floatIndex].rect.x();
            int floatHeight = isHorizontalWritingMode() ? max(floats[floatIndex].rect.height(), newSize.height()) 
                                                                 : max(floats[floatIndex].rect.width(), newSize.width());
            floatHeight = min(floatHeight, numeric_limits<int>::max() - floatTop);
            line->markDirty();
            markLinesDirtyInBlockRange(line->blockLogicalHeight(), floatTop + floatHeight, line);
            floats[floatIndex].rect.setSize(newSize);
            dirtiedByFloat = true;
        }
        floatIndex++;
    }
}

RootInlineBox* RenderBlock::determineStartPosition(bool& firstLine, bool& fullLayout, bool& previousLineBrokeCleanly, 
                                                   InlineBidiResolver& resolver, Vector<FloatWithRect>& floats, unsigned& numCleanFloats,
                                                   bool& useRepaintBounds, int& repaintLogicalTop, int& repaintLogicalBottom)
{
    RootInlineBox* curr = 0;
    RootInlineBox* last = 0;

    bool dirtiedByFloat = false;
    if (!fullLayout) {
        // Paginate all of the clean lines.
        bool paginated = view()->layoutState() && view()->layoutState()->isPaginated();
        int paginationDelta = 0;
        size_t floatIndex = 0;
        for (curr = firstRootBox(); curr && !curr->isDirty(); curr = curr->nextRootBox()) {
            if (paginated) {
                paginationDelta -= curr->paginationStrut();
                adjustLinePositionForPagination(curr, paginationDelta);
                if (paginationDelta) {
                    if (containsFloats() || !floats.isEmpty()) {
                        // FIXME: Do better eventually.  For now if we ever shift because of pagination and floats are present just go to a full layout.
                        fullLayout = true; 
                        break;
                    }
                    
                    if (!useRepaintBounds)
                        useRepaintBounds = true;
                        
                    repaintLogicalTop = min(repaintLogicalTop, curr->logicalTopVisualOverflow() + min(paginationDelta, 0));
                    repaintLogicalBottom = max(repaintLogicalBottom, curr->logicalBottomVisualOverflow() + max(paginationDelta, 0));
                    curr->adjustBlockDirectionPosition(paginationDelta);
                }                
            }

            // If a new float has been inserted before this line or before its last known float,just do a full layout.
            checkFloatsInCleanLine(curr, floats, floatIndex, fullLayout, dirtiedByFloat);
            if (dirtiedByFloat || fullLayout)
                break;
        }
        // Check if a new float has been inserted after the last known float.
        if (!curr && floatIndex < floats.size())
            fullLayout = true;
    }

    if (fullLayout) {
        // Nuke all our lines.
        if (firstRootBox()) {
            RenderArena* arena = renderArena();
            curr = firstRootBox(); 
            while (curr) {
                RootInlineBox* next = curr->nextRootBox();
                curr->deleteLine(arena);
                curr = next;
            }
            ASSERT(!firstLineBox() && !lastLineBox());
        }
    } else {
        if (curr) {
            // We have a dirty line.
            if (RootInlineBox* prevRootBox = curr->prevRootBox()) {
                // We have a previous line.
                if (!dirtiedByFloat && (!prevRootBox->endsWithBreak() || (prevRootBox->lineBreakObj()->isText() && prevRootBox->lineBreakPos() >= toRenderText(prevRootBox->lineBreakObj())->textLength())))
                    // The previous line didn't break cleanly or broke at a newline
                    // that has been deleted, so treat it as dirty too.
                    curr = prevRootBox;
            }
        } else {
            // No dirty lines were found.
            // If the last line didn't break cleanly, treat it as dirty.
            if (lastRootBox() && !lastRootBox()->endsWithBreak())
                curr = lastRootBox();
        }

        // If we have no dirty lines, then last is just the last root box.
        last = curr ? curr->prevRootBox() : lastRootBox();
    }

    numCleanFloats = 0;
    if (!floats.isEmpty()) {
        int savedLogicalHeight = logicalHeight();
        // Restore floats from clean lines.
        RootInlineBox* line = firstRootBox();
        while (line != curr) {
            if (Vector<RenderBox*>* cleanLineFloats = line->floatsPtr()) {
                Vector<RenderBox*>::iterator end = cleanLineFloats->end();
                for (Vector<RenderBox*>::iterator f = cleanLineFloats->begin(); f != end; ++f) {
                    FloatingObject* floatingObject = insertFloatingObject(*f);
                    ASSERT(!floatingObject->m_originatingLine);
                    floatingObject->m_originatingLine = line;
                    setLogicalHeight(logicalTopForChild(*f) - marginBeforeForChild(*f));
                    positionNewFloats();
                    ASSERT(floats[numCleanFloats].object == *f);
                    numCleanFloats++;
                }
            }
            line = line->nextRootBox();
        }
        setLogicalHeight(savedLogicalHeight);
    }

    firstLine = !last;
    previousLineBrokeCleanly = !last || last->endsWithBreak();

    RenderObject* startObj;
    int pos = 0;
    if (last) {
        setLogicalHeight(last->blockLogicalHeight());
        startObj = last->lineBreakObj();
        pos = last->lineBreakPos();
        resolver.setStatus(last->lineBreakBidiStatus());
    } else {
        bool ltr = style()->isLeftToRightDirection();
        Direction direction = ltr ? LeftToRight : RightToLeft;
        resolver.setLastStrongDir(direction);
        resolver.setLastDir(direction);
        resolver.setEorDir(direction);
        resolver.setContext(BidiContext::create(ltr ? 0 : 1, direction, style()->unicodeBidi() == Override, FromStyleOrDOM));

        startObj = bidiFirst(this, &resolver);
    }

    resolver.setPosition(InlineIterator(this, startObj, pos));

    return curr;
}

RootInlineBox* RenderBlock::determineEndPosition(RootInlineBox* startLine, Vector<FloatWithRect>& floats, size_t floatIndex, InlineIterator& cleanLineStart, BidiStatus& cleanLineBidiStatus, int& logicalTop)
{
    RootInlineBox* last = 0;
    for (RootInlineBox* curr = startLine->nextRootBox(); curr; curr = curr->nextRootBox()) {
        if (!curr->isDirty()) {
            bool encounteredNewFloat = false;
            bool dirtiedByFloat = false;
            checkFloatsInCleanLine(curr, floats, floatIndex, encounteredNewFloat, dirtiedByFloat);
            if (encounteredNewFloat)
                return 0;
        }
        if (curr->isDirty())
            last = 0;
        else if (!last)
            last = curr;
    }

    if (!last)
        return 0;

    // At this point, |last| is the first line in a run of clean lines that ends with the last line
    // in the block.

    RootInlineBox* prev = last->prevRootBox();
    cleanLineStart = InlineIterator(this, prev->lineBreakObj(), prev->lineBreakPos());
    cleanLineBidiStatus = prev->lineBreakBidiStatus();
    logicalTop = prev->blockLogicalHeight();

    for (RootInlineBox* line = last; line; line = line->nextRootBox())
        line->extractLine(); // Disconnect all line boxes from their render objects while preserving
                             // their connections to one another.

    return last;
}

bool RenderBlock::matchedEndLine(const InlineBidiResolver& resolver, const InlineIterator& endLineStart, const BidiStatus& endLineStatus, RootInlineBox*& endLine,
                                 int& endLogicalTop, int& repaintLogicalBottom, int& repaintLogicalTop)
{
    if (resolver.position() == endLineStart) {
        if (resolver.status() != endLineStatus)
            return false;

        int delta = logicalHeight() - endLogicalTop;
        if (!delta || !m_floatingObjects)
            return true;

        // See if any floats end in the range along which we want to shift the lines vertically.
        int logicalTop = min(logicalHeight(), endLogicalTop);

        RootInlineBox* lastLine = endLine;
        while (RootInlineBox* nextLine = lastLine->nextRootBox())
            lastLine = nextLine;

        int logicalBottom = lastLine->blockLogicalHeight() + abs(delta);

        FloatingObjectSet& floatingObjectSet = m_floatingObjects->set();
        FloatingObjectSetIterator end = floatingObjectSet.end();
        for (FloatingObjectSetIterator it = floatingObjectSet.begin(); it != end; ++it) {
            FloatingObject* f = *it;
            if (logicalBottomForFloat(f) >= logicalTop && logicalBottomForFloat(f) < logicalBottom)
                return false;
        }

        return true;
    }

    // The first clean line doesn't match, but we can check a handful of following lines to try
    // to match back up.
    static int numLines = 8; // The # of lines we're willing to match against.
    RootInlineBox* line = endLine;
    for (int i = 0; i < numLines && line; i++, line = line->nextRootBox()) {
        if (line->lineBreakObj() == resolver.position().m_obj && line->lineBreakPos() == resolver.position().m_pos) {
            // We have a match.
            if (line->lineBreakBidiStatus() != resolver.status())
                return false; // ...but the bidi state doesn't match.
            RootInlineBox* result = line->nextRootBox();

            // Set our logical top to be the block height of endLine.
            if (result)
                endLogicalTop = line->blockLogicalHeight();

            int delta = logicalHeight() - endLogicalTop;
            if (delta && m_floatingObjects) {
                // See if any floats end in the range along which we want to shift the lines vertically.
                int logicalTop = min(logicalHeight(), endLogicalTop);

                RootInlineBox* lastLine = endLine;
                while (RootInlineBox* nextLine = lastLine->nextRootBox())
                    lastLine = nextLine;

                int logicalBottom = lastLine->blockLogicalHeight() + abs(delta);

                FloatingObjectSet& floatingObjectSet = m_floatingObjects->set();
                FloatingObjectSetIterator end = floatingObjectSet.end();
                for (FloatingObjectSetIterator it = floatingObjectSet.begin(); it != end; ++it) {
                    FloatingObject* f = *it;
                    if (logicalBottomForFloat(f) >= logicalTop && logicalBottomForFloat(f) < logicalBottom)
                        return false;
                }
            }

            // Now delete the lines that we failed to sync.
            RootInlineBox* boxToDelete = endLine;
            RenderArena* arena = renderArena();
            while (boxToDelete && boxToDelete != result) {
                repaintLogicalTop = min(repaintLogicalTop, boxToDelete->logicalTopVisualOverflow());
                repaintLogicalBottom = max(repaintLogicalBottom, boxToDelete->logicalBottomVisualOverflow());
                RootInlineBox* next = boxToDelete->nextRootBox();
                boxToDelete->deleteLine(arena);
                boxToDelete = next;
            }

            endLine = result;
            return result;
        }
    }

    return false;
}

static inline bool skipNonBreakingSpace(const InlineIterator& it, bool isLineEmpty, bool previousLineBrokeCleanly)
{
    if (it.m_obj->style()->nbspMode() != SPACE || it.current() != noBreakSpace)
        return false;

    // FIXME: This is bad.  It makes nbsp inconsistent with space and won't work correctly
    // with m_minWidth/m_maxWidth.
    // Do not skip a non-breaking space if it is the first character
    // on a line after a clean line break (or on the first line, since previousLineBrokeCleanly starts off
    // |true|).
    if (isLineEmpty && previousLineBrokeCleanly)
        return false;

    return true;
}

static inline bool shouldCollapseWhiteSpace(const RenderStyle* style, bool isLineEmpty, bool previousLineBrokeCleanly)
{
    return style->collapseWhiteSpace() || (style->whiteSpace() == PRE_WRAP && (!isLineEmpty || !previousLineBrokeCleanly));
}

static bool inlineFlowRequiresLineBox(RenderInline* flow)
{
    // FIXME: Right now, we only allow line boxes for inlines that are truly empty.
    // We need to fix this, though, because at the very least, inlines containing only
    // ignorable whitespace should should also have line boxes. 
    return !flow->firstChild() && flow->hasInlineDirectionBordersPaddingOrMargin();
}

bool RenderBlock::requiresLineBox(const InlineIterator& it, bool isLineEmpty, bool previousLineBrokeCleanly)
{
    if (it.m_obj->isFloatingOrPositioned())
        return false;

    if (it.m_obj->isRenderInline() && !inlineFlowRequiresLineBox(toRenderInline(it.m_obj)))
        return false;

    if (!shouldCollapseWhiteSpace(it.m_obj->style(), isLineEmpty, previousLineBrokeCleanly) || it.m_obj->isBR())
        return true;

    UChar current = it.current();
    return current != ' ' && current != '\t' && current != softHyphen && (current != '\n' || it.m_obj->preservesNewline()) 
            && !skipNonBreakingSpace(it, isLineEmpty, previousLineBrokeCleanly);
}

bool RenderBlock::generatesLineBoxesForInlineChild(RenderObject* inlineObj, bool isLineEmpty, bool previousLineBrokeCleanly)
{
    ASSERT(inlineObj->parent() == this);

    InlineIterator it(this, inlineObj, 0);
    while (!it.atEnd() && !requiresLineBox(it, isLineEmpty, previousLineBrokeCleanly))
        it.increment();

    return !it.atEnd();
}

// FIXME: The entire concept of the skipTrailingWhitespace function is flawed, since we really need to be building
// line boxes even for containers that may ultimately collapse away.  Otherwise we'll never get positioned
// elements quite right.  In other words, we need to build this function's work into the normal line
// object iteration process.
// NB. this function will insert any floating elements that would otherwise
// be skipped but it will not position them.
void RenderBlock::skipTrailingWhitespace(InlineIterator& iterator, bool isLineEmpty, bool previousLineBrokeCleanly)
{
    while (!iterator.atEnd() && !requiresLineBox(iterator, isLineEmpty, previousLineBrokeCleanly)) {
        RenderObject* object = iterator.m_obj;
        if (object->isFloating()) {
            insertFloatingObject(toRenderBox(object));
        } else if (object->isPositioned())
            setStaticPositions(this, toRenderBox(object));
        iterator.increment();
    }
}

void RenderBlock::skipLeadingWhitespace(InlineBidiResolver& resolver, bool isLineEmpty, bool previousLineBrokeCleanly,
    FloatingObject* lastFloatFromPreviousLine, LineWidth& width)
{
    while (!resolver.position().atEnd() && !requiresLineBox(resolver.position(), isLineEmpty, previousLineBrokeCleanly)) {
        RenderObject* object = resolver.position().m_obj;
        if (object->isFloating())
            positionNewFloatOnLine(insertFloatingObject(toRenderBox(object)), lastFloatFromPreviousLine, width);
        else if (object->isPositioned())
            setStaticPositions(this, toRenderBox(object));
        resolver.increment();
    }
    resolver.commitExplicitEmbedding();
}

// This is currently just used for list markers and inline flows that have line boxes. Neither should 
// have an effect on whitespace at the start of the line. 
static bool shouldSkipWhitespaceAfterStartObject(RenderBlock* block, RenderObject* o, LineMidpointState& lineMidpointState)
{
    RenderObject* next = bidiNext(block, o);
    if (next && !next->isBR() && next->isText() && toRenderText(next)->textLength() > 0) {
        RenderText* nextText = toRenderText(next);
        UChar nextChar = nextText->characters()[0];
        if (nextText->style()->isCollapsibleWhiteSpace(nextChar)) {
            addMidpoint(lineMidpointState, InlineIterator(0, o, 0));
            return true;
        }
    }

    return false;
}

static inline float textWidth(RenderText* text, unsigned from, unsigned len, const Font& font, float xPos, bool isFixedPitch, bool collapseWhiteSpace)
{
    if (isFixedPitch || (!from && len == text->textLength()) || text->style()->hasTextCombine())
        return text->width(from, len, font, xPos);
    return font.width(TextRun(text->characters() + from, len, !collapseWhiteSpace, xPos));
}

static void tryHyphenating(RenderText* text, const Font& font, const AtomicString& localeIdentifier, int minimumPrefixLength, int minimumSuffixLength, int lastSpace, int pos, float xPos, int availableWidth, bool isFixedPitch, bool collapseWhiteSpace, int lastSpaceWordSpacing, InlineIterator& lineBreak, int nextBreakable, bool& hyphenated)
{
    // Map 'hyphenate-limit-{before,after}: auto;' to 2.
    if (minimumPrefixLength < 0)
        minimumPrefixLength = 2;

    if (minimumSuffixLength < 0)
        minimumSuffixLength = 2;

    if (pos - lastSpace <= minimumSuffixLength)
        return;

    const AtomicString& hyphenString = text->style()->hyphenString();
    int hyphenWidth = font.width(TextRun(hyphenString.characters(), hyphenString.length()));

    float maxPrefixWidth = availableWidth - xPos - hyphenWidth - lastSpaceWordSpacing;
    // If the maximum width available for the prefix before the hyphen is small, then it is very unlikely
    // that an hyphenation opportunity exists, so do not bother to look for it.
    if (maxPrefixWidth <= font.pixelSize() * 5 / 4)
        return;

    unsigned prefixLength = font.offsetForPosition(TextRun(text->characters() + lastSpace, pos - lastSpace, !collapseWhiteSpace, xPos + lastSpaceWordSpacing), maxPrefixWidth, false);
    if (prefixLength < static_cast<unsigned>(minimumPrefixLength))
        return;

    prefixLength = lastHyphenLocation(text->characters() + lastSpace, pos - lastSpace, min(prefixLength, static_cast<unsigned>(pos - lastSpace - minimumSuffixLength)) + 1, localeIdentifier);
    // FIXME: The following assumes that the character at lastSpace is a space (and therefore should not factor
    // into hyphenate-limit-before) unless lastSpace is 0. This is wrong in the rare case of hyphenating
    // the first word in a text node which has leading whitespace.
    if (!prefixLength || prefixLength - (lastSpace ? 1 : 0) < static_cast<unsigned>(minimumPrefixLength))
        return;

    ASSERT(pos - lastSpace - prefixLength >= static_cast<unsigned>(minimumSuffixLength));

#if !ASSERT_DISABLED
    float prefixWidth = hyphenWidth + textWidth(text, lastSpace, prefixLength, font, xPos, isFixedPitch, collapseWhiteSpace) + lastSpaceWordSpacing;
    ASSERT(xPos + prefixWidth <= availableWidth);
#else
    UNUSED_PARAM(isFixedPitch);
#endif

    lineBreak.moveTo(text, lastSpace + prefixLength, nextBreakable);
    hyphenated = true;
}

class LineWidth {
public:
    LineWidth(RenderBlock* block, bool isFirstLine)
        : m_block(block)
        , m_uncommittedWidth(0)
        , m_committedWidth(0)
        , m_overhangWidth(0)
        , m_left(0)
        , m_right(0)
        , m_availableWidth(0)
        , m_isFirstLine(isFirstLine)
    {
        ASSERT(block);
        updateAvailableWidth();
    }
    bool fitsOnLine() const { return currentWidth() <= m_availableWidth; }
    bool fitsOnLine(float extra) const { return currentWidth() + extra <= m_availableWidth; }
    float currentWidth() const { return m_committedWidth + m_uncommittedWidth; }

    // FIXME: We should eventually replace these three functions by ones that work on a higher abstraction.
    float uncommittedWidth() const { return m_uncommittedWidth; }
    float committedWidth() const { return m_committedWidth; }
    float availableWidth() const { return m_availableWidth; }

    void updateAvailableWidth();
    void shrinkAvailableWidthForNewFloatIfNeeded(RenderBlock::FloatingObject*);
    void addUncommittedWidth(float delta) { m_uncommittedWidth += delta; }
    void commit()
    {
        m_committedWidth += m_uncommittedWidth;
        m_uncommittedWidth = 0;
    }
    void applyOverhang(RenderRubyRun*, RenderObject* startRenderer, RenderObject* endRenderer);
    void fitBelowFloats();

private:
    void computeAvailableWidthFromLeftAndRight()
    {
        m_availableWidth = max(0, m_right - m_left) + m_overhangWidth;
    }

private:
    RenderBlock* m_block;
    float m_uncommittedWidth;
    float m_committedWidth;
    float m_overhangWidth; // The amount by which |m_availableWidth| has been inflated to account for possible contraction due to ruby overhang.
    int m_left;
    int m_right;
    float m_availableWidth;
    bool m_isFirstLine;
};

inline void LineWidth::updateAvailableWidth()
{
    int height = m_block->logicalHeight();
    m_left = m_block->logicalLeftOffsetForLine(height, m_isFirstLine);
    m_right = m_block->logicalRightOffsetForLine(height, m_isFirstLine);

    computeAvailableWidthFromLeftAndRight();
}

inline void LineWidth::shrinkAvailableWidthForNewFloatIfNeeded(RenderBlock::FloatingObject* newFloat)
{
    int height = m_block->logicalHeight();
    if (height < m_block->logicalTopForFloat(newFloat) || height >= m_block->logicalBottomForFloat(newFloat))
        return;

    if (newFloat->type() == RenderBlock::FloatingObject::FloatLeft)
        m_left = m_block->logicalRightForFloat(newFloat);
    else
        m_right = m_block->logicalLeftForFloat(newFloat);

    computeAvailableWidthFromLeftAndRight();
}

void LineWidth::applyOverhang(RenderRubyRun* rubyRun, RenderObject* startRenderer, RenderObject* endRenderer)
{
    int startOverhang;
    int endOverhang;
    rubyRun->getOverhang(m_isFirstLine, startRenderer, endRenderer, startOverhang, endOverhang);

    startOverhang = min<int>(startOverhang, m_committedWidth);
    m_availableWidth += startOverhang;

    endOverhang = max(min<int>(endOverhang, m_availableWidth - currentWidth()), 0);
    m_availableWidth += endOverhang;
    m_overhangWidth += startOverhang + endOverhang;
}

void LineWidth::fitBelowFloats()
{
    ASSERT(!m_committedWidth);
    ASSERT(!fitsOnLine());

    int floatLogicalBottom;
    int lastFloatLogicalBottom = m_block->logicalHeight();
    float newLineWidth = m_availableWidth;
    while (true) {
        floatLogicalBottom = m_block->nextFloatLogicalBottomBelow(lastFloatLogicalBottom);
        if (!floatLogicalBottom)
            break;

        newLineWidth = m_block->availableLogicalWidthForLine(floatLogicalBottom, m_isFirstLine);
        lastFloatLogicalBottom = floatLogicalBottom;
        if (newLineWidth >= m_uncommittedWidth)
            break;
    }

    if (newLineWidth > m_availableWidth) {
        m_block->setLogicalHeight(lastFloatLogicalBottom);
        m_availableWidth = newLineWidth + m_overhangWidth;
    }
}

InlineIterator RenderBlock::findNextLineBreak(InlineBidiResolver& resolver, bool firstLine, bool& isLineEmpty, LineBreakIteratorInfo& lineBreakIteratorInfo, bool& previousLineBrokeCleanly, 
                                              bool& hyphenated, EClear* clear, FloatingObject* lastFloatFromPreviousLine, Vector<RenderBox*>& positionedBoxes)
{
    ASSERT(resolver.position().root() == this);

    bool appliedStartWidth = resolver.position().m_pos > 0;
    LineMidpointState& lineMidpointState = resolver.midpointState();

    LineWidth width(this, firstLine);

    skipLeadingWhitespace(resolver, isLineEmpty, previousLineBrokeCleanly, lastFloatFromPreviousLine, width);

    if (resolver.position().atEnd())
        return resolver.position();

    // This variable is used only if whitespace isn't set to PRE, and it tells us whether
    // or not we are currently ignoring whitespace.
    bool ignoringSpaces = false;
    InlineIterator ignoreStart;
    
    // This variable tracks whether the very last character we saw was a space.  We use
    // this to detect when we encounter a second space so we know we have to terminate
    // a run.
    bool currentCharacterIsSpace = false;
    bool currentCharacterIsWS = false;
    RenderObject* trailingSpaceObject = 0;
    Vector<RenderBox*, 4> trailingPositionedBoxes;

    InlineIterator lBreak = resolver.position();

    // FIXME: It is error-prone to split the position object out like this.
    // Teach this code to work with objects instead of this split tuple.
    RenderObject* o = resolver.position().m_obj;
    RenderObject* last = o;
    unsigned pos = resolver.position().m_pos;
    int nextBreakable = resolver.position().m_nextBreakablePosition;
    bool atStart = true;

    bool prevLineBrokeCleanly = previousLineBrokeCleanly;
    previousLineBrokeCleanly = false;

    hyphenated = false;

    bool autoWrapWasEverTrueOnLine = false;
    bool floatsFitOnLine = true;
    
    // Firefox and Opera will allow a table cell to grow to fit an image inside it under
    // very specific circumstances (in order to match common WinIE renderings). 
    // Not supporting the quirk has caused us to mis-render some real sites. (See Bugzilla 10517.) 
    bool allowImagesToBreak = !document()->inQuirksMode() || !isTableCell() || !style()->logicalWidth().isIntrinsicOrAuto();

    EWhiteSpace currWS = style()->whiteSpace();
    EWhiteSpace lastWS = currWS;
    while (o) {
        RenderObject* next = bidiNext(this, o);

        currWS = o->isReplaced() ? o->parent()->style()->whiteSpace() : o->style()->whiteSpace();
        lastWS = last->isReplaced() ? last->parent()->style()->whiteSpace() : last->style()->whiteSpace();
        
        bool autoWrap = RenderStyle::autoWrap(currWS);
        autoWrapWasEverTrueOnLine = autoWrapWasEverTrueOnLine || autoWrap;

#if ENABLE(SVG)
        bool preserveNewline = o->isSVGInlineText() ? false : RenderStyle::preserveNewline(currWS);
#else
        bool preserveNewline = RenderStyle::preserveNewline(currWS);
#endif

        bool collapseWhiteSpace = RenderStyle::collapseWhiteSpace(currWS);
            
        if (o->isBR()) {
            if (width.fitsOnLine()) {
                lBreak.moveToStartOf(o);
                lBreak.increment();

                // A <br> always breaks a line, so don't let the line be collapsed
                // away. Also, the space at the end of a line with a <br> does not
                // get collapsed away.  It only does this if the previous line broke
                // cleanly.  Otherwise the <br> has no effect on whether the line is
                // empty or not.
                if (prevLineBrokeCleanly)
                    isLineEmpty = false;
                trailingSpaceObject = 0;
                previousLineBrokeCleanly = true;

                if (!isLineEmpty && clear)
                    *clear = o->style()->clear();
            }
            goto end;
        }

        if (o->isFloatingOrPositioned()) {
            // add to special objects...
            if (o->isFloating()) {
                RenderBox* floatBox = toRenderBox(o);
                FloatingObject* f = insertFloatingObject(floatBox);
                // check if it fits in the current line.
                // If it does, position it now, otherwise, position
                // it after moving to next line (in newLine() func)
                if (floatsFitOnLine && width.fitsOnLine(logicalWidthForFloat(f))) {
                    positionNewFloatOnLine(f, lastFloatFromPreviousLine, width);
                    if (lBreak.m_obj == o) {
                        ASSERT(!lBreak.m_pos);
                        lBreak.increment();
                    }
                } else
                    floatsFitOnLine = false;
            } else if (o->isPositioned()) {
                // If our original display wasn't an inline type, then we can
                // go ahead and determine our static inline position now.
                RenderBox* box = toRenderBox(o);
                bool isInlineType = box->style()->isOriginalDisplayInlineType();
                if (!isInlineType)
                    box->layer()->setStaticInlinePosition(borderAndPaddingStart());
                else  {
                    // If our original display was an INLINE type, then we can go ahead
                    // and determine our static y position now.
                    box->layer()->setStaticBlockPosition(logicalHeight());
                }
                
                // If we're ignoring spaces, we have to stop and include this object and
                // then start ignoring spaces again.
                if (isInlineType || o->container()->isRenderInline()) {
                    if (ignoringSpaces) {
                        ignoreStart.m_obj = o;
                        ignoreStart.m_pos = 0;
                        addMidpoint(lineMidpointState, ignoreStart); // Stop ignoring spaces.
                        addMidpoint(lineMidpointState, ignoreStart); // Start ignoring again.
                    }
                    if (trailingSpaceObject)
                        trailingPositionedBoxes.append(box);
                } else
                    positionedBoxes.append(box);
            }
        } else if (o->isRenderInline()) {
            // Right now, we should only encounter empty inlines here.
            ASSERT(!o->firstChild());
    
            RenderInline* flowBox = toRenderInline(o);
            
            // Now that some inline flows have line boxes, if we are already ignoring spaces, we need 
            // to make sure that we stop to include this object and then start ignoring spaces again. 
            // If this object is at the start of the line, we need to behave like list markers and 
            // start ignoring spaces.
            if (inlineFlowRequiresLineBox(flowBox)) {
                isLineEmpty = false;
                if (ignoringSpaces) {
                    trailingSpaceObject = 0;
                    trailingPositionedBoxes.clear();
                    addMidpoint(lineMidpointState, InlineIterator(0, o, 0)); // Stop ignoring spaces.
                    addMidpoint(lineMidpointState, InlineIterator(0, o, 0)); // Start ignoring again.
                } else if (style()->collapseWhiteSpace() && resolver.position().m_obj == o
                    && shouldSkipWhitespaceAfterStartObject(this, o, lineMidpointState)) {
                    // Like with list markers, we start ignoring spaces to make sure that any 
                    // additional spaces we see will be discarded.
                    currentCharacterIsSpace = true;
                    currentCharacterIsWS = true;
                    ignoringSpaces = true;
                }
            }

            width.addUncommittedWidth(borderPaddingMarginStart(flowBox) + borderPaddingMarginEnd(flowBox));
        } else if (o->isReplaced()) {
            RenderBox* replacedBox = toRenderBox(o);

            // Break on replaced elements if either has normal white-space.
            if ((autoWrap || RenderStyle::autoWrap(lastWS)) && (!o->isImage() || allowImagesToBreak)) {
                width.commit();
                lBreak.moveToStartOf(o);
            }

            if (ignoringSpaces)
                addMidpoint(lineMidpointState, InlineIterator(0, o, 0));

            isLineEmpty = false;
            ignoringSpaces = false;
            currentCharacterIsSpace = false;
            currentCharacterIsWS = false;
            trailingSpaceObject = 0;
            trailingPositionedBoxes.clear();

            // Optimize for a common case. If we can't find whitespace after the list
            // item, then this is all moot.
            int replacedLogicalWidth = logicalWidthForChild(replacedBox) + marginStartForChild(replacedBox) + marginEndForChild(replacedBox) + inlineLogicalWidth(o);
            if (o->isListMarker()) {
                if (style()->collapseWhiteSpace() && shouldSkipWhitespaceAfterStartObject(this, o, lineMidpointState)) {
                    // Like with inline flows, we start ignoring spaces to make sure that any 
                    // additional spaces we see will be discarded.
                    currentCharacterIsSpace = true;
                    currentCharacterIsWS = true;
                    ignoringSpaces = true;
                }
                if (toRenderListMarker(o)->isInside())
                    width.addUncommittedWidth(replacedLogicalWidth);
            } else
                width.addUncommittedWidth(replacedLogicalWidth);
            if (o->isRubyRun())
                width.applyOverhang(toRenderRubyRun(o), last, next);
        } else if (o->isText()) {
            if (!pos)
                appliedStartWidth = false;

            RenderText* t = toRenderText(o);

#if ENABLE(SVG)
            bool isSVGText = t->isSVGInlineText();
#endif

            RenderStyle* style = t->style(firstLine);
            if (style->hasTextCombine() && o->isCombineText())
                toRenderCombineText(o)->combineText();

            int strlen = t->textLength();
            int len = strlen - pos;
            const UChar* str = t->characters();

            const Font& f = style->font();
            bool isFixedPitch = f.isFixedPitch();
            bool canHyphenate = style->hyphens() == HyphensAuto && WebCore::canHyphenate(style->locale());

            int lastSpace = pos;
            float wordSpacing = o->style()->wordSpacing();
            float lastSpaceWordSpacing = 0;

            // Non-zero only when kerning is enabled, in which case we measure words with their trailing
            // space, then subtract its width.
            float wordTrailingSpaceWidth = f.typesettingFeatures() & Kerning ? f.width(TextRun(&space, 1)) + wordSpacing : 0;

            float wrapW = width.uncommittedWidth() + inlineLogicalWidth(o, !appliedStartWidth, true);
            float charWidth = 0;
            bool breakNBSP = autoWrap && o->style()->nbspMode() == SPACE;
            // Auto-wrapping text should wrap in the middle of a word only if it could not wrap before the word,
            // which is only possible if the word is the first thing on the line, that is, if |w| is zero.
            bool breakWords = o->style()->breakWords() && ((autoWrap && !width.committedWidth()) || currWS == PRE);
            bool midWordBreak = false;
            bool breakAll = o->style()->wordBreak() == BreakAllWordBreak && autoWrap;
            float hyphenWidth = 0;

            if (t->isWordBreak()) {
                width.commit();
                lBreak.moveToStartOf(o);
                ASSERT(!len);
            }

            while (len) {
                bool previousCharacterIsSpace = currentCharacterIsSpace;
                bool previousCharacterIsWS = currentCharacterIsWS;
                UChar c = str[pos];
                currentCharacterIsSpace = c == ' ' || c == '\t' || (!preserveNewline && (c == '\n'));

		  // SAMSUNG CHANGE >>
		  bool isExtendedUnicode = false;
		  int extendedCharCount = 0;
		  if (c >= 0x3041 && len > 1) {
		      TextRun run("", 0);
		      WidthIterator it(NULL, run);
		      if (isExtendedUnicode = it.isExtendedUnicodeChar(c, str[pos+1]))
		          extendedCharCount = 1;
		  }
		  // SAMSUNG CHANGE <<
                if (!collapseWhiteSpace || !currentCharacterIsSpace)
                    isLineEmpty = false;

                if (c == softHyphen && autoWrap && !hyphenWidth && style->hyphens() != HyphensNone) {
                    const AtomicString& hyphenString = style->hyphenString();
                    hyphenWidth = f.width(TextRun(hyphenString.characters(), hyphenString.length()));
                    width.addUncommittedWidth(hyphenWidth);
                }

                bool applyWordSpacing = false;
                
                currentCharacterIsWS = currentCharacterIsSpace || (breakNBSP && c == noBreakSpace);

                if ((breakAll || breakWords) && !midWordBreak) {
                    wrapW += charWidth;
		      // SAMSUNG CHANGE
                    charWidth = textWidth(t, pos, 1+ extendedCharCount, f, width.committedWidth() + wrapW, isFixedPitch, collapseWhiteSpace);
                    midWordBreak = width.committedWidth() + wrapW + charWidth > width.availableWidth();
                }

                if (lineBreakIteratorInfo.first != t) {
                    lineBreakIteratorInfo.first = t;
                    lineBreakIteratorInfo.second.reset(str, strlen);
                }

                bool betweenWords = c == '\n' || (currWS != PRE && !atStart && isBreakable(lineBreakIteratorInfo.second, pos, nextBreakable, breakNBSP) && (style->hyphens() != HyphensNone || (pos && str[pos - 1] != softHyphen)));

				// MPSG100003245 Start 
				if( !betweenWords && o && lBreak.m_obj && (o != lBreak.m_obj)) { //We are parsing inside a object which is not yet selected as a object to break.
											// lBreak.obj is still containing the previous selected object to break.
											// To make this object selectable and current position as breakbale position, we somehow have to set betweenWords for this character true.	
					if(lBreak.m_obj->node() && lBreak.m_obj->node()->isTextNode()){
						RenderText* prev = toRenderText(lBreak.m_obj->node()->renderer());
						const UChar* prestr = prev->characters();
						UChar nextchar = prestr[lBreak.m_pos];
						bool nextCharacterIsSpace = nextchar == ' ' || nextchar == '\t' || nextchar == '\n';
						bool hasPreviousObjectparsedFully = ( (lBreak.m_pos == prev->textLength() - 1 && width.uncommittedWidth() != 0) || nextCharacterIsSpace ) ; // Check if previous object string is parsed completely.
																												  // That is lBreak.pos before the last character and we have some uncommitted width for the last character.
						if(hasPreviousObjectparsedFully)
							betweenWords = true;

					}
				}
				// MPSG100003245 End 


                if (betweenWords || midWordBreak) {
                    bool stoppedIgnoringSpaces = false;
                    if (ignoringSpaces) {
                        if (!currentCharacterIsSpace) {
                            // Stop ignoring spaces and begin at this
                            // new point.
                            ignoringSpaces = false;
                            lastSpaceWordSpacing = 0;
                            lastSpace = pos; // e.g., "Foo    goo", don't add in any of the ignored spaces.
                            addMidpoint(lineMidpointState, InlineIterator(0, o, pos));
                            stoppedIgnoringSpaces = true;
                        } else {
                            // Just keep ignoring these spaces.
                            pos++;
                            len--;
                            continue;
                        }
                    }

                    float additionalTmpW;
                    if (wordTrailingSpaceWidth && currentCharacterIsSpace)
                        additionalTmpW = textWidth(t, lastSpace, pos + 1 - lastSpace, f, width.currentWidth(), isFixedPitch, collapseWhiteSpace) - wordTrailingSpaceWidth + lastSpaceWordSpacing;
                    else// SAMSUNG CHANGE
                        additionalTmpW = textWidth(t, lastSpace, pos+ extendedCharCount  - lastSpace, f, width.currentWidth(), isFixedPitch, collapseWhiteSpace) + lastSpaceWordSpacing;
                    width.addUncommittedWidth(additionalTmpW);
                    if (!appliedStartWidth) {
                        width.addUncommittedWidth(inlineLogicalWidth(o, true, false));
                        appliedStartWidth = true;
                    }
                    
                    applyWordSpacing =  wordSpacing && currentCharacterIsSpace && !previousCharacterIsSpace;

                    if (!width.committedWidth() && autoWrap && !width.fitsOnLine())
                        width.fitBelowFloats();

                    if (autoWrap || breakWords) {
                        // If we break only after white-space, consider the current character
                        // as candidate width for this line.
                        bool lineWasTooWide = false;
                        if (width.fitsOnLine() && currentCharacterIsWS && o->style()->breakOnlyAfterWhiteSpace() && !midWordBreak) {
				// SAMSUNG CHANGE
                            int charWidth = textWidth(t, pos, 1+ extendedCharCount, f, width.currentWidth(), isFixedPitch, collapseWhiteSpace) + (applyWordSpacing ? wordSpacing : 0);
                            // Check if line is too big even without the extra space
                            // at the end of the line. If it is not, do nothing. 
                            // If the line needs the extra whitespace to be too long, 
                            // then move the line break to the space and skip all 
                            // additional whitespace.
                            if (!width.fitsOnLine(charWidth)) {
                                lineWasTooWide = true;
                                lBreak.moveTo(o, pos, nextBreakable);
                                skipTrailingWhitespace(lBreak, isLineEmpty, previousLineBrokeCleanly);
                            }
                        }
                        if (lineWasTooWide || !width.fitsOnLine()) {
                            if (canHyphenate && !width.fitsOnLine()) {
                                tryHyphenating(t, f, style->locale(), style->hyphenationLimitBefore(), style->hyphenationLimitAfter(), lastSpace, pos, width.currentWidth() - additionalTmpW, width.availableWidth(), isFixedPitch, collapseWhiteSpace, lastSpaceWordSpacing, lBreak, nextBreakable, hyphenated);
                                if (hyphenated)
                                    goto end;
                            }
                            if (lBreak.atTextParagraphSeparator()) {
                                if (!stoppedIgnoringSpaces && pos > 0) {
                                    // We need to stop right before the newline and then start up again.
                                    addMidpoint(lineMidpointState, InlineIterator(0, o, pos - 1)); // Stop
                                    addMidpoint(lineMidpointState, InlineIterator(0, o, pos)); // Start
                                }
                                lBreak.increment();
                                previousLineBrokeCleanly = true;
                            }
                            if (lBreak.m_obj && lBreak.m_pos && lBreak.m_obj->isText() && toRenderText(lBreak.m_obj)->textLength() && toRenderText(lBreak.m_obj)->characters()[lBreak.m_pos - 1] == softHyphen && style->hyphens() != HyphensNone)
                                hyphenated = true;
                            goto end; // Didn't fit. Jump to the end.
                        } else {
                            if (!betweenWords || (midWordBreak && !autoWrap))
                                width.addUncommittedWidth(-additionalTmpW);
                            if (hyphenWidth) {
                                // Subtract the width of the soft hyphen out since we fit on a line.
                                width.addUncommittedWidth(-hyphenWidth);
                                hyphenWidth = 0;
                            }
                        }
                    }

                    if (c == '\n' && preserveNewline) {
                        if (!stoppedIgnoringSpaces && pos > 0) {
                            // We need to stop right before the newline and then start up again.
                            addMidpoint(lineMidpointState, InlineIterator(0, o, pos - 1)); // Stop
                            addMidpoint(lineMidpointState, InlineIterator(0, o, pos)); // Start
                        }
                        lBreak.moveTo(o, pos, nextBreakable);
                        lBreak.increment();
                        previousLineBrokeCleanly = true;
                        return lBreak;
                    }

                    if (autoWrap && betweenWords) {
                        width.commit();
                        wrapW = 0;
                        lBreak.moveTo(o, pos, nextBreakable);
                        // Auto-wrapping text should not wrap in the middle of a word once it has had an
                        // opportunity to break after a word.
                        breakWords = false;
                    }
                    
                    if (midWordBreak) {
                        // Remember this as a breakable position in case
                        // adding the end width forces a break.
                        lBreak.moveTo(o, pos, nextBreakable);
                        midWordBreak &= (breakWords || breakAll);
                    }

                    if (betweenWords) {
                        lastSpaceWordSpacing = applyWordSpacing ? wordSpacing : 0;
                        lastSpace = pos;
                    }
                    
                    if (!ignoringSpaces && o->style()->collapseWhiteSpace()) {
                        // If we encounter a newline, or if we encounter a
                        // second space, we need to go ahead and break up this
                        // run and enter a mode where we start collapsing spaces.
                        if (currentCharacterIsSpace && previousCharacterIsSpace) {
                            ignoringSpaces = true;
                            
                            // We just entered a mode where we are ignoring
                            // spaces. Create a midpoint to terminate the run
                            // before the second space. 
                            addMidpoint(lineMidpointState, ignoreStart);
                        }
                    }
                } else if (ignoringSpaces) {
                    // Stop ignoring spaces and begin at this
                    // new point.
                    ignoringSpaces = false;
                    lastSpaceWordSpacing = applyWordSpacing ? wordSpacing : 0;
                    lastSpace = pos; // e.g., "Foo    goo", don't add in any of the ignored spaces.
                    addMidpoint(lineMidpointState, InlineIterator(0, o, pos));
                }

#if ENABLE(SVG)
                if (isSVGText && pos > 0) {
                    // Force creation of new InlineBoxes for each absolute positioned character (those that start new text chunks).
                    if (static_cast<RenderSVGInlineText*>(t)->characterStartsNewTextChunk(pos)) {
                        addMidpoint(lineMidpointState, InlineIterator(0, o, pos - 1));
                        addMidpoint(lineMidpointState, InlineIterator(0, o, pos));
                    }
                }
#endif

                if (currentCharacterIsSpace && !previousCharacterIsSpace) {
                    ignoreStart.m_obj = o;
                    ignoreStart.m_pos = pos;
                }

                if (!currentCharacterIsWS && previousCharacterIsWS) {
                    if (autoWrap && o->style()->breakOnlyAfterWhiteSpace())
                        lBreak.moveTo(o, pos, nextBreakable);
                }
                
                if (collapseWhiteSpace && currentCharacterIsSpace && !ignoringSpaces)
                    trailingSpaceObject = o;
                else if (!o->style()->collapseWhiteSpace() || !currentCharacterIsSpace) {
                    trailingSpaceObject = 0;
                    trailingPositionedBoxes.clear();
                }
		  // SAMSUNG CHANGE
		  if (isExtendedUnicode) {
		      pos += 2;
		      len -= 2;
		  } else {
                pos++;
                len--;
		  }
                atStart = false;
            }

            // IMPORTANT: pos is > length here!
            float additionalTmpW = ignoringSpaces ? 0 : textWidth(t, lastSpace, pos - lastSpace, f, width.currentWidth(), isFixedPitch, collapseWhiteSpace) + lastSpaceWordSpacing;
            width.addUncommittedWidth(additionalTmpW + inlineLogicalWidth(o, !appliedStartWidth, true));

            if (!width.fitsOnLine()) {
                if (canHyphenate)
                    tryHyphenating(t, f, style->locale(), style->hyphenationLimitBefore(), style->hyphenationLimitAfter(), lastSpace, pos, width.currentWidth() - additionalTmpW, width.availableWidth(), isFixedPitch, collapseWhiteSpace, lastSpaceWordSpacing, lBreak, nextBreakable, hyphenated);

                if (!hyphenated && lBreak.m_obj && lBreak.m_pos && lBreak.m_obj->isText() && toRenderText(lBreak.m_obj)->textLength() && toRenderText(lBreak.m_obj)->characters()[lBreak.m_pos - 1] == softHyphen && style->hyphens() != HyphensNone)
                    hyphenated = true;

                if (hyphenated)
                    goto end;
            }
        } else
            ASSERT_NOT_REACHED();

        bool checkForBreak = autoWrap;
        if (width.committedWidth() && !width.fitsOnLine() && lBreak.m_obj && currWS == NOWRAP)
            checkForBreak = true;
        else if (next && o->isText() && next->isText() && !next->isBR()) {
            if (autoWrap || (next->style()->autoWrap())) {
                if (currentCharacterIsSpace)
                    checkForBreak = true;
                else {
                    checkForBreak = false;
                    RenderText* nextText = toRenderText(next);
                    if (nextText->textLength()) {
                        UChar c = nextText->characters()[0];
                        if (c == ' ' || c == '\t' || (c == '\n' && !next->preservesNewline()))
                            // If the next item on the line is text, and if we did not end with
                            // a space, then the next text run continues our word (and so it needs to
                            // keep adding to |tmpW|.  Just update and continue.
                            checkForBreak = true;
                    } else if (nextText->isWordBreak())
                        checkForBreak = true;

                    if (!width.fitsOnLine() && !width.committedWidth())
                        width.fitBelowFloats();

                    bool canPlaceOnLine = width.fitsOnLine() || !autoWrapWasEverTrueOnLine;
                    if (canPlaceOnLine && checkForBreak) {
                        width.commit();
                        lBreak.moveToStartOf(next);
                    }
                }
            }
        }

        if (checkForBreak && !width.fitsOnLine()) {
            // if we have floats, try to get below them.
            if (currentCharacterIsSpace && !ignoringSpaces && o->style()->collapseWhiteSpace()) {
                trailingSpaceObject = 0;
                trailingPositionedBoxes.clear();
            }

            if (width.committedWidth())
                goto end;

            width.fitBelowFloats();

            // |width| may have been adjusted because we got shoved down past a float (thus
            // giving us more room), so we need to retest, and only jump to
            // the end label if we still don't fit on the line. -dwh
            if (!width.fitsOnLine())
                goto end;
        }

        if (!o->isFloatingOrPositioned()) {
            last = o;
            if (last->isReplaced() && autoWrap && (!last->isImage() || allowImagesToBreak) && (!last->isListMarker() || toRenderListMarker(last)->isInside())) {
                width.commit();
                lBreak.moveToStartOf(next);
            }
        }

        o = next;
        nextBreakable = -1;

        // Clear out our character space bool, since inline <pre>s don't collapse whitespace
        // with adjacent inline normal/nowrap spans.
        if (!collapseWhiteSpace)
            currentCharacterIsSpace = false;
        
        pos = 0;
        atStart = false;
    }
    
    if (width.fitsOnLine() || lastWS == NOWRAP)
        lBreak.clear();

 end:
    if (lBreak == resolver.position() && (!lBreak.m_obj || !lBreak.m_obj->isBR())) {
        // we just add as much as possible
        if (style()->whiteSpace() == PRE) {
            // FIXME: Don't really understand this case.
            if (pos != 0) {
                // FIXME: This should call moveTo which would clear m_nextBreakablePosition
                // this code as-is is likely wrong.
                lBreak.m_obj = o;
                lBreak.m_pos = pos - 1;
            } else
                lBreak.moveTo(last, last->isText() ? last->length() : 0);
        } else if (lBreak.m_obj) {
            // Don't ever break in the middle of a word if we can help it.
            // There's no room at all. We just have to be on this line,
            // even though we'll spill out.
            lBreak.moveTo(o, pos);
        }
    }

    // make sure we consume at least one char/object.
    if (lBreak == resolver.position())
        lBreak.increment();

    // Sanity check our midpoints.
    checkMidpoints(lineMidpointState, lBreak);
        
    if (trailingSpaceObject) {
        // This object is either going to be part of the last midpoint, or it is going
        // to be the actual endpoint.  In both cases we just decrease our pos by 1 level to
        // exclude the space, allowing it to - in effect - collapse into the newline.
        if (lineMidpointState.numMidpoints % 2) {
            // Find the trailing space object's midpoint.
            int trailingSpaceMidpoint = lineMidpointState.numMidpoints - 1;
            for ( ; trailingSpaceMidpoint >= 0 && lineMidpointState.midpoints[trailingSpaceMidpoint].m_obj != trailingSpaceObject; --trailingSpaceMidpoint) { }
            ASSERT(trailingSpaceMidpoint >= 0);
            lineMidpointState.midpoints[trailingSpaceMidpoint].m_pos--;

            // Now make sure every single trailingPositionedBox following the trailingSpaceMidpoint properly stops and starts 
            // ignoring spaces.
            size_t currentMidpoint = trailingSpaceMidpoint + 1;
            for (size_t i = 0; i < trailingPositionedBoxes.size(); ++i) {
                if (currentMidpoint >= lineMidpointState.numMidpoints) {
                    // We don't have a midpoint for this box yet.
                    InlineIterator ignoreStart(this, trailingPositionedBoxes[i], 0);
                    addMidpoint(lineMidpointState, ignoreStart); // Stop ignoring.
                    addMidpoint(lineMidpointState, ignoreStart); // Start ignoring again.
                } else {
                    ASSERT(lineMidpointState.midpoints[currentMidpoint].m_obj == trailingPositionedBoxes[i]);
                    ASSERT(lineMidpointState.midpoints[currentMidpoint + 1].m_obj == trailingPositionedBoxes[i]);
                }
                currentMidpoint += 2;
            }
        } else if (!lBreak.m_obj && trailingSpaceObject->isText()) {
            // Add a new end midpoint that stops right at the very end.
            RenderText* text = toRenderText(trailingSpaceObject);
            unsigned length = text->textLength();
            unsigned pos = length >= 2 ? length - 2 : UINT_MAX;
            InlineIterator endMid(0, trailingSpaceObject, pos);
            addMidpoint(lineMidpointState, endMid);
            for (size_t i = 0; i < trailingPositionedBoxes.size(); ++i) {
                ignoreStart.m_obj = trailingPositionedBoxes[i];
                ignoreStart.m_pos = 0;
                addMidpoint(lineMidpointState, ignoreStart); // Stop ignoring spaces.
                addMidpoint(lineMidpointState, ignoreStart); // Start ignoring again.
            }
        }
    }

    // We might have made lBreak an iterator that points past the end
    // of the object. Do this adjustment to make it point to the start
    // of the next object instead to avoid confusing the rest of the
    // code.
    if (lBreak.m_pos > 0) {
        lBreak.m_pos--;
        lBreak.increment();
    }

    return lBreak;
}

void RenderBlock::addOverflowFromInlineChildren()
{
    int endPadding = hasOverflowClip() ? paddingEnd() : 0;
    // FIXME: Need to find another way to do this, since scrollbars could show when we don't want them to.
    if (hasOverflowClip() && !endPadding && node() && node()->rendererIsEditable() && node() == node()->rootEditableElement() && style()->isLeftToRightDirection())
        endPadding = 1;
    for (RootInlineBox* curr = firstRootBox(); curr; curr = curr->nextRootBox()) {
        addLayoutOverflow(curr->paddedLayoutOverflowRect(endPadding));
        if (!hasOverflowClip())
            addVisualOverflow(curr->visualOverflowRect(curr->lineTop(), curr->lineBottom()));
    }
}

void RenderBlock::deleteEllipsisLineBoxes()
{
    for (RootInlineBox* curr = firstRootBox(); curr; curr = curr->nextRootBox())
        curr->clearTruncation();
}

void RenderBlock::checkLinesForTextOverflow()
{
    // Determine the width of the ellipsis using the current font.
    // FIXME: CSS3 says this is configurable, also need to use 0x002E (FULL STOP) if horizontal ellipsis is "not renderable"
    TextRun ellipsisRun(&horizontalEllipsis, 1);
    DEFINE_STATIC_LOCAL(AtomicString, ellipsisStr, (&horizontalEllipsis, 1));
    const Font& firstLineFont = firstLineStyle()->font();
    const Font& font = style()->font();
    int firstLineEllipsisWidth = firstLineFont.width(ellipsisRun);
    int ellipsisWidth = (font == firstLineFont) ? firstLineEllipsisWidth : font.width(ellipsisRun);

    // For LTR text truncation, we want to get the right edge of our padding box, and then we want to see
    // if the right edge of a line box exceeds that.  For RTL, we use the left edge of the padding box and
    // check the left edge of the line box to see if it is less
    // Include the scrollbar for overflow blocks, which means we want to use "contentWidth()"
    bool ltr = style()->isLeftToRightDirection();
    for (RootInlineBox* curr = firstRootBox(); curr; curr = curr->nextRootBox()) {
        int blockRightEdge = logicalRightOffsetForLine(curr->y(), curr == firstRootBox());
        int blockLeftEdge = logicalLeftOffsetForLine(curr->y(), curr == firstRootBox());
        int lineBoxEdge = ltr ? curr->x() + curr->logicalWidth() : curr->x();
        if ((ltr && lineBoxEdge > blockRightEdge) || (!ltr && lineBoxEdge < blockLeftEdge)) {
            // This line spills out of our box in the appropriate direction.  Now we need to see if the line
            // can be truncated.  In order for truncation to be possible, the line must have sufficient space to
            // accommodate our truncation string, and no replaced elements (images, tables) can overlap the ellipsis
            // space.
            int width = curr == firstRootBox() ? firstLineEllipsisWidth : ellipsisWidth;
            int blockEdge = ltr ? blockRightEdge : blockLeftEdge;
            if (curr->lineCanAccommodateEllipsis(ltr, blockEdge, lineBoxEdge, width))
                curr->placeEllipsis(ellipsisStr, ltr, blockLeftEdge, blockRightEdge, width);
        }
    }
}

bool RenderBlock::positionNewFloatOnLine(FloatingObject* newFloat, FloatingObject* lastFloatFromPreviousLine, LineWidth& width)
{
    if (!positionNewFloats())
        return false;

    width.shrinkAvailableWidthForNewFloatIfNeeded(newFloat);

    if (!newFloat->m_paginationStrut)
        return true;

    FloatingObjectSet& floatingObjectSet = m_floatingObjects->set();
    ASSERT(floatingObjectSet.last() == newFloat);

    int floatLogicalTop = logicalTopForFloat(newFloat);
    int paginationStrut = newFloat->m_paginationStrut;

    if (floatLogicalTop - paginationStrut != logicalHeight())
        return true;

    FloatingObjectSetIterator it = floatingObjectSet.end();
    --it; // Last float is newFloat, skip that one.
    FloatingObjectSetIterator begin = floatingObjectSet.begin();
    while (it != begin) {
        --it;
        FloatingObject* f = *it;
        if (f == lastFloatFromPreviousLine)
            break;
        if (logicalTopForFloat(f) == logicalHeight()) {
            ASSERT(!f->m_paginationStrut);
            f->m_paginationStrut = paginationStrut;
            RenderBox* o = f->m_renderer;
            setLogicalTopForChild(o, logicalTopForChild(o) + marginBeforeForChild(o) + paginationStrut);
            if (o->isRenderBlock())
                toRenderBlock(o)->setChildNeedsLayout(true, false);
            o->layoutIfNeeded();
            setLogicalTopForFloat(f, logicalTopForFloat(f) + f->m_paginationStrut);
        }
    }

    setLogicalHeight(logicalHeight() + paginationStrut);
    width.updateAvailableWidth();

    return true;
}

}
