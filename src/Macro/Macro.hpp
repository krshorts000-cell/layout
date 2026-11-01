#pragma once

#include <Geode/Geode.hpp>
#include "../gdr/gdr.hpp"
#include <vector>
#include <string>
#include <filesystem>
#include <algorithm>

using namespace geode::prelude;

enum state {
    none,
    recording,
    playing
};

struct input : gdr::Input {
    input() = default;

    input(int frame, int button, bool player2, bool down)
        : Input(frame, button, player2, down) {}

    bool operator==(const input& other) const {
        return frame == other.frame &&
               player2 == other.player2 &&
               button == other.button &&
               down == other.down;
    }
};

struct Macro : gdr::Replay<Macro, input> {
    Macro() : Replay("xdBot", "v2.6-Nako") {}

public:
    bool xdBotMacro = true;

    static void recordAction(int frame, int button, bool player2, bool hold);
    static int save(std::string author, std::string desc, std::string levelName, bool json = true);
    static void updateInfo(GJBaseGameLayer* bgl);
    static void togglePlaying();
    static void resetPlayback();
    static bool flipControls();
};

class Global {
    Global() {}

public:
    static Global& get() {
        static Global instance;
        return instance;
    }

    static float getTPS() { return 240.f; }
    static int getCurrentFrame(bool editor = false);

    state state = none;
    Macro macro;

    size_t currentAction = 0;
    int previousFrame = 0;
    int frameOffset = 0;
    bool macro_ignore_inputs = true;
    bool ignoreRecordAction = false;
    bool stopPlaying = false;
};
