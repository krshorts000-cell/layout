#pragma once
#include <Geode/Geode.hpp>
#include "AudioWaveformManager.hpp"
#include "Syncatron.hpp"

using namespace geode::prelude;

enum class TimelineAction {
    Add,
    Delete,
    Toggle
};

class WaveformTimelineNode : public CCNode {
protected:
    float m_width = 440.f;
    float m_height = 85.f;
    float m_songLength = 0.f;
    float m_playbackTime = 0.f;
    float m_windowStart = 0.f;
    float m_windowEnd = 0.f;

    bool m_isZoomed = false;
    bool m_snapToPeak = true;

    std::vector<float> m_samples;
    CCDrawNode* m_drawNode = nullptr;

    std::function<void(float time, bool added)> m_onGuidelineChange;

public:
    static WaveformTimelineNode* create(float width, float height) {
        auto ret = new WaveformTimelineNode();
        if (ret && ret->init(width, height)) {
            ret->autorelease();
            return ret;
        }
        CC_SAFE_DELETE(ret);
        return nullptr;
    }

    bool init(float width, float height) {
        if (!CCNode::init()) return false;
        m_width = width;
        m_height = height;
        this->setContentSize({m_width, m_height});
        this->setAnchorPoint({0.5f, 0.5f});

        m_drawNode = CCDrawNode::create();
        this->addChild(m_drawNode);

        redraw();
        return true;
    }

    void setSamples(const std::vector<float>& samples, float songLength) {
        m_samples = samples;
        m_songLength = std::max(1.f, songLength);
        if (!m_isZoomed || m_windowEnd <= m_windowStart) {
            m_windowStart = 0.f;
            m_windowEnd = m_songLength;
        }
        redraw();
    }

    void setPlaybackTime(float timeSec) {
        m_playbackTime = std::clamp(timeSec, 0.f, m_songLength);
        if (m_isZoomed) {
            float span = m_windowEnd - m_windowStart;
            if (m_playbackTime > m_windowEnd - 0.5f || m_playbackTime < m_windowStart) {
                m_windowStart = std::clamp(m_playbackTime - span / 3.f, 0.f, std::max(0.f, m_songLength - span));
                m_windowEnd = m_windowStart + span;
            }
        }
        redraw();
    }

    float getPlaybackTime() const {
        return m_playbackTime;
    }

    void setSnapToPeak(bool snap) {
        m_snapToPeak = snap;
    }

    bool isSnapToPeak() const {
        return m_snapToPeak;
    }

    float getCurrentSpan() const {
        return std::max(0.1f, m_windowEnd - m_windowStart);
    }

    float getCurrentCenterTime() const {
        return (m_windowStart + m_windowEnd) / 2.f;
    }

    float getTimeAtX(float localX) const {
        float ratio = std::clamp(localX / m_width, 0.f, 1.f);
        return m_windowStart + ratio * getCurrentSpan();
    }

    void zoomAt(float centerTime, float zoomFactor) {
        if (m_songLength <= 0.f) return;

        float currentSpan = getCurrentSpan();
        float newSpan = std::clamp(currentSpan * zoomFactor, 0.5f, m_songLength);

        float ratio = (centerTime - m_windowStart) / (currentSpan > 0.01f ? currentSpan : 1.f);
        ratio = std::clamp(ratio, 0.f, 1.f);

        float newStart = centerTime - ratio * newSpan;
        if (newStart < 0.f) newStart = 0.f;
        float newEnd = newStart + newSpan;
        if (newEnd > m_songLength) {
            newEnd = m_songLength;
            newStart = std::max(0.f, newEnd - newSpan);
        }

        m_windowStart = newStart;
        m_windowEnd = newEnd;
        m_isZoomed = (newSpan < m_songLength - 0.5f);
        redraw();
    }

    void setZoomSpan(float targetSpan) {
        if (m_songLength <= 0.f) return;

        targetSpan = std::clamp(targetSpan, 0.5f, m_songLength);
        float center = getCurrentCenterTime();

        float newStart = center - targetSpan / 2.f;
        if (newStart < 0.f) newStart = 0.f;
        float newEnd = newStart + targetSpan;
        if (newEnd > m_songLength) {
            newEnd = m_songLength;
            newStart = std::max(0.f, newEnd - targetSpan);
        }

        m_windowStart = newStart;
        m_windowEnd = newEnd;
        m_isZoomed = (targetSpan < m_songLength - 0.5f);
        redraw();
    }

    bool isZoomed() const {
        return m_isZoomed;
    }

    void pan(float deltaSeconds) {
        if (m_songLength <= 0.f) return;
        float span = getCurrentSpan();
        m_windowStart = std::clamp(m_windowStart + deltaSeconds, 0.f, std::max(0.f, m_songLength - span));
        m_windowEnd = m_windowStart + span;
        redraw();
    }

    float getWindowStart() const { return m_windowStart; }
    float getWindowEnd() const { return m_windowEnd; }
    float getSongLength() const { return m_songLength; }

    void setOnGuidelineChange(std::function<void(float, bool)> cb) {
        m_onGuidelineChange = std::move(cb);
    }

    float snapTimeToNearestPeak(float t) {
        if (m_samples.empty() || m_songLength <= 0.f) return t;

        int centerIdx = static_cast<int>(t / m_songLength * m_samples.size());
        float span = getCurrentSpan();
        float searchWindow = std::clamp(span * 0.04f, 0.02f, 0.08f);

        int radius = static_cast<int>(searchWindow / m_songLength * m_samples.size());
        radius = std::clamp(radius, 1, 25);

        int bestIdx = centerIdx;
        float maxAmp = -1.f;
        for (int i = centerIdx - radius; i <= centerIdx + radius; ++i) {
            if (i >= 0 && i < static_cast<int>(m_samples.size())) {
                if (m_samples[i] > maxAmp) {
                    maxAmp = m_samples[i];
                    bestIdx = i;
                }
            }
        }

        if (maxAmp > 0.06f) {
            return static_cast<float>(bestIdx) / m_samples.size() * m_songLength;
        }
        return t;
    }

    bool addAtTime(float timeSec) {
        if (m_snapToPeak) {
            timeSec = snapTimeToNearestPeak(timeSec);
        }
        timeSec = std::round(timeSec * 100.f) / 100.f;

        auto& guides = Syncatron::getGuidelines();
        for (float g : guides) {
            if (std::abs(g - timeSec) < 0.015f) return false;
        }
        guides.push_back(timeSec);
        std::sort(guides.begin(), guides.end());
        if (m_onGuidelineChange) m_onGuidelineChange(timeSec, true);
        Syncatron::applyGuidelinesToEditor(0.0f);
        redraw();
        return true;
    }

    bool deleteNearestTo(float timeSec, float tolerance = 0.5f) {
        auto& guides = Syncatron::getGuidelines();
        if (guides.empty()) return false;

        int bestIdx = -1;
        float bestDist = 999999.f;
        for (size_t i = 0; i < guides.size(); ++i) {
            float d = std::abs(guides[i] - timeSec);
            if (d < tolerance && d < bestDist) {
                bestDist = d;
                bestIdx = static_cast<int>(i);
            }
        }
        if (bestIdx >= 0) {
            float removed = guides[bestIdx];
            guides.erase(guides.begin() + bestIdx);
            if (m_onGuidelineChange) m_onGuidelineChange(removed, false);
            Syncatron::applyGuidelinesToEditor(0.0f);
            redraw();
            return true;
        }
        return false;
    }

    bool deleteLast() {
        auto& guides = Syncatron::getGuidelines();
        if (guides.empty()) return false;
        float removed = guides.back();
        guides.pop_back();
        if (m_onGuidelineChange) m_onGuidelineChange(removed, false);
        Syncatron::applyGuidelinesToEditor(0.0f);
        redraw();
        return true;
    }

    bool onTimelineClick(const CCPoint& localPoint, TimelineAction action) {
        if (localPoint.x < 0.f || localPoint.x > m_width ||
            localPoint.y < 0.f || localPoint.y > m_height) {
            return false;
        }

        // Bottom mini-map click: jump viewport
        if (localPoint.y < 9.f && m_songLength > 0.f) {
            float jumpCenter = (localPoint.x / m_width) * m_songLength;
            float span = getCurrentSpan();
            float newStart = std::clamp(jumpCenter - span / 2.f, 0.f, std::max(0.f, m_songLength - span));
            m_windowStart = newStart;
            m_windowEnd = newStart + span;
            redraw();
            return true;
        }

        float span = getCurrentSpan();
        float ratio = localPoint.x / m_width;
        float clickedTime = m_windowStart + ratio * span;

        if (action == TimelineAction::Add) {
            return addAtTime(clickedTime);
        } else if (action == TimelineAction::Delete) {
            float tol = std::max(0.08f, (14.f / m_width) * span);
            return deleteNearestTo(clickedTime, tol);
        } else {
            // Toggle
            float tol = std::max(0.04f, (6.f / m_width) * span);
            if (deleteNearestTo(clickedTime, tol)) {
                return true;
            }
            return addAtTime(clickedTime);
        }
    }

    void redraw() {
        if (!m_drawNode) return;
        m_drawNode->clear();

        float w = m_width;
        float h = m_height;
        float waveH = h - 10.f;

        // 1. Background dark card
        m_drawNode->drawRect(
            ccp(0, 0), ccp(w, h),
            ccc4f(0.05f, 0.06f, 0.10f, 0.96f),
            1.2f,
            ccc4f(0.22f, 0.32f, 0.50f, 0.6f)
        );

        // 2. Waveform center baseline
        m_drawNode->drawSegment(
            ccp(0, 10.f + waveH / 2.f), ccp(w, 10.f + waveH / 2.f),
            0.6f,
            ccc4f(0.18f, 0.25f, 0.38f, 0.6f)
        );

        float span = getCurrentSpan();

        // 3. Audio Waveform bars
        if (!m_samples.empty() && m_songLength > 0.f) {
            float stepX = 1.5f;
            int numBars = static_cast<int>(w / stepX);

            for (int i = 0; i < numBars; ++i) {
                float barX = i * stepX;
                float timeAtBar = m_windowStart + (barX / w) * span;

                float sampleRatio = timeAtBar / m_songLength;
                if (sampleRatio < 0.f || sampleRatio > 1.f) continue;

                int sIdx = static_cast<int>(sampleRatio * m_samples.size());
                sIdx = std::clamp(sIdx, 0, static_cast<int>(m_samples.size()) - 1);

                float amp = m_samples[sIdx];
                float barHeight = std::max(2.f, amp * (waveH - 12.f));
                float yBot = 10.f + (waveH - barHeight) / 2.f;
                float yTop = 10.f + (waveH + barHeight) / 2.f;

                ccColor4F barCol = (timeAtBar <= m_playbackTime)
                    ? ccc4f(0.2f, 0.88f, 0.98f, 0.92f)  // Played (electric cyan)
                    : ccc4f(0.14f, 0.48f, 0.78f, 0.72f); // Unplayed (rich blue)

                m_drawNode->drawSegment(ccp(barX, yBot), ccp(barX, yTop), 0.75f, barCol);
            }
        } else {
            // Placeholder grid bars if song not loaded
            for (float x = 0; x < w; x += 14.f) {
                m_drawNode->drawSegment(ccp(x, 10.f + waveH * 0.35f), ccp(x, 10.f + waveH * 0.65f), 0.5f, ccc4f(0.2f, 0.3f, 0.45f, 0.4f));
            }
        }

        // 4. Guideline vertical lines and diamond heads
        const auto& guides = Syncatron::getGuidelines();
        for (float gTime : guides) {
            if (gTime < m_windowStart || gTime > m_windowEnd) continue;

            float gx = ((gTime - m_windowStart) / span) * w;

            // Orange guideline vertical line
            m_drawNode->drawSegment(
                ccp(gx, 10.f), ccp(gx, h),
                1.3f,
                ccc4f(1.0f, 0.55f, 0.05f, 0.95f)
            );

            // Diamond head at the top
            CCPoint diamond[] = {
                ccp(gx, h - 1.f),
                ccp(gx - 3.5f, h - 7.f),
                ccp(gx, h - 11.f),
                ccp(gx + 3.5f, h - 7.f)
            };
            m_drawNode->drawPolygon(diamond, 4, ccc4f(1.0f, 0.75f, 0.15f, 1.0f), 0.f, ccc4f(0, 0, 0, 0));
        }

        // 5. Playhead white cursor
        if (m_playbackTime >= m_windowStart && m_playbackTime <= m_windowEnd) {
            float cx = ((m_playbackTime - m_windowStart) / span) * w;
            m_drawNode->drawSegment(
                ccp(cx, 10.f), ccp(cx, h),
                1.2f,
                ccc4f(1.0f, 1.0f, 1.0f, 1.0f)
            );

            CCPoint cursorHead[] = {
                ccp(cx - 3.5f, h),
                ccp(cx + 3.5f, h),
                ccp(cx, h - 7.f)
            };
            m_drawNode->drawPolygon(cursorHead, 3, ccc4f(1.0f, 1.0f, 1.0f, 1.0f), 0.f, ccc4f(0, 0, 0, 0));
        }

        // 6. Bottom Mini-Map Overview Track (0px to 9px)
        m_drawNode->drawRect(
            ccp(0, 0), ccp(w, 8.f),
            ccc4f(0.04f, 0.05f, 0.08f, 1.0f),
            0.5f,
            ccc4f(0.15f, 0.20f, 0.30f, 0.5f)
        );

        if (m_songLength > 0.f) {
            for (float gTime : guides) {
                float minix = (gTime / m_songLength) * w;
                m_drawNode->drawSegment(ccp(minix, 1.f), ccp(minix, 7.f), 0.8f, ccc4f(1.0f, 0.6f, 0.1f, 0.8f));
            }

            float vx1 = (m_windowStart / m_songLength) * w;
            float vx2 = (m_windowEnd / m_songLength) * w;
            float vWidth = std::max(4.f, vx2 - vx1);

            m_drawNode->drawRect(
                ccp(vx1, 0.5f), ccp(vx1 + vWidth, 7.5f),
                ccc4f(0.2f, 0.75f, 1.0f, 0.45f),
                0.8f,
                ccc4f(1.0f, 0.85f, 0.3f, 0.95f)
            );
        }
    }
};
