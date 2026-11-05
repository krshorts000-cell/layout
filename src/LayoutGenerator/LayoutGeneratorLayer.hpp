#pragma once

#include "../PoolObject/PoolEnums.hpp"
#include <Geode/Geode.hpp>

using namespace geode::prelude;

class PlayerData;
class PlayerTrailData;
class PoolObject;

class LayoutGeneratorLayer : public CCLayer
{
public:
    static LayoutGeneratorLayer *create();

protected:
    bool m_isBuilding = false;

    float m_boundsCeil = 0.f;

    float m_boundsFloor = 0.f;

    bool m_canPlaceNextFrame = false;

    float m_elapsedTime = 0.f;

    int m_fishId = 0;

    float m_gamemodePortalY = 0.f;

    int m_halfBeatCount = 0;

    // show a warning once per session when generating in platformer mode
    bool m_hasShownPlatformerWarning = false;

    bool m_hasTappedThisGamemode = false;

    bool m_isClickingLastFrame = false;

    const PoolObject *m_lastPlacedFish = nullptr;

    CCPoint m_lastPlacedFishPos;

    GameObject *m_lastPlacedJumpIndicator = nullptr;

    PoolState m_lastPlayerGamemode = PoolState::GAMEMODE_CUBE;

    CCPoint m_lastSpikeBottomPos;

    CCPoint m_lastSpikeTopPos;

    CCPoint m_lastSlopeBottomPos;

    CCPoint m_lastSlopeTopPos;

    bool m_lastWaveRisingBottom = false;

    bool m_lastWaveRisingTop = false;

    int m_placeAgainTimer = -1;

    std::unordered_set<int> m_placedObjectUniqueIds;

    // stores the player position to check if the playtest is paused
    // also has the benefit of not spamming objects in platformer
    CCPoint m_playerPosPauseCheck;

    std::vector<PlayerTrailData> m_playerTrail;

    PoolTap m_shouldTap = PoolTap::NO;

    int m_shouldTapTimer = -1;

    float m_tapBalance = 0.f;

    // Live Route Builder variables
    float m_lastLiveGroundX = -9999.f;
    float m_lastLiveGroundY = 105.f;
    float m_jumpArcPeakY = 0.f;
    bool m_wasGroundedLastFrame = true;
    float m_jumpArcStartX = 0.f;
    float m_jumpArcStartY = 0.f;
    float m_jumpGroundY = 105.f;

public:
    void buildStart();
    void buildStartWithAIMacro();
    void buildStop();
    bool m_usingAIMacro = false;

protected:
    bool init() override;

    void reset();

    void update(float dt) override;

    void updateLiveGroundedObstacles(PlayerData* pd);

    // creates an object in the LevelEditorLayer and adds it to m_placedObjectUniqueIds
    GameObject *createObject(int key, CCPoint position);

    const PoolObject *fishLegally(PlayerData *pd, float dt, int excludeTags, int requireTap);

    void placeFish(PlayerData *pd, const PoolObject *fish, bool dedup = false, bool useLastY = false);

    void placeCreditText(std::string text, CCPoint pos);

    void placeDBlock(CCPoint pos);

    void placeDebugTrailBar(CCPoint pos);

    void placeDebugTrailClicking(CCPoint pos, bool isClicking);

    void placeJumpIndicator(CCPoint pos, int state);

    void placeLabel(std::string text, CCPoint pos);

    void sendClick(bool hold);

    void placeSpikeBoundary(
        bool bottom,
        CCPoint bottomPos,
        bool top,
        CCPoint topPos,
        const PlayerTrailData &midTrail,
        float dedupDistance);

    void placeSpikeInBounds(CCPoint pos, const PlayerTrailData &trail, bool flipY);

    void placeSlopeInBounds(CCPoint pos, const PlayerTrailData &trail, bool isCeiling, bool isRising);

    void placeClickPillar(CCPoint playerPos, const PlayerTrailData &trail, float clearance);
    void placeReleasePillar(CCPoint playerPos, const PlayerTrailData &trail, float clearance);

    bool doesRectInterfereWithTrail(CCRect rect, float playerX, bool isBlock, bool isMini);

    bool isSafeToPlace(int objectId, CCPoint pos, bool flipY, float clearance);

    CCRect getPlayerHitboxAt(const PlayerTrailData &trail, float safeMargin = 0.f);

    void fillSpiderTrail(CCPoint pos);

    bool isOutOfBounds(float y, float height, bool hasUpperBound, float boundsCeil, float boundsFloor);

    bool isOutOfBounds(float y, float height, PlayerData *pd);

    bool isOutOfBounds(float y, float height, const PlayerTrailData &trail);

    GameObject *getObjectNearPoint(CCPoint point, float radius, int objectId = -1);

    CCRect getObjectRect(GameObject *obj);

public:
    bool getIsBuilding();

    void onBuildButton(CCObject *);

    void onSettingsButton(CCObject *);

    void playtestStopped();

    void onExit() override;
};
