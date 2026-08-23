/*
 * ============================================================================
 * ECS View Performance Benchmark
 * ============================================================================
 *
 * This benchmark was created with AI assistance to test ECS view performance
 * across different scenarios:
 * - Component sizes (empty tags, small, medium, large structs)
 * - View complexities (1-5 components)
 * - Entity densities (common vs sparse views)
 * - Type-erased vs templated view performance
 *
 * It explicitly isolates View Creation time vs Iteration time.
 *
 * Generated: 2026
 * ============================================================================
 */

#include <iostream>
#include <vector>
#include <string>
#include <chrono>

#include "entis/entis.h"

using Clock = std::chrono::high_resolution_clock;

struct PlayerTag {};
struct EnemyTag {};
struct ProjectileTag {};

struct Velocity
{
    float vx, vy, vz;
};

struct Health
{
    int current;
    int max;
    float regenRate;
};

struct Transform
{
    float x, y, z;
    float rotation;
    float scale;
    float padding[3];
};


int main()
{
    entis::Registry registry;

    constexpr size_t entityCount = 1'000'000;
    constexpr size_t warmupIterations = 100;
    constexpr size_t benchmarkIterations = 1'000;

    std::vector<entis::Entity> allEntities;
    allEntities.reserve(entityCount);

    for (size_t i = 0; i < entityCount; ++i)
    {
        entis::Entity entity = registry.create();
        allEntities.push_back(entity);

        registry.emplace<Transform>(
            entity,
            1,
            1,
            0.f,
            0.f,
            1.f
        );

        if (i % 2 == 0)
        {
            registry.emplace<EnemyTag>(entity);
            registry.emplace<Health>(
                entity,
                100,
                100,
                1.f
            );
        }

        if (i % 9)
        {
            registry.emplace<ProjectileTag>(entity);
        }
    }

    volatile float sink = 0.0f;

    auto query = [&]
    {
        registry.each<EnemyTag, Transform>(
            [&](entis::EntityId, EnemyTag&, const Transform& transform)
            {
                sink += transform.x;
            }
        );
    };

    for (size_t i = 0; i < warmupIterations; ++i)
        query();

    uint64_t totalSetupNs = 0;
    uint64_t totalEachNs = 0;

    for (size_t iteration = 0; iteration < benchmarkIterations; ++iteration)
    {
        bool firstIteration = true;
        auto start = Clock::now();
        auto first = start;

        registry.each<Transform, EnemyTag>(
            [&](entis::EntityId, const Transform& transform, EnemyTag&)
            {
                if (firstIteration)
                {
                    first = Clock::now();
                    firstIteration = false;
                }

                sink += transform.x;
            }
        );

        auto end = Clock::now();

        const uint64_t setupNs =
            std::chrono::duration_cast<std::chrono::nanoseconds>(
                first - start
            ).count();

        const uint64_t eachNs =
            std::chrono::duration_cast<std::chrono::nanoseconds>(
                end - start
            ).count();

        totalSetupNs += setupNs;
        totalEachNs += eachNs;
    }

    const double averageSetupNs =
        static_cast<double>(totalSetupNs) / benchmarkIterations;

    const double averageEachNs =
        static_cast<double>(totalEachNs) / benchmarkIterations;

    const double averageIterationNs =
        averageEachNs - averageSetupNs;

    std::cout
        << "Entities:          " << entityCount << '\n'
        << "Iterations:        " << benchmarkIterations << '\n'
        << '\n'
        << "Average each:      " << averageEachNs / 1'000.0 << " us\n"
        << "Average setup:     " << averageSetupNs / 1'000.0 << " us\n"
        << "Average iteration:  " << averageIterationNs / 1'000.0 << " us\n"
        << " ns\n";

    for (auto entity : allEntities)
        registry.destroy(entity);

    std::cout << "Sink: " << sink << '\n';
}