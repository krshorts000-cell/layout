#pragma once
#include <Geode/Geode.hpp>
#include <vector>
#include <filesystem>
#include <fstream>

using namespace geode::prelude;

struct MacroEvent {
    size_t frame;      // Номер кадра (при стандартных 240 FPS)
    bool isDown;       // true = нажатие (press), false = отпускание (release)
    bool isPlayer2;    // Для режима двух игроков (дуалы)
    int button;        // 1 = прыжок, 2 = влево, 3 = вправо
};

enum class MacroState {
    Idle,
    Recording,
    Playing
};

class MacroManager {
private:
    MacroState m_state = MacroState::Idle;
    float m_fps = 240.0f;
    size_t m_currentFrame = 0;
    size_t m_playbackIndex = 0;
    std::vector<MacroEvent> m_events;

    MacroManager() = default;

public:
    static MacroManager* get();

    MacroState getState() const { return m_state; }
    bool isRecording() const { return m_state == MacroState::Recording; }
    bool isPlaying() const { return m_state == MacroState::Playing; }
    size_t getEventCount() const { return m_events.size(); }
    const std::vector<MacroEvent>& getEvents() const { return m_events; }

    void startRecording(float fps = 240.0f);
    void stopRecording();

    void recordInput(bool isDown, bool isPlayer2 = false, int button = 1);
    void addEventDirect(size_t frame, bool isDown, bool isPlayer2 = false, int button = 1);

    void startPlayback();
    void stopPlayback();

    void update(float dt, PlayerObject* player1, PlayerObject* player2);

    void clear();

    std::string saveToFile(const std::string& baseName = "layout_macro");
    bool loadFromFile(const std::filesystem::path& path);
};
