#include <Geode/Geode.hpp>
#include "LayoutGenerator/LayoutGeneratorLayer.hpp"
#include "LevelEditorLayer.cpp"
#include "Macro/Macro.hpp"

using namespace geode::prelude;

#include <Geode/modify/EditorUI.hpp>
class $modify(MyEditorUI, EditorUI)
{
    LayoutGeneratorLayer *getBuilder()
    {
        return static_cast<MyLevelEditorLayer *>(m_editorLayer)->m_fields->m_builder;
    }

    void onPlaytest(CCObject* sender)
    {
        auto &g = Global::get();
        if (m_editorLayer && m_editorLayer->m_playbackMode == PlaybackMode::Not) {
            g.currentAction = 0;
            g.previousFrame = 0;
            Macro::resetPlayback();
            if (g.state == state::playing || g.state == state::recording) {
                m_editorLayer->m_gameState.m_currentProgress = 0;
            }
        }
        EditorUI::onPlaytest(sender);
    }

    bool init(LevelEditorLayer *editorLayer)
    {
        if (!EditorUI::init(editorLayer))
            return false;

        auto buildSprite = ButtonSprite::createWithSpriteFrameName("button.png"_spr);
        buildSprite->setScale(0.83f);
        auto buildButton = CCMenuItemSpriteExtra::create(
            buildSprite,
            getBuilder(),
            menu_selector(LayoutGeneratorLayer::onBuildButton));
        buildButton->setID("build-button"_spr);

        // Bot button: sleek toggle button in playback-menu
        auto botSprite = ButtonSprite::create("Bot", "bigFont.fnt", "GJ_button_02.png", 0.6f);
        botSprite->setScale(0.83f);
        auto botButton = CCMenuItemSpriteExtra::create(
            botSprite,
            this,
            menu_selector(MyEditorUI::onToggleBot));
        botButton->setID("bot-button"_spr);

        // @geode-ignore(unknown-resource)
        auto settingsSprite = CircleButtonSprite::createWithSpriteFrameName("geode.loader/settings.png", 1.f, CircleBaseColor::DarkAqua);
        settingsSprite->setScale(0.83f);
        auto settingsButton = CCMenuItemSpriteExtra::create(
            settingsSprite,
            getBuilder(),
            menu_selector(LayoutGeneratorLayer::onSettingsButton));
        settingsButton->setID("settings-button"_spr);

        auto menu = getChildByID("playback-menu");
        if (menu) {
            menu->addChild(buildButton);
            menu->addChild(botButton);
            menu->addChild(settingsButton);
            m_uiItems->addObject(buildButton);
            m_uiItems->addObject(botButton);
            m_uiItems->addObject(settingsButton);
            menu->updateLayout();
        }

        return true;
    }

    void onToggleBot(CCObject*) {
        Macro::togglePlaying();
    }
};
