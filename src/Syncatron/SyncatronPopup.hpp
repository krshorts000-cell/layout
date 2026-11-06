#pragma once
#include <Geode/Geode.hpp>
#include <Geode/ui/Popup.hpp>
#include <Geode/ui/BasedButtonSprite.hpp>
#include <Geode/ui/Layout.hpp>
#include "WaveformTimelineNode.hpp"
#include "AudioWaveformManager.hpp"
#include "Syncatron.hpp"

using namespace geode::prelude;

class SyncatronPopup : public geode::Popup {
protected:
    std::function<void()> m_onBuild;

    WaveformTimelineNode* m_waveform = nullptr;
    CCLabelBMFont* m_titleLabel = nullptr;
    CCLabelBMFont* m_infoLabel = nullptr;
    CCLabelBMFont* m_timeLabel = nullptr;
    CCMenuItemSpriteExtra* m_playBtn = nullptr;
    CCMenuItemSpriteExtra* m_snapBtn = nullptr;
    CCMenuItemSpriteExtra* m_zoomBtn = nullptr;

    std::string m_songPath;
    float m_songLength = 0.f;
    bool m_isPlaying = false;

    void updateStatus() {
        int count = static_cast<int>(Syncatron::getGuidelines().size());
        if (m_infoLabel) {
            std::string snapStr = (m_waveform && m_waveform->isSnapToPeak()) ? "ON" : "OFF";
            std::string zoomStr = (m_waveform && m_waveform->isZoomed()) ? "15s View" : "Full View";
            m_infoLabel->setString(
                fmt::format("Guidelines: {} | Snap: {} | {}", count, snapStr, zoomStr).c_str()
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

    void updateZoomButton() {
        if (!m_zoomBtn || !m_waveform) return;
        m_zoomBtn->removeAllChildren();
        bool zoomed = m_waveform->isZoomed();
        const char* bg = zoomed ? "GJ_button_02.png" : "GJ_button_05.png";
        std::string txt = zoomed ? "ZOOM: 15s" : "FULL SONG";
        auto spr = ButtonSprite::create(txt.c_str(), "chatFont.fnt", bg, 0.42f);
        spr->setScale(0.72f);
        m_zoomBtn->addChild(spr);
        m_zoomBtn->setContentSize(spr->getContentSize() * 0.72f);
        spr->setPosition(m_zoomBtn->getContentSize() / 2.f);
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

    void onZoomToggle(CCObject*) {
        if (!m_waveform) return;
        m_waveform->setZoomMode(!m_waveform->isZoomed(), 15.f);
        updateZoomButton();
        updateStatus();
    }

    void onPanLeft(CCObject*) {
        if (!m_waveform) return;
        m_waveform->pan(-5.f);
        updateStatus();
    }

    void onPanRight(CCObject*) {
        if (!m_waveform) return;
        m_waveform->pan(5.f);
        updateStatus();
    }

    void onBrowseOsu(CCObject*) {
        if (Syncatron::pickAndLoadOsuFile()) {
            if (m_waveform) m_waveform->redraw();
            updateStatus();
        }
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

    bool ccTouchBegan(CCTouch* touch, CCEvent* event) override {
        if (m_waveform) {
            auto loc = m_waveform->convertToNodeSpace(touch->getLocation());
            if (m_waveform->onTimelineClick(loc)) {
                updateStatus();
                return true;
            }
        }
        return Popup::ccTouchBegan(touch, event);
    }

    void onClose(CCObject* obj) override {
        auto* engine = FMODAudioEngine::sharedEngine();
        if (engine) {
            engine->stopAllMusic(true);
        }
        Popup::onClose(obj);
    }

    bool setup(float width, float height, std::function<void()> onBuild) {
        if (!Popup::init(width, height)) return false;
        m_onBuild = std::move(onBuild);

        this->setTitle("Music Waveform & Timeline Sync");

        float w = m_size.width;
        float h = m_size.height;

        // Initialize guidelines from current level
        Syncatron::loadGuidelinesFromLevel();

        // 1. TOP HEADER: Track Title & Info
        std::string songTitle = AudioWaveformManager::getCurrentSongTitle();
        if (songTitle.length() > 36) songTitle = songTitle.substr(0, 33) + "...";

        m_titleLabel = CCLabelBMFont::create(songTitle.c_str(), "goldFont.fnt");
        m_titleLabel->setScale(0.52f);
        m_titleLabel->setPosition({w / 2.f, h - 38.f});
        m_mainLayer->addChild(m_titleLabel);

        m_infoLabel = CCLabelBMFont::create("Click on the waveform below to add / remove guidelines!", "chatFont.fnt");
        m_infoLabel->setScale(0.65f);
        m_infoLabel->setColor(ccc3(180, 240, 255));
        m_infoLabel->setPosition({w / 2.f, h - 56.f});
        m_mainLayer->addChild(m_infoLabel);

        // 2. MIDDLE: INTERACTIVE WAVEFORM CANVAS
        float waveW = 420.f;
        float waveH = 80.f;
        m_waveform = WaveformTimelineNode::create(waveW, waveH);
        m_waveform->setPosition({w / 2.f, h - 105.f});
        m_waveform->setOnGuidelineChange([this](float timeSec, bool added) {
            Notification::create(
                fmt::format("{} guide at {:.2f}s", added ? "Added" : "Removed", timeSec),
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

        // 3. TIMELINE CONTROLS ROW: Play/Pause, Time, Zoom, Pan
        auto playMenu = CCMenu::create();
        playMenu->setPosition({w / 2.f, h - 165.f});
        playMenu->setContentSize({420.f, 32.f});
        playMenu->setLayout(RowLayout::create()->setGap(8.f));

        auto playDummy = CCSprite::create();
        m_playBtn = CCMenuItemSpriteExtra::create(playDummy, this, menu_selector(SyncatronPopup::onPlayToggle));
        updatePlayButton();
        playMenu->addChild(m_playBtn);

        m_timeLabel = CCLabelBMFont::create("00:00.00 / 00:00.00", "bigFont.fnt");
        m_timeLabel->setScale(0.35f);
        m_timeLabel->setColor(ccc3(255, 230, 120));
        playMenu->addChild(m_timeLabel);

        auto panLSpr = ButtonSprite::create("< 5s", "chatFont.fnt", "GJ_button_04.png", 0.42f);
        panLSpr->setScale(0.72f);
        auto panLBtn = CCMenuItemSpriteExtra::create(panLSpr, this, menu_selector(SyncatronPopup::onPanLeft));
        playMenu->addChild(panLBtn);

        auto zoomDummy = CCSprite::create();
        m_zoomBtn = CCMenuItemSpriteExtra::create(zoomDummy, this, menu_selector(SyncatronPopup::onZoomToggle));
        updateZoomButton();
        playMenu->addChild(m_zoomBtn);

        auto panRSpr = ButtonSprite::create("5s >", "chatFont.fnt", "GJ_button_04.png", 0.42f);
        panRSpr->setScale(0.72f);
        auto panRBtn = CCMenuItemSpriteExtra::create(panRSpr, this, menu_selector(SyncatronPopup::onPanRight));
        playMenu->addChild(panRBtn);

        auto snapDummy = CCSprite::create();
        m_snapBtn = CCMenuItemSpriteExtra::create(snapDummy, this, menu_selector(SyncatronPopup::onSnapToggle));
        updateSnapButton();
        playMenu->addChild(m_snapBtn);

        playMenu->updateLayout();
        m_mainLayer->addChild(playMenu);

        // 4. SUB ACTION ROW: [ IMPORT .OSU ] [ CLEAR ALL ]
        auto subMenu = CCMenu::create();
        subMenu->setPosition({w / 2.f, h - 200.f});
        subMenu->setContentSize({420.f, 28.f});
        subMenu->setLayout(RowLayout::create()->setGap(10.f));

        auto osuSpr = ButtonSprite::create("IMPORT .OSU MAP", "chatFont.fnt", "GJ_button_05.png", 0.45f);
        osuSpr->setScale(0.75f);
        auto osuBtn = CCMenuItemSpriteExtra::create(osuSpr, this, menu_selector(SyncatronPopup::onBrowseOsu));
        subMenu->addChild(osuBtn);

        auto clearSpr = ButtonSprite::create("CLEAR ALL GUIDES", "chatFont.fnt", "GJ_button_06.png", 0.45f);
        clearSpr->setScale(0.75f);
        auto clearBtn = CCMenuItemSpriteExtra::create(clearSpr, this, menu_selector(SyncatronPopup::onClearClick));
        subMenu->addChild(clearBtn);

        subMenu->updateLayout();
        m_mainLayer->addChild(subMenu);

        // 5. BOTTOM ACTION ROW: [ APPLY GUIDELINES ] [ BUILD SYNC LAYOUT ]
        auto bottomMenu = CCMenu::create();
        bottomMenu->setPosition({w / 2.f, 26.f});
        bottomMenu->setContentSize({420.f, 40.f});
        bottomMenu->setLayout(RowLayout::create()->setGap(12.f));

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
        if (ret && ret->setup(460.f, 280.f, std::move(onBuild))) {
            ret->autorelease();
            return ret;
        }
        CC_SAFE_DELETE(ret);
        return nullptr;
    }
};
