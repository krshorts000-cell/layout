#include <Geode/Geode.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/binding/LevelEditorLayer.hpp>
#include <Geode/binding/PlayLayer.hpp>
#include "Macro/Macro.hpp"

using namespace geode::prelude;

class $modify(BGLHook, GJBaseGameLayer) {
    struct Fields {
        bool macroInput = false;
    };

    void processCommands(float dt, bool isHalfTick, bool isLastTick) {
        auto &g = Global::get();

        PlayLayer *pl = PlayLayer::get();
        auto *editorLayer = typeinfo_cast<LevelEditorLayer *>(this);
        bool editorPlaytest = !pl && editorLayer && editorLayer->m_playbackMode == PlaybackMode::Playing;

        if (!pl && !editorPlaytest) {
            return GJBaseGameLayer::processCommands(dt, isHalfTick, isLastTick);
        }

        if (editorPlaytest) {
            GJBaseGameLayer::processCommands(dt, isHalfTick, isLastTick);

            if (g.state != state::playing)
                return;

            int frame = Global::getCurrentFrame(true);
            if (frame < g.previousFrame) {
                g.currentAction = 0;
            }
            g.previousFrame = frame;
            handlePlaying(frame, true);
            return;
        }

        GJBaseGameLayer::processCommands(dt, isHalfTick, isLastTick);

        if (g.state == state::none)
            return;

        int frame = Global::getCurrentFrame();
        if (frame < g.previousFrame) {
            g.currentAction = 0;
        }
        g.previousFrame = frame;

        if (g.state == state::playing)
            handlePlaying(frame, false);
    }

    void handlePlaying(int frame, bool isEditor) {
        auto &g = Global::get();
        if (m_levelEndAnimationStarted)
            return;

        if (m_player1 && m_player1->m_isDead) {
            m_player1->releaseAllButtons();
            if (m_player2) m_player2->releaseAllButtons();
            return;
        }

        m_fields->macroInput = true;

        while (g.currentAction < g.macro.inputs.size() &&
               frame >= g.macro.inputs[g.currentAction].frame) {
            auto input = g.macro.inputs[g.currentAction];

            if (Macro::flipControls())
                input.player2 = !input.player2;

            GJBaseGameLayer::handleButton(input.down, input.button, input.player2);

            if (isEditor && m_player1 && !input.player2) {
                if (input.down) m_player1->pushButton((PlayerButton)input.button);
                else m_player1->releaseButton((PlayerButton)input.button);
            }

            g.currentAction++;
        }

        m_fields->macroInput = false;
    }

    void handleButton(bool hold, int button, bool player2) {
        auto &g = Global::get();

        int frame = Global::getCurrentFrame();
        bool isRecording = (g.state == state::recording);
        bool isPlaying = (g.state == state::playing);

        // Initial check for ignore input during playback
        if (isPlaying && g.macro_ignore_inputs && !m_fields->macroInput)
            return;

        // Original call
        GJBaseGameLayer::handleButton(hold, button, player2);

        // Regular Recording
        if (isRecording && !g.ignoreRecordAction && !m_fields->macroInput &&
            (!m_player1 || !m_player1->m_isDead)) {
            if (!m_levelSettings || !m_levelSettings->m_twoPlayerMode)
                player2 = false;
            Macro::recordAction(frame, button, player2, hold);
        }
    }
};
