#include "Syncatron.hpp"
#include "../Macro/Macro.hpp"
#include <Geode/binding/DrawGridLayer.hpp>
#include <fstream>
#include <sstream>
#include <cmath>
#include <algorithm>

#ifdef GEODE_IS_WINDOWS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commdlg.h>
#endif

namespace Syncatron {
    static std::vector<BeatInfo> s_activeBeats;
    static std::vector<float> s_rawTimestamps;
    static std::vector<float> s_guidelines;
    static size_t s_beatCursor = 0;
    static float s_offsetSeconds = 0.0f;
    static MapMetadata s_metadata;
    static std::filesystem::path s_lastLoadedPath;

    std::vector<BeatInfo>& getActiveBeats() {
        return s_activeBeats;
    }

    bool hasActiveBeats() {
        return !s_activeBeats.empty() || !s_guidelines.empty();
    }

    void clearActiveBeats() {
        s_activeBeats.clear();
        s_rawTimestamps.clear();
        s_guidelines.clear();
        s_beatCursor = 0;
        s_metadata = MapMetadata{};
    }

    std::vector<float>& getGuidelines() {
        return s_guidelines;
    }

    void setGuidelines(const std::vector<float>& guides) {
        s_guidelines = guides;
        std::sort(s_guidelines.begin(), s_guidelines.end());

        s_activeBeats.clear();
        for (float t : s_guidelines) {
            BeatInfo b;
            b.timeSeconds = t;
            b.frame240 = static_cast<int>(std::round(t * 240.0f));
            s_activeBeats.push_back(b);
        }
        applyBeatsToMacro();
    }

    void loadGuidelinesFromLevel() {
        auto editor = LevelEditorLayer::get();
        if (!editor || !editor->m_levelSettings) return;

        std::string raw = editor->m_levelSettings->m_guidelineString;
        if (raw.empty()) return;

        s_guidelines.clear();
        std::stringstream ss(raw);
        std::string item;
        bool isTime = true;
        while (std::getline(ss, item, '~')) {
            if (item.empty()) continue;
            if (isTime) {
                try {
                    float t = std::stof(item);
                    if (t >= 0.f) s_guidelines.push_back(t);
                } catch (...) {}
            }
            isTime = !isTime;
        }

        std::sort(s_guidelines.begin(), s_guidelines.end());
        s_activeBeats.clear();
        for (float t : s_guidelines) {
            BeatInfo b;
            b.timeSeconds = t;
            b.frame240 = static_cast<int>(std::round(t * 240.0f));
            s_activeBeats.push_back(b);
        }
        applyBeatsToMacro();
    }

    MapMetadata& getMetadata() {
        return s_metadata;
    }

    float getOffset() {
        return s_offsetSeconds;
    }

    static void recalculateBeats() {
        s_activeBeats.clear();
        s_guidelines.clear();
        s_beatCursor = 0;

        if (s_rawTimestamps.empty()) return;

        float lastT = -1.0f;
        for (float rawT : s_rawTimestamps) {
            float t = rawT + s_offsetSeconds;
            if (t < 0.0f) continue;
            if (lastT >= 0.0f && std::abs(t - lastT) < 0.02f) continue;

            lastT = t;
            BeatInfo b;
            b.timeSeconds = t;
            b.frame240 = static_cast<int>(std::round(t * 240.0f));
            s_activeBeats.push_back(b);
            s_guidelines.push_back(t);
        }

        s_metadata.totalNotes = static_cast<int>(s_activeBeats.size());
        if (!s_activeBeats.empty()) {
            s_metadata.durationSeconds = s_activeBeats.back().timeSeconds;
        }
        applyBeatsToMacro();
    }

    void setOffset(float offsetSec) {
        s_offsetSeconds = offsetSec;
        recalculateBeats();
    }

    void adjustOffset(float deltaSec) {
        s_offsetSeconds += deltaSec;
        recalculateBeats();
    }

    void resetBeatCursor() {
        s_beatCursor = 0;
    }

    bool checkBeat(float elapsedTime) {
        // Prefer s_activeBeats, fallback to s_guidelines
        if (!s_activeBeats.empty()) {
            if (s_beatCursor >= s_activeBeats.size()) return false;
            if (elapsedTime >= s_activeBeats[s_beatCursor].timeSeconds) {
                s_beatCursor++;
                return true;
            }
            return false;
        }

        if (!s_guidelines.empty()) {
            if (s_beatCursor >= s_guidelines.size()) return false;
            if (elapsedTime >= s_guidelines[s_beatCursor]) {
                s_beatCursor++;
                return true;
            }
        }
        return false;
    }

    std::string generateGuidelineString(float colorCode) {
        const auto& list = !s_guidelines.empty() ? s_guidelines : [&]() {
            static std::vector<float> times;
            times.clear();
            for (auto const& b : s_activeBeats) times.push_back(b.timeSeconds);
            return times;
        }();

        if (list.empty()) return "";
        std::string res;
        for (float t : list) {
            res += fmt::format("{:.2f}~{:.1f}~", t, colorCode);
        }
        return res;
    }

    bool applyGuidelinesToEditor(float colorCode) {
        auto editor = LevelEditorLayer::get();
        if (!editor || !editor->m_levelSettings) return false;

        std::string guidelineStr = generateGuidelineString(colorCode);
        editor->m_levelSettings->m_guidelineString = guidelineStr;

        // Instantly reload visual time markers on DrawGridLayer
        if (auto grid = editor->getChildByType<DrawGridLayer>(0)) {
            grid->loadTimeMarkers(guidelineStr);
        }

        return true;
    }

    bool clearGuidelines() {
        auto editor = LevelEditorLayer::get();
        if (!editor || !editor->m_levelSettings) return false;

        s_guidelines.clear();
        s_activeBeats.clear();
        s_beatCursor = 0;

        editor->m_levelSettings->m_guidelineString = "";
        if (auto grid = editor->getChildByType<DrawGridLayer>(0)) {
            grid->loadTimeMarkers("");
        }

        return true;
    }

    bool applyBeatsToMacro() {
        const auto& times = !s_guidelines.empty() ? s_guidelines : [&]() {
            static std::vector<float> tms;
            tms.clear();
            for (auto const& b : s_activeBeats) tms.push_back(b.timeSeconds);
            return tms;
        }();

        if (times.empty()) return false;

        auto& g = Global::get();
        g.macro.inputs.clear();
        g.macro.framerate = 240.f;

        for (size_t i = 0; i < times.size(); ++i) {
            int downFrame = static_cast<int>(std::round(times[i] * 240.0f));
            int holdFrames = 15;
            if (i + 1 < times.size()) {
                int nextFrame = static_cast<int>(std::round(times[i + 1] * 240.0f));
                int gap = nextFrame - downFrame;
                if (gap > 4) {
                    holdFrames = std::clamp(gap / 2, 8, 30);
                }
            }
            int upFrame = downFrame + holdFrames;

            g.macro.inputs.push_back(input(downFrame, 1, false, true));
            g.macro.inputs.push_back(input(upFrame, 1, false, false));
        }

        std::stable_sort(g.macro.inputs.begin(), g.macro.inputs.end());
        Macro::resetPlayback();
        return true;
    }

    bool loadOsuFile(const std::filesystem::path& path, float offsetSeconds) {
        std::ifstream file(path);
        if (!file.is_open()) return false;

        s_lastLoadedPath = path;
        s_offsetSeconds = offsetSeconds;
        s_metadata = MapMetadata{};
        s_metadata.fileName = path.filename().string();

        std::string line;
        bool inMetadata = false;
        bool inHitObjects = false;
        s_rawTimestamps.clear();

        while (std::getline(file, line)) {
            while (!line.empty() && (line.back() == '\r' || line.back() == '\n' || line.back() == ' ')) {
                line.pop_back();
            }
            if (line.empty()) continue;

            if (line == "[Metadata]") {
                inMetadata = true;
                inHitObjects = false;
                continue;
            }
            if (line == "[HitObjects]") {
                inHitObjects = true;
                inMetadata = false;
                continue;
            }

            if (inMetadata) {
                if (line[0] == '[') inMetadata = false;
                else if (line.rfind("Title:", 0) == 0) {
                    s_metadata.title = line.substr(6);
                }
                else if (line.rfind("Artist:", 0) == 0) {
                    s_metadata.artist = line.substr(7);
                }
            }

            if (inHitObjects) {
                if (line[0] == '[') break;

                std::stringstream ss(line);
                std::string xStr, yStr, timeStr;
                if (std::getline(ss, xStr, ',') &&
                    std::getline(ss, yStr, ',') &&
                    std::getline(ss, timeStr, ',')) {
                    try {
                        double timeMs = std::stod(timeStr);
                        float timeSec = static_cast<float>(timeMs / 1000.0);
                        if (timeSec >= 0.0f) {
                            s_rawTimestamps.push_back(timeSec);
                        }
                    } catch (...) {}
                }
            }
        }

        if (s_rawTimestamps.empty()) return false;

        std::sort(s_rawTimestamps.begin(), s_rawTimestamps.end());
        recalculateBeats();

        applyGuidelinesToEditor(0.0f);

        return true;
    }

    std::vector<std::filesystem::path> getAvailableOsuFiles() {
        std::vector<std::filesystem::path> files;

        std::vector<std::filesystem::path> searchDirs = {
            geode::dirs::getModConfigDir() / "osuMaps",
            geode::dirs::getGameDir() / "osuMaps",
            std::filesystem::path("D:/layout/osuMaps")
        };

        for (const auto& dir : searchDirs) {
            std::error_code ec;
            if (std::filesystem::exists(dir, ec) && std::filesystem::is_directory(dir, ec)) {
                for (const auto& entry : std::filesystem::directory_iterator(dir, ec)) {
                    if (entry.is_regular_file() && entry.path().extension() == ".osu") {
                        files.push_back(entry.path());
                    }
                }
            }
        }
        return files;
    }

    bool pickAndLoadOsuFile() {
#ifdef GEODE_IS_WINDOWS
        char szFile[MAX_PATH] = {0};
        OPENFILENAMEA ofn;
        ZeroMemory(&ofn, sizeof(ofn));
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = NULL;
        ofn.lpstrFile = szFile;
        ofn.nMaxFile = sizeof(szFile);
        ofn.lpstrFilter = "osu! Beatmaps (*.osu)\0*.osu\0All Files (*.*)\0*.*\0";
        ofn.nFilterIndex = 1;
        ofn.lpstrTitle = "Select osu! Beatmap for Sync-a-tron";
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

        if (GetOpenFileNameA(&ofn)) {
            std::filesystem::path p(szFile);
            if (loadOsuFile(p, s_offsetSeconds)) {
                geode::Notification::create(
                    fmt::format("Sync-a-tron: Loaded {} notes ({})", s_metadata.totalNotes, s_metadata.fileName),
                    geode::NotificationIcon::Success
                )->show();
                return true;
            } else {
                geode::Notification::create("Sync-a-tron: No notes found in .osu file!", geode::NotificationIcon::Error)->show();
            }
        }
#else
        auto files = getAvailableOsuFiles();
        if (!files.empty()) {
            return loadOsuFile(files.front(), s_offsetSeconds);
        }
        geode::Notification::create("Place .osu map into osuMaps folder", geode::NotificationIcon::Info)->show();
#endif
        return false;
    }
}
