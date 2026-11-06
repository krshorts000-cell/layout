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
    CCNode* m_rulerContainer = nullptr;
    std::vector<CCLabelBMFont*> m_rulerLabels;

    std::function<void(float time, bool added)> m_onGuidelineChange;

public:
    const std::vector<float>& getSamples() const {
        return m_samples;
    }

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

        m_rulerContainer = CCNode::create();
        this->addChild(m_rulerContainer, 10);

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
        float miniH = 10.f;
        float rulerH = 14.f;
        float waveH = h - miniH - rulerH;
        float waveY = miniH;
        float rulerY = h - rulerH;
        float centerY = waveY + waveH / 2.f;

        // 1. Audacity Waveform Background & Border
        m_drawNode->drawRect(
            ccp(0, waveY), ccp(w, rulerY),
            ccc4f(0.08f, 0.09f, 0.13f, 0.98f),
            1.0f,
            ccc4f(0.18f, 0.22f, 0.28f, 0.8f)
        );

        // 2. Audacity Centerline (0.0 line)
        m_drawNode->drawSegment(
            ccp(0, centerY), ccp(w, centerY),
            0.8f,
            ccc4f(0.25f, 0.32f, 0.42f, 0.7f)
        );

        // Grid lines at +/- 0.5 amplitude
        m_drawNode->drawSegment(ccp(0, waveY + waveH * 0.25f), ccp(w, waveY + waveH * 0.25f), 0.4f, ccc4f(0.13f, 0.16f, 0.22f, 0.5f));
        m_drawNode->drawSegment(ccp(0, waveY + waveH * 0.75f), ccp(w, waveY + waveH * 0.75f), 0.4f, ccc4f(0.13f, 0.16f, 0.22f, 0.5f));

        float span = getCurrentSpan();

        // 3. Audio Waveform bars (Two-tone Audacity Peak + RMS)
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
                if (amp <= 0.005f) continue;

                // Outer Peak
                float peakH = std::max(1.5f, amp * (waveH - 4.f));
                float yBot = centerY - peakH / 2.f;
                float yTop = centerY + peakH / 2.f;

                bool isPlayed = (timeAtBar <= m_playbackTime);
                ccColor4F peakCol = isPlayed
                    ? ccc4f(0.15f, 0.82f, 0.95f, 0.90f)
                    : ccc4f(0.18f, 0.44f, 0.72f, 0.80f);

                m_drawNode->drawSegment(ccp(barX, yBot), ccp(barX, yTop), 0.8f, peakCol);

                // Inner RMS Core
                float rmsH = std::pow(amp, 1.35f) * (waveH - 4.f) * 0.7f;
                if (rmsH >= 1.5f) {
                    float rmsBot = centerY - rmsH / 2.f;
                    float rmsTop = centerY + rmsH / 2.f;
                    ccColor4F rmsCol = isPlayed
                        ? ccc4f(0.70f, 0.95f, 1.0f, 0.95f)
                        : ccc4f(0.38f, 0.68f, 0.98f, 0.95f);
                    m_drawNode->drawSegment(ccp(barX, rmsBot), ccp(barX, rmsTop), 0.7f, rmsCol);
                }
            }
        } else {
            // Idle placeholder
            for (float x = 0; x < w; x += 14.f) {
                m_drawNode->drawSegment(ccp(x, centerY - 6.f), ccp(x, centerY + 6.f), 0.5f, ccc4f(0.18f, 0.22f, 0.32f, 0.4f));
            }
        }

        // 4. Audacity Top Time Ruler
        m_drawNode->drawRect(
            ccp(0, rulerY), ccp(w, h),
            ccc4f(0.12f, 0.14f, 0.18f, 1.0f),
            0.6f,
            ccc4f(0.24f, 0.28f, 0.36f, 0.9f)
        );

        float majorInterval = 10.0f;
        float minorInterval = 2.0f;
        if (span <= 1.5f) {
            majorInterval = 0.25f;
            minorInterval = 0.05f;
        } else if (span <= 3.5f) {
            majorInterval = 0.5f;
            minorInterval = 0.1f;
        } else if (span <= 8.0f) {
            majorInterval = 1.0f;
            minorInterval = 0.25f;
        } else if (span <= 20.0f) {
            majorInterval = 2.0f;
            minorInterval = 0.5f;
        } else if (span <= 60.0f) {
            majorInterval = 5.0f;
            minorInterval = 1.0f;
        } else if (span <= 120.0f) {
            majorInterval = 10.0f;
            minorInterval = 2.0f;
        } else {
            majorInterval = 30.0f;
            minorInterval = 5.0f;
        }

        for (auto* lbl : m_rulerLabels) {
            lbl->setVisible(false);
        }
        size_t lblIdx = 0;

        float firstMinor = std::floor(m_windowStart / minorInterval) * minorInterval;
        for (float t = firstMinor; t <= m_windowEnd + minorInterval; t += minorInterval) {
            if (t < m_windowStart || t > m_windowEnd) continue;
            float tx = ((t - m_windowStart) / span) * w;

            bool isMajor = std::abs(std::fmod(t + 0.0001f, majorInterval)) < 0.001f ||
                           std::abs(std::fmod(t + 0.0001f, majorInterval) - majorInterval) < 0.001f;

            if (isMajor) {
                m_drawNode->drawSegment(ccp(tx, rulerY), ccp(tx, h), 0.9f, ccc4f(0.65f, 0.72f, 0.85f, 0.9f));

                if (m_rulerContainer) {
                    if (lblIdx >= m_rulerLabels.size()) {
                        auto* lbl = CCLabelBMFont::create("", "chatFont.fnt");
                        lbl->setScale(0.30f);
                        lbl->setColor(ccc3(190, 205, 225));
                        lbl->setAnchorPoint({0.0f, 1.0f});
                        m_rulerContainer->addChild(lbl);
                        m_rulerLabels.push_back(lbl);
                    }
                    auto* lbl = m_rulerLabels[lblIdx++];

                    std::string timeStr;
                    int m = static_cast<int>(t) / 60;
                    float s = std::fmod(t, 60.0f);
                    if (majorInterval < 1.0f) {
                        timeStr = fmt::format("{:02}:{:04.1f}", m, s);
                    } else {
                        timeStr = fmt::format("{:02}:{:02.0f}", m, s);
                    }
                    lbl->setString(timeStr.c_str());
                    lbl->setPosition({std::clamp(tx + 2.f, 2.f, w - 30.f), h - 1.f});
                    lbl->setVisible(true);
                }
            } else {
                m_drawNode->drawSegment(ccp(tx, rulerY), ccp(tx, rulerY + 5.f), 0.5f, ccc4f(0.40f, 0.45f, 0.55f, 0.6f));
            }
        }

        // 5. Guideline vertical lines and diamond heads
        const auto& guides = Syncatron::getGuidelines();
        for (float gTime : guides) {
            if (gTime < m_windowStart || gTime > m_windowEnd) continue;

            float gx = ((gTime - m_windowStart) / span) * w;

            m_drawNode->drawSegment(
                ccp(gx, waveY), ccp(gx, rulerY),
                1.4f,
                ccc4f(1.0f, 0.55f, 0.05f, 0.95f)
            );

            CCPoint diamond[] = {
                ccp(gx, rulerY - 1.f),
                ccp(gx - 3.5f, rulerY - 6.f),
                ccp(gx, rulerY - 10.f),
                ccp(gx + 3.5f, rulerY - 6.f)
            };
            m_drawNode->drawPolygon(diamond, 4, ccc4f(1.0f, 0.78f, 0.15f, 1.0f), 0.f, ccc4f(0, 0, 0, 0));
        }

        // 6. Playhead cursor (Audacity white playback line & pointer)
        if (m_playbackTime >= m_windowStart && m_playbackTime <= m_windowEnd) {
            float cx = ((m_playbackTime - m_windowStart) / span) * w;
            m_drawNode->drawSegment(
                ccp(cx, waveY), ccp(cx, h),
                1.3f,
                ccc4f(1.0f, 1.0f, 1.0f, 1.0f)
            );

            CCPoint cursorHead[] = {
                ccp(cx - 4.f, h),
                ccp(cx + 4.f, h),
                ccp(cx, h - 7.f)
            };
            m_drawNode->drawPolygon(cursorHead, 3, ccc4f(1.0f, 1.0f, 1.0f, 1.0f), 0.f, ccc4f(0, 0, 0, 0));
        }

        // 7. Bottom Mini-Map Overview Track (0px to 9px)
        m_drawNode->drawRect(
            ccp(0, 0), ccp(w, 8.f),
            ccc4f(0.05f, 0.06f, 0.09f, 1.0f),
            0.5f,
            ccc4f(0.18f, 0.22f, 0.30f, 0.6f)
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
