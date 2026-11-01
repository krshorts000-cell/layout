#include <Geode/Geode.hpp>

#if __has_include(<smjs.object-collab/include/object_collab_optional.hpp>)
#include <smjs.object-collab/include/object_collab_optional.hpp>
#define HAS_OBJECT_COLLAB 1
#else
#define HAS_OBJECT_COLLAB 0
#endif

#include "GameObjectPool/GameObjectPool.hpp"
#include "Settings/ObjectSettings.hpp"
#include "Macro/Macro.hpp"
#include <Geode/modify/PlayLayer.hpp>

using namespace geode::prelude;

class $modify(MyPlayLayer, PlayLayer) {
    void resetLevel() {
        PlayLayer::resetLevel();

        auto &g = Global::get();
        g.currentAction = 0;
        g.previousFrame = 0;

        if (g.state == state::recording) {
            g.macro.inputs.clear();
            g.macro.frameFixes.clear();
            g.macro.framerate = 240.f;
            Macro::updateInfo(this);
        }
    }

    void onQuit() {
        auto &g = Global::get();
        if (g.state == state::recording) {
            g.state = state::none;
        }
        PlayLayer::onQuit();
    }
};

$on_mod(Loaded) {
    if (!Mod::get()->registerCustomSettingType("objects", &ObjectSettings::parse)) {
        log::error("Failed to register custom setting type");
    }
}

$on_game(Loaded) {
    std::unordered_map<std::string, uint32_t> customObjects;
#if HAS_OBJECT_COLLAB
    Result<object_collab::OptionalRegister> result = object_collab::getOptionalRegister();
    if (result) {
        log::info("Found Object Collab");
        for (auto [objectId, info] : result.unwrap()) {
            customObjects[info.id] = objectId;
        }
    }
#endif
    GameObjectPool::generatePool(customObjects);
    ObjectSettingsNode::generateLayout(customObjects);
};
