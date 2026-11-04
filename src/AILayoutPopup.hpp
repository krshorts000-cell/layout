#pragma once
#include <Geode/Geode.hpp>
#include <Geode/ui/Popup.hpp>
#include <Geode/ui/BasedButtonSprite.hpp>
#include <Geode/ui/Layout.hpp>

using namespace geode::prelude;

class AILayoutPopup : public geode::Popup {
protected:
    std::function<void()> m_onGenerate;
    std::string m_selectedDifficulty;
    std::string m_selectedStyle;
    int m_clicksCount = 120;

    CCLabelBMFont* m_lengthLabel = nullptr;
    std::vector<std::pair<std::string, CCMenuItemSpriteExtra*>> m_diffButtons;
    std::vector<std::pair<std::string, CCMenuItemSpriteExtra*>> m_styleButtons;

    void updateLengthDisplay() {
        if (!m_lengthLabel) return;
        float seconds = m_clicksCount / 4.f;
        m_lengthLabel->setString(fmt::format("{} Clicks (~{:.0f}s)", m_clicksCount, seconds).c_str());
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
        if (tag == 0) m_selectedDifficulty = "Easy";
        else if (tag == 1) m_selectedDifficulty = "Medium";
        else if (tag == 2) m_selectedDifficulty = "Hard";
        else if (tag == 3) m_selectedDifficulty = "Extreme Demon";
        refreshDifficultyButtons();
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

    void onGenerateClick(CCObject*) {
        auto mod = Mod::get();
        mod->setSettingValue<std::string>("ai-difficulty", m_selectedDifficulty);
        mod->setSettingValue<std::string>("ai-gameplay-style", m_selectedStyle);
        mod->setSettingValue<int64_t>("ai-clicks-count", m_clicksCount);

        this->onClose(nullptr);

        if (m_onGenerate) {
            m_onGenerate();
        }
    }

    void onSettingsClick(CCObject*) {
        geode::openSettingsPopup(Mod::get(), false);
    }

    bool setup(float width, float height, std::function<void()> onGenerate) {
        if (!Popup::init(width, height)) return false;

        m_onGenerate = std::move(onGenerate);

        auto mod = Mod::get();
        m_selectedDifficulty = mod->hasSetting("ai-difficulty") ? mod->getSettingValue<std::string>("ai-difficulty") : "Medium";
        m_selectedStyle = mod->hasSetting("ai-gameplay-style") ? mod->getSettingValue<std::string>("ai-gameplay-style") : "Flow & Rhythmic";
        m_clicksCount = mod->hasSetting("ai-clicks-count") ? static_cast<int>(mod->getSettingValue<int64_t>("ai-clicks-count")) : 120;
        if (m_clicksCount < 20) m_clicksCount = 20;
        if (m_clicksCount > 500) m_clicksCount = 500;

        this->setTitle("AI Layout Generator");

        float w = m_size.width;
        float h = m_size.height;

        // 1. DIFFICULTY SECTION
        auto diffLabel = CCLabelBMFont::create("Difficulty:", "goldFont.fnt");
        diffLabel->setScale(0.55f);
        diffLabel->setPosition({w / 2.f, h - 45.f});
        m_mainLayer->addChild(diffLabel);

        auto diffMenu = CCMenu::create();
        diffMenu->setPosition({w / 2.f, h - 70.f});
        diffMenu->setContentSize({340.f, 30.f});
        diffMenu->setLayout(RowLayout::create()->setGap(8.f));

        std::vector<std::tuple<int, std::string, const char*, const char*>> diffs = {
            {0, "Easy", "Easy", "GJ_button_01.png"},
            {1, "Medium", "Medium", "GJ_button_02.png"},
            {2, "Hard", "Hard", "GJ_button_05.png"},
            {3, "Extreme Demon", "Demon", "GJ_button_06.png"}
        };

        for (auto const& [tag, diffName, label, bg] : diffs) {
            auto spr = ButtonSprite::create(label, "bigFont.fnt", bg, 0.5f);
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
        styleLabel->setScale(0.55f);
        styleLabel->setPosition({w / 2.f, h - 98.f});
        m_mainLayer->addChild(styleLabel);

        auto styleMenu = CCMenu::create();
        styleMenu->setPosition({w / 2.f, h - 122.f});
        styleMenu->setContentSize({340.f, 30.f});
        styleMenu->setLayout(RowLayout::create()->setGap(8.f));

        std::vector<std::tuple<int, std::string, const char*, const char*>> styles = {
            {0, "Flow & Rhythmic", "Flow", "GJ_button_01.png"},
            {1, "Tech & Precision", "Tech", "GJ_button_02.png"},
            {2, "Sweeping & Wide", "Wide", "GJ_button_03.png"},
            {3, "Fast Paced", "Fast", "GJ_button_05.png"}
        };

        for (auto const& [tag, styleName, label, bg] : styles) {
            auto spr = ButtonSprite::create(label, "bigFont.fnt", bg, 0.5f);
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

        // 3. LENGTH SECTION
        auto lengthTitle = CCLabelBMFont::create("Level Length:", "goldFont.fnt");
        lengthTitle->setScale(0.55f);
        lengthTitle->setPosition({w / 2.f, h - 150.f});
        m_mainLayer->addChild(lengthTitle);

        auto lengthMenu = CCMenu::create();
        lengthMenu->setPosition({w / 2.f, h - 175.f});
        lengthMenu->setContentSize({340.f, 28.f});
        lengthMenu->setLayout(RowLayout::create()->setGap(10.f));

        auto leftSpr = CCSprite::createWithSpriteFrameName("edit_leftBtn_001.png");
        leftSpr->setScale(0.8f);
        auto minusBtn = CCMenuItemSpriteExtra::create(
            leftSpr,
            this,
            menu_selector(AILayoutPopup::onStepMinus)
        );
        lengthMenu->addChild(minusBtn);

        m_lengthLabel = CCLabelBMFont::create("", "bigFont.fnt");
        m_lengthLabel->setScale(0.5f);
        m_lengthLabel->setColor(ccc3(255, 230, 100));
        updateLengthDisplay();
        lengthMenu->addChild(m_lengthLabel);

        auto rightSpr = CCSprite::createWithSpriteFrameName("edit_rightBtn_001.png");
        rightSpr->setScale(0.8f);
        auto plusBtn = CCMenuItemSpriteExtra::create(
            rightSpr,
            this,
            menu_selector(AILayoutPopup::onStepPlus)
        );
        lengthMenu->addChild(plusBtn);

        lengthMenu->updateLayout();
        m_mainLayer->addChild(lengthMenu);

        // Quick Presets
        auto presetsMenu = CCMenu::create();
        presetsMenu->setPosition({w / 2.f, h - 200.f});
        presetsMenu->setContentSize({340.f, 22.f});
        presetsMenu->setLayout(RowLayout::create()->setGap(6.f));

        std::vector<std::pair<const char*, int>> presets = {
            {"15s", 60},
            {"30s", 120},
            {"60s", 240},
            {"90s", 360}
        };
        for (auto const& [name, count] : presets) {
            auto pSpr = ButtonSprite::create(name, "chatFont.fnt", "GJ_button_04.png", 0.45f);
            pSpr->setScale(0.7f);
            auto pBtn = CCMenuItemSpriteExtra::create(
                pSpr,
                this,
                menu_selector(AILayoutPopup::onPresetClick)
            );
            pBtn->setTag(count);
            presetsMenu->addChild(pBtn);
        }
        presetsMenu->updateLayout();
        m_mainLayer->addChild(presetsMenu);

        // 4. BOTTOM ACTION ROW
        auto bottomMenu = CCMenu::create();
        bottomMenu->setPosition({w / 2.f, 26.f});
        bottomMenu->setContentSize({340.f, 40.f});
        bottomMenu->setLayout(RowLayout::create()->setGap(20.f));

        auto genSpr = ButtonSprite::create("GENERATE", "goldFont.fnt", "GJ_button_01.png", 0.8f);
        genSpr->setScale(0.9f);
        auto genBtn = CCMenuItemSpriteExtra::create(
            genSpr,
            this,
            menu_selector(AILayoutPopup::onGenerateClick)
        );
        bottomMenu->addChild(genBtn);

        // @geode-ignore(unknown-resource)
        auto gearSpr = CircleButtonSprite::createWithSpriteFrameName("geode.loader/settings.png", 0.85f, CircleBaseColor::DarkAqua);
        gearSpr->setScale(0.75f);
        auto gearBtn = CCMenuItemSpriteExtra::create(
            gearSpr,
            this,
            menu_selector(AILayoutPopup::onSettingsClick)
        );
        bottomMenu->addChild(gearBtn);

        bottomMenu->updateLayout();
        m_mainLayer->addChild(bottomMenu);

        return true;
    }

public:
    static AILayoutPopup* create(std::function<void()> onGenerate) {
        auto ret = new AILayoutPopup();
        if (ret && ret->setup(390.f, 260.f, std::move(onGenerate))) {
            ret->autorelease();
            return ret;
        }
        CC_SAFE_DELETE(ret);
        return nullptr;
    }
};
