#include <iostream>
#include <random>
#include <bits/chrono.h>

#include "neonECS/neonECS.h"

struct Position
{
    float x, y;
};

struct Velocity
{
    float vx, vy;
};

struct Renderable
{
    char glyph;
};

struct Health
{
    int hp;
};

struct EnemyTag
{
};

struct BulletTag
{
};

struct ParticleTag
{
};

float randomFloat(float min, float max)
{
    static std::mt19937 rng{std::random_device{}()};
    std::uniform_real_distribution<float> dist(min, max);
    return dist(rng);
}

int main()
{
    using namespace Neon::ECS;

    Registry registry;

    using Clock = std::chrono::high_resolution_clock;

    auto toMicroseconds = [](Clock::time_point start, Clock::time_point end)
    {
        return std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    };

    constexpr int enemyCount = 2400;
    constexpr int bulletCount = 45000;
    constexpr int particleCount = 15000;
    constexpr int frameCount = 10000;

    double totalFrameTimeUs = 0.0;

    // Entity lifecycle
    double totalCreateUs = 0.0;
    double totalDestroyUs = 0.0;

    // View timings
    double totalPosViewCreateUs = 0.0;        // 1 component (Position)
    double totalPosViewIterUs = 0.0;

    double totalPosVelViewCreateUs = 0.0;     // 2 components (Position, Velocity)
    double totalPosVelViewIterUs = 0.0;

    double totalPosRenderViewCreateUs = 0.0;  // 2 components (Position, Renderable)
    double totalPosRenderViewIterUs = 0.0;

    double totalEnemyViewCreateUs = 0.0;      // 3 components (Position, Velocity, EnemyTag)
    double totalEnemyViewIterUs = 0.0;

    double totalParticleViewCreateUs = 0.0;   // 3 components (Position, Velocity, ParticleTag)
    double totalParticleViewIterUs = 0.0;

    std::size_t totalDrawCount = 0;

    std::vector<Entity> allEntities;

    std::cout << "Spawning entities...\n";

    auto createStart = Clock::now();

    // Enemies
    for (int i = 0; i < enemyCount; ++i)
    {
        Entity e = registry.createEntity();
        allEntities.push_back(e);

        registry.emplace<Position>(e, randomFloat(-1000.f, 1000.f), randomFloat(-1000.f, 1000.f));
        registry.emplace<Velocity>(e, randomFloat(-1.f, 1.f), randomFloat(-1.f, 1.f));
        registry.emplace<EnemyTag>(e);

        if (i % 2 == 0) // 50% Renderable
        {
            registry.emplace<Renderable>(e, 'E');
        }

        if (i % 4 == 0) // 25% Health
        {
            registry.emplace<Health>(e, 100);
        }
    }

    // Bullets
    for (int i = 0; i < bulletCount; ++i)
    {
        Entity b = registry.createEntity();
        allEntities.push_back(b);

        registry.emplace<Position>(b, randomFloat(-200.f, 200.f), randomFloat(-200.f, 200.f));
        registry.emplace<Velocity>(b, randomFloat(-5.f, 5.f), randomFloat(-5.f, 5.f));
        registry.emplace<BulletTag>(b);

        if (i % 3 == 0) // ~33% Renderable
        {
            registry.emplace<Renderable>(b, '*');
        }
    }

    // Particles
    for (int i = 0; i < particleCount; ++i)
    {
        Entity p = registry.createEntity();
        allEntities.push_back(p);

        registry.emplace<Position>(p, randomFloat(-500.f, 500.f), randomFloat(-500.f, 500.f));
        registry.emplace<Velocity>(p, randomFloat(-3.f, 3.f), randomFloat(-3.f, 3.f));
        registry.emplace<ParticleTag>(p);
    }

    auto createEnd = Clock::now();
    totalCreateUs = static_cast<double>(toMicroseconds(createStart, createEnd));

    std::cout << "Starting simulation...\n";

    for (int frame = 1; frame <= frameCount; ++frame)
    {
        auto frameStart = Clock::now();

        // 1-component view: Position only (measure overhead)
        {
            auto t0 = Clock::now();
            auto& posView = registry.view<Position>();
            auto t1 = Clock::now();

            for (auto [entity, pos] : posView)
            {
                (void)entity;
                (void)pos;
            }

            auto t2 = Clock::now();

            totalPosViewCreateUs += static_cast<double>(toMicroseconds(t0, t1));
            totalPosViewIterUs += static_cast<double>(toMicroseconds(t1, t2));
        }

        // Movement: 2-component view (Position, Velocity)
        {
            auto t0 = Clock::now();
            auto& movables = registry.view<Position, Velocity>();
            auto t1 = Clock::now();

            for (auto [entity, pos, vel] : movables)
            {
                (void)entity;
                pos.x += vel.vx;
                pos.y += vel.vy;
            }

            auto t2 = Clock::now();

            totalPosVelViewCreateUs += static_cast<double>(toMicroseconds(t0, t1));
            totalPosVelViewIterUs += static_cast<double>(toMicroseconds(t1, t2));
        }

        // Enemy AI: 3-component view (Position, Velocity, EnemyTag)
        {
            auto t0 = Clock::now();
            auto& enemies = registry.view<Position, Velocity, EnemyTag>();
            auto t1 = Clock::now();

            for (auto [entity, pos, vel, enemyTag] : enemies)
            {
                (void)enemyTag;

                float dx = -pos.x;
                float dy = -pos.y;
                float lenSq = dx * dx + dy * dy;

                if (lenSq > 0.0001f)
                {
                    float invLen = 1.0f / std::sqrt(lenSq);
                    dx *= invLen;
                    dy *= invLen;

                    vel.vx = 0.9f * vel.vx + 0.1f * dx;
                    vel.vy = 0.9f * vel.vy + 0.1f * dy;
                }

                if (lenSq > 500.f * 500.f && registry.has<Health>(entity))
                {
                    auto &hp = registry.get<Health>(entity);
                    hp.hp -= 1;
                }
            }

            auto t2 = Clock::now();

            totalEnemyViewCreateUs += static_cast<double>(toMicroseconds(t0, t1));
            totalEnemyViewIterUs += static_cast<double>(toMicroseconds(t1, t2));
        }

        // Particles: 3-component view (Position, Velocity, ParticleTag)
        {
            auto t0 = Clock::now();
            auto& particles = registry.view<Position, Velocity, ParticleTag>();
            auto t1 = Clock::now();

            for (auto [entity, pos, vel, particleTag] : particles)
            {
                (void)entity;
                (void)pos;
                (void)particleTag;

                vel.vx *= 0.95f;
                vel.vy *= 0.95f;
            }

            auto t2 = Clock::now();

            totalParticleViewCreateUs += static_cast<double>(toMicroseconds(t0, t1));
            totalParticleViewIterUs += static_cast<double>(toMicroseconds(t1, t2));
        }

        // Render: 2-component view (Position, Renderable)
        {
            auto t0 = Clock::now();
            auto& renderables = registry.view<Position, Renderable>();
            auto t1 = Clock::now();

            std::size_t frameDrawCount = 0;

            for (auto [entity, pos, render] : renderables)
            {
                (void)entity;
                (void)pos;
                (void)render;
                ++frameDrawCount;
            }

            auto t2 = Clock::now();

            totalPosRenderViewCreateUs += static_cast<double>(toMicroseconds(t0, t1));
            totalPosRenderViewIterUs += static_cast<double>(toMicroseconds(t1, t2));
            totalDrawCount += frameDrawCount;
        }

        auto frameEnd = Clock::now();
        totalFrameTimeUs += static_cast<double>(toMicroseconds(frameStart, frameEnd));
    }

    auto destroyStart = Clock::now();
    for (Entity e : allEntities)
    {
        registry.destroy(e);
    }
    auto destroyEnd = Clock::now();
    totalDestroyUs = static_cast<double>(toMicroseconds(destroyStart, destroyEnd));

    std::cout << "\n=== ECS Timing Summary (" << frameCount << " frames) ===\n\n";

    double avgFrameMs = (totalFrameTimeUs / frameCount) / 1000.0;
    std::cout << "Total frame time: " << (totalFrameTimeUs / 1000.0) << " ms"
              << "  |  avg: " << avgFrameMs << " ms / frame\n\n";

    std::cout << "[Entity lifecycle]\n";
    std::cout << "  Creation:    " << (totalCreateUs / 1000.0) << " ms total\n";
    std::cout << "  Destruction: " << (totalDestroyUs / 1000.0) << " ms total\n\n";

    auto perFrameMs = [frameCount](double totalUs)
    {
        return (totalUs / frameCount) / 1000.0;
    };

    std::cout << "[Views]\n";

    std::cout << "  1-component (Position)\n";
    std::cout << "    View create: " << (totalPosViewCreateUs / 1000.0) << " ms total"
              << "  |  " << perFrameMs(totalPosViewCreateUs) << " ms / frame\n";
    std::cout << "    Iterate:     " << (totalPosViewIterUs / 1000.0) << " ms total"
              << "  |  " << perFrameMs(totalPosViewIterUs) << " ms / frame\n\n";

    std::cout << "  2-component (Position, Velocity)\n";
    std::cout << "    View create: " << (totalPosVelViewCreateUs / 1000.0) << " ms total"
              << "  |  " << perFrameMs(totalPosVelViewCreateUs) << " ms / frame\n";
    std::cout << "    Iterate:     " << (totalPosVelViewIterUs / 1000.0) << " ms total"
              << "  |  " << perFrameMs(totalPosVelViewIterUs) << " ms / frame\n\n";

    std::cout << "  2-component (Position, Renderable)\n";
    std::cout << "    View create: " << (totalPosRenderViewCreateUs / 1000.0) << " ms total"
              << "  |  " << perFrameMs(totalPosRenderViewCreateUs) << " ms / frame\n";
    std::cout << "    Iterate:     " << (totalPosRenderViewIterUs / 1000.0) << " ms total"
              << "  |  " << perFrameMs(totalPosRenderViewIterUs) << " ms / frame\n\n";

    std::cout << "  3-component (Position, Velocity, EnemyTag)\n";
    std::cout << "    View create: " << (totalEnemyViewCreateUs / 1000.0) << " ms total"
              << "  |  " << perFrameMs(totalEnemyViewCreateUs) << " ms / frame\n";
    std::cout << "    Iterate:     " << (totalEnemyViewIterUs / 1000.0) << " ms total"
              << "  |  " << perFrameMs(totalEnemyViewIterUs) << " ms / frame\n\n";

    std::cout << "  3-component (Position, Velocity, ParticleTag)\n";
    std::cout << "    View create: " << (totalParticleViewCreateUs / 1000.0) << " ms total"
              << "  |  " << perFrameMs(totalParticleViewCreateUs) << " ms / frame\n";
    std::cout << "    Iterate:     " << (totalParticleViewIterUs / 1000.0) << " ms total"
              << "  |  " << perFrameMs(totalParticleViewIterUs) << " ms / frame\n\n";

    std::cout << "[Rendering]\n";
    std::cout << "  Total drawn glyphs: " << totalDrawCount << "\n";

    return 0;
}
