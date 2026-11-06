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

    struct MapMetadata {
        std::string fileName = "None";
        std::string title = "No Map Loaded";
        std::string artist = "Unknown";
        int totalNotes = 0;
        float durationSeconds = 0.0f;
    };

    // Stored active beats from the imported .osu map
    std::vector<BeatInfo>& getActiveBeats();
    bool hasActiveBeats();
    void clearActiveBeats();

    // Direct guidelines list (for interactive waveform placement & sync)
    std::vector<float>& getGuidelines();
    void setGuidelines(const std::vector<float>& guides);
    void loadGuidelinesFromLevel();

    // Current map metadata
    MapMetadata& getMetadata();

    // Offset in seconds
    float getOffset();
    void setOffset(float offsetSec);
    void adjustOffset(float deltaSec);

    // Check if player elapsed time hits the next beat during layout generation
    bool checkBeat(float elapsedTime);
    void resetBeatCursor();

    // Load .osu file with current offset
    bool loadOsuFile(const std::filesystem::path& path, float offsetSeconds = 0.0f);

    // Pick an .osu file via Windows dialog and load it
    bool pickAndLoadOsuFile();

    // List available .osu files in local osuMaps folders
    std::vector<std::filesystem::path> getAvailableOsuFiles();

    // Generate GD guideline string "t1~0~t2~0~...~tn~0~"
    std::string generateGuidelineString(float colorCode = 0.0f);

    // Apply guideline string directly to current LevelEditorLayer & DrawGridLayer
    bool applyGuidelinesToEditor(float colorCode = 0.0f);

    // Audacity Label Track Support (.txt)
    bool importAudacityLabels(const std::filesystem::path& path);
    bool exportAudacityLabels(const std::filesystem::path& path);
    bool pickAndImportAudacityLabels();
    bool pickAndExportAudacityLabels();

    // Audacity-style Beat & Onset Finder (sensitivity: 0 = Low/Drops, 1 = Medium/Standard, 2 = High/Dense)
    int autoDetectBeats(const std::vector<float>& samples, float songLength, int sensitivity = 1);

    // Clear guidelines from current level
    bool clearGuidelines();

    // Load active beats as macro inputs for bot / layout generation
    bool applyBeatsToMacro();
}
