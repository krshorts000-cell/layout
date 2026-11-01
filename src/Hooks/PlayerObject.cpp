#include <Geode/Geode.hpp>
#include <Geode/modify/PlayerObject.hpp>
#include "../Macro/MacroManager.hpp"

using namespace geode::prelude;

class $modify(MyPlayerObject, PlayerObject) {
    bool pushButton(PlayerButton btn) {
        bool res = PlayerObject::pushButton(btn);

        auto macro = MacroManager::get();
        if (macro->isRecording()) {
            bool isP2 = (m_gameLayer && this == m_gameLayer->m_player2);
            macro->recordInput(true, isP2, static_cast<int>(btn));
        }

        return res;
    }

    bool releaseButton(PlayerButton btn) {
        bool res = PlayerObject::releaseButton(btn);

        auto macro = MacroManager::get();
        if (macro->isRecording()) {
            bool isP2 = (m_gameLayer && this == m_gameLayer->m_player2);
            macro->recordInput(false, isP2, static_cast<int>(btn));
        }

        return res;
    }
};
