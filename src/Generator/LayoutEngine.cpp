#include "LayoutEngine.hpp"
#include "../Macro/MacroManager.hpp"
#include <random>

bool LayoutEngine::generate(LevelEditorLayer* editorLayer) {
    if (!editorLayer) return false;

    // Считываем настройки мода
    float bpm = Mod::get()->getSettingValue<float>("bpm");
    int obstacleCount = static_cast<int>(Mod::get()->getSettingValue<int64_t>("obstacle-count"));
    bool includeOrbs = Mod::get()->getSettingValue<bool>("include-orbs");
    bool autoReplay = Mod::get()->getSettingValue<bool>("auto-replay");

    // Константы физики Geometry Dash 2.2 (1x скорость)
    const float unitsPerFrame = 1.29825f; // 311.58 units/sec / 240 TPS
    const float groundY = 105.0f;
    const float blockSize = 30.0f;

    // Расчет расстояния за 1 бит
    float secondsPerBeat = 60.0f / bpm;
    float unitsPerBeat = secondsPerBeat * 311.58f;

    auto& macro = MacroManager::get();
    macro.clear();

    float currentX = 380.0f; // Начальная точка генерации
    std::mt19937 rng(42);
    std::uniform_int_distribution<int> typeDist(0, includeOrbs ? 3 : 1);

    int placedObjects = 0;

    for (int i = 0; i < obstacleCount; ++i) {
        currentX += unitsPerBeat;
        int patternType = typeDist(rng);

        switch (patternType) {
            case 0: { // Одиночный шип
                editorLayer->createObject(8, ccp(currentX, groundY), true);
                placedObjects++;

                // Рассчитываем точный кадр прыжка (за 35 единиц до шипа)
                float jumpX = currentX - 35.0f;
                int pressFrame = static_cast<int>(jumpX / unitsPerFrame);
                int releaseFrame = pressFrame + 18;

                macro.recordAction(pressFrame, 1, false, true);
                macro.recordAction(releaseFrame, 1, false, false);
                break;
            }
            case 1: { // Двойной шип
                editorLayer->createObject(8, ccp(currentX, groundY), true);
                editorLayer->createObject(8, ccp(currentX + blockSize, groundY), true);
                placedObjects += 2;

                // Более ранний прыжок под двойной шип
                float jumpX = currentX - 42.0f;
                int pressFrame = static_cast<int>(jumpX / unitsPerFrame);
                int releaseFrame = pressFrame + 22;

                macro.recordAction(pressFrame, 1, false, true);
                macro.recordAction(releaseFrame, 1, false, false);
                currentX += blockSize;
                break;
            }
            case 2: { // Желтая сфера над шипом
                float orbY = groundY + 55.0f;
                editorLayer->createObject(8, ccp(currentX, groundY), true);
                editorLayer->createObject(36, ccp(currentX, orbY), true);
                placedObjects += 2;

                // Клик точно на позиции сферы
                float hitX = currentX - 6.0f;
                int pressFrame = static_cast<int>(hitX / unitsPerFrame);
                int releaseFrame = pressFrame + 6;

                macro.recordAction(pressFrame, 1, false, true);
                macro.recordAction(releaseFrame, 1, false, false);
                break;
            }
            case 3: { // Платформа с прыжком
                float platY = groundY + 60.0f;
                editorLayer->createObject(1, ccp(currentX, platY), true);
                editorLayer->createObject(1, ccp(currentX + blockSize, platY), true);
                editorLayer->createObject(8, ccp(currentX, groundY), true);
                placedObjects += 3;

                float jumpX = currentX - 40.0f;
                int pressFrame = static_cast<int>(jumpX / unitsPerFrame);
                int releaseFrame = pressFrame + 20;

                macro.recordAction(pressFrame, 1, false, true);
                macro.recordAction(releaseFrame, 1, false, false);
                currentX += blockSize;
                break;
            }
        }
    }

    std::sort(macro.inputs.begin(), macro.inputs.end());
    std::string savedPath = macro.saveToFile("xdbot_generated");

    FLAlertLayer::create(
        "Лейаут и XDBot готовы!",
        fmt::format(
            "Создано объектов: {}\nСгенерировано кликов: {}\nМакрос записан в движок XDBot!",
            placedObjects,
            macro.inputs.size()
        ),
        "OK"
    )->show();

    if (autoReplay) {
        macro.state = MacroState::Playing;
        macro.resetPlayback();
        Notification::create("Бот активирован! Запустите плейтест.", NotificationIcon::Success)->show();
    }

    return true;
}
