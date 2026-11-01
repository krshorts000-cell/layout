#include <Geode/Geode.hpp>
#include <Geode/modify/EditorUI.hpp>
#include "../Generator/LayoutEngine.hpp"
#include "../Macro/MacroManager.hpp"

using namespace geode::prelude;

class $modify(MyEditorUI, EditorUI) {
    bool init(LevelEditorLayer* editorLayer) {
        if (!EditorUI::init(editorLayer)) return false;

        auto winSize = CCDirector::sharedDirector()->getWinSize();

        // Создаем отдельное меню инструментов под панелью воспроизведения, чтобы не перекрывать шестеренку
        auto customMenu = CCMenu::create();
        customMenu->setID("xdbot-layout-menu"_spr);
        customMenu->setLayout(RowLayout::create()->setGap(6.0f));

        // 1. Кнопка Gen (генерация лейаута)
        auto genSpr = ButtonSprite::create("Gen", "goldFont.fnt", "GJ_button_01.png", 0.65f);
        auto genBtn = CCMenuItemSpriteExtra::create(
            genSpr,
            this,
            menu_selector(MyEditorUI::onGenerateLayout)
        );
        genBtn->setID("xdbot-gen-btn"_spr);

        // 2. Кнопка Bot (включение XDBot авто-прохождения)
        auto botSpr = ButtonSprite::create("Bot", "bigFont.fnt", "GJ_button_02.png", 0.65f);
        auto botBtn = CCMenuItemSpriteExtra::create(
            botSpr,
            this,
            menu_selector(MyEditorUI::onToggleBot)
        );
        botBtn->setID("xdbot-bot-btn"_spr);

        // 3. Кнопка Rec (живая запись кликов игрока)
        auto recSpr = ButtonSprite::create("Rec", "bigFont.fnt", "GJ_button_06.png", 0.65f);
        auto recBtn = CCMenuItemSpriteExtra::create(
            recSpr,
            this,
            menu_selector(MyEditorUI::onToggleRec)
        );
        recBtn->setID("xdbot-rec-btn"_spr);

        customMenu->addChild(genBtn);
        customMenu->addChild(botBtn);
        customMenu->addChild(recBtn);
        customMenu->updateLayout();

        // Позиционируем меню чуть ниже верхних кнопок в левом верхнем углу
        customMenu->setPosition(ccp(110.0f, winSize.height - 56.0f));

        this->addChild(customMenu);
        m_uiItems->addObject(genBtn);
        m_uiItems->addObject(botBtn);
        m_uiItems->addObject(recBtn);

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
};
