#include <chrono>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <vector>

#include "entis/entis.h"

struct Transform
{
    int layer;
    int flags;
    float x;
    float y;
    float scale;
};

struct EnemyTag
{
};

struct Health
{
    int current;
    int max;
    float regen;
};

struct ProjectileTag
{
};

using Clock = std::chrono::steady_clock;

static uint64_t elapsedNs(const auto start, const auto end)
{
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
        end - start
    ).count();
}

int main()
{
    constexpr size_t entityCount = 1'000'000;
    constexpr size_t warmupIterations = 100;
    constexpr size_t benchmarkIterations = 1'000;

    entis::Registry registry;

    std::vector<entis::Entity> allEntities;
    allEntities.reserve(entityCount);

    // -------------------------------------------------------------------------
    // Entity creation + component insertion benchmark
    // -------------------------------------------------------------------------

    uint64_t createNs = 0;
    uint64_t transformNs = 0;
    uint64_t enemyTagNs = 0;
    uint64_t healthNs = 0;
    uint64_t projectileTagNs = 0;

    size_t enemyCount = 0;
    size_t projectileCount = 0;

    for (size_t i = 0; i < entityCount; ++i)
    {
        auto start = Clock::now();

        entis::Entity entity = registry.create();

        auto end = Clock::now();
        createNs += elapsedNs(start, end);

        allEntities.push_back(entity);

        start = Clock::now();

        registry.emplace<Transform>(
            entity,
            1,
            1,
            0.f,
            0.f,
            1.f
        );

        end = Clock::now();
        transformNs += elapsedNs(start, end);

        if (i % 2 == 0)
        {
            ++enemyCount;

            start = Clock::now();

            entity.emplace<EnemyTag>();

            end = Clock::now();
            enemyTagNs += elapsedNs(start, end);

            start = Clock::now();

            entity.emplace<Health>(
                100,
                100,
                1.f
            );

            end = Clock::now();
            healthNs += elapsedNs(start, end);
        }

        if (i % 9)
        {
            ++projectileCount;

            start = Clock::now();

            entity.emplace<ProjectileTag>();

            end = Clock::now();
            projectileTagNs += elapsedNs(start, end);
        }
    }

    volatile float sink = 0.0f;

    // -------------------------------------------------------------------------
    // Create view
    // -------------------------------------------------------------------------

    uint64_t viewCreationNs = 0;

    auto viewStart = Clock::now();

    auto view = registry.view<EnemyTag, Transform>();

    auto viewEnd = Clock::now();

    viewCreationNs = elapsedNs(viewStart, viewEnd);

    // -------------------------------------------------------------------------
    // Warmup
    // -------------------------------------------------------------------------

    for (size_t i = 0; i < warmupIterations; ++i)
    {
        for (const auto& [entity, tag, transform] : view)
        {
            (void)entity;
            (void)tag;

            sink += transform.x;
        }
    }

    // -------------------------------------------------------------------------
    // View iteration benchmark
    // -------------------------------------------------------------------------

    uint64_t iterationNs = 0;
    size_t matchingEntities = 0;

    for (size_t iteration = 0;
         iteration < benchmarkIterations;
         ++iteration)
    {
        const auto start = Clock::now();

        size_t count = 0;

        for (auto [entity, tag, transform] : view)
        {
            (void)entity;
            (void)tag;

            sink += transform.x;
            ++count;
        }

        const auto end = Clock::now();

        iterationNs += elapsedNs(start, end);
        matchingEntities = count;
    }

    // -------------------------------------------------------------------------
    // Results
    // -------------------------------------------------------------------------

    const double avgCreateNs =
        static_cast<double>(createNs) / entityCount;

    const double avgTransformNs =
        static_cast<double>(transformNs) / entityCount;

    const double avgEnemyTagNs =
        static_cast<double>(enemyTagNs) / enemyCount;

    const double avgHealthNs =
        static_cast<double>(healthNs) / enemyCount;

    const double avgProjectileTagNs =
        static_cast<double>(projectileTagNs) / projectileCount;

    const double avgIterationNs =
        static_cast<double>(iterationNs) / benchmarkIterations;

    const double avgIterationPerEntityNs =
        avgIterationNs / matchingEntities;

    std::cout << std::fixed << std::setprecision(2);

    std::cout
        << "========================================\n"
        << "        ENTIS BENCHMARK - CURRENT\n"
        << "========================================\n\n"

        << "Entities\n"
        << "  Total:             " << entityCount << '\n'
        << "  Enemy:             " << enemyCount << '\n'
        << "  Projectile:        " << projectileCount << '\n'
        << "  View matches:      " << matchingEntities << "\n\n"

        << "Component insertion\n"
        << "  create()\n"
        << "    Total:           " << createNs << " ns\n"
        << "    Per entity:      " << avgCreateNs << " ns\n"

        << "  Transform\n"
        << "    Total:           " << transformNs << " ns\n"
        << "    Per entity:      " << avgTransformNs << " ns\n"

        << "  EnemyTag\n"
        << "    Total:           " << enemyTagNs << " ns\n"
        << "    Per insertion:   " << avgEnemyTagNs << " ns\n"

        << "  Health\n"
        << "    Total:           " << healthNs << " ns\n"
        << "    Per insertion:   " << avgHealthNs << " ns\n"

        << "  ProjectileTag\n"
        << "    Total:           " << projectileTagNs << " ns\n"
        << "    Per insertion:   " << avgProjectileTagNs << " ns\n\n"

        << "View\n"
        << "  Creation:          " << viewCreationNs << " ns\n"
        << "  Creation:          " << viewCreationNs / 1'000.0 << " us\n"
        << "  Iteration avg:     " << avgIterationNs << " ns\n"
        << "  Iteration avg:     " << avgIterationNs / 1'000.0 << " us\n"
        << "  Per entity:        " << avgIterationPerEntityNs << " ns\n\n"

        << "Sink: " << sink << '\n';
}