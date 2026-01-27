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


float randomFloat(float min, float max)
{
    static std::mt19937 rng{std::random_device{}()};
    std::uniform_real_distribution<float> dist(min, max);
    return dist(rng);
}

int main()
{
    using namespace Neon::ECS;

    Registry normalRegistry;
    TypeErasedRegistry& registry = normalRegistry.asTypeErased();

    using Clock = std::chrono::high_resolution_clock;

    auto toMicroseconds = [](Clock::time_point start, Clock::time_point end)
    {
        return std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    };

    constexpr int enemyCount = 2400;
    constexpr int frameCount = 1000;

    double totalFrameTimeUs = 0.0;

    // View timings
    double totalPosViewCreateUs = 0.0;        // 1 component (Position)
    double totalPosViewIterUs = 0.0;

    double totalPosVelViewCreateUs = 0.0;     // 2 components (Position, Velocity)
    double totalPosVelViewIterUs = 0.0;

    double totalDrawableViewCreateUs = 0.0;   // 3 components (Position, Velocity, Renderable)
    double totalDrawableViewIterUs = 0.0;

    std::size_t totalDrawCount = 0;

    std::vector<Entity> allEntities;

    std::cout << "Spawning entities...\n";

    registry.registerType<Position>();
    registry.registerType<Velocity>();
    registry.registerType<Renderable>();

    // Enemies
    for (int i = 0; i < enemyCount; ++i)
    {
        Entity e = normalRegistry.createEntity();
        allEntities.push_back(e);

        normalRegistry.emplace<Position>(e, randomFloat(-1000.f, 1000.f), randomFloat(-1000.f, 1000.f));
        registry.emplace(e, typeid(Velocity).hash_code(), new Velocity(randomFloat(-1.f, 1.f), randomFloat(-1.f, 1.f)));

        if (i % 2 == 0) // 50% Renderable
        {
            normalRegistry.emplace<Renderable>(e, 'E');
        }
    }

    std::cout << "Starting simulation...\n";

    for (int frame = 1; frame <= frameCount; ++frame)
    {
        auto frameStart = Clock::now();

        // 1-component view: Position only (measure overhead)
        {
            auto t0 = Clock::now();
            auto posView = registry.view({typeid(Position).hash_code()});
            auto t1 = Clock::now();

            for(int i = 0; i < posView.size(); ++i)
            {
                auto [entityId, components] = posView.at(i);
                auto* pos = static_cast<Position *>(components[0]);
                (void)pos;
            }


            auto t2 = Clock::now();

            totalPosViewCreateUs += static_cast<double>(toMicroseconds(t0, t1));
            totalPosViewIterUs += static_cast<double>(toMicroseconds(t1, t2));
        }

        // Movement: 2-component view (Position, Velocity)
        {
            auto t0 = Clock::now();
            auto movables = registry.view({typeid(Position).hash_code(), typeid(Velocity).hash_code()});
            auto t1 = Clock::now();

            for (int i = 0; i < movables.size(); ++i)
            {
                auto [entityId, components] = movables.at(i);
                (void)entityId;
                auto* pos = static_cast<Position *>(components[0]);
                auto* vel = static_cast<Velocity *>(components[1]);
                pos->x += vel->vx;
                pos->y += vel->vy;
            }

            auto t2 = Clock::now();

            totalPosVelViewCreateUs += static_cast<double>(toMicroseconds(t0, t1));
            totalPosVelViewIterUs += static_cast<double>(toMicroseconds(t1, t2));
        }

        // Particles: 3-component view (Position, Velocity, ParticleTag)
        {
            auto t0 = Clock::now();
            auto drawables = registry.view({typeid(Position).hash_code(), typeid(Velocity).hash_code(), typeid(Renderable).hash_code()});
            auto t1 = Clock::now();

            for (int i = 0; i < drawables.size(); ++i)
            {
                auto [entityId, components] = drawables.at(i);
                (void)entityId;
                auto* pos = static_cast<Position *>(components[0]);
                auto* vel = static_cast<Velocity *>(components[1]);
                auto* renderable = static_cast<Renderable *>(components[2]);
                (void)pos;
                (void)renderable;

                vel->vx *= 0.95f;
                vel->vy *= 0.95f;
            }

            auto t2 = Clock::now();

            totalDrawableViewCreateUs += static_cast<double>(toMicroseconds(t0, t1));
            totalDrawableViewIterUs += static_cast<double>(toMicroseconds(t1, t2));
        }


        auto frameEnd = Clock::now();
        totalFrameTimeUs += static_cast<double>(toMicroseconds(frameStart, frameEnd));
    }

    // auto destroyStart = Clock::now();
    // for (Entity e : allEntities)
    // {
    //     registry.destroy(e);
    // }
    // auto destroyEnd = Clock::now();
    // totalDestroyUs = static_cast<double>(toMicroseconds(destroyStart, destroyEnd));
    //
    // std::cout << "\n=== ECS Timing Summary (" << frameCount << " frames) ===\n\n";
    //
    // double avgFrameMs = (totalFrameTimeUs / frameCount) / 1000.0;
    // std::cout << "Total frame time: " << (totalFrameTimeUs / 1000.0) << " ms"
    //           << "  |  avg: " << avgFrameMs << " ms / frame\n\n";
    //
    // std::cout << "[Entity lifecycle]\n";
    // std::cout << "  Creation:    " << (totalCreateUs / 1000.0) << " ms total\n";
    // std::cout << "  Destruction: " << (totalDestroyUs / 1000.0) << " ms total\n\n";

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

    std::cout << "  3-component (Position, Velocity, Renderable)\n";
    std::cout << "    View create: " << (totalDrawableViewCreateUs / 1000.0) << " ms total"
              << "  |  " << perFrameMs(totalDrawableViewCreateUs) << " ms / frame\n";
    std::cout << "    Iterate:     " << (totalDrawableViewIterUs / 1000.0) << " ms total"
              << "  |  " << perFrameMs(totalDrawableViewIterUs) << " ms / frame\n\n";

    std::cout << "[Rendering]\n";
    std::cout << "  Total drawn glyphs: " << totalDrawCount << "\n";

    return 0;
}
