#include "Syncatron.hpp"
#include "../Macro/Macro.hpp"
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
    static size_t s_beatCursor = 0;

    std::vector<BeatInfo>& getActiveBeats() {
        return s_activeBeats;
    }

    bool hasActiveBeats() {
        return !s_activeBeats.empty();
    }

    void clearActiveBeats() {
        s_activeBeats.clear();
        s_beatCursor = 0;
    }

    void resetBeatCursor() {
        s_beatCursor = 0;
    }

    bool checkBeat(float elapsedTime) {
        if (s_activeBeats.empty()) return false;
        if (s_beatCursor >= s_activeBeats.size()) return false;

        // Trigger onBeat when player time reaches the current beat timestamp
        if (elapsedTime >= s_activeBeats[s_beatCursor].timeSeconds) {
            s_beatCursor++;
            return true;
        }
        return false;
    }

    std::string generateGuidelineString(const std::vector<float>& timestamps) {
        if (timestamps.empty()) return "";
        std::string res;
        for (float t : timestamps) {
            res += fmt::format("{:.2f}~0~", t);
        }
        return res;
    }

    bool applyGuidelinesToEditor(const std::string& guidelineStr) {
        auto editor = LevelEditorLayer::get();
        if (!editor || !editor->m_levelSettings) return false;

        editor->m_levelSettings->m_guidelineString = guidelineStr;
        return true;
    }

    bool applyBeatsToMacro() {
        if (s_activeBeats.empty()) return false;

        auto& g = Global::get();
        g.macro.inputs.clear();
        g.macro.framerate = 240.f;

        // Generate down and up clicks from beat timestamps
        for (size_t i = 0; i < s_activeBeats.size(); ++i) {
            int downFrame = s_activeBeats[i].frame240;
            int holdFrames = 15;
            if (i + 1 < s_activeBeats.size()) {
                int gap = s_activeBeats[i + 1].frame240 - downFrame;
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

        std::string line;
        bool inHitObjects = false;
        std::vector<float> timestamps;

        while (std::getline(file, line)) {
            while (!line.empty() && (line.back() == '\r' || line.back() == '\n' || line.back() == ' ')) {
                line.pop_back();
            }
            if (line.empty()) continue;

            if (line == "[HitObjects]") {
                inHitObjects = true;
                continue;
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
                        float timeSec = static_cast<float>(timeMs / 1000.0) + offsetSeconds;
                        if (timeSec >= 0.0f) {
                            timestamps.push_back(timeSec);
                        }
                    } catch (...) {}
                }
            }
        }

        if (timestamps.empty()) return false;

        std::sort(timestamps.begin(), timestamps.end());

        s_activeBeats.clear();
        s_beatCursor = 0;
        std::vector<float> uniqueTimes;
        float lastT = -1.0f;
        for (float t : timestamps) {
            if (lastT >= 0.0f && std::abs(t - lastT) < 0.02f) continue;
            lastT = t;
            uniqueTimes.push_back(t);

            BeatInfo b;
            b.timeSeconds = t;
            b.frame240 = static_cast<int>(std::round(t * 240.0f));
            s_activeBeats.push_back(b);
        }

        std::string guidelines = generateGuidelineString(uniqueTimes);
        applyGuidelinesToEditor(guidelines);
        applyBeatsToMacro();

        return true;
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
            if (loadOsuFile(p, 0.0f)) {
                geode::Notification::create(
                    fmt::format("Sync-a-tron: Loaded {} notes from {}", s_activeBeats.size(), p.filename().string()),
                    geode::NotificationIcon::Success
                )->show();
                return true;
            } else {
                geode::Notification::create("Sync-a-tron: No notes found in .osu file!", geode::NotificationIcon::Error)->show();
            }
        }
#else
        auto dir = geode::dirs::getModConfigDir() / "osuMaps";
        std::filesystem::create_directories(dir);
        geode::Notification::create("Place .osu map into osuMaps folder", geode::NotificationIcon::Info)->show();
#endif
        return false;
    }
}
