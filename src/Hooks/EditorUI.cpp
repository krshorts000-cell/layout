#include <Geode/Geode.hpp>
#include <Geode/modify/EditorUI.hpp>
#include "../Generator/LayoutEngine.hpp"
#include "../Macro/MacroManager.hpp"

using namespace geode::prelude;

class $modify(MyEditorUI, EditorUI) {
    void onPlaytest(CCObject* sender) {
        EditorUI::onPlaytest(sender);
        MacroManager::get().resetPlayback();
    }

    bool init(LevelEditorLayer* editorLayer) {
        if (!EditorUI::init(editorLayer)) return false;

        auto menu = this->getChildByID("playback-menu");
        if (!menu) return true;

        // 1. Build Button (Exact LayoutGenerator icon from resources/button.png)
        auto buildSprite = ButtonSprite::createWithSpriteFrameName("button.png"_spr);
        buildSprite->setScale(0.83f);
        auto buildButton = CCMenuItemSpriteExtra::create(
            buildSprite,
            this,
            menu_selector(MyEditorUI::onGenerateLayout)
        );
        buildButton->setID("build-button"_spr);

        // 2. Bot Toggle Button
        auto botSprite = ButtonSprite::create("Bot", "bigFont.fnt", "GJ_button_02.png", 0.6f);
        botSprite->setScale(0.83f);
        auto botButton = CCMenuItemSpriteExtra::create(
            botSprite,
            this,
            menu_selector(MyEditorUI::onToggleBot)
        );
        botButton->setID("bot-button"_spr);

        // 3. Rec Toggle Button
        auto recSprite = ButtonSprite::create("Rec", "bigFont.fnt", "GJ_button_06.png", 0.6f);
        recSprite->setScale(0.83f);
        auto recButton = CCMenuItemSpriteExtra::create(
            recSprite,
            this,
            menu_selector(MyEditorUI::onToggleRec)
        );
        recButton->setID("rec-button"_spr);

        // 4. Settings Button (Geode gear icon in DarkAqua circle, exact LayoutGenerator style)
        // @geode-ignore(unknown-resource)
        auto settingsSprite = CircleButtonSprite::createWithSpriteFrameName("geode.loader/settings.png", 1.f, CircleBaseColor::DarkAqua);
        settingsSprite->setScale(0.83f);
        auto settingsButton = CCMenuItemSpriteExtra::create(
            settingsSprite,
            this,
            menu_selector(MyEditorUI::onSettingsButton)
        );
        settingsButton->setID("settings-button"_spr);

        menu->addChild(buildButton);
        menu->addChild(botButton);
        menu->addChild(recButton);
        menu->addChild(settingsButton);

        m_uiItems->addObject(buildButton);
        m_uiItems->addObject(botButton);
        m_uiItems->addObject(recButton);
        m_uiItems->addObject(settingsButton);

        menu->updateLayout();

        return true;
    }

    void onGenerateLayout(CCObject*) {
        if (!m_editorLayer) return;
        LayoutEngine::generate(m_editorLayer);
    }

    void onToggleBot(CCObject*) {
        MacroManager::get().togglePlaying();
    }

    void onToggleRec(CCObject*) {
        MacroManager::get().toggleRecording();
    }

    void onSettingsButton(CCObject*) {
        geode::openSettingsPopup(Mod::get(), false);
    }
};
