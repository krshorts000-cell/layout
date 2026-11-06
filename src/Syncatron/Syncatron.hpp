#pragma once
#include <Geode/Geode.hpp>
#include <vector>
#include <string>
#include <optional>
#include <filesystem>

namespace Syncatron {
    struct BeatInfo {
        float timeSeconds = 0.0f;
        int frame240 = 0;
    };

    // Global vector of active beat timestamps
    std::vector<BeatInfo>& getActiveBeats();
    bool hasActiveBeats();
    void clearActiveBeats();

    // Check if player elapsed time hits the next beat
    bool checkBeat(float elapsedTime);
    void resetBeatCursor();

    // Pick an .osu file via Windows dialog and load it
    bool pickAndLoadOsuFile();

    // Parse .osu file, extract timestamps, generate guidelines, and load into editor
    bool loadOsuFile(const std::filesystem::path& path, float offsetSeconds = 0.0f);

    // Generate GD guideline string "t1~0~t2~0~...~tn~0~"
    std::string generateGuidelineString(const std::vector<float>& timestamps);

    // Apply guideline string to current LevelEditorLayer
    bool applyGuidelinesToEditor(const std::string& guidelineStr);

    // Load active beats as macro inputs for bot / layout generation
    bool applyBeatsToMacro();
}
