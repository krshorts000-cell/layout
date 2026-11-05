#pragma once

#include <Geode/Geode.hpp>
#include "LayoutGenerator/LayoutGeneratorLayer.hpp"

using namespace geode::prelude;

#include <Geode/modify/LevelEditorLayer.hpp>
class $modify(MyLevelEditorLayer, LevelEditorLayer)
{
    struct Fields
    {
        LayoutGeneratorLayer *m_builder = nullptr;
    };

    bool init(GJGameLevel *level, bool noUI)
    {
        // create builder before init, so EditorUI has access to the builder in its init
        auto builder = LayoutGeneratorLayer::create();
        addChild(builder);
        builder->setID("builder"_spr);
        m_fields->m_builder = builder;

        if (!LevelEditorLayer::init(level, noUI))
            return false;

        return true;
    }

    void onStopPlaytest()
    {
        LevelEditorLayer::onStopPlaytest();

        if (auto builder = m_fields->m_builder)
            builder->playtestStopped();
    }

    void playerTookDamage(PlayerObject* player)
    {
        if (auto builder = m_fields->m_builder)
        {
            if (builder->getIsBuilding())
            {
                // In Live Builder or AI build mode: noclip / invincibility!
                // Prevent damage, death animation, playtest stop, and red X marker.
                return;
            }
        }
        LevelEditorLayer::playerTookDamage(player);
    }

    void postUpdate(float dt)
    {
        if (auto builder = m_fields->m_builder)
        {
            if (builder->getIsBuilding())
            {
                if (m_player1)
                {
                    m_player1->m_maybeIsColliding = false;
                    m_player1->m_isDead = false;
                    m_player1->m_ignoreDamage = true;
                }
                if (m_player2)
                {
                    m_player2->m_maybeIsColliding = false;
                    m_player2->m_isDead = false;
                    m_player2->m_ignoreDamage = true;
                }
            }
        }
        LevelEditorLayer::postUpdate(dt);
    }
};
