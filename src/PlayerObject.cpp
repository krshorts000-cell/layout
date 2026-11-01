#pragma once

#include <Geode/Geode.hpp>
#include "PlayerData/PlayerData.hpp"
#include "LevelEditorLayer.cpp"
#include "LayoutGenerator/LayoutGeneratorLayer.hpp"
#include "Macro/MacroManager.hpp"

using namespace geode::prelude;

#include <Geode/modify/PlayerObject.hpp>
class $modify(MyPlayerObject, PlayerObject)
{
    struct Fields
    {
        std::vector<PlayerTrailData> m_queuedTrail;
    };

    void pushButton(PlayerButton btn)
    {
        PlayerObject::pushButton(btn);

        if (btn == PlayerButton::Jump) {
            if (auto editor = static_cast<MyLevelEditorLayer*>(LevelEditorLayer::get())) {
                auto builder = editor->m_fields->m_builder;
                if (builder && builder->getIsBuilding()) {
                    int frame = static_cast<int>(editor->m_gameState.m_currentProgress / 2);
                    MacroManager::get().recordAction(frame, 1, false, true);
                }
            }
        }
    }

    void releaseButton(PlayerButton btn)
    {
        PlayerObject::releaseButton(btn);

        if (btn == PlayerButton::Jump) {
            if (auto editor = static_cast<MyLevelEditorLayer*>(LevelEditorLayer::get())) {
                auto builder = editor->m_fields->m_builder;
                if (builder && builder->getIsBuilding()) {
                    int frame = static_cast<int>(editor->m_gameState.m_currentProgress / 2);
                    MacroManager::get().recordAction(frame, 1, false, false);
                }
            }
        }
    }

    void updateJump(float dt)
    {
        if (auto editor = static_cast<MyLevelEditorLayer *>(LevelEditorLayer::get()))
        {
            auto builder = editor->m_fields->m_builder;
            if (builder && builder->getIsBuilding())
            {
                auto pd = new PlayerData();
                pd->setPlayer(this);

                auto trail = PlayerTrailData::fromPlayerData(*pd);

                // jump indicators v3
                if (m_isBird || m_isSwing)
                {
                    if (m_stateRingJump && m_jumpBuffered && !m_isDashing)
                        trail.makeJumpIndicator = true;
                }
                // cube, ball, robot, spider
                else if (!m_isShip && !m_isDart)
                {
                    if (m_isOnGround && m_jumpBuffered && (!m_isRobot || m_stateRingJump) && !m_isDashing)
                        trail.makeJumpIndicator = true;
                }

                m_fields->m_queuedTrail.push_back(trail);
            }
        }

        PlayerObject::updateJump(dt);
    }
};
