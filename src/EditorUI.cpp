#include <Geode/Geode.hpp>
#include <Geode/utils/web.hpp>
#include <matjson.hpp>
#include <Geode/binding/GameManager.hpp>
#include <Geode/binding/PlayerObject.hpp>
#include "LayoutGenerator/LayoutGeneratorLayer.hpp"
#include "LevelEditorLayer.cpp"
#include "Macro/Macro.hpp"

using namespace geode::prelude;

#include <Geode/modify/EditorUI.hpp>
class $modify(MyEditorUI, EditorUI)
{
    struct Fields
    {
        TaskHolder<web::WebResponse> m_aiTask;
    };
    LayoutGeneratorLayer *getBuilder()
    {
        return static_cast<MyLevelEditorLayer *>(m_editorLayer)->m_fields->m_builder;
    }

    void onPlaytest(CCObject* sender)
    {
        auto &g = Global::get();
        auto builder = getBuilder();
        bool isBuilding = builder && builder->getIsBuilding();

        if (!isBuilding) {
            if (auto gm = GameManager::get()) {
                gm->setGameVariable("0173", false);
                gm->setGameVariable("0064", false);
            }
            if (m_editorLayer) {
                if (m_editorLayer->m_player1) m_editorLayer->m_player1->m_ignoreDamage = false;
                if (m_editorLayer->m_player2) m_editorLayer->m_player2->m_ignoreDamage = false;
            }
        }

        if (m_editorLayer && m_editorLayer->m_playbackMode == PlaybackMode::Not) {
            g.currentAction = 0;
            g.previousFrame = 0;
            Macro::resetPlayback();
            if (g.state == state::playing || g.state == state::recording) {
                m_editorLayer->m_gameState.m_currentProgress = 0;
            }
        }
        EditorUI::onPlaytest(sender);

        if (!isBuilding && m_editorLayer) {
            if (m_editorLayer->m_player1) m_editorLayer->m_player1->m_ignoreDamage = false;
            if (m_editorLayer->m_player2) m_editorLayer->m_player2->m_ignoreDamage = false;
        } else if (isBuilding && m_editorLayer) {
            if (m_editorLayer->m_player1) m_editorLayer->m_player1->m_ignoreDamage = true;
            if (m_editorLayer->m_player2) m_editorLayer->m_player2->m_ignoreDamage = true;
        }
    }

    bool init(LevelEditorLayer *editorLayer)
    {
        if (!EditorUI::init(editorLayer))
            return false;

        auto buildSprite = ButtonSprite::createWithSpriteFrameName("button.png"_spr);
        buildSprite->setScale(0.83f);
        auto buildButton = CCMenuItemSpriteExtra::create(
            buildSprite,
            getBuilder(),
            menu_selector(LayoutGeneratorLayer::onBuildButton));
        buildButton->setID("build-button"_spr);

        // AI button: sleek purple button in playback-menu
        auto aiSprite = ButtonSprite::create("AI", "bigFont.fnt", "GJ_button_04.png", 0.6f);
        aiSprite->setScale(0.83f);
        auto aiButton = CCMenuItemSpriteExtra::create(
            aiSprite,
            this,
            menu_selector(MyEditorUI::onAIGenerate));
        aiButton->setID("ai-button"_spr);

        // Bot button: sleek toggle button in playback-menu
        auto botSprite = ButtonSprite::create("Bot", "bigFont.fnt", "GJ_button_02.png", 0.6f);
        botSprite->setScale(0.83f);
        auto botButton = CCMenuItemSpriteExtra::create(
            botSprite,
            this,
            menu_selector(MyEditorUI::onToggleBot));
        botButton->setID("bot-button"_spr);

        // @geode-ignore(unknown-resource)
        auto settingsSprite = CircleButtonSprite::createWithSpriteFrameName("geode.loader/settings.png", 1.f, CircleBaseColor::DarkAqua);
        settingsSprite->setScale(0.83f);
        auto settingsButton = CCMenuItemSpriteExtra::create(
            settingsSprite,
            getBuilder(),
            menu_selector(LayoutGeneratorLayer::onSettingsButton));
        settingsButton->setID("settings-button"_spr);

        auto menu = getChildByID("playback-menu");
        if (menu) {
            menu->addChild(buildButton);
            menu->addChild(aiButton);
            menu->addChild(botButton);
            menu->addChild(settingsButton);
            m_uiItems->addObject(buildButton);
            m_uiItems->addObject(aiButton);
            m_uiItems->addObject(botButton);
            m_uiItems->addObject(settingsButton);
            menu->updateLayout();
        }

        return true;
    }

    void onToggleBot(CCObject*) {
        Macro::togglePlaying();
    }

    void onAIGenerate(CCObject*) {
        auto mod = Mod::get();
        std::string apiKey = mod->getSettingValue<std::string>("ai-api-key");
        std::string apiUrl = mod->getSettingValue<std::string>("ai-api-url");
        std::string model = mod->getSettingValue<std::string>("ai-model");
        std::string userPrompt = mod->getSettingValue<std::string>("ai-prompt");

        // Auto-fix if Geode input stripped underscores
        if (apiKey.rfind("shlive", 0) == 0) {
            apiKey = "sh_live_" + apiKey.substr(6);
        }
        if (apiKey.empty() || apiKey.length() < 10) {
            apiKey = "sh_live_75896f1b1bed8ab25e24e537ab1ec40e6a20f9e9bba51f03";
        }

        // Auto-fix if URL was corrupted by in-game character filter
        if (apiUrl.find("http://") != 0 && apiUrl.find("https://") != 0) {
            apiUrl = "https://api.shugg.ru/v1/chat/completions";
        }
        if (apiUrl.rfind("https://api.shugg.ru/v1", 0) == 0 && apiUrl.find("/chat/completions") == std::string::npos) {
            apiUrl = "https://api.shugg.ru/v1/chat/completions";
        }

        Notification::create("AI: Contacting Shugg...", NotificationIcon::Loading)->show();

        std::string systemPrompt = 
            "You are an expert Geometry Dash level layout designer. You generate human-playable, rhythmic gameplay click sequences at 240 FPS (TPS).\n"
            "CRITICAL RULES FOR PLAYABILITY:\n"
            "1. Never create impossible spam. All inputs must be humanly sightreadable and playable (Hard/Demon difficulty, NOT impossible).\n"
            "2. For Wave mode: Hold duration (down=true) must be between 30 and 75 frames (approx 0.15s - 0.35s). Release duration (down=false) must be between 30 and 75 frames. Never switch faster than 28 frames.\n"
            "3. For Cube mode: A standard jump takes ~50 frames to land. Consecutive jumps must be at least 55 to 110 frames apart so the cube can land before jumping again.\n"
            "4. Create musical, rhythmic variety: mix long sweeping slopes (60-75 frames) with punchy sync clicks (30-45 frames).\n"
            "5. Output ONLY a raw valid JSON array of objects with 'frame' (strictly increasing integer starting at frame 20 up to 1000) and 'down' (boolean). Format: [{\"frame\": 20, \"down\": true}, {\"frame\": 80, \"down\": false}, ...]. No markdown, no backticks, no text.";

        float bpm = mod->hasSetting("bpm") ? static_cast<float>(mod->getSettingValue<double>("bpm")) : 128.f;
        std::string effectivePrompt = userPrompt;
        if (effectivePrompt.empty() || effectivePrompt == "generate an intense sync wave layout with tight clicks at 240 fps") {
            effectivePrompt = fmt::format("Create a fun, rhythmic and humanly beatable layout section synced to ~{:.0f} BPM with smooth musical flow and readable slopes.", bpm);
        }

        std::vector<matjson::Value> msgs;
        msgs.push_back(matjson::makeObject({
            {"role", "system"},
            {"content", systemPrompt}
        }));
        msgs.push_back(matjson::makeObject({
            {"role", "user"},
            {"content", effectivePrompt}
        }));

        // Shugg.ru API expects ONLY model and messages (no temperature)
        matjson::Value body = matjson::makeObject({
            {"model", model.empty() ? "shugg" : model},
            {"messages", msgs}
        });

        auto req = web::WebRequest();
        req.header("Authorization", "Bearer " + apiKey);
        req.header("Content-Type", "application/json");
        req.bodyString(body.dump());

        m_fields->m_aiTask.spawn(
            "AI Macro Generation",
            req.post(apiUrl),
            [this](web::WebResponse res) {
                if (!res.ok()) {
                    log::error("[AI] Request failed: code {}, body {}", res.code(), res.string().unwrapOr(""));
                    Notification::create(fmt::format("AI Request failed ({})", res.code()), NotificationIcon::Error)->show();
                    return;
                }

                auto parseRes = matjson::parse(res.string().unwrapOr("{}"));
                if (!parseRes) {
                    Notification::create("AI: Failed to parse API response", NotificationIcon::Error)->show();
                    return;
                }

                auto json = parseRes.unwrap();
                if (!json.contains("choices") || !json["choices"].isArray()) {
                    Notification::create("AI: Invalid response structure", NotificationIcon::Error)->show();
                    return;
                }

                auto choices = json["choices"].asArray().unwrap();
                if (choices.empty()) {
                    Notification::create("AI: Empty choices returned", NotificationIcon::Error)->show();
                    return;
                }

                std::string content = choices[0]["message"]["content"].asString().unwrapOr("");

                size_t startIdx = content.find('[');
                size_t endIdx = content.rfind(']');
                if (startIdx == std::string::npos || endIdx == std::string::npos || endIdx <= startIdx) {
                    Notification::create("AI: Could not find inputs array in response", NotificationIcon::Error)->show();
                    return;
                }

                std::string arrayStr = content.substr(startIdx, endIdx - startIdx + 1);
                auto arrayParse = matjson::parse(arrayStr);
                if (!arrayParse || !arrayParse.unwrap().isArray()) {
                    Notification::create("AI: Malformed inputs array", NotificationIcon::Error)->show();
                    return;
                }

                auto inputsList = arrayParse.unwrap().asArray().unwrap();
                auto &g = Global::get();
                g.macro.inputs.clear();

                for (auto &item : inputsList) {
                    if (!item.isObject()) continue;
                    int frame = item["frame"].asInt().unwrapOr(-1);
                    bool down = item["down"].asBool().unwrapOr(false);
                    if (frame >= 0) {
                        g.macro.inputs.push_back(input(frame, 1, false, down));
                    }
                }

                if (g.macro.inputs.empty()) {
                    Notification::create("AI: 0 inputs parsed", NotificationIcon::Warning)->show();
                    return;
                }

                std::stable_sort(g.macro.inputs.begin(), g.macro.inputs.end());
                g.state = state::playing;
                Macro::resetPlayback();

                Notification::create(fmt::format("AI: Loaded {} clicks! Building layout...", g.macro.inputs.size()), NotificationIcon::Success)->show();

                if (auto builder = getBuilder()) {
                    builder->buildStart();
                }
            }
        );
    }
};
