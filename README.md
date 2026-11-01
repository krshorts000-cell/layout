# Layout & Auto-Macro Bot (Geometry Dash 2.2 / Geode)

Полноценный мод для **Geometry Dash 2.2+ (Geode)**, который объединяет:
1. **Процедурную генерацию лейаутов**: расставляет платформы, шипы и сферы под ритм (BPM) песни.
2. **Автоматическую запись макроса**: синхронно с генерацией каждого препятствия записывает точные кадры нажатий и отпусканий клавиш (240 FPS).
3. **Встроенный бот воспроизведения (Replay Bot)**: воспроизводит макрос прямо в редакторе уровней через `pushButton`/`releaseButton`, проходя уровень на 100% автоматически.
4. **Экспорт макросов**: сохраняет макрос в формате JSON в папку мода, который можно загружать в сторонние боты или использовать для шоукейсов.

---

## Структура проекта

- **[mod.json](file:///C:/Users/HI/.gemini/antigravity/scratch/layout-macro-generator/mod.json)** — конфигурация мода, зависимости и внутриигровые настройки (BPM, число препятствий, авто-плейбек).
- **[CMakeLists.txt](file:///C:/Users/HI/.gemini/antigravity/scratch/layout-macro-generator/CMakeLists.txt)** — CMake конфигурация со сборкой всех поддиректорий.
- **[src/Macro/MacroManager.hpp](file:///C:/Users/HI/.gemini/antigravity/scratch/layout-macro-generator/src/Macro/MacroManager.hpp)** / **[MacroManager.cpp](file:///C:/Users/HI/.gemini/antigravity/scratch/layout-macro-generator/src/Macro/MacroManager.cpp)** — ядро записи, воспроизведения и экспорта макросов.
- **[src/Generator/LayoutEngine.hpp](file:///C:/Users/HI/.gemini/antigravity/scratch/layout-macro-generator/src/Generator/LayoutEngine.hpp)** / **[LayoutEngine.cpp](file:///C:/Users/HI/.gemini/antigravity/scratch/layout-macro-generator/src/Generator/LayoutEngine.cpp)** — математический расчет траекторий, BPM-таймингов и спавн объектов.
- **[src/Hooks/EditorUI.cpp](file:///C:/Users/HI/.gemini/antigravity/scratch/layout-macro-generator/src/Hooks/EditorUI.cpp)** — кнопки **[Gen]** и **[Bot]** на верхней панели редактора.
- **[src/Hooks/LevelEditorLayer.cpp](file:///C:/Users/HI/.gemini/antigravity/scratch/layout-macro-generator/src/Hooks/LevelEditorLayer.cpp)** — хук обновления кадра при плейтесте в редакторе.
- **[src/Hooks/PlayerObject.cpp](file:///C:/Users/HI/.gemini/antigravity/scratch/layout-macro-generator/src/Hooks/PlayerObject.cpp)** — перехват ручных кликов для записи под живой геймплей.

---

## Как собрать и запустить

1. Откройте терминал в папке проекта:
   ```powershell
   cd "C:\Users\HI\.gemini\antigravity\scratch\layout-macro-generator"
   ```
2. Соберите и запустите мод через Geode CLI:
   ```powershell
   geode build
   geode run
   ```

## Как пользоваться в игре

1. Откройте любой уровень в **Редакторе Geometry Dash**.
2. В верхнем меню (рядом с кнопкой проигрывания музыки) появятся две новые кнопки:
   - **`[Gen]`** — генерирует лейаут по заданному BPM, рассчитывает тайминги и сохраняет файл макроса.
   - **`[Bot]`** — включает автоматического бота и запускает тестовый прогон: игрок сам идеально проходит только что созданный уровень!
3. Настройки мода (BPM, количество препятствий, авто-проигрывание) можно менять прямо во внутриигровом меню Geode.
