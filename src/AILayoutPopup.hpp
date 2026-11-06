#pragma once
#include <Geode/Geode.hpp>
#include <Geode/ui/Popup.hpp>
#include <Geode/ui/BasedButtonSprite.hpp>
#include <Geode/ui/Layout.hpp>
#include "Syncatron/Syncatron.hpp"

using namespace geode::prelude;

class AILayoutPopup : public geode::Popup {
protected:
    std::function<void()> m_onGenerate;
    std::function<void()> m_onLiveBuild;
    std::string m_selectedDifficulty;
    std::string m_selectedStyle;
    int m_clicksCount = 120;
    int m_timingFrames = 4;
    float m_obstacleDist = 21.f;
    bool m_fillBlocks = true;
    int m_fillDepth = 1;
    bool m_clickOnly = false;
    std::string m_patternMode = "Turn Pillars";

    CCLabelBMFont* m_lengthLabel = nullptr;
    CCLabelBMFont* m_distLabel = nullptr;
    CCMenuItemSpriteExtra* m_blocksToggleBtn = nullptr;
    CCMenuItemSpriteExtra* m_clickOnlyBtn = nullptr;
    CCMenuItemSpriteExtra* m_patternModeBtn = nullptr;
    std::vector<std::pair<int, CCMenuItemSpriteExtra*>> m_depthButtons;
    std::vector<std::pair<std::string, CCMenuItemSpriteExtra*>> m_diffButtons;
    std::vector<std::pair<std::string, CCMenuItemSpriteExtra*>> m_styleButtons;

    void updateLengthDisplay() {
        if (!m_lengthLabel) return;
        float seconds = m_clicksCount / 4.f;
        m_lengthLabel->setString(fmt::format("{} Clicks (~{:.0f}s)", m_clicksCount, seconds).c_str());
    }

    void updateDistanceDisplay() {
        if (!m_distLabel) return;
        m_distLabel->setString(fmt::format("{} Frames (~{:.0f}px)", m_timingFrames, m_obstacleDist).c_str());
    }

    void refreshDifficultyButtons() {
        for (auto& [diffName, btn] : m_diffButtons) {
            bool active = (m_selectedDifficulty == diffName);
            ccColor3B col = active ? ccc3(255, 255, 255) : ccc3(120, 120, 120);
            btn->setColor(col);
            if (auto rgba = typeinfo_cast<CCRGBAProtocol*>(btn->getNormalImage())) {
                rgba->setColor(col);
            }
            btn->setScale(active ? 0.82f : 0.72f);
        }
    }

    void refreshStyleButtons() {
        for (auto& [styleName, btn] : m_styleButtons) {
            bool active = (m_selectedStyle == styleName);
            ccColor3B col = active ? ccc3(255, 255, 255) : ccc3(120, 120, 120);
            btn->setColor(col);
            if (auto rgba = typeinfo_cast<CCRGBAProtocol*>(btn->getNormalImage())) {
                rgba->setColor(col);
            }
            btn->setScale(active ? 0.82f : 0.72f);
        }
    }

    void onSelectDifficulty(CCObject* sender) {
        int tag = sender->getTag();
        if (tag == 0) {
            m_selectedDifficulty = "Easy";
            m_timingFrames = 8;
            m_obstacleDist = 42.f;
        }
        else if (tag == 1) {
            m_selectedDifficulty = "Medium";
            m_timingFrames = 5;
            m_obstacleDist = 26.f;
        }
        else if (tag == 2) {
            m_selectedDifficulty = "Hard";
            m_timingFrames = 3;
            m_obstacleDist = 16.f;
        }
        else if (tag == 3) {
            m_selectedDifficulty = "Extreme Demon";
            m_timingFrames = 1;
            m_obstacleDist = 6.f;
        }
        refreshDifficultyButtons();
        updateDistanceDisplay();
    }

    void onSelectStyle(CCObject* sender) {
        int tag = sender->getTag();
        if (tag == 0) m_selectedStyle = "Flow & Rhythmic";
        else if (tag == 1) m_selectedStyle = "Tech & Precision";
        else if (tag == 2) m_selectedStyle = "Sweeping & Wide";
        else if (tag == 3) m_selectedStyle = "Fast Paced";
        refreshStyleButtons();
    }

    void onStepMinus(CCObject*) {
        m_clicksCount = std::max(20, m_clicksCount - 20);
        updateLengthDisplay();
    }

    void onStepPlus(CCObject*) {
        m_clicksCount = std::min(500, m_clicksCount + 20);
        updateLengthDisplay();
    }

    void onPresetClick(CCObject* sender) {
        m_clicksCount = sender->getTag();
        updateLengthDisplay();
    }

    void onDistMinus(CCObject*) {
        m_timingFrames = std::max(1, m_timingFrames - 1);
        m_obstacleDist = m_timingFrames * 5.2f;
        updateDistanceDisplay();
    }

    void onDistPlus(CCObject*) {
        m_timingFrames = std::min(15, m_timingFrames + 1);
        m_obstacleDist = m_timingFrames * 5.2f;
        updateDistanceDisplay();
    }

    void onDistPresetClick(CCObject* sender) {
        m_timingFrames = sender->getTag();
        m_obstacleDist = m_timingFrames * 5.2f;
        updateDistanceDisplay();
    }

    void updateBlocksButton() {
        if (!m_blocksToggleBtn) return;
        m_blocksToggleBtn->removeAllChildren();

        std::string text = m_fillBlocks ? "BLOCKS: ON" : "BLOCKS: OFF";
        const char* bg = m_fillBlocks ? "GJ_button_01.png" : "GJ_button_06.png";
        auto spr = ButtonSprite::create(text.c_str(), "bigFont.fnt", bg, 0.45f);
        spr->setScale(0.78f);
        m_blocksToggleBtn->addChild(spr);
        m_blocksToggleBtn->setContentSize(spr->getContentSize() * 0.78f);
        spr->setPosition(m_blocksToggleBtn->getContentSize() / 2.f);
    }

    void onToggleBlocks(CCObject*) {
        m_fillBlocks = !m_fillBlocks;
        updateBlocksButton();
    }

    void refreshDepthButtons() {
        for (auto const& [depth, btn] : m_depthButtons) {
            bool active = (depth == m_fillDepth && m_fillBlocks);
            ccColor3B col = active ? ccc3(255, 255, 255) : ccc3(120, 120, 120);
            btn->setColor(col);
            if (auto rgba = typeinfo_cast<CCRGBAProtocol*>(btn->getNormalImage())) {
                rgba->setColor(col);
            }
            btn->setScale(active ? 0.72f : 0.65f);
        }
    }

    void onSelectDepth(CCObject* sender) {
        m_fillDepth = sender->getTag();
        refreshDepthButtons();
    }

    void updateClickOnlyButton() {
        if (!m_clickOnlyBtn) return;
        m_clickOnlyBtn->removeAllChildren();

        std::string text = m_clickOnly ? "HOLD ONLY" : "ALWAYS";
        const char* bg = m_clickOnly ? "GJ_button_01.png" : "GJ_button_04.png";
        auto spr = ButtonSprite::create(text.c_str(), "chatFont.fnt", bg, 0.42f);
        spr->setScale(0.68f);
        m_clickOnlyBtn->addChild(spr);
        m_clickOnlyBtn->setContentSize(spr->getContentSize() * 0.68f);
        spr->setPosition(m_clickOnlyBtn->getContentSize() / 2.f);
    }

    void onToggleClickOnly(CCObject*) {
        m_clickOnly = !m_clickOnly;
        updateClickOnlyButton();
    }

    void updatePatternModeButton() {
        if (!m_patternModeBtn) return;
        m_patternModeBtn->removeAllChildren();

        bool isPillars = (m_patternMode == "Turn Pillars");
        std::string text = isPillars ? "PILLARS" : "CORRIDOR";
        const char* bg = isPillars ? "GJ_button_01.png" : "GJ_button_04.png";
        auto spr = ButtonSprite::create(text.c_str(), "chatFont.fnt", bg, 0.42f);
        spr->setScale(0.68f);
        m_patternModeBtn->addChild(spr);
        m_patternModeBtn->setContentSize(spr->getContentSize() * 0.68f);
        spr->setPosition(m_patternModeBtn->getContentSize() / 2.f);
    }

    void onTogglePatternMode(CCObject*) {
        if (m_patternMode == "Turn Pillars") {
            m_patternMode = "Full Corridor";
        } else {
            m_patternMode = "Turn Pillars";
        }
        updatePatternModeButton();
    }

    void saveSettings() {
        auto mod = Mod::get();
        mod->setSettingValue<std::string>("ai-difficulty", m_selectedDifficulty);
        mod->setSettingValue<std::string>("ai-gameplay-style", m_selectedStyle);
        mod->setSettingValue<int64_t>("ai-clicks-count", m_clicksCount);
        mod->setSettingValue<int64_t>("timing-window-frames", m_timingFrames);
        mod->setSettingValue<double>("obstacle-distance", static_cast<double>(m_obstacleDist));
        mod->setSettingValue<bool>("corridor-fill", m_fillBlocks);
        mod->setSettingValue<int64_t>("corridor-fill-depth", m_fillDepth);
        mod->setSettingValue<bool>("live-build-click-only", m_clickOnly);
        mod->setSettingValue<std::string>("layout-pattern-mode", m_patternMode);
    }

    void onGenerateClick(CCObject*) {
        saveSettings();
        this->onClose(nullptr);
        if (m_onGenerate) {
            m_onGenerate();
        }
    }

    void onLiveBuildClick(CCObject*) {
        saveSettings();
        this->onClose(nullptr);
        if (m_onLiveBuild) {
            m_onLiveBuild();
        }
    }

    void onSettingsClick(CCObject*) {
        geode::openSettingsPopup(Mod::get(), false);
    }

    void onSyncatronClick(CCObject*) {
        if (Syncatron::pickAndLoadOsuFile()) {
            this->onClose(nullptr);
            if (m_onLiveBuild) {
                m_onLiveBuild();
            }
        }
    }

    bool setup(float width, float height, std::function<void()> onGenerate, std::function<void()> onLiveBuild) {
        if (!Popup::init(width, height)) return false;

        m_onGenerate = std::move(onGenerate);
        m_onLiveBuild = std::move(onLiveBuild);

        auto mod = Mod::get();
        m_selectedDifficulty = mod->hasSetting("ai-difficulty") ? mod->getSettingValue<std::string>("ai-difficulty") : "Medium";
        m_selectedStyle = mod->hasSetting("ai-gameplay-style") ? mod->getSettingValue<std::string>("ai-gameplay-style") : "Flow & Rhythmic";
        m_clicksCount = mod->hasSetting("ai-clicks-count") ? static_cast<int>(mod->getSettingValue<int64_t>("ai-clicks-count")) : 120;
        if (m_clicksCount < 20) m_clicksCount = 20;
        if (m_clicksCount > 500) m_clicksCount = 500;

        m_timingFrames = mod->hasSetting("timing-window-frames") ? static_cast<int>(mod->getSettingValue<int64_t>("timing-window-frames")) : 4;
        if (m_timingFrames < 1) m_timingFrames = 1;
        if (m_timingFrames > 20) m_timingFrames = 20;

        m_obstacleDist = mod->hasSetting("obstacle-distance") ? static_cast<float>(mod->getSettingValue<double>("obstacle-distance")) : (m_timingFrames * 5.2f);
        if (m_obstacleDist < 5.f) m_obstacleDist = 5.f;
        if (m_obstacleDist > 120.f) m_obstacleDist = 120.f;

        m_fillBlocks = mod->hasSetting("corridor-fill") ? mod->getSettingValue<bool>("corridor-fill") : true;
        m_fillDepth = mod->hasSetting("corridor-fill-depth") ? static_cast<int>(mod->getSettingValue<int64_t>("corridor-fill-depth")) : 1;
        if (m_fillDepth < 1) m_fillDepth = 1;
        if (m_fillDepth > 3) m_fillDepth = 3;

        m_clickOnly = mod->hasSetting("live-build-click-only") ? mod->getSettingValue<bool>("live-build-click-only") : false;
        m_patternMode = mod->hasSetting("layout-pattern-mode") ? mod->getSettingValue<std::string>("layout-pattern-mode") : "Turn Pillars";

        this->setTitle("Layout Generator");

        float w = m_size.width;
        float h = m_size.height;

        // 1. DIFFICULTY SECTION
        auto diffLabel = CCLabelBMFont::create("Difficulty:", "goldFont.fnt");
        diffLabel->setScale(0.5f);
        diffLabel->setPosition({w / 2.f, h - 38.f});
        m_mainLayer->addChild(diffLabel);

        auto diffMenu = CCMenu::create();
        diffMenu->setPosition({w / 2.f, h - 60.f});
        diffMenu->setContentSize({360.f, 26.f});
        diffMenu->setLayout(RowLayout::create()->setGap(8.f));

        std::vector<std::tuple<int, std::string, const char*, const char*>> diffs = {
            {0, "Easy", "Easy", "GJ_button_01.png"},
            {1, "Medium", "Medium", "GJ_button_02.png"},
            {2, "Hard", "Hard", "GJ_button_05.png"},
            {3, "Extreme Demon", "Demon", "GJ_button_06.png"}
        };

        for (auto const& [tag, diffName, label, bg] : diffs) {
            auto spr = ButtonSprite::create(label, "bigFont.fnt", bg, 0.45f);
            auto btn = CCMenuItemSpriteExtra::create(
                spr,
                this,
                menu_selector(AILayoutPopup::onSelectDifficulty)
            );
            btn->setTag(tag);
            m_diffButtons.push_back({diffName, btn});
            diffMenu->addChild(btn);
        }
        diffMenu->updateLayout();
        m_mainLayer->addChild(diffMenu);
        refreshDifficultyButtons();

        // 2. STYLE SECTION
        auto styleLabel = CCLabelBMFont::create("Gameplay Style:", "goldFont.fnt");
        styleLabel->setScale(0.5f);
        styleLabel->setPosition({w / 2.f, h - 84.f});
        m_mainLayer->addChild(styleLabel);

        auto styleMenu = CCMenu::create();
        styleMenu->setPosition({w / 2.f, h - 106.f});
        styleMenu->setContentSize({360.f, 26.f});
        styleMenu->setLayout(RowLayout::create()->setGap(8.f));

        std::vector<std::tuple<int, std::string, const char*, const char*>> styles = {
            {0, "Flow & Rhythmic", "Flow", "GJ_button_01.png"},
            {1, "Tech & Precision", "Tech", "GJ_button_02.png"},
            {2, "Sweeping & Wide", "Wide", "GJ_button_03.png"},
            {3, "Fast Paced", "Fast", "GJ_button_05.png"}
        };

        for (auto const& [tag, styleName, label, bg] : styles) {
            auto spr = ButtonSprite::create(label, "bigFont.fnt", bg, 0.45f);
            auto btn = CCMenuItemSpriteExtra::create(
                spr,
                this,
                menu_selector(AILayoutPopup::onSelectStyle)
            );
            btn->setTag(tag);
            m_styleButtons.push_back({styleName, btn});
            styleMenu->addChild(btn);
        }
        styleMenu->updateLayout();
        m_mainLayer->addChild(styleMenu);
        refreshStyleButtons();

        // 3. OBSTACLE CLEARANCE & TIMING WINDOW (NO 2ND PATHS)
        auto distTitle = CCLabelBMFont::create("Click Window (Strict Timing):", "goldFont.fnt");
        distTitle->setScale(0.5f);
        distTitle->setPosition({w / 2.f, h - 130.f});
        m_mainLayer->addChild(distTitle);

        auto distMenu = CCMenu::create();
        distMenu->setPosition({w / 2.f, h - 152.f});
        distMenu->setContentSize({360.f, 26.f});
        distMenu->setLayout(RowLayout::create()->setGap(8.f));

        auto dMinus = CCSprite::createWithSpriteFrameName("edit_leftBtn_001.png");
        dMinus->setScale(0.75f);
        auto dMinusBtn = CCMenuItemSpriteExtra::create(dMinus, this, menu_selector(AILayoutPopup::onDistMinus));
        distMenu->addChild(dMinusBtn);

        m_distLabel = CCLabelBMFont::create("", "bigFont.fnt");
        m_distLabel->setScale(0.46f);
        m_distLabel->setColor(ccc3(100, 240, 255));
        updateDistanceDisplay();
        distMenu->addChild(m_distLabel);

        auto dPlus = CCSprite::createWithSpriteFrameName("edit_rightBtn_001.png");
        dPlus->setScale(0.75f);
        auto dPlusBtn = CCMenuItemSpriteExtra::create(dPlus, this, menu_selector(AILayoutPopup::onDistPlus));
        distMenu->addChild(dPlusBtn);

        std::vector<std::pair<const char*, int>> distPresets = {
            {"1F (Demon)", 1},
            {"3F (Hard)", 3},
            {"5F (Normal)", 5},
            {"8F (Easy)", 8}
        };
        for (auto const& [name, val] : distPresets) {
            auto pSpr = ButtonSprite::create(name, "chatFont.fnt", "GJ_button_04.png", 0.42f);
            pSpr->setScale(0.65f);
            auto pBtn = CCMenuItemSpriteExtra::create(pSpr, this, menu_selector(AILayoutPopup::onDistPresetClick));
            pBtn->setTag(val);
            distMenu->addChild(pBtn);
        }

        distMenu->updateLayout();
        m_mainLayer->addChild(distMenu);

        // 4. AI LENGTH SECTION
        auto lengthTitle = CCLabelBMFont::create("AI Macro Length:", "goldFont.fnt");
        lengthTitle->setScale(0.5f);
        lengthTitle->setPosition({w / 2.f, h - 176.f});
        m_mainLayer->addChild(lengthTitle);

        auto lengthMenu = CCMenu::create();
        lengthMenu->setPosition({w / 2.f, h - 198.f});
        lengthMenu->setContentSize({360.f, 26.f});
        lengthMenu->setLayout(RowLayout::create()->setGap(8.f));

        auto leftSpr = CCSprite::createWithSpriteFrameName("edit_leftBtn_001.png");
        leftSpr->setScale(0.75f);
        auto minusBtn = CCMenuItemSpriteExtra::create(leftSpr, this, menu_selector(AILayoutPopup::onStepMinus));
        lengthMenu->addChild(minusBtn);

        m_lengthLabel = CCLabelBMFont::create("", "bigFont.fnt");
        m_lengthLabel->setScale(0.48f);
        m_lengthLabel->setColor(ccc3(255, 230, 100));
        updateLengthDisplay();
        lengthMenu->addChild(m_lengthLabel);

        auto rightSpr = CCSprite::createWithSpriteFrameName("edit_rightBtn_001.png");
        rightSpr->setScale(0.75f);
        auto plusBtn = CCMenuItemSpriteExtra::create(rightSpr, this, menu_selector(AILayoutPopup::onStepPlus));
        lengthMenu->addChild(plusBtn);

        std::vector<std::pair<const char*, int>> presets = {
            {"15s", 60},
            {"30s", 120},
            {"60s", 240}
        };
        for (auto const& [name, count] : presets) {
            auto pSpr = ButtonSprite::create(name, "chatFont.fnt", "GJ_button_04.png", 0.42f);
            pSpr->setScale(0.68f);
            auto pBtn = CCMenuItemSpriteExtra::create(pSpr, this, menu_selector(AILayoutPopup::onPresetClick));
            pBtn->setTag(count);
            lengthMenu->addChild(pBtn);
        }
        lengthMenu->updateLayout();
        m_mainLayer->addChild(lengthMenu);

        // 5. CORRIDOR BLOCKS (КВАДРАТЫ) TOGGLE SECTION
        auto blocksTitle = CCLabelBMFont::create("Corridor Blocks (Квадраты):", "goldFont.fnt");
        blocksTitle->setScale(0.5f);
        blocksTitle->setPosition({w / 2.f, h - 222.f});
        m_mainLayer->addChild(blocksTitle);

        auto blocksMenu = CCMenu::create();
        blocksMenu->setPosition({w / 2.f, h - 246.f});
        blocksMenu->setContentSize({360.f, 26.f});
        blocksMenu->setLayout(RowLayout::create()->setGap(10.f));

        auto dummySpr = CCSprite::create();
        m_blocksToggleBtn = CCMenuItemSpriteExtra::create(
            dummySpr,
            this,
            menu_selector(AILayoutPopup::onToggleBlocks)
        );
        updateBlocksButton();
        blocksMenu->addChild(m_blocksToggleBtn);

        std::vector<std::pair<const char*, int>> depthPresets = {
            {"1L", 1},
            {"2L", 2}
        };
        for (auto const& [name, val] : depthPresets) {
            auto pSpr = ButtonSprite::create(name, "chatFont.fnt", "GJ_button_04.png", 0.42f);
            pSpr->setScale(0.68f);
            auto pBtn = CCMenuItemSpriteExtra::create(pSpr, this, menu_selector(AILayoutPopup::onSelectDepth));
            pBtn->setTag(val);
            m_depthButtons.push_back({val, pBtn});
            blocksMenu->addChild(pBtn);
        }
        refreshDepthButtons();

        auto clickOnlyDummy = CCSprite::create();
        m_clickOnlyBtn = CCMenuItemSpriteExtra::create(
            clickOnlyDummy,
            this,
            menu_selector(AILayoutPopup::onToggleClickOnly)
        );
        updateClickOnlyButton();
        blocksMenu->addChild(m_clickOnlyBtn);

        auto patternDummy = CCSprite::create();
        m_patternModeBtn = CCMenuItemSpriteExtra::create(
            patternDummy,
            this,
            menu_selector(AILayoutPopup::onTogglePatternMode)
        );
        updatePatternModeButton();
        blocksMenu->addChild(m_patternModeBtn);

        blocksMenu->updateLayout();
        m_mainLayer->addChild(blocksMenu);

        // 6. BOTTOM ACTION ROW: [ AI GENERATE ] [ LIVE BUILD ] (Settings)
        auto bottomMenu = CCMenu::create();
        bottomMenu->setPosition({w / 2.f, 24.f});
        bottomMenu->setContentSize({370.f, 40.f});
        bottomMenu->setLayout(RowLayout::create()->setGap(14.f));

        auto genSpr = ButtonSprite::create("AI GENERATE", "goldFont.fnt", "GJ_button_01.png", 0.7f);
        genSpr->setScale(0.85f);
        auto genBtn = CCMenuItemSpriteExtra::create(genSpr, this, menu_selector(AILayoutPopup::onGenerateClick));
        bottomMenu->addChild(genBtn);

        auto liveSpr = ButtonSprite::create("LIVE BUILD", "goldFont.fnt", "GJ_button_02.png", 0.7f);
        liveSpr->setScale(0.85f);
        auto liveBtn = CCMenuItemSpriteExtra::create(liveSpr, this, menu_selector(AILayoutPopup::onLiveBuildClick));
        bottomMenu->addChild(liveBtn);

        auto syncSpr = ButtonSprite::create("SYNC (.OSU)", "goldFont.fnt", "GJ_button_04.png", 0.7f);
        syncSpr->setScale(0.85f);
        auto syncBtn = CCMenuItemSpriteExtra::create(syncSpr, this, menu_selector(AILayoutPopup::onSyncatronClick));
        bottomMenu->addChild(syncBtn);

        // @geode-ignore(unknown-resource)
        auto gearSpr = CircleButtonSprite::createWithSpriteFrameName("geode.loader/settings.png", 0.85f, CircleBaseColor::DarkAqua);
        gearSpr->setScale(0.75f);
        auto gearBtn = CCMenuItemSpriteExtra::create(gearSpr, this, menu_selector(AILayoutPopup::onSettingsClick));
        bottomMenu->addChild(gearBtn);

        bottomMenu->updateLayout();
        m_mainLayer->addChild(bottomMenu);

        return true;
    }

public:
    static AILayoutPopup* create(std::function<void()> onGenerate, std::function<void()> onLiveBuild) {
        auto ret = new AILayoutPopup();
        if (ret && ret->setup(400.f, 325.f, std::move(onGenerate), std::move(onLiveBuild))) {
            ret->autorelease();
            return ret;
        }
        CC_SAFE_DELETE(ret);
        return nullptr;
    }
};
