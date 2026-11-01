#include <Geode/Geode.hpp>
#include <Geode/modify/PlayerObject.hpp>
#include "../Macro/MacroManager.hpp"

using namespace geode::prelude;

class $modify(MyPlayerObject, PlayerObject) {
    void pushButton(PlayerButton btn) {
        PlayerObject::pushButton(btn);

        auto macro = MacroManager::get();
        if (macro->isRecording()) {
            bool isP2 = (m_gameLayer && this == m_gameLayer->m_player2);
            macro->recordInput(true, isP2, static_cast<int>(btn));
        }
    }

    void releaseButton(PlayerButton btn) {
        PlayerObject::releaseButton(btn);

        auto macro = MacroManager::get();
        if (macro->isRecording()) {
            bool isP2 = (m_gameLayer && this == m_gameLayer->m_player2);
            macro->recordInput(false, isP2, static_cast<int>(btn));
        }
    }
};
