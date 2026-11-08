#pragma once
#include <Geode/Geode.hpp>
#include <Geode/ui/Popup.hpp>
#include <Geode/ui/BasedButtonSprite.hpp>
#include <Geode/ui/Layout.hpp>
#include "WaveformTimelineNode.hpp"
#include "AudioWaveformManager.hpp"
#include "Syncatron.hpp"

#ifdef GEODE_IS_WINDOWS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

using namespace geode::prelude;

class SyncatronPopup : public geode::Popup, public CCMouseDelegate {
protected:
    std::function<void()> m_onBuild;

    WaveformTimelineNode* m_waveform = nullptr;
    CCLabelBMFont* m_titleLabel = nullptr;
    CCLabelBMFont* m_infoLabel = nullptr;
    CCLabelBMFont* m_timeLabel = nullptr;
    CCMenuItemSpriteExtra* m_playBtn = nullptr;
    CCMenuItemSpriteExtra* m_snapBtn = nullptr;
    CCMenuItemSpriteExtra* m_modeBtn = nullptr;
    CCMenuItemSpriteExtra* m_sensBtn = nullptr;

    std::string m_songPath;
    float m_songLength = 0.f;
    bool m_isPlaying = false;
    bool m_deleteMode = false;
    int m_beatSensitivity = 1;

    CCPoint m_touchStartPos;
    CCPoint m_touchPrevPos;
    bool m_isDragging = false;
    bool m_touchIsDelete = false;
    bool m_wasRButtonDown = false;

    void updateStatus() {
        int count = static_cast<int>(Syncatron::getGuidelines().size());
        if (m_infoLabel && m_waveform) {
            std::string snapStr = m_waveform->isSnapToPeak() ? "ON" : "OFF";
            std::string modeStr = m_deleteMode ? "DELETE" : "ADD";
            float span = m_waveform->getCurrentSpan();
            float winStart = m_waveform->getWindowStart();
            float winEnd = m_waveform->getWindowEnd();

            m_infoLabel->setString(
                fmt::format("Mode: {} | Span: {:.1f}s ({:02}:{:04.1f} - {:02}:{:04.1f}) | Guides: {} | Snap: {}",
                    modeStr, span,
                    static_cast<int>(winStart) / 60, std::fmod(winStart, 60.f),
                    static_cast<int>(winEnd) / 60, std::fmod(winEnd, 60.f),
                    count, snapStr
                ).c_str()
            );
        }
        updateTimeDisplay(m_waveform ? m_waveform->getWindowStart() : 0.f);
    }

    void updateTimeDisplay(float curT) {
        if (!m_timeLabel) return;
        int curM = static_cast<int>(curT) / 60;
        float curS = std::fmod(curT, 60.f);
        int totM = static_cast<int>(m_songLength) / 60;
        float totS = std::fmod(m_songLength, 60.f);
        m_timeLabel->setString(fmt::format("{:02}:{:05.2f} / {:02}:{:05.2f}", curM, curS, totM, totS).c_str());
    }

    void updateModeButton() {
        if (!m_modeBtn) return;
        m_modeBtn->removeAllChildren();
        const char* bg = m_deleteMode ? "GJ_button_06.png" : "GJ_button_01.png";
        std::string txt = m_deleteMode ? "MODE: DELETE" : "MODE: ADD";
        auto spr = ButtonSprite::create(txt.c_str(), "chatFont.fnt", bg, 0.42f);
        spr->setScale(0.72f);
        m_modeBtn->addChild(spr);
        m_modeBtn->setContentSize(spr->getContentSize() * 0.72f);
        spr->setPosition(m_modeBtn->getContentSize() / 2.f);
    }

    void onModeToggle(CCObject*) {
        m_deleteMode = !m_deleteMode;
        updateModeButton();
        updateStatus();
    }

    void updateSnapButton() {
        if (!m_snapBtn || !m_waveform) return;
        m_snapBtn->removeAllChildren();
        bool snap = m_waveform->isSnapToPeak();
        const char* bg = snap ? "GJ_button_01.png" : "GJ_button_04.png";
        std::string txt = snap ? "SNAP: ON" : "SNAP: OFF";
        auto spr = ButtonSprite::create(txt.c_str(), "chatFont.fnt", bg, 0.42f);
        spr->setScale(0.72f);
        m_snapBtn->addChild(spr);
        m_snapBtn->setContentSize(spr->getContentSize() * 0.72f);
        spr->setPosition(m_snapBtn->getContentSize() / 2.f);
    }

    void updatePlayButton() {
        if (!m_playBtn) return;
        m_playBtn->removeAllChildren();
        const char* icon = m_isPlaying ? "GJ_pauseBtn_001.png" : "GJ_playMusicBtn_001.png";
        auto spr = CCSprite::createWithSpriteFrameName(icon);
        spr->setScale(0.85f);
        m_playBtn->addChild(spr);
        m_playBtn->setContentSize(spr->getContentSize() * 0.85f);
        spr->setPosition(m_playBtn->getContentSize() / 2.f);
    }

    void onPlayToggle(CCObject*) {
        auto* engine = FMODAudioEngine::sharedEngine();
        if (m_isPlaying) {
            engine->pauseMusic(0);
            m_isPlaying = false;
        } else {
            if (!m_songPath.empty()) {
                engine->stopAllMusic(true);
                engine->playMusic(m_songPath, false, 0.f, 0);
                float curT = m_waveform ? m_waveform->getWindowStart() : 0.f;
                engine->setMusicTimeMS(static_cast<unsigned int>(curT * 1000.f), false, 0);
                m_isPlaying = true;
            }
        }
        updatePlayButton();
    }

    void onSnapToggle(CCObject*) {
        if (!m_waveform) return;
        m_waveform->setSnapToPeak(!m_waveform->isSnapToPeak());
        updateSnapButton();
        updateStatus();
    }

    void onZoomPreset(CCObject* sender) {
        if (!m_waveform) return;
        int tag = sender->getTag();
        if (tag == 0) {
            m_waveform->setZoomSpan(m_songLength);
        } else {
            m_waveform->setZoomSpan(static_cast<float>(tag));
        }
        updateStatus();
    }

    void onZoomStep(CCObject* sender) {
        if (!m_waveform) return;
        int tag = sender->getTag();
        float factor = (tag > 0) ? 0.65f : 1.50f;
        m_waveform->zoomAt(m_waveform->getCurrentCenterTime(), factor);
        updateStatus();
    }

    void onPanStep(CCObject* sender) {
        if (!m_waveform) return;
        int tag = sender->getTag();
        float span = m_waveform->getCurrentSpan();
        float delta = (tag > 0) ? (span * 0.35f) : (-span * 0.35f);
        m_waveform->pan(delta);
        updateStatus();
    }

    void onAddAtCursor(CCObject*) {
        if (!m_waveform) return;
        float curT = m_waveform->getPlaybackTime();
        if (m_waveform->addAtTime(curT)) {
            updateStatus();
        }
    }

    void onDeleteAtCursor(CCObject*) {
        if (!m_waveform) return;
        float curT = m_waveform->getPlaybackTime();
        if (m_waveform->deleteNearestTo(curT, 0.6f)) {
            updateStatus();
        }
    }

    void onUndoLast(CCObject*) {
        if (!m_waveform) return;
        if (m_waveform->deleteLast()) {
            updateStatus();
        }
    }

    void onBrowseOsu(CCObject*) {
        if (Syncatron::pickAndLoadOsuFile()) {
            if (m_waveform) m_waveform->redraw();
            updateStatus();
        }
    }

    void updateSensButton() {
        if (!m_sensBtn) return;
        m_sensBtn->removeAllChildren();
        const char* bg = "GJ_button_01.png";
        std::string txt = "SENS: MED";
        if (m_beatSensitivity == 0) {
            bg = "GJ_button_04.png";
            txt = "SENS: LOW";
        } else if (m_beatSensitivity == 2) {
            bg = "GJ_button_02.png";
            txt = "SENS: HIGH";
        }
        auto spr = ButtonSprite::create(txt.c_str(), "chatFont.fnt", bg, 0.40f);
        spr->setScale(0.65f);
        m_sensBtn->addChild(spr);
        m_sensBtn->setContentSize(spr->getContentSize() * 0.65f);
        spr->setPosition(m_sensBtn->getContentSize() / 2.f);
    }

    void onSensToggle(CCObject*) {
        m_beatSensitivity = (m_beatSensitivity + 1) % 3;
        updateSensButton();
    }

    void onAutoBeats(CCObject*) {
        if (!m_waveform) return;
        if (m_waveform->getSamples().empty()) {
            Notification::create("Loading waveform... please wait a moment", NotificationIcon::Info)->show();
            return;
        }
        try {
            int detected = Syncatron::autoDetectBeats(m_waveform->getSamples(), m_waveform->getSongLength(), m_beatSensitivity);
            if (detected > 0) {
                m_waveform->redraw();
                updateStatus();
            }
        } catch (const std::exception& e) {
            log::error("Exception in onAutoBeats: {}", e.what());
            Notification::create("Beat detection failed", NotificationIcon::Error)->show();
        } catch (...) {
            log::error("Unknown error in onAutoBeats");
        }
    }

    void onImportAudacity(CCObject*) {
        if (Syncatron::pickAndImportAudacityLabels()) {
            if (m_waveform) m_waveform->redraw();
            updateStatus();
        }
    }

    void onExportAudacity(CCObject*) {
        Syncatron::pickAndExportAudacityLabels();
    }

    void onClearClick(CCObject*) {
        Syncatron::clearGuidelines();
        if (m_waveform) m_waveform->redraw();
        updateStatus();
        Notification::create("Guidelines cleared", NotificationIcon::Info)->show();
    }

    void onApplyClick(CCObject*) {
        Syncatron::applyGuidelinesToEditor(0.0f);
        Notification::create(
            fmt::format("Applied {} guidelines to level!", Syncatron::getGuidelines().size()),
            NotificationIcon::Success
        )->show();
    }

    void onBuildClick(CCObject*) {
        if (Syncatron::getGuidelines().empty()) {
            Notification::create("Click on the waveform to place guidelines first!", NotificationIcon::Warning)->show();
            return;
        }

        Syncatron::applyGuidelinesToEditor(0.0f);
        Syncatron::applyBeatsToMacro();

        this->onClose(nullptr);
        if (m_onBuild) {
            m_onBuild();
        }
    }

    void updatePlayback(float dt) {
#ifdef GEODE_IS_WINDOWS
        bool isRButtonDown = (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0;
        if (isRButtonDown && !m_wasRButtonDown) {
            auto mousePos = geode::cocos::getMousePos();
            if (m_waveform) {
                auto loc = m_waveform->convertToNodeSpace(mousePos);
                auto size = m_waveform->getContentSize();
                if (loc.x >= 0.f && loc.x <= size.width &&
                    loc.y >= 0.f && loc.y <= size.height) {
                    m_waveform->onTimelineClick(loc, TimelineAction::Delete);
                    updateStatus();
                }
            }
        }
        m_wasRButtonDown = isRButtonDown;
#endif

        auto* engine = FMODAudioEngine::sharedEngine();
        if (m_isPlaying && engine && engine->isMusicPlaying(0)) {
            float t = static_cast<float>(engine->getMusicTimeMS(0)) / 1000.f;
            if (m_waveform) {
                m_waveform->setPlaybackTime(t);
            }
            updateTimeDisplay(t);
        } else if (m_isPlaying) {
            m_isPlaying = false;
            updatePlayButton();
        }
    }

    void scrollWheel(float y, float x) override {
        if (!m_waveform) return;

        auto mousePos = geode::cocos::getMousePos();
        auto nodeLoc = m_waveform->convertToNodeSpace(mousePos);
        auto size = m_waveform->getContentSize();

        if (nodeLoc.x >= 0.f && nodeLoc.x <= size.width &&
            nodeLoc.y >= 0.f && nodeLoc.y <= size.height) {
            float centerT = m_waveform->getTimeAtX(nodeLoc.x);
            if (y > 0.05f) {
                m_waveform->zoomAt(centerT, 0.75f);
            } else if (y < -0.05f) {
                m_waveform->zoomAt(centerT, 1.33f);
            }
            updateStatus();
        }
    }

    bool ccTouchBegan(CCTouch* touch, CCEvent* event) override {
        if (m_waveform) {
            auto loc = m_waveform->convertToNodeSpace(touch->getLocation());
            auto size = m_waveform->getContentSize();
            if (loc.x >= 0.f && loc.x <= size.width &&
                loc.y >= 0.f && loc.y <= size.height) {
                m_touchStartPos = touch->getLocation();
                m_touchPrevPos = touch->getLocation();
                m_isDragging = false;

                bool isRight = false;
                bool isShift = false;
#ifdef GEODE_IS_WINDOWS
                isRight = (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0;
                isShift = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
#endif
                m_touchIsDelete = isRight || isShift || m_deleteMode;
                return true;
            }
        }
        return Popup::ccTouchBegan(touch, event);
    }

    void ccTouchMoved(CCTouch* touch, CCEvent* event) override {
        if (m_waveform) {
            auto loc = touch->getLocation();
            if (loc.getDistance(m_touchStartPos) > 6.f) {
                m_isDragging = true;
                float dx = loc.x - m_touchPrevPos.x;
                float span = m_waveform->getCurrentSpan();
                float dt = -(dx / m_waveform->getContentSize().width) * span;
                m_waveform->pan(dt);
                updateStatus();
            }
            m_touchPrevPos = loc;
        } else {
            Popup::ccTouchMoved(touch, event);
        }
    }

    void ccTouchEnded(CCTouch* touch, CCEvent* event) override {
        if (m_waveform) {
            if (!m_isDragging) {
                auto loc = m_waveform->convertToNodeSpace(touch->getLocation());
                TimelineAction action = m_touchIsDelete ? TimelineAction::Delete : TimelineAction::Add;
                m_waveform->onTimelineClick(loc, action);
                updateStatus();
            }
            m_isDragging = false;
        } else {
            Popup::ccTouchEnded(touch, event);
        }
    }

    void onClose(CCObject* obj) override {
        CCDirector::sharedDirector()->getMouseDispatcher()->removeDelegate(this);
        auto* engine = FMODAudioEngine::sharedEngine();
        if (engine) {
            engine->stopAllMusic(true);
        }
        Popup::onClose(obj);
    }

    bool setup(float width, float height, std::function<void()> onBuild) {
        if (!Popup::init(width, height)) return false;
        m_onBuild = std::move(onBuild);

        this->setTitle("Audacity Sync-a-tron");

        CCDirector::sharedDirector()->getMouseDispatcher()->addDelegate(this);

        float w = m_size.width;
        float h = m_size.height;

        Syncatron::loadGuidelinesFromLevel();

        // 1. TOP HEADER: Track Title & Controls Info
        std::string songTitle = AudioWaveformManager::getCurrentSongTitle();
        if (songTitle.length() > 38) songTitle = songTitle.substr(0, 35) + "...";

        m_titleLabel = CCLabelBMFont::create(songTitle.c_str(), "goldFont.fnt");
        m_titleLabel->setScale(0.50f);
        m_titleLabel->setPosition({w / 2.f, h - 36.f});
        m_mainLayer->addChild(m_titleLabel);

        m_infoLabel = CCLabelBMFont::create("L-Click: ADD | R-Click: DELETE | Scroll: ZOOM", "chatFont.fnt");
        m_infoLabel->setScale(0.58f);
        m_infoLabel->setColor(ccc3(180, 240, 255));
        m_infoLabel->setPosition({w / 2.f, h - 52.f});
        m_mainLayer->addChild(m_infoLabel);

        // 2. INTERACTIVE AUDACITY WAVEFORM & TIME RULER CANVAS
        float waveW = 450.f;
        float waveH = 92.f;
        m_waveform = WaveformTimelineNode::create(waveW, waveH);
        m_waveform->setPosition({w / 2.f, h - 105.f});
        m_waveform->setOnGuidelineChange([this](float timeSec, bool added) {
            Notification::create(
                fmt::format("{} guide at {:.2f}s", added ? "Added" : "Deleted", timeSec),
                added ? NotificationIcon::Success : NotificationIcon::Warning
            )->show();
            this->updateStatus();
        });
        m_mainLayer->addChild(m_waveform);

        // Extract waveform samples from song
        m_songPath = AudioWaveformManager::getCurrentSongPath();
        if (!m_songPath.empty()) {
            AudioWaveformManager::extractSamplesAsync(
                m_songPath,
                static_cast<int>(waveW * 2.f),
                [this](std::vector<float> samples, float length) {
                    m_songLength = length;
                    if (m_waveform) {
                        m_waveform->setSamples(samples, length);
                    }
                    this->updateStatus();
                }
            );
        }

        // 3. AUDACITY AI & LABELS ROW (y = h - 162.f):
        // [ BEAT FINDER ] [ SENS: MED ] [ IMPORT LABELS ] [ EXPORT LABELS ] [ .OSU ]
        auto audacityMenu = CCMenu::create();
        audacityMenu->setPosition({w / 2.f, h - 162.f});
        audacityMenu->setContentSize({450.f, 26.f});
        audacityMenu->setLayout(RowLayout::create()->setGap(6.f));

        auto beatSpr = ButtonSprite::create("BEAT FINDER", "chatFont.fnt", "GJ_button_01.png", 0.40f);
        beatSpr->setScale(0.66f);
        auto beatBtn = CCMenuItemSpriteExtra::create(beatSpr, this, menu_selector(SyncatronPopup::onAutoBeats));
        audacityMenu->addChild(beatBtn);

        auto sensDummy = CCSprite::create();
        m_sensBtn = CCMenuItemSpriteExtra::create(sensDummy, this, menu_selector(SyncatronPopup::onSensToggle));
        updateSensButton();
        audacityMenu->addChild(m_sensBtn);

        auto impSpr = ButtonSprite::create("IMPORT LABELS", "chatFont.fnt", "GJ_button_05.png", 0.40f);
        impSpr->setScale(0.66f);
        auto impBtn = CCMenuItemSpriteExtra::create(impSpr, this, menu_selector(SyncatronPopup::onImportAudacity));
        audacityMenu->addChild(impBtn);

        auto expSpr = ButtonSprite::create("EXPORT LABELS", "chatFont.fnt", "GJ_button_05.png", 0.40f);
        expSpr->setScale(0.66f);
        auto expBtn = CCMenuItemSpriteExtra::create(expSpr, this, menu_selector(SyncatronPopup::onExportAudacity));
        audacityMenu->addChild(expBtn);

        auto osuSpr = ButtonSprite::create(".OSU MAP", "chatFont.fnt", "GJ_button_04.png", 0.40f);
        osuSpr->setScale(0.66f);
        auto osuBtn = CCMenuItemSpriteExtra::create(osuSpr, this, menu_selector(SyncatronPopup::onBrowseOsu));
        audacityMenu->addChild(osuBtn);

        audacityMenu->updateLayout();
        m_mainLayer->addChild(audacityMenu);

        // 4. EDIT & PRECISION ROW (y = h - 192.f):
        // [ MODE: ADD/DEL ] [ + CURSOR ] [ - CURSOR ] [ - UNDO ] [ SNAP ] [ CLEAR ALL ]
        auto editMenu = CCMenu::create();
        editMenu->setPosition({w / 2.f, h - 192.f});
        editMenu->setContentSize({450.f, 26.f});
        editMenu->setLayout(RowLayout::create()->setGap(6.f));

        auto modeDummy = CCSprite::create();
        m_modeBtn = CCMenuItemSpriteExtra::create(modeDummy, this, menu_selector(SyncatronPopup::onModeToggle));
        updateModeButton();
        editMenu->addChild(m_modeBtn);

        auto addCurSpr = ButtonSprite::create("+ CURSOR", "chatFont.fnt", "GJ_button_01.png", 0.40f);
        addCurSpr->setScale(0.66f);
        auto addCurBtn = CCMenuItemSpriteExtra::create(addCurSpr, this, menu_selector(SyncatronPopup::onAddAtCursor));
        editMenu->addChild(addCurBtn);

        auto delCurSpr = ButtonSprite::create("- CURSOR", "chatFont.fnt", "GJ_button_06.png", 0.40f);
        delCurSpr->setScale(0.66f);
        auto delCurBtn = CCMenuItemSpriteExtra::create(delCurSpr, this, menu_selector(SyncatronPopup::onDeleteAtCursor));
        editMenu->addChild(delCurBtn);

        auto undoSpr = ButtonSprite::create("- UNDO", "chatFont.fnt", "GJ_button_05.png", 0.40f);
        undoSpr->setScale(0.66f);
        auto undoBtn = CCMenuItemSpriteExtra::create(undoSpr, this, menu_selector(SyncatronPopup::onUndoLast));
        editMenu->addChild(undoBtn);

        auto snapDummy = CCSprite::create();
        m_snapBtn = CCMenuItemSpriteExtra::create(snapDummy, this, menu_selector(SyncatronPopup::onSnapToggle));
        updateSnapButton();
        editMenu->addChild(m_snapBtn);

        auto clearSpr = ButtonSprite::create("CLEAR ALL", "chatFont.fnt", "GJ_button_06.png", 0.40f);
        clearSpr->setScale(0.66f);
        auto clearBtn = CCMenuItemSpriteExtra::create(clearSpr, this, menu_selector(SyncatronPopup::onClearClick));
        editMenu->addChild(clearBtn);

        editMenu->updateLayout();
        m_mainLayer->addChild(editMenu);

        // 5. PLAYBACK & PAN ROW (y = h - 222.f):
        // [ Play/Pause ] [ << PAN ] Time Readout [ PAN >> ] [ - ZOOM ] [ + ZOOM ]
        auto playMenu = CCMenu::create();
        playMenu->setPosition({w / 2.f, h - 222.f});
        playMenu->setContentSize({450.f, 26.f});
        playMenu->setLayout(RowLayout::create()->setGap(6.f));

        auto playDummy = CCSprite::create();
        m_playBtn = CCMenuItemSpriteExtra::create(playDummy, this, menu_selector(SyncatronPopup::onPlayToggle));
        updatePlayButton();
        playMenu->addChild(m_playBtn);

        auto panLSpr = ButtonSprite::create("<< PAN", "chatFont.fnt", "GJ_button_04.png", 0.40f);
        panLSpr->setScale(0.65f);
        auto panLBtn = CCMenuItemSpriteExtra::create(panLSpr, this, menu_selector(SyncatronPopup::onPanStep));
        panLBtn->setTag(-1);
        playMenu->addChild(panLBtn);

        m_timeLabel = CCLabelBMFont::create("00:00.00 / 00:00.00", "bigFont.fnt");
        m_timeLabel->setScale(0.33f);
        m_timeLabel->setColor(ccc3(255, 230, 120));
        playMenu->addChild(m_timeLabel);

        auto panRSpr = ButtonSprite::create("PAN >>", "chatFont.fnt", "GJ_button_04.png", 0.40f);
        panRSpr->setScale(0.65f);
        auto panRBtn = CCMenuItemSpriteExtra::create(panRSpr, this, menu_selector(SyncatronPopup::onPanStep));
        panRBtn->setTag(1);
        playMenu->addChild(panRBtn);

        auto zoomOutSpr = ButtonSprite::create("- ZOOM", "chatFont.fnt", "GJ_button_04.png", 0.40f);
        zoomOutSpr->setScale(0.65f);
        auto zoomOutBtn = CCMenuItemSpriteExtra::create(zoomOutSpr, this, menu_selector(SyncatronPopup::onZoomStep));
        zoomOutBtn->setTag(-1);
        playMenu->addChild(zoomOutBtn);

        auto zoomInSpr = ButtonSprite::create("+ ZOOM", "chatFont.fnt", "GJ_button_04.png", 0.40f);
        zoomInSpr->setScale(0.65f);
        auto zoomInBtn = CCMenuItemSpriteExtra::create(zoomInSpr, this, menu_selector(SyncatronPopup::onZoomStep));
        zoomInBtn->setTag(1);
        playMenu->addChild(zoomInBtn);

        playMenu->updateLayout();
        m_mainLayer->addChild(playMenu);

        // 6. ZOOM PRESETS ROW (y = h - 252.f):
        // [ FULL ] [ 30s ] [ 10s ] [ 3s ] [ 1s ]
        auto presetMenu = CCMenu::create();
        presetMenu->setPosition({w / 2.f, h - 252.f});
        presetMenu->setContentSize({450.f, 24.f});
        presetMenu->setLayout(RowLayout::create()->setGap(8.f));

        std::vector<std::pair<const char*, int>> presets = {
            {"FULL SONG", 0},
            {"30 SECONDS", 30},
            {"10 SECONDS", 10},
            {"3 SECONDS", 3},
            {"1 SECOND", 1}
        };
        for (auto const& [name, sec] : presets) {
            auto pSpr = ButtonSprite::create(name, "chatFont.fnt", (sec == 0) ? "GJ_button_05.png" : "GJ_button_04.png", 0.40f);
            pSpr->setScale(0.65f);
            auto pBtn = CCMenuItemSpriteExtra::create(pSpr, this, menu_selector(SyncatronPopup::onZoomPreset));
            pBtn->setTag(sec);
            presetMenu->addChild(pBtn);
        }

        presetMenu->updateLayout();
        m_mainLayer->addChild(presetMenu);

        // 7. BOTTOM ACTION ROW: [ APPLY TO EDITOR ] [ BUILD SYNC LAYOUT ] (y = 26.f)
        auto bottomMenu = CCMenu::create();
        bottomMenu->setPosition({w / 2.f, 26.f});
        bottomMenu->setContentSize({450.f, 38.f});
        bottomMenu->setLayout(RowLayout::create()->setGap(14.f));

        auto applySpr = ButtonSprite::create("APPLY TO EDITOR", "goldFont.fnt", "GJ_button_02.png", 0.65f);
        applySpr->setScale(0.85f);
        auto applyBtn = CCMenuItemSpriteExtra::create(applySpr, this, menu_selector(SyncatronPopup::onApplyClick));
        bottomMenu->addChild(applyBtn);

        auto buildSpr = ButtonSprite::create("BUILD SYNC LAYOUT", "goldFont.fnt", "GJ_button_01.png", 0.65f);
        buildSpr->setScale(0.85f);
        auto buildBtn = CCMenuItemSpriteExtra::create(buildSpr, this, menu_selector(SyncatronPopup::onBuildClick));
        bottomMenu->addChild(buildBtn);

        bottomMenu->updateLayout();
        m_mainLayer->addChild(bottomMenu);

        this->schedule(schedule_selector(SyncatronPopup::updatePlayback), 1.f / 60.f);

        updateStatus();
        return true;
    }

public:
    static SyncatronPopup* create(std::function<void()> onBuild) {
        auto ret = new SyncatronPopup();
        if (ret && ret->setup(480.f, 328.f, std::move(onBuild))) {
            ret->autorelease();
            return ret;
        }
        CC_SAFE_DELETE(ret);
        return nullptr;
    }
};
