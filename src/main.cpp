#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>

using namespace geode::prelude;

static ccColor4F g_p1TrailColor;
static ccColor4F g_p2TrailColor;
static ccColor4F g_p1IndicatorColor;
static ccColor4F g_p2IndicatorColor;
static bool g_trailEnabled = false;
static bool g_clickIndicator = false;
static bool g_releaseIndicator = false;
static bool g_holdIndicator = false;
static float g_clickIndicatorSize = 1.f;
static float g_releaseIndicatorSize = 1.f;

void updateSettings() {
    g_trailEnabled = Mod::get()->getSettingValue<bool>("enable-trail");
    g_p1TrailColor = ccc4FFromccc4B(Mod::get()->getSettingValue<ccColor4B>("p1-trail-color"));
    g_p2TrailColor = ccc4FFromccc4B(Mod::get()->getSettingValue<ccColor4B>("p2-trail-color"));
    g_clickIndicator = Mod::get()->getSettingValue<bool>("enable-click-indicator");
    g_releaseIndicator = Mod::get()->getSettingValue<bool>("enable-release-indicator");
    g_clickIndicatorSize = Mod::get()->getSettingValue<float>("click-indicator-size");
    g_releaseIndicatorSize = Mod::get()->getSettingValue<float>("release-indicator-size");
    g_p1IndicatorColor = ccc4FFromccc4B(Mod::get()->getSettingValue<ccColor4B>("p1-indicator-color"));
    g_p2IndicatorColor = ccc4FFromccc4B(Mod::get()->getSettingValue<ccColor4B>("p2-indicator-color"));
    g_holdIndicator = Mod::get()->getSettingValue<bool>("enable-hold-indicator");
}

void darkenColor(ccColor4F& color) {
    color.r *= 0.8f;
    color.g *= 0.8f;
    color.b *= 0.8f;
}

void setHookEnabled(std::string_view name, bool enabled) {
    for (Hook* hook : Mod::get()->getHooks()) {
        if (hook->getDisplayName() == name) {
            (void)(enabled ? hook->enable() : hook->disable());
            break;
        }
    }
}

class $modify(ProPlayLayer, PlayLayer) {

    struct Fields { 
        CCDrawNode* m_drawNode = nullptr;
        CCPoint m_previousP1Position = {0, 0};
        CCPoint m_previousP2Position = {0, 0};
        bool m_p1Holding = false;
        bool m_p2Holding = false;
    };

    void postUpdate(float dt) {
        PlayLayer::postUpdate(dt);

        if (!g_trailEnabled) {
            return;
        }

        auto f = m_fields.self();

        if (!f->m_drawNode) {
            return;
        }

        if (f->m_previousP1Position.y != 0) {
            auto color = g_p1TrailColor;

            if (g_holdIndicator && f->m_p1Holding) {
                darkenColor(color);
            }

            f->m_drawNode->drawSegment(f->m_previousP1Position, m_player1->getPosition(), 0.5f, color);
        }

        f->m_previousP1Position = m_player1->getPosition();

        if (!m_gameState.m_isDualMode) {
            return;
        }

        if (f->m_previousP2Position.y != 0) {
            auto color = g_p2TrailColor;

            if (g_holdIndicator && f->m_p2Holding) {
                darkenColor(color);
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
            setHookEnabled("PlayLayer::postUpdate", false);
            setHookEnabled("GJBaseGameLayer::handleButton", false);

            return;
        }

        setHookEnabled("PlayLayer::postUpdate", g_trailEnabled);
        setHookEnabled("GJBaseGameLayer::handleButton", true);

        auto f = m_fields.self();
        
        f->m_drawNode = CCDrawNode::create();
        f->m_drawNode->setID("drawy-node"_spr);
        f->m_drawNode->setBlendFunc({GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA});
        f->m_drawNode->m_bUseArea = false;

        m_objectLayer->addChild(f->m_drawNode, 500);
    }

};

class $modify(GJBaseGameLayer) {

    void drawSquare(CCDrawNode* drawNode, const CCPoint& pos, const ccColor4F& color, float size) {
        drawNode->drawRect(
            pos - ccp(2, 2) * size,
            pos + ccp(2, 2) * size,
            color,
            0.f,
            {0.f, 0.f, 0.f, 0.f}
        );
    }

    void drawTriangle(CCDrawNode* drawNode, const CCPoint& pos, const ccColor4F& color, float size) {
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

        if ((down && g_clickIndicator) || (!down && g_releaseIndicator)) {        
            if (isPlayer1) {
                down ? drawSquare(f->m_drawNode, m_player1->getPosition(), g_p1IndicatorColor, g_clickIndicatorSize)
                    : drawTriangle(f->m_drawNode, m_player1->getPosition(), g_p1IndicatorColor, g_releaseIndicatorSize);
            }

            if (isPlayer2) {
                down ? drawSquare(f->m_drawNode, m_player2->getPosition(), g_p2IndicatorColor, g_clickIndicatorSize)
                    : drawTriangle(f->m_drawNode, m_player2->getPosition(), g_p2IndicatorColor, g_releaseIndicatorSize);
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

$on_mod(Loaded) {
    updateSettings();

    listenForAllSettingChanges([](auto) {
        updateSettings();
    });
}