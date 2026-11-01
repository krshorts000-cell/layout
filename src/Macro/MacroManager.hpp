#pragma once
#include <Geode/Geode.hpp>
#include <vector>
#include <filesystem>
#include <fstream>
#include <algorithm>

using namespace geode::prelude;

enum class MacroState {
    None,
    Recording,
    Playing
};

struct MacroInput {
    int frame = 0;
    int button = 1;      // 1 = Jump, 2 = Left, 3 = Right
    bool player2 = false;
    bool isDown = false; // true = Press, false = Release

    bool operator<(const MacroInput& other) const {
        return frame < other.frame;
    }
};

class MacroManager {
public:
    static MacroManager& get() {
        static MacroManager instance;
        return instance;
    }

    MacroState state = MacroState::None;
    size_t currentAction = 0;
    std::vector<MacroInput> inputs;
    bool isSimulating = false;

    bool isPlaying() const { return state == MacroState::Playing; }
    bool isRecording() const { return state == MacroState::Recording; }

    void recordAction(int frame, int button, bool player2, bool down) {
        inputs.push_back({frame, button, player2, down});
        log::debug("[XDBot] Записано: кадр={}, кнопка={}, зажата={}", frame, button, down);
    }

    void handlePlaying(GJBaseGameLayer* layer, int frame) {
        if (!layer) return;

        isSimulating = true;
        while (currentAction < inputs.size() && frame >= inputs[currentAction].frame) {
            const auto& input = inputs[currentAction];
            layer->handleButton(input.isDown, input.button, input.player2);
            currentAction++;
        }
        isSimulating = false;
    }

    void resetPlayback() {
        currentAction = 0;
        isSimulating = false;
    }

    void clear() {
        inputs.clear();
        currentAction = 0;
        state = MacroState::None;
        isSimulating = false;
    }

    void togglePlaying() {
        if (state == MacroState::Playing) {
            state = MacroState::None;
            resetPlayback();
            Notification::create("Бот: ВЫКЛ", NotificationIcon::Warning)->show();
        } else {
            if (inputs.empty()) {
                Notification::create("Нет макроса! Нажмите Gen или Rec.", NotificationIcon::Error)->show();
                return;
            }
            std::sort(inputs.begin(), inputs.end());
            state = MacroState::Playing;
            resetPlayback();
            Notification::create(fmt::format("Бот: ВКЛ (XDBot, кликов: {})", inputs.size()), NotificationIcon::Success)->show();
        }
    }

    void toggleRecording() {
        if (state == MacroState::Recording) {
            state = MacroState::None;
            std::sort(inputs.begin(), inputs.end());
            Notification::create(fmt::format("Запись завершена (кликов: {})", inputs.size()), NotificationIcon::Success)->show();
            saveToFile("recorded_macro");
        } else {
            clear();
            state = MacroState::Recording;
            Notification::create("Запись макроса началась! Играйте уровень.", NotificationIcon::Info)->show();
        }
    }

    std::string saveToFile(const std::string& baseName = "xdbot_macro");
    bool loadFromFile(const std::filesystem::path& path);
};
