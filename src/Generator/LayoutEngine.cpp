#include "LayoutEngine.hpp"
#include "../Macro/MacroManager.hpp"
#include <random>

bool LayoutEngine::generate(LevelEditorLayer* editorLayer) {
    if (!editorLayer) return false;

    // Считываем настройки мода
    float bpm = static_cast<float>(Mod::get()->getSettingValue<double>("bpm"));
    int obstacleCount = Mod::get()->getSettingValue<int64_t>("obstacle-count");
    bool includeOrbs = Mod::get()->getSettingValue<bool>("include-orbs");
    bool autoReplay = Mod::get()->getSettingValue<bool>("auto-replay");

    // Физические константы Geometry Dash (1x скорость)
    const float speedUnitsPerSec = 311.58f;
    const float fps = 240.0f;
    const float groundY = 105.0f;
    const float blockSize = 30.0f;

    // Расчет расстояния за 1 бит
    float secondsPerBeat = 60.0f / bpm;
    float unitsPerBeat = secondsPerBeat * speedUnitsPerSec;

    // Запускаем запись макроса
    auto macro = MacroManager::get();
    macro->startRecording(fps);

    float currentX = 350.0f; // Начальная точка генерации
    std::mt19937 rng(1337);  // Детерминированный генератор
    std::uniform_int_distribution<int> typeDist(0, includeOrbs ? 3 : 1);

    int placedObjects = 0;

    for (int i = 0; i < obstacleCount; ++i) {
        currentX += unitsPerBeat;
        int patternType = typeDist(rng);

        switch (patternType) {
            case 0: { // Одиночный шип на полу
                editorLayer->createObject(8, ccp(currentX, groundY), false);
                placedObjects++;

                // Рассчитываем тайминг прыжка перед шипом
                float jumpX = currentX - 40.0f;
                size_t pressFrame = static_cast<size_t>((jumpX / speedUnitsPerSec) * fps);
                size_t releaseFrame = pressFrame + 18; // Длительность нажатия (полный прыжок)

                macro->addEventDirect(pressFrame, true, false, 1);
                macro->addEventDirect(releaseFrame, false, false, 1);
                break;
            }
            case 1: { // Двойной шип на полу
                editorLayer->createObject(8, ccp(currentX, groundY), false);
                editorLayer->createObject(8, ccp(currentX + blockSize, groundY), false);
                placedObjects += 2;

                // Более ранний прыжок для двойного шипа
                float jumpX = currentX - 45.0f;
                size_t pressFrame = static_cast<size_t>((jumpX / speedUnitsPerSec) * fps);
                size_t releaseFrame = pressFrame + 22;

                macro->addEventDirect(pressFrame, true, false, 1);
                macro->addEventDirect(releaseFrame, false, false, 1);
                currentX += blockSize;
                break;
            }
            case 2: { // Желтая сфера над шипом
                float orbY = groundY + 55.0f;
                editorLayer->createObject(8, ccp(currentX, groundY), false);     // Шип внизу
                editorLayer->createObject(36, ccp(currentX, orbY), false);       // Желтая сфера
                placedObjects += 2;

                // Клик прямо в сфере
                float hitX = currentX - 8.0f;
                size_t pressFrame = static_cast<size_t>((hitX / speedUnitsPerSec) * fps);
                size_t releaseFrame = pressFrame + 8;

                macro->addEventDirect(pressFrame, true, false, 1);
                macro->addEventDirect(releaseFrame, false, false, 1);
                break;
            }
            case 3: { // Платформа с прыжком на нее
                float platY = groundY + 60.0f;
                editorLayer->createObject(1, ccp(currentX, platY), false);
                editorLayer->createObject(1, ccp(currentX + blockSize, platY), false);
                editorLayer->createObject(8, ccp(currentX, groundY), false); // Шип под платформой
                placedObjects += 3;

                float jumpX = currentX - 42.0f;
                size_t pressFrame = static_cast<size_t>((jumpX / speedUnitsPerSec) * fps);
                size_t releaseFrame = pressFrame + 20;

                macro->addEventDirect(pressFrame, true, false, 1);
                macro->addEventDirect(releaseFrame, false, false, 1);
                currentX += blockSize;
                break;
            }
        }
    }

    // Завершаем запись и сохраняем макрос
    macro->stopRecording();
    std::string savedPath = macro->saveToFile("generated_layout");

    FLAlertLayer::create(
        "Лейаут готов!",
        fmt::format(
            "Создано объектов: {}\nЗаписано кликов: {}\nФайл макроса сохранен в:\n{}",
            placedObjects,
            macro->getEventCount(),
            savedPath
        ),
        "Отлично"
    )->show();

    // Если включен авто-реплей — подготавливаем воспроизведение
    if (autoReplay) {
        macro->startPlayback();
    }

    return true;
}
