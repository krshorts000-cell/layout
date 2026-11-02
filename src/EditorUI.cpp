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

        if (apiKey.empty()) {
            Notification::create("AI Error: API key is empty!", NotificationIcon::Error)->show();
            return;
        }

        Notification::create("AI: Contacting Shugg...", NotificationIcon::Loading)->show();

        std::string systemPrompt = 
            "You are an AI Geometry Dash macro creator. You generate input click sequences at 240 FPS (TPS). "
            "Output ONLY raw valid JSON (no markdown, no backticks, no explanations). "
            "Format: [{\"frame\": 15, \"down\": true}, {\"frame\": 45, \"down\": false}, ...]. "
            "Frames must be strictly increasing integers from 10 to 900. "
            "For wave: hold duration between 12 and 45 frames, release duration between 12 and 45 frames.";

        std::vector<matjson::Value> msgs;
        msgs.push_back(matjson::makeObject({
            {"role", "system"},
            {"content", systemPrompt}
        }));
        msgs.push_back(matjson::makeObject({
            {"role", "user"},
            {"content", userPrompt}
        }));

        matjson::Value body = matjson::makeObject({
            {"model", model},
            {"messages", msgs},
            {"temperature", 0.7},
            {"max_tokens", 1500}
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
