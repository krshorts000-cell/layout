#include <Geode/Geode.hpp>
#include <Geode/modify/EditorUI.hpp>
#include "../Generator/LayoutEngine.hpp"
#include "../Macro/MacroManager.hpp"

using namespace geode::prelude;

class $modify(MyEditorUI, EditorUI) {
    bool init(LevelEditorLayer* editorLayer) {
        if (!EditorUI::init(editorLayer)) return false;

        // 1. Кнопка генерации лейаута + записи макроса
        auto genSpr = ButtonSprite::create("Gen", "goldFont.fnt", "GJ_button_01.png", 0.75f);
        auto genBtn = CCMenuItemSpriteExtra::create(
            genSpr,
            this,
            menu_selector(MyEditorUI::onGenerateLayout)
        );
        genBtn->setID("layout-gen-button"_spr);

        // 2. Кнопка запуска/остановки бота-воспроизведения
        auto botSpr = ButtonSprite::create("Bot", "bigFont.fnt", "GJ_button_02.png", 0.75f);
        auto botBtn = CCMenuItemSpriteExtra::create(
            botSpr,
            this,
            menu_selector(MyEditorUI::onToggleBot)
        );
        botBtn->setID("layout-bot-button"_spr);

        // Ищем меню воспроизведения в верхней панели редактора
        if (auto menu = this->getChildByID("playback-menu")) {
            menu->addChild(genBtn);
            menu->addChild(botBtn);
            m_uiItems->addObject(genBtn);
            m_uiItems->addObject(botBtn);
            menu->updateLayout();
        }

        return true;
    }

    void onGenerateLayout(CCObject*) {
        if (!m_editorLayer) return;
        LayoutEngine::generate(m_editorLayer);
    }

    void onToggleBot(CCObject*) {
        auto macro = MacroManager::get();
        if (macro->isPlaying()) {
            macro->stopPlayback();
            Notification::create("Бот остановлен", NotificationIcon::Warning)->show();
        } else {
            if (macro->getEventCount() == 0) {
                Notification::create("Нет записанного макроса! Сначала нажмите Gen.", NotificationIcon::Error)->show();
                return;
            }
            macro->startPlayback();
            // Запускаем тест уровня в редакторе
            if (m_editorLayer && m_editorLayer->m_editorUI) {
                m_editorLayer->m_editorUI->onPlaytest(nullptr);
            }
        }
    }
};
