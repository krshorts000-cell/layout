#include "MacroManager.hpp"
#include <algorithm>
#include <chrono>

MacroManager* MacroManager::get() {
    static MacroManager instance;
    return &instance;
}

void MacroManager::startRecording(float fps) {
    m_fps = fps;
    m_currentFrame = 0;
    m_playbackIndex = 0;
    m_events.clear();
    m_state = MacroState::Recording;
    log::info("[MacroManager] Запись макроса начата (FPS: {})", m_fps);
}

void MacroManager::stopRecording() {
    if (m_state == MacroState::Recording) {
        m_state = MacroState::Idle;
        std::sort(m_events.begin(), m_events.end(), [](const MacroEvent& a, const MacroEvent& b) {
            return a.frame < b.frame;
        });
        log::info("[MacroManager] Запись завершена. Всего событий: {}", m_events.size());
    }
}

void MacroManager::recordInput(bool isDown, bool isPlayer2, int button) {
    if (m_state != MacroState::Recording) return;

    MacroEvent ev {
        .frame = m_currentFrame,
        .isDown = isDown,
        .isPlayer2 = isPlayer2,
        .button = button
    };
    m_events.push_back(ev);
}

void MacroManager::addEventDirect(size_t frame, bool isDown, bool isPlayer2, int button) {
    MacroEvent ev {
        .frame = frame,
        .isDown = isDown,
        .isPlayer2 = isPlayer2,
        .button = button
    };
    m_events.push_back(ev);
}

void MacroManager::startPlayback() {
    if (m_events.empty()) {
        Notification::create("Макрос пуст!", NotificationIcon::Warning)->show();
        return;
    }

    std::sort(m_events.begin(), m_events.end(), [](const MacroEvent& a, const MacroEvent& b) {
        return a.frame < b.frame;
    });

    m_currentFrame = 0;
    m_playbackIndex = 0;
    m_state = MacroState::Playing;
    Notification::create("Бот запущен!", NotificationIcon::Info)->show();
    log::info("[MacroManager] Воспроизведение макроса начато");
}

void MacroManager::stopPlayback() {
    if (m_state == MacroState::Playing) {
        m_state = MacroState::Idle;
        m_playbackIndex = 0;
        log::info("[MacroManager] Воспроизведение макроса остановлено");
    }
}

void MacroManager::update(float dt, PlayerObject* player1, PlayerObject* player2) {
    if (m_state == MacroState::Idle) return;

    size_t deltaFrames = static_cast<size_t>(std::max(1.0f, std::round(dt * m_fps)));
    m_currentFrame += deltaFrames;

    if (m_state == MacroState::Playing) {
        while (m_playbackIndex < m_events.size() && m_events[m_playbackIndex].frame <= m_currentFrame) {
            const auto& ev = m_events[m_playbackIndex];
            PlayerObject* targetPlayer = (ev.isPlayer2 && player2) ? player2 : player1;

            if (targetPlayer) {
                PlayerButton btn = static_cast<PlayerButton>(ev.button);
                if (ev.isDown) {
                    targetPlayer->pushButton(btn);
                } else {
                    targetPlayer->releaseButton(btn);
                }
            }
            m_playbackIndex++;
        }

        if (m_playbackIndex >= m_events.size()) {
            stopPlayback();
            Notification::create("Макрос успешно выполнен!", NotificationIcon::Success)->show();
        }
    }
}

void MacroManager::clear() {
    m_state = MacroState::Idle;
    m_currentFrame = 0;
    m_playbackIndex = 0;
    m_events.clear();
}

std::string MacroManager::saveToFile(const std::string& baseName) {
    if (m_events.empty()) return "";

    auto saveDir = Mod::get()->getSaveDir() / "macros";
    std::filesystem::create_directories(saveDir);

    auto timestamp = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    auto filePath = saveDir / fmt::format("{}_{}.json", baseName, timestamp);

    std::ofstream file(filePath);
    file << "{\n";
    file << "  \"fps\": " << m_fps << ",\n";
    file << "  \"generator\": \"LayoutMacroBot\",\n";
    file << "  \"total_inputs\": " << m_events.size() << ",\n";
    file << "  \"inputs\": [\n";
    for (size_t i = 0; i < m_events.size(); ++i) {
        const auto& ev = m_events[i];
        file << "    {\"frame\": " << ev.frame 
             << ", \"down\": " << (ev.isDown ? "true" : "false")
             << ", \"player2\": " << (ev.isPlayer2 ? "true" : "false")
             << ", \"btn\": " << ev.button << "}";
        if (i + 1 < m_events.size()) file << ",";
        file << "\n";
    }
    file << "  ]\n";
    file << "}\n";
    file.close();

    log::info("[MacroManager] Макрос сохранен в {}", filePath.string());
    return filePath.string();
}

bool MacroManager::loadFromFile(const std::filesystem::path& path) {
    if (!std::filesystem::exists(path)) return false;

    std::ifstream file(path);
    if (!file.is_open()) return false;

    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    auto parseRes = matjson::parse(content);
    if (!parseRes.isOk()) return false;

    auto root = parseRes.unwrap();
    clear();

    if (root.contains("fps")) {
        m_fps = root["fps"].asDouble().unwrapOr(240.0);
    }

    if (root.contains("inputs") && root["inputs"].isArray()) {
        for (auto& item : root["inputs"]) {
            MacroEvent ev;
            ev.frame = static_cast<size_t>(item["frame"].asInt().unwrapOr(0));
            ev.isDown = item["down"].asBool().unwrapOr(false);
            ev.isPlayer2 = item["player2"].asBool().unwrapOr(false);
            ev.button = item["btn"].asInt().unwrapOr(1);
            m_events.push_back(ev);
        }
    }

    return true;
}
