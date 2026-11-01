#include "Macro.hpp"
#include <Geode/binding/PlayLayer.hpp>
#include <Geode/binding/LevelEditorLayer.hpp>
#include <Geode/binding/GJGameLevel.hpp>
#include <Geode/binding/GameManager.hpp>
#include <Geode/loader/Dirs.hpp>
#include <fstream>
#include <chrono>

int Global::getCurrentFrame(bool editor) {
    PlayLayer *pl = PlayLayer::get();
    auto *editorLayer = LevelEditorLayer::get();
    auto &g = Global::get();

    int frame = 0;

    if (!pl) {
        if (!editorLayer || editorLayer->m_playbackMode != PlaybackMode::Playing)
            return 0;

        frame = static_cast<int>(editorLayer->m_gameState.m_currentProgress / 2);
    } else {
        if (!g.macro.xdBotMacro && g.state == state::playing) {
            frame = pl->m_gameState.m_currentProgress;
        } else {
            frame = static_cast<int>(pl->m_gameState.m_levelTime * getTPS());
            frame++;
        }
    }

    frame -= g.frameOffset;
    if (frame < 0)
        return 0;

    return frame;
}

void Macro::updateInfo(GJBaseGameLayer* bgl) {
    if (!bgl) return;
    auto &g = Global::get();
    GJGameLevel *level = bgl->m_level;

    g.macro.botInfo.name = "xdBot";
    g.macro.botInfo.version = "v2.6-Nako";
    g.macro.framerate = 240.f;
    g.macro.gameVersion = 2.2081f;
    g.macro.version = 1.0f;
    g.macro.author = "LayoutGenerator";
    g.macro.description = "Generated with LayoutGenerator (xdBot GDR format)";

    if (level) {
        g.macro.levelInfo.id = level->m_levelID.value();
        g.macro.levelInfo.name = level->m_levelName;
    }
}

bool Macro::flipControls() {
    PlayLayer *pl = PlayLayer::get();
    if (!pl) return false;
    return pl->m_levelSettings->m_platformerMode ? false : GameManager::get()->getGameVariable("0010");
}

void Macro::recordAction(int frame, int button, bool player2, bool hold) {
    auto &g = Global::get();
    PlayLayer *pl = PlayLayer::get();
    auto *editorLayer = LevelEditorLayer::get();
    if (!pl && !editorLayer) return;

    if (g.macro.inputs.empty()) {
        Macro::updateInfo(pl ? static_cast<GJBaseGameLayer*>(pl) : static_cast<GJBaseGameLayer*>(editorLayer));
    }

    if (Macro::flipControls())
        player2 = !player2;

    // Deduplication check: do not record duplicate state for the same button and player
    if (!g.macro.inputs.empty()) {
        const auto& last = g.macro.inputs.back();
        if (last.button == button && last.player2 == player2 && last.down == hold) {
            return;
        }
    }

    g.macro.inputs.push_back(input(frame, button, player2, hold));
    log::debug("[XDBotMacro] Recorded frame={}, btn={}, 2p={}, down={}", frame, button, player2, hold);
}

void Macro::resetPlayback() {
    auto &g = Global::get();
    g.currentAction = 0;
    g.previousFrame = 0;
}

void Macro::togglePlaying() {
    auto &g = Global::get();
    if (g.state == state::playing) {
        g.state = state::none;
        resetPlayback();
        if (auto pl = PlayLayer::get()) {
            if (pl->m_player1) pl->m_player1->releaseAllButtons();
            if (pl->m_player2) pl->m_player2->releaseAllButtons();
        }
        if (auto el = LevelEditorLayer::get()) {
            if (el->m_player1) el->m_player1->releaseAllButtons();
            if (el->m_player2) el->m_player2->releaseAllButtons();
        }
        Notification::create("Bot: OFF", NotificationIcon::Warning)->show();
    } else {
        if (g.macro.inputs.empty()) {
            Notification::create("No macro! Generate layout first.", NotificationIcon::Error)->show();
            return;
        }
        std::sort(g.macro.inputs.begin(), g.macro.inputs.end());
        g.state = state::playing;
        resetPlayback();
        Notification::create(fmt::format("Bot: ON ({} inputs)", g.macro.inputs.size()), NotificationIcon::Success)->show();
    }
}

int Macro::save(std::string author, std::string desc, std::string levelName, bool json) {
    auto &g = Global::get();
    if (g.macro.inputs.empty())
        return 31;

    std::sort(g.macro.inputs.begin(), g.macro.inputs.end());

    std::string safeName = levelName.empty() ? "layout" : levelName;
    for (char& c : safeName) {
        if (c == '/' || c == '\\' || c == ':' || c == '*' || c == '?' || c == '"' || c == '<' || c == '>' || c == '|') {
            c = '_';
        }
    }

    g.macro.author = author;
    g.macro.description = desc;
    g.macro.framerate = 240.f;
    g.macro.duration = static_cast<float>(g.macro.inputs.back().frame) / g.macro.framerate;
    g.macro.lastRecordedFrame = g.macro.inputs.back().frame;
    g.macro.botInfo.name = "xdBot";
    g.macro.botInfo.version = "v2.6-Nako";
    g.macro.levelInfo.name = safeName;

    // Export both JSON and msgpack
    auto dataJson = g.macro.exportData(true);
    auto dataGdr = g.macro.exportData(false);

    std::vector<std::filesystem::path> targetDirs = {
        std::filesystem::path("D:/SteamLibrary/steamapps/common/Geometry Dash/macros"),
        geode::dirs::getGameDir() / "macros",
        Mod::get()->getSaveDir() / "macros"
    };

    auto timestamp = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    for (const auto& dir : targetDirs) {
        std::error_code ec;
        std::filesystem::create_directories(dir, ec);

        auto jsonPath = dir / fmt::format("{}_{}.gdr.json", safeName, timestamp);
        auto gdrPath = dir / fmt::format("{}_{}.gdr", safeName, timestamp);
        auto altJsonPath = dir / fmt::format("{}_{}.json", safeName, timestamp);

        std::ofstream fj(jsonPath, std::ios::binary);
        if (fj.is_open()) {
            fj.write(reinterpret_cast<const char*>(dataJson.data()), dataJson.size());
            fj.close();
        }

        std::ofstream fg(gdrPath, std::ios::binary);
        if (fg.is_open()) {
            fg.write(reinterpret_cast<const char*>(dataGdr.data()), dataGdr.size());
            fg.close();
        }

        std::ofstream faj(altJsonPath, std::ios::binary);
        if (faj.is_open()) {
            faj.write(reinterpret_cast<const char*>(dataJson.data()), dataJson.size());
            faj.close();
        }
    }

    log::info("[XDBotMacro] Saved macro with {} inputs", g.macro.inputs.size());
    return 0;
}
