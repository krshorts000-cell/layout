#pragma once
#include <Geode/Geode.hpp>
#include "AudioWaveformManager.hpp"
#include "Syncatron.hpp"

using namespace geode::prelude;

class WaveformTimelineNode : public CCNode {
protected:
    float m_width = 420.f;
    float m_height = 80.f;
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
            // Keep playback cursor in view if zoomed
            if (m_playbackTime > m_windowEnd - 2.f || m_playbackTime < m_windowStart) {
                float span = m_windowEnd - m_windowStart;
                m_windowStart = std::clamp(m_playbackTime - 2.f, 0.f, std::max(0.f, m_songLength - span));
                m_windowEnd = m_windowStart + span;
            }
        }
        redraw();
    }

    void setSnapToPeak(bool snap) {
        m_snapToPeak = snap;
    }

    bool isSnapToPeak() const {
        return m_snapToPeak;
    }

    void setZoomMode(bool zoomed, float windowSeconds = 15.f) {
        m_isZoomed = zoomed;
        if (m_isZoomed) {
            float span = std::min(m_songLength, windowSeconds);
            m_windowStart = std::clamp(m_playbackTime - span / 3.f, 0.f, std::max(0.f, m_songLength - span));
            m_windowEnd = m_windowStart + span;
        } else {
            m_windowStart = 0.f;
            m_windowEnd = m_songLength;
        }
        redraw();
    }

    bool isZoomed() const {
        return m_isZoomed;
    }

    void pan(float deltaSeconds) {
        if (!m_isZoomed) return;
        float span = m_windowEnd - m_windowStart;
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
        int radius = static_cast<int>(0.06f / m_songLength * m_samples.size());
        radius = std::clamp(radius, 1, 15);

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

        // Only snap if significant amplitude
        if (maxAmp > 0.08f) {
            return static_cast<float>(bestIdx) / m_samples.size() * m_songLength;
        }
        return t;
    }

    bool onTimelineClick(const CCPoint& localPoint) {
        if (localPoint.x < 0.f || localPoint.x > m_width ||
            localPoint.y < 0.f || localPoint.y > m_height) {
            return false;
        }

        float span = m_windowEnd - m_windowStart;
        if (span <= 0.01f) span = 1.f;

        float ratio = localPoint.x / m_width;
        float clickedTime = m_windowStart + ratio * span;

        if (m_snapToPeak) {
            clickedTime = snapTimeToNearestPeak(clickedTime);
        }
        clickedTime = std::round(clickedTime * 100.f) / 100.f;

        // Check if guideline exists within tolerance
        float tolTime = std::max(0.04f, (5.f / m_width) * span);
        auto& guides = Syncatron::getGuidelines();

        int foundIdx = -1;
        for (size_t i = 0; i < guides.size(); ++i) {
            if (std::abs(guides[i] - clickedTime) < tolTime) {
                foundIdx = static_cast<int>(i);
                break;
            }
        }

        bool added = false;
        if (foundIdx >= 0) {
            float removedTime = guides[foundIdx];
            guides.erase(guides.begin() + foundIdx);
            added = false;
            if (m_onGuidelineChange) m_onGuidelineChange(removedTime, false);
        } else {
            guides.push_back(clickedTime);
            std::sort(guides.begin(), guides.end());
            added = true;
            if (m_onGuidelineChange) m_onGuidelineChange(clickedTime, true);
        }

        Syncatron::applyGuidelinesToEditor(0.0f);
        redraw();
        return true;
    }

    void redraw() {
        if (!m_drawNode) return;
        m_drawNode->clear();

        float w = m_width;
        float h = m_height;

        // 1. Background dark card
        m_drawNode->drawRect(
            ccp(0, 0), ccp(w, h),
            ccc4f(0.06f, 0.07f, 0.11f, 0.95f),
            1.2f,
            ccc4f(0.25f, 0.35f, 0.55f, 0.6f)
        );

        // 2. Center baseline
        m_drawNode->drawSegment(
            ccp(0, h / 2.f), ccp(w, h / 2.f),
            0.6f,
            ccc4f(0.18f, 0.25f, 0.38f, 0.6f)
        );

        float span = m_windowEnd - m_windowStart;
        if (span <= 0.01f) span = 1.f;

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
                float barHeight = std::max(2.f, amp * (h - 14.f));
                float yBot = (h - barHeight) / 2.f;
                float yTop = (h + barHeight) / 2.f;

                ccColor4F barCol = (timeAtBar <= m_playbackTime)
                    ? ccc4f(0.2f, 0.85f, 0.95f, 0.9f)   // Played (bright cyan)
                    : ccc4f(0.15f, 0.50f, 0.80f, 0.7f); // Unplayed (deep blue)

                m_drawNode->drawSegment(ccp(barX, yBot), ccp(barX, yTop), 0.75f, barCol);
            }
        } else {
            // Placeholder grid bars if song not loaded
            for (float x = 0; x < w; x += 14.f) {
                m_drawNode->drawSegment(ccp(x, h * 0.35f), ccp(x, h * 0.65f), 0.5f, ccc4f(0.2f, 0.3f, 0.45f, 0.4f));
            }
        }

        // 4. Guideline vertical lines and diamond heads
        const auto& guides = Syncatron::getGuidelines();
        for (float gTime : guides) {
            if (gTime < m_windowStart || gTime > m_windowEnd) continue;

            float gx = ((gTime - m_windowStart) / span) * w;

            // Orange guideline vertical line
            m_drawNode->drawSegment(
                ccp(gx, 0), ccp(gx, h),
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
                ccp(cx, 0), ccp(cx, h),
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
    }
};
