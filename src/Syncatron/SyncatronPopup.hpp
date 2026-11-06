#pragma once
#include <Geode/Geode.hpp>
#include <Geode/ui/Popup.hpp>
#include <Geode/ui/BasedButtonSprite.hpp>
#include <Geode/ui/Layout.hpp>
#include "Syncatron.hpp"

using namespace geode::prelude;

class SyncatronPopup : public geode::Popup {
protected:
    std::function<void()> m_onBuild;

    CCLabelBMFont* m_fileLabel = nullptr;
    CCLabelBMFont* m_titleLabel = nullptr;
    CCLabelBMFont* m_notesLabel = nullptr;
    CCLabelBMFont* m_offsetLabel = nullptr;

    void updateDisplay() {
        auto& meta = Syncatron::getMetadata();
        if (m_fileLabel) {
            std::string fn = meta.fileName.empty() ? "None (Click Browse)" : meta.fileName;
            if (fn.length() > 32) fn = fn.substr(0, 29) + "...";
            m_fileLabel->setString(fmt::format("File: {}", fn).c_str());
        }
        if (m_titleLabel) {
            std::string t = meta.title.empty() ? "No Track Selected" : (meta.artist + " - " + meta.title);
            if (t.length() > 38) t = t.substr(0, 35) + "...";
            m_titleLabel->setString(t.c_str());
        }
        if (m_notesLabel) {
            if (meta.totalNotes > 0) {
                m_notesLabel->setString(fmt::format("Notes: {} (~{:.1f}s)", meta.totalNotes, meta.durationSeconds).c_str());
            } else {
                m_notesLabel->setString("Notes: 0 (Load a .osu beatmap)");
            }
        }
        if (m_offsetLabel) {
            float off = Syncatron::getOffset();
            int ms = static_cast<int>(std::round(off * 1000.f));
            m_offsetLabel->setString(fmt::format("Offset: {:+.2f}s ({:+d} ms)", off, ms).c_str());
        }
    }

    void onBrowseClick(CCObject*) {
        if (Syncatron::pickAndLoadOsuFile()) {
            updateDisplay();
        }
    }

    void onOffsetStep(CCObject* sender) {
        int tag = sender->getTag();
        if (tag == 0) {
            Syncatron::setOffset(0.0f);
        } else {
            float delta = tag / 1000.f; // tag in milliseconds
            Syncatron::adjustOffset(delta);
        }
        Syncatron::applyGuidelinesToEditor(0.0f);
        updateDisplay();
    }

    void onApplyGuidelinesClick(CCObject*) {
        if (Syncatron::applyGuidelinesToEditor(0.0f)) {
            Notification::create(
                fmt::format("Sync-a-tron: Applied {} guidelines!", Syncatron::getActiveBeats().size()),
                NotificationIcon::Success
            )->show();
        } else {
            Notification::create("Failed to apply guidelines!", NotificationIcon::Error)->show();
        }
    }

    void onClearClick(CCObject*) {
        Syncatron::clearGuidelines();
        Syncatron::clearActiveBeats();
        updateDisplay();
        Notification::create("Sync-a-tron: Guidelines cleared", NotificationIcon::Info)->show();
    }

    void onBuildClick(CCObject*) {
        if (!Syncatron::hasActiveBeats()) {
            Notification::create("Load a .osu beatmap first!", NotificationIcon::Warning)->show();
            return;
        }

        Syncatron::applyGuidelinesToEditor(0.0f);
        Syncatron::applyBeatsToMacro();

        this->onClose(nullptr);
        if (m_onBuild) {
            m_onBuild();
        }
    }

    bool setup(float width, float height, std::function<void()> onBuild) {
        if (!Popup::init(width, height)) return false;
        m_onBuild = std::move(onBuild);

        this->setTitle("Sync-a-tron (osu! Sync)");

        float w = m_size.width;
        float h = m_size.height;

        // 1. TOP CARD: BEATMAP DETAILS
        auto cardBg = CCScale9Sprite::create("square02_small.png");
        cardBg->setContentSize({360.f, 75.f});
        cardBg->setPosition({w / 2.f, h - 75.f});
        cardBg->setOpacity(90);
        cardBg->setColor(ccc3(20, 20, 30));
        m_mainLayer->addChild(cardBg);

        m_fileLabel = CCLabelBMFont::create("File: None", "bigFont.fnt");
        m_fileLabel->setScale(0.38f);
        m_fileLabel->setColor(ccc3(255, 220, 100));
        m_fileLabel->setPosition({w / 2.f, h - 52.f});
        m_mainLayer->addChild(m_fileLabel);

        m_titleLabel = CCLabelBMFont::create("No Track Selected", "goldFont.fnt");
        m_titleLabel->setScale(0.42f);
        m_titleLabel->setPosition({w / 2.f, h - 72.f});
        m_mainLayer->addChild(m_titleLabel);

        m_notesLabel = CCLabelBMFont::create("Notes: 0", "chatFont.fnt");
        m_notesLabel->setScale(0.65f);
        m_notesLabel->setColor(ccc3(180, 240, 255));
        m_notesLabel->setPosition({w / 2.f, h - 94.f});
        m_mainLayer->addChild(m_notesLabel);

        // 2. BROWSE BUTTON
        auto browseMenu = CCMenu::create();
        browseMenu->setPosition({w / 2.f, h - 132.f});
        browseMenu->setContentSize({360.f, 32.f});
        browseMenu->setLayout(RowLayout::create()->setGap(8.f));

        auto browseSpr = ButtonSprite::create("BROWSE .OSU BEATMAP", "bigFont.fnt", "GJ_button_01.png", 0.5f);
        browseSpr->setScale(0.85f);
        auto browseBtn = CCMenuItemSpriteExtra::create(browseSpr, this, menu_selector(SyncatronPopup::onBrowseClick));
        browseMenu->addChild(browseBtn);
        browseMenu->updateLayout();
        m_mainLayer->addChild(browseMenu);

        // 3. OFFSET CONTROLS
        m_offsetLabel = CCLabelBMFont::create("Offset: +0.00s (0 ms)", "goldFont.fnt");
        m_offsetLabel->setScale(0.42f);
        m_offsetLabel->setPosition({w / 2.f, h - 165.f});
        m_mainLayer->addChild(m_offsetLabel);

        auto offsetMenu = CCMenu::create();
        offsetMenu->setPosition({w / 2.f, h - 192.f});
        offsetMenu->setContentSize({360.f, 28.f});
        offsetMenu->setLayout(RowLayout::create()->setGap(6.f));

        std::vector<std::pair<const char*, int>> steps = {
            {"-50ms", -50},
            {"-10ms", -10},
            {"RESET", 0},
            {"+10ms", +10},
            {"+50ms", +50}
        };

        for (auto const& [name, ms] : steps) {
            const char* bg = (ms == 0) ? "GJ_button_05.png" : "GJ_button_04.png";
            auto btnSpr = ButtonSprite::create(name, "chatFont.fnt", bg, 0.42f);
            btnSpr->setScale(0.72f);
            auto btn = CCMenuItemSpriteExtra::create(btnSpr, this, menu_selector(SyncatronPopup::onOffsetStep));
            btn->setTag(ms);
            offsetMenu->addChild(btn);
        }
        offsetMenu->updateLayout();
        m_mainLayer->addChild(offsetMenu);

        // 4. BOTTOM ACTION ROW
        auto bottomMenu = CCMenu::create();
        bottomMenu->setPosition({w / 2.f, 28.f});
        bottomMenu->setContentSize({370.f, 40.f});
        bottomMenu->setLayout(RowLayout::create()->setGap(10.f));

        auto applySpr = ButtonSprite::create("APPLY GUIDES", "goldFont.fnt", "GJ_button_02.png", 0.65f);
        applySpr->setScale(0.82f);
        auto applyBtn = CCMenuItemSpriteExtra::create(applySpr, this, menu_selector(SyncatronPopup::onApplyGuidelinesClick));
        bottomMenu->addChild(applyBtn);

        auto buildSpr = ButtonSprite::create("BUILD LAYOUT", "goldFont.fnt", "GJ_button_01.png", 0.65f);
        buildSpr->setScale(0.82f);
        auto buildBtn = CCMenuItemSpriteExtra::create(buildSpr, this, menu_selector(SyncatronPopup::onBuildClick));
        bottomMenu->addChild(buildBtn);

        auto clearSpr = ButtonSprite::create("CLEAR", "chatFont.fnt", "GJ_button_06.png", 0.55f);
        clearSpr->setScale(0.82f);
        auto clearBtn = CCMenuItemSpriteExtra::create(clearSpr, this, menu_selector(SyncatronPopup::onClearClick));
        bottomMenu->addChild(clearBtn);

        bottomMenu->updateLayout();
        m_mainLayer->addChild(bottomMenu);

        updateDisplay();
        return true;
    }

public:
    static SyncatronPopup* create(std::function<void()> onBuild) {
        auto ret = new SyncatronPopup();
        if (ret && ret->setup(390.f, 260.f, std::move(onBuild))) {
            ret->autorelease();
            return ret;
        }
        CC_SAFE_DELETE(ret);
        return nullptr;
    }
};
