#include <Geode/Geode.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/binding/LevelEditorLayer.hpp>
#include "Macro/MacroManager.hpp"
#include "LayoutGenerator/LayoutGeneratorLayer.hpp"

using namespace geode::prelude;

class $modify(MyBaseGameLayer, GJBaseGameLayer) {
    struct Fields {
        bool macroInput = false;
    };

    void processCommands(float dt, bool isHalfTick, bool isLastTick) {
        GJBaseGameLayer::processCommands(dt, isHalfTick, isLastTick);

        // ONLY play during Editor Playtest when user explicitly enabled Bot
        auto* editorLayer = typeinfo_cast<LevelEditorLayer*>(this);
        if (!editorLayer || editorLayer->m_playbackMode != PlaybackMode::Playing) {
            return;
        }

        auto& macro = MacroManager::get();
        if (!macro.isPlaying()) return;

        // Do not replay while LayoutGenerator is actively building
        auto builder = typeinfo_cast<LayoutGeneratorLayer*>(editorLayer->getChildByID("builder"_spr));
        if (builder && builder->getIsBuilding()) {
            return;
        }

        int frame = static_cast<int>(editorLayer->m_gameState.m_currentProgress / 2);
        m_fields->macroInput = true;
        macro.handlePlaying(this, frame);
        m_fields->macroInput = false;
    }
};
