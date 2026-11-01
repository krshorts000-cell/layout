#include "MacroManager.hpp"
#include <Geode/loader/Dirs.hpp>
#include <chrono>
#include <sstream>

std::string MacroManager::saveToFile(const std::string& baseName) {
    if (inputs.empty()) return "";

    std::string safeName = baseName.empty() ? "layout" : baseName;
    for (char& c : safeName) {
        if (c == '/' || c == '\\' || c == ':' || c == '*' || c == '?' || c == '"' || c == '<' || c == '>' || c == '|') {
            c = '_';
        }
    }

    auto timestamp = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    // 1. Target macro directories (Steam library, game root dir, and mod save dir)
    std::vector<std::filesystem::path> targetDirs = {
        std::filesystem::path("D:/SteamLibrary/steamapps/common/Geometry Dash/macros"),
        geode::dirs::getGameDir() / "macros",
        Mod::get()->getSaveDir() / "macros"
    };

    float duration = static_cast<float>(inputs.back().frame) / 240.f;

    // Full XDBot / GDR JSON format (completely matching NakoMellia/XDBotFork)
    std::stringstream ss;
    ss << "{\n";
    ss << "  \"author\": \"LayoutGenerator\",\n";
    ss << "  \"description\": \"Automatically recorded during layout generation\",\n";
    ss << "  \"version\": 1.0,\n";
    ss << "  \"gameVersion\": 2.2081,\n";
    ss << "  \"duration\": " << duration << ",\n";
    ss << "  \"framerate\": 240.0,\n";
    ss << "  \"seed\": 0,\n";
    ss << "  \"coins\": 0,\n";
    ss << "  \"ldm\": false,\n";
    ss << "  \"bot\": {\n";
    ss << "    \"name\": \"xdBot\",\n";
    ss << "    \"version\": \"v2.6-Nako\"\n";
    ss << "  },\n";
    ss << "  \"level\": {\n";
    ss << "    \"id\": 0,\n";
    ss << "    \"name\": \"" << safeName << "\"\n";
    ss << "  },\n";
    ss << "  \"inputs\": [\n";
    for (size_t i = 0; i < inputs.size(); ++i) {
        const auto& ev = inputs[i];
        ss << "    {\"frame\": " << ev.frame 
           << ", \"btn\": " << ev.button 
           << ", \"2p\": " << (ev.player2 ? "true" : "false") 
           << ", \"down\": " << (ev.isDown ? "true" : "false") 
           << "}";
        if (i + 1 < inputs.size()) ss << ",";
        ss << "\n";
    }
    ss << "  ],\n";
    ss << "  \"frameFixes\": []\n";
    ss << "}\n";
    std::string jsonContent = ss.str();

    std::string primarySavedPath = "";

    // Save to all target locations
    for (const auto& dir : targetDirs) {
        std::error_code ec;
        std::filesystem::create_directories(dir, ec);

        auto gdrPath = dir / fmt::format("{}_{}.gdr.json", safeName, timestamp);
        auto jsonPath = dir / fmt::format("{}_{}.json", safeName, timestamp);

        std::ofstream fileGdr(gdrPath);
        if (fileGdr.is_open()) {
            fileGdr << jsonContent;
            fileGdr.close();
            if (primarySavedPath.empty()) {
                primarySavedPath = gdrPath.string();
            }
        }

        std::ofstream fileJson(jsonPath);
        if (fileJson.is_open()) {
            fileJson << jsonContent;
            fileJson.close();
        }
    }

    log::info("[LayoutMacro] Saved macro in XDBot format to {}", primarySavedPath);
    return primarySavedPath;
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
            inp.isDown = item.contains("down") ? item["down"].asBool().unwrapOr(false) : item["isDown"].asBool().unwrapOr(false);
            inp.player2 = item.contains("2p") ? item["2p"].asBool().unwrapOr(false) : item["player2"].asBool().unwrapOr(false);
            inp.button = item.contains("btn") ? item["btn"].asInt().unwrapOr(1) : item["button"].asInt().unwrapOr(1);
            inputs.push_back(inp);
        }
    }

    std::sort(inputs.begin(), inputs.end());
    return true;
}
