#include <Geode/Geode.hpp>
#include <Geode/modify/LevelEditorLayer.hpp>
#include "../Macro/MacroManager.hpp"

using namespace geode::prelude;

class $modify(MyLevelEditorLayer, LevelEditorLayer) {
    void update(float dt) {
        LevelEditorLayer::update(dt);

        // Обновляем состояние макроса во время плейтеста редактора
        auto macro = MacroManager::get();
        if (macro->isPlaying() || macro->isRecording()) {
            macro->update(dt, m_player1, m_player2);
        }
    }

    void onStopPlaytest() {
        LevelEditorLayer::onStopPlaytest();

        auto macro = MacroManager::get();
        if (macro->isPlaying()) {
            macro->stopPlayback();
        }
    }
};
