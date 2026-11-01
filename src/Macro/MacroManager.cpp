#include "MacroManager.hpp"
#include <chrono>

std::string MacroManager::saveToFile(const std::string& baseName) {
    if (inputs.empty()) return "";

    auto saveDir = Mod::get()->getSaveDir() / "macros";
    std::filesystem::create_directories(saveDir);

    auto timestamp = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    auto filePath = saveDir / fmt::format("{}_{}.json", baseName, timestamp);

    std::ofstream file(filePath);
    file << "{\n";
    file << "  \"fps\": 240.0,\n";
    file << "  \"bot\": \"XDBot-Integrated\",\n";
    file << "  \"total_inputs\": " << inputs.size() << ",\n";
    file << "  \"inputs\": [\n";
    for (size_t i = 0; i < inputs.size(); ++i) {
        const auto& ev = inputs[i];
        file << "    {\"frame\": " << ev.frame 
             << ", \"down\": " << (ev.isDown ? "true" : "false")
             << ", \"player2\": " << (ev.player2 ? "true" : "false")
             << ", \"btn\": " << ev.button << "}";
        if (i + 1 < inputs.size()) file << ",";
        file << "\n";
    }
    file << "  ]\n";
    file << "}\n";
    file.close();

    log::info("[XDBot] Макрос сохранен в {}", filePath.string());
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

    if (root.contains("inputs") && root["inputs"].isArray()) {
        for (auto& item : root["inputs"]) {
            MacroInput inp;
            inp.frame = item["frame"].asInt().unwrapOr(0);
            inp.isDown = item["down"].asBool().unwrapOr(false);
            inp.player2 = item["player2"].asBool().unwrapOr(false);
            inp.button = item["btn"].asInt().unwrapOr(1);
            inputs.push_back(inp);
        }
    }

    std::sort(inputs.begin(), inputs.end());
    return true;
}
