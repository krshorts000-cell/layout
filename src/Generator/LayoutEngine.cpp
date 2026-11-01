#include "LayoutEngine.hpp"
#include "../Macro/MacroManager.hpp"
#include <random>

bool LayoutEngine::generate(LevelEditorLayer* editorLayer) {
    if (!editorLayer) return false;

    // Read mod settings
    float bpm = Mod::get()->getSettingValue<float>("bpm");
    if (bpm <= 0.0f) bpm = 128.0f;
    int obstacleCount = static_cast<int>(Mod::get()->getSettingValue<int64_t>("obstacle-count"));
    if (obstacleCount <= 0) obstacleCount = 30;
    bool includeOrbs = Mod::get()->getSettingValue<bool>("include-orbs");
    bool autoReplay = Mod::get()->getSettingValue<bool>("auto-replay");

    // Physics constants for GD 2.2 (1x speed = 311.57559 units/sec at 240 TPS)
    const float unitsPerFrame = 1.2982316f;
    const float groundY = 120.0f; // 105.0f (floor collision) + 15.0f (half block height)
    const float blockSize = 30.0f;

    // Distance per beat
    float secondsPerBeat = 60.0f / bpm;
    float unitsPerBeat = secondsPerBeat * 311.57559f;
    if (unitsPerBeat < 130.0f) unitsPerBeat = 130.0f; // Ensure minimum spacing to land

    auto& macro = MacroManager::get();
    macro.clear();

    // Start placing right in view of the editor camera (player spawns at X=15)
    float currentX = 180.0f;
    std::mt19937 rng(1337);
    std::uniform_int_distribution<int> typeDist(0, includeOrbs ? 3 : 2);

    int placedObjects = 0;

    for (int i = 0; i < obstacleCount; ++i) {
        currentX += unitsPerBeat;
        int patternType = typeDist(rng);

        switch (patternType) {
            case 0: { // Single Spike
                if (editorLayer->createObject(8, ccp(currentX, groundY), true)) {
                    placedObjects++;
                }

                float jumpX = currentX - 38.0f;
                int pressFrame = static_cast<int>((jumpX - 15.0f) / unitsPerFrame);
                if (pressFrame < 0) pressFrame = 0;
                int releaseFrame = pressFrame + 16;

                macro.recordAction(pressFrame, 1, false, true);
                macro.recordAction(releaseFrame, 1, false, false);
                break;
            }
            case 1: { // Double Spike
                if (editorLayer->createObject(8, ccp(currentX, groundY), true)) placedObjects++;
                if (editorLayer->createObject(8, ccp(currentX + blockSize, groundY), true)) placedObjects++;

                float jumpX = currentX - 44.0f;
                int pressFrame = static_cast<int>((jumpX - 15.0f) / unitsPerFrame);
                if (pressFrame < 0) pressFrame = 0;
                int releaseFrame = pressFrame + 22;

                macro.recordAction(pressFrame, 1, false, true);
                macro.recordAction(releaseFrame, 1, false, false);

                currentX += blockSize;
                break;
            }
            case 2: { // Platform with jump (2 blocks elevated with a ground spike)
                float platY = groundY + 45.0f;
                if (editorLayer->createObject(8, ccp(currentX, groundY), true)) placedObjects++;
                if (editorLayer->createObject(1, ccp(currentX + blockSize, platY), true)) placedObjects++;
                if (editorLayer->createObject(1, ccp(currentX + blockSize * 2.0f, platY), true)) placedObjects++;

                // Jump onto platform
                float jumpX = currentX - 42.0f;
                int pressFrame = static_cast<int>((jumpX - 15.0f) / unitsPerFrame);
                if (pressFrame < 0) pressFrame = 0;
                int releaseFrame = pressFrame + 18;

                macro.recordAction(pressFrame, 1, false, true);
                macro.recordAction(releaseFrame, 1, false, false);

                // Jump off platform
                float jumpOffX = currentX + blockSize * 1.8f;
                int press2 = static_cast<int>((jumpOffX - 15.0f) / unitsPerFrame);
                int release2 = press2 + 16;
                macro.recordAction(press2, 1, false, true);
                macro.recordAction(release2, 1, false, false);

                currentX += blockSize * 2.0f;
                break;
            }
            case 3: { // Yellow Orb jump (spike on floor, orb above)
                float orbY = groundY + 50.0f;
                if (editorLayer->createObject(8, ccp(currentX, groundY), true)) placedObjects++;
                if (editorLayer->createObject(36, ccp(currentX, orbY), true)) placedObjects++;

                // Jump from ground before spike
                float jumpX = currentX - 45.0f;
                int press1 = static_cast<int>((jumpX - 15.0f) / unitsPerFrame);
                if (press1 < 0) press1 = 0;
                int release1 = press1 + 10;
                macro.recordAction(press1, 1, false, true);
                macro.recordAction(release1, 1, false, false);

                // Click orb in air
                float hitX = currentX - 5.0f;
                int press2 = static_cast<int>((hitX - 15.0f) / unitsPerFrame);
                int release2 = press2 + 6;
                macro.recordAction(press2, 1, false, true);
                macro.recordAction(release2, 1, false, false);
                break;
            }
        }
    }

    std::sort(macro.inputs.begin(), macro.inputs.end());
    std::string savedPath = macro.saveToFile("xdbot_generated");

    FLAlertLayer::create(
        "Layout & Bot Ready",
        fmt::format(
            "Created objects: {}\nGenerated clicks: {}\nXDBot macro prepared and loaded!",
            placedObjects,
            macro.inputs.size()
        ),
        "OK"
    )->show();

    if (autoReplay) {
        macro.state = MacroState::Playing;
        macro.resetPlayback();
        Notification::create("Bot: ON! Start playtest.", NotificationIcon::Success)->show();
    }

    return true;
}
