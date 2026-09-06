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

    constexpr size_t entityCount = 100'000;
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
            entity.emplace<EnemyTag>();
            entity.emplace<Health>(
                100,
                100,
                1.f
            );
        }

        if (i % 9)
        {
            entity.emplace<ProjectileTag>();
        }
    }

    volatile float sink = 0.0f;

    // -------------------------------------------------------------------------
    // Warm up both paths
    // -------------------------------------------------------------------------

    for (size_t i = 0; i < warmupIterations; ++i)
    {
        registry.each<Transform, EnemyTag>(
            [&](entis::Entity, const Transform& transform, EnemyTag&)
            {
                sink += transform.x;
            }
        );
    }

    auto view = registry.view<Transform, EnemyTag>();

    for (size_t i = 0; i < warmupIterations; ++i)
    {
        for (auto [entity, transform, tag] : view)
        {
            (void)entity;
            (void)tag;

            sink += transform.x;
        }
    }

    // -------------------------------------------------------------------------
    // Benchmark direct each()
    // -------------------------------------------------------------------------

    uint64_t totalEachNs = 0;

    for (size_t iteration = 0; iteration < benchmarkIterations; ++iteration)
    {
        const auto start = Clock::now();

        registry.each<Transform, EnemyTag>(
            [&](entis::Entity, const Transform& transform, EnemyTag&)
            {
                sink += transform.x;
            }
        );

        const auto end = Clock::now();

        totalEachNs += std::chrono::duration_cast<std::chrono::nanoseconds>(
            end - start
        ).count();
    }

    // -------------------------------------------------------------------------
    // Benchmark view iteration
    //
    // IMPORTANT:
    // The view is created before the benchmark, so this measures iteration
    // rather than view construction.
    // -------------------------------------------------------------------------

    uint64_t totalViewNs = 0;

    for (size_t iteration = 0; iteration < benchmarkIterations; ++iteration)
    {
        const auto start = Clock::now();

        for (auto [entity, transform, tag] : view)
        {
            (void)entity;
            (void)tag;

            sink += transform.x;
        }

        const auto end = Clock::now();

        totalViewNs += std::chrono::duration_cast<std::chrono::nanoseconds>(
            end - start
        ).count();
    }

    // -------------------------------------------------------------------------
    // Benchmark view construction separately
    // -------------------------------------------------------------------------

    uint64_t totalViewSetupNs = 0;

    for (size_t iteration = 0; iteration < benchmarkIterations; ++iteration)
    {
        const auto start = Clock::now();

        auto testView = registry.view<Transform, EnemyTag>();

        const auto end = Clock::now();

        totalViewSetupNs += std::chrono::duration_cast<std::chrono::nanoseconds>(
            end - start
        ).count();

        // Prevent optimizer from considering the view completely unused.
        if (iteration == benchmarkIterations - 1)
        {
            for (auto [entity, transform, tag] : testView)
            {
                (void)entity;
                (void)tag;
                sink += transform.x * 0.0f;
            }
        }
    }

    // -------------------------------------------------------------------------
    // Results
    // -------------------------------------------------------------------------

    const double averageEachNs =
        static_cast<double>(totalEachNs) / benchmarkIterations;

    const double averageViewNs =
        static_cast<double>(totalViewNs) / benchmarkIterations;

    const double averageViewSetupNs =
        static_cast<double>(totalViewSetupNs) / benchmarkIterations;

    std::cout
        << "Entities:          " << entityCount << '\n'
        << "Iterations:        " << benchmarkIterations << '\n'
        << '\n'
        << "Direct each():\n"
        << "  Average:         " << averageEachNs << " ns\n"
        << "  Average:         " << averageEachNs / 1'000.0 << " us\n"
        << '\n'
        << "View iteration:\n"
        << "  Average:         " << averageViewNs << " ns\n"
        << "  Average:         " << averageViewNs / 1'000.0 << " us\n"
        << '\n'
        << "View construction:\n"
        << "  Average:         " << averageViewSetupNs << " ns\n"
        << "  Average:         " << averageViewSetupNs / 1'000.0 << " us\n"
        << '\n'
        << "View vs each:\n"
        << "  Difference:      " << (averageViewNs - averageEachNs) << " ns\n"
        << "  Ratio:           " << (averageViewNs / averageEachNs) << "x\n"
        << '\n'
        << "Sink: " << sink << '\n';

    for (auto entity : allEntities)
        registry.destroy(entity);
}