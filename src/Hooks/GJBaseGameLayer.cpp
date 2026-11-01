#include <Geode/Geode.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/LevelEditorLayer.hpp>
#include "../Macro/MacroManager.hpp"

using namespace geode::prelude;

class $modify(MyBaseGameLayer, GJBaseGameLayer) {
    struct Fields {
        bool macroInput = false;
    };

    void processCommands(float dt, bool isHalfTick, bool isLastTick) {
        PlayLayer* pl = PlayLayer::get();
        auto* editorLayer = typeinfo_cast<LevelEditorLayer*>(this);
        bool editorPlaytest = !pl && editorLayer && editorLayer->m_playbackMode == PlaybackMode::Playing;

        GJBaseGameLayer::processCommands(dt, isHalfTick, isLastTick);

        if (!pl && !editorPlaytest) return;

        auto& macro = MacroManager::get();

        if (editorPlaytest) {
            if (macro.isPlaying()) {
                int frame = static_cast<int>(editorLayer->m_gameState.m_currentProgress / 2);
                m_fields->macroInput = true;
                macro.handlePlaying(this, frame);
                m_fields->macroInput = false;
            }
            return;
        }

        if (pl) {
            int frame = static_cast<int>(pl->m_gameState.m_levelTime * 240.f);
            if (macro.isPlaying()) {
                m_fields->macroInput = true;
                macro.handlePlaying(this, frame);
                m_fields->macroInput = false;
            }
        }
    }

    void handleButton(bool down, int button, bool player2) {
        auto& macro = MacroManager::get();

        if (macro.isRecording() && !m_fields->macroInput) {
            int frame = 0;
            if (auto pl = typeinfo_cast<PlayLayer*>(this)) {
                frame = static_cast<int>(pl->m_gameState.m_levelTime * 240.f);
            } else if (auto el = typeinfo_cast<LevelEditorLayer*>(this)) {
                frame = static_cast<int>(el->m_gameState.m_currentProgress / 2);
            }
            macro.recordAction(frame, button, player2, down);
        }

        GJBaseGameLayer::handleButton(down, button, player2);
    }
};

class $modify(MyPlayLayerHook, PlayLayer) {
    void resetLevel() {
        PlayLayer::resetLevel();
        MacroManager::get().resetPlayback();
    }
};

class $modify(MyEditorLayerHook, LevelEditorLayer) {
    void onPlaytest(CCObject* sender) {
        LevelEditorLayer::onPlaytest(sender);
        MacroManager::get().resetPlayback();
    }

    void onStopPlaytest() {
        LevelEditorLayer::onStopPlaytest();
        MacroManager::get().resetPlayback();
    }
};
