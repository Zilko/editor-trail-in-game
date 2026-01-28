#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>

using namespace geode::prelude;

class $modify(ProPlayLayer, PlayLayer) {

    struct Fields {
        CCDrawNode* m_drawNode = nullptr;
        CCPoint m_previousP1Position = {0, 0};
        CCPoint m_previousP2Position = {0, 0};
        ccColor4F m_p1TrailColor = false;
        ccColor4F m_p2TrailColor = false;
        ccColor4F m_p1IndicatorColor = false;
        ccColor4F m_p2IndicatorColor = false;
        bool m_trailEnabled = false;
        bool m_p1Holding = false;
        bool m_p2Holding = false;
        bool m_clickIndicator = false;
        bool m_releaseIndicator = false;
        bool m_holdIndicator = false;
        float m_clickIndicatorSize = 1.f;
        float m_releaseIndicatorSize = 1.f;
    };

    void darken(ccColor4F& color) {
        color.r *= 0.73f;
        color.g *= 0.73f;
        color.b *= 0.73f;
    }

    void postUpdate(float dt) {
        PlayLayer::postUpdate(dt);

        auto f = m_fields.self();
        
        if (!f->m_drawNode || !f->m_trailEnabled) {
            return;
        }

        if (f->m_previousP1Position.y != 0) {
            auto color = f->m_p1TrailColor;

            if (f->m_holdIndicator && f->m_p1Holding) {
                darken(color);
            }

            f->m_drawNode->drawSegment(f->m_previousP1Position, m_player1->getPosition(), 0.5f, color);
        }

        f->m_previousP1Position = m_player1->getPosition();

        if (!m_gameState.m_isDualMode) {
            return;
        }

        if (f->m_previousP2Position.y != 0) {
            auto color = f->m_p2TrailColor;

            if (f->m_holdIndicator && f->m_p2Holding) {
                darken(color);
            }
        
            f->m_drawNode->drawSegment(f->m_previousP2Position, m_player2->getPosition(), 0.5f, color);
        }

        f->m_previousP2Position = m_player2->getPosition();
    }

    void resetLevel() {
        PlayLayer::resetLevel();
        auto f = m_fields.self();

        if (f->m_drawNode) {
            f->m_drawNode->clear();
            f->m_previousP1Position.y = 0;
            f->m_previousP2Position.y = 0;
            f->m_p1Holding = false;
            f->m_p2Holding = false;
        }
    }

    void setupHasCompleted() {
        PlayLayer::setupHasCompleted();

        if (!Mod::get()->getSettingValue<bool>("enabled")) {
            return;
        }

        auto f = m_fields.self();

        f->m_trailEnabled = Mod::get()->getSettingValue<bool>("enable-trail");
        f->m_p1TrailColor = ccc4FFromccc4B(Mod::get()->getSettingValue<ccColor4B>("p1-trail-color"));
        f->m_p2TrailColor = ccc4FFromccc4B(Mod::get()->getSettingValue<ccColor4B>("p2-trail-color"));
        f->m_clickIndicator = Mod::get()->getSettingValue<bool>("enable-click-indicator");
        f->m_releaseIndicator = Mod::get()->getSettingValue<bool>("enable-release-indicator");
        f->m_clickIndicatorSize = Mod::get()->getSettingValue<float>("click-indicator-size");
        f->m_releaseIndicatorSize = Mod::get()->getSettingValue<float>("release-indicator-size");
        f->m_p1IndicatorColor = ccc4FFromccc4B(Mod::get()->getSettingValue<ccColor4B>("p1-indicator-color"));
        f->m_p2IndicatorColor = ccc4FFromccc4B(Mod::get()->getSettingValue<ccColor4B>("p2-indicator-color"));
        f->m_holdIndicator = Mod::get()->getSettingValue<bool>("enable-hold-indicator");
        
        f->m_drawNode = CCDrawNode::create();
        f->m_drawNode->setID("drawy-node"_spr);
        f->m_drawNode->setBlendFunc({GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA});
        f->m_drawNode->m_bUseArea = false;

        m_objectLayer->addChild(f->m_drawNode, 500);
    }

};

class $modify(GJBaseGameLayer) {

    void drawSquare(CCDrawNode* drawNode, const CCPoint& pos, const ccColor4F& color, float size) {
        if (!drawNode) {
            return;
        }

        drawNode->drawRect(
            pos - ccp(2, 2) * size,
            pos + ccp(2, 2) * size,
            color,
            0.f,
            {0.f, 0.f, 0.f, 0.f}
        );
    }

    void drawTriangle(CCDrawNode* drawNode, const CCPoint& pos, const ccColor4F& color, float size) {
        if (!drawNode) {
            return;
        }
        
        drawNode->drawPolygon(
            std::array<CCPoint, 3>{
                pos + ccp(0, 2) * size,
                pos + ccp(-2, -2) * size,
                pos + ccp(2, -2) * size
            }.data(),
            3, color, 0.f, {0.f, 0.f, 0.f, 0.f}
        );
    }

    void handleButton(bool down, int button, bool player1) {
        GJBaseGameLayer::handleButton(down, button, player1);

        if (!PlayLayer::get() || button != 1) {
            return;
        }

        auto f = static_cast<ProPlayLayer*>(PlayLayer::get())->m_fields.self();
            
        if (!f->m_drawNode) {
            return;
        }

        auto isPlayer1 = !m_gameState.m_isDualMode || player1 || (!m_levelSettings->m_twoPlayerMode && m_gameState.m_isDualMode);
        auto isPlayer2 = m_gameState.m_isDualMode && (!player1 || !m_levelSettings->m_twoPlayerMode);

        if ((down && f->m_clickIndicator) || (!down && f->m_releaseIndicator)) {        
            if (isPlayer1) {
                down ? drawSquare(f->m_drawNode, m_player1->getPosition(), f->m_p1IndicatorColor, f->m_clickIndicatorSize)
                    : drawTriangle(f->m_drawNode, m_player1->getPosition(), f->m_p1IndicatorColor, f->m_releaseIndicatorSize);
            }

            if (isPlayer2) {
                down ? drawSquare(f->m_drawNode, m_player2->getPosition(), f->m_p2IndicatorColor, f->m_clickIndicatorSize)
                    : drawTriangle(f->m_drawNode, m_player2->getPosition(), f->m_p2IndicatorColor, f->m_releaseIndicatorSize);
            }
        }

        if (!m_levelSettings->m_twoPlayerMode) {
            f->m_p1Holding = down;
            f->m_p2Holding = down;
        } else {
            if (isPlayer1) {
                f->m_p1Holding = down;
            }

            if (isPlayer2) {
                f->m_p2Holding = down;
            }
        }
    }

};