#include <Geode/Geode.hpp>
#include <Geode/utils/web.hpp>
#include <matjson.hpp>
#include <Geode/binding/GameManager.hpp>
#include <Geode/binding/PlayerObject.hpp>
#include "LayoutGenerator/LayoutGeneratorLayer.hpp"
#include "LevelEditorLayer.cpp"
#include "Macro/Macro.hpp"
#include "AILayoutPopup.hpp"

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

        // Build button: native green circular badge with hammer icon
        auto buildSprite = CircleButtonSprite::createWithSpriteFrameName("GJ_hammerIcon_001.png", 0.9f, CircleBaseColor::Green);
        buildSprite->setScale(0.72f);
        auto buildButton = CCMenuItemSpriteExtra::create(
            buildSprite,
            getBuilder(),
            menu_selector(LayoutGeneratorLayer::onBuildButton));
        buildButton->setID("build-button"_spr);

        // AI button: native magenta circular badge with magic wand icon
        auto aiSprite = CircleButtonSprite::createWithSpriteFrameName("GJ_sMagicIcon_001.png", 1.0f, CircleBaseColor::Pink);
        aiSprite->setScale(0.72f);
        auto aiButton = CCMenuItemSpriteExtra::create(
            aiSprite,
            this,
            menu_selector(MyEditorUI::onOpenAIPopup));
        aiButton->setID("ai-button"_spr);

        // Bot button: native cyan circular badge with play icon
        auto botSprite = CircleButtonSprite::createWithSpriteFrameName("GJ_playEditorBtn_001.png", 0.85f, CircleBaseColor::Cyan);
        botSprite->setScale(0.72f);
        auto botButton = CCMenuItemSpriteExtra::create(
            botSprite,
            this,
            menu_selector(MyEditorUI::onToggleBot));
        botButton->setID("bot-button"_spr);

        // Settings button: native dark aqua circular badge with gear icon
        // @geode-ignore(unknown-resource)
        auto settingsSprite = CircleButtonSprite::createWithSpriteFrameName("geode.loader/settings.png", 0.85f, CircleBaseColor::DarkAqua);
        settingsSprite->setScale(0.72f);
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

    void onOpenAIPopup(CCObject*) {
        auto popup = AILayoutPopup::create([this]() {
            this->onAIGenerate(nullptr);
        });
        if (popup) {
            popup->show();
        }
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

        std::string difficulty = mod->hasSetting("ai-difficulty") ? mod->getSettingValue<std::string>("ai-difficulty") : "Medium";
        std::string style = mod->hasSetting("ai-gameplay-style") ? mod->getSettingValue<std::string>("ai-gameplay-style") : "Flow & Rhythmic";
        int minHold = mod->hasSetting("ai-min-hold") ? static_cast<int>(mod->getSettingValue<int64_t>("ai-min-hold")) : 35;
        int maxHold = mod->hasSetting("ai-max-hold") ? static_cast<int>(mod->getSettingValue<int64_t>("ai-max-hold")) : 80;
        int clicksCount = mod->hasSetting("ai-clicks-count") ? static_cast<int>(mod->getSettingValue<int64_t>("ai-clicks-count")) : 120;
        float bpm = mod->hasSetting("bpm") ? static_cast<float>(mod->getSettingValue<double>("bpm")) : 128.f;

        if (clicksCount < 10) clicksCount = 10;
        if (clicksCount > 500) clicksCount = 500;
        if (minHold < 15) minHold = 15;
        if (maxHold < minHold + 10) maxHold = minHold + 35;

        Notification::create(fmt::format("AI: Generating {} inputs ({}, {})...", clicksCount, difficulty, style), NotificationIcon::Loading)->show();

        std::string systemPrompt = fmt::format(
            "You are an expert Geometry Dash level layout designer at 240 FPS (TPS).\n"
            "Generate a long, playable, high quality click sequence for Geometry Dash.\n"
            "CRITICAL CONSTRAINTS:\n"
            "- Total inputs count: exactly {}\n"
            "- Target Difficulty: {}\n"
            "- Movement Style: {}\n"
            "- Song BPM: ~{:.0f}\n"
            "- Minimum gap between consecutive inputs: {} frames\n"
            "- Maximum gap between consecutive inputs: {} frames\n"
            "- CRITICAL RULE: Each item's 'frame' MUST be at least {} frames greater than the previous item's 'frame' (e.g. 0, {}, {}, ...). "
            "NEVER increment by 1 or 2 frames! The level must span multiple thousands of frames for full gameplay length!\n"
            "- Inputs must strictly alternate: down=true, down=false, down=true, down=false...\n"
            "Output ONLY a raw valid JSON array: [{{\"frame\": 0, \"down\": true}}, ...]. Strictly increasing frame numbers. No markdown, no text.",
            clicksCount, difficulty, style, bpm, minHold, maxHold, minHold, minHold + 15, minHold * 2 + 30
        );

        std::string effectivePrompt = userPrompt;
        if (effectivePrompt.empty() || effectivePrompt == "generate an intense sync wave layout with tight clicks at 240 fps") {
            effectivePrompt = fmt::format("Generate an extensive, long {} difficulty section with exactly {} inputs in '{}' style synced to ~{:.0f} BPM.", difficulty, clicksCount, style, bpm);
        } else {
            effectivePrompt = fmt::format("{} (CRITICAL: generate an extensive, long sequence of exactly {} inputs with at least {} frames gap between inputs)", userPrompt, clicksCount, minHold);
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

        // Shugg.ru API expects model, messages and max_tokens
        int maxTokens = std::clamp(clicksCount * 30, 2048, 8000);
        matjson::Value body = matjson::makeObject({
            {"model", (model.empty() || model == "shupp") ? "shugg" : model},
            {"messages", msgs},
            {"max_tokens", maxTokens}
        });

        auto req = web::WebRequest();
        req.header("Authorization", "Bearer " + apiKey);
        req.header("Content-Type", "application/json");
        req.bodyString(body.dump());

        m_fields->m_aiTask.spawn(
            "AI Macro Generation",
            req.post(apiUrl),
            [this, minHold, maxHold](web::WebResponse res) {
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

                int currentCalculatedFrame = 0;
                bool expectedDown = true;

                for (size_t i = 0; i < inputsList.size(); ++i) {
                    auto &item = inputsList[i];
                    if (!item.isObject()) continue;
                    int frame = item["frame"].asInt().unwrapOr(-1);
                    bool down = item.contains("down") ? item["down"].asBool().unwrapOr(expectedDown) : expectedDown;

                    if (frame < 0) continue;

                    // Defensive auto-healing: guarantee proper rhythmic spacing even if AI returned small deltas
                    if (g.macro.inputs.empty()) {
                        currentCalculatedFrame = std::max(0, frame);
                    } else {
                        int delta = frame - g.macro.inputs.back().frame;
                        if (delta < minHold) {
                            int rhythmicStep = minHold + ((maxHold - minHold) > 0 ? (static_cast<int>(i) * 17) % (maxHold - minHold + 1) : 0);
                            currentCalculatedFrame = g.macro.inputs.back().frame + rhythmicStep;
                        } else {
                            currentCalculatedFrame = frame;
                        }
                    }

                    g.macro.inputs.push_back(input(currentCalculatedFrame, 1, false, down));
                    expectedDown = !down;
                }

                if (g.macro.inputs.empty()) {
                    Notification::create("AI: 0 inputs parsed", NotificationIcon::Warning)->show();
                    return;
                }

                std::stable_sort(g.macro.inputs.begin(), g.macro.inputs.end());
                Macro::resetPlayback();

                float durationSec = g.macro.inputs.empty() ? 0.f : (g.macro.inputs.back().frame / 240.f);
                Notification::create(fmt::format("AI: Loaded {} clicks ({:.1f}s)! Building...", g.macro.inputs.size(), durationSec), NotificationIcon::Success)->show();

                if (auto builder = getBuilder()) {
                    builder->buildStartWithAIMacro();
                }
            }
        );
    }
};
