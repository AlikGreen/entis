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
#include <random>
#include <tuple>
#include <iomanip>

// Include your header
#include "neonECS/neonECS.h"

using namespace Neon::ECS;
using Clock = std::chrono::high_resolution_clock;

// ============================================================================
// COMPONENTS
// ============================================================================

// --- Tags (0 Bytes) ---
struct PlayerTag {};
struct EnemyTag {};
struct ProjectileTag {};

// --- Small (12 Bytes) ---
struct Velocity {
    float vx, vy, vz;
};

struct Health {
    int current;
    int max;
    float regenRate;
};

// --- Medium (32 Bytes) ---
struct Transform {
    float x, y, z;
    float rotation;
    float scale;
    float padding[3]; // Pad to 32 bytes
};

struct CombatStats {
    int attackPower;
    float attackSpeed;
    float lastAttackTime;
    float critChance;
    float armor;
    int level;
};

// --- Large (64 Bytes) ---
struct AI {
    enum class State { Idle, Chasing, Attacking, Fleeing };
    State state;
    uint64_t targetEntityId;
    float stateTimer;
    float config[10]; // Padding
};

// --- Huge (128 Bytes) ---
struct ExtendedStats {
    float data[32];
};

struct Damage { int amount; float radius; };
struct Lifetime { float remaining; };

// ============================================================================
// UTILS & STATS
// ============================================================================

class Random {
    std::mt19937 rng{std::random_device{}()};
public:
    float range(float min, float max) { return std::uniform_real_distribution<float>(min, max)(rng); }
    int range(int min, int max) { return std::uniform_int_distribution<int>(min, max)(rng); }
    bool chance(float p) { return range(0.0f, 1.0f) < p; }
} rnd;

struct ViewTimings {
    std::string name;
    size_t componentCount;
    size_t totalBytes;
    size_t entityCount;
    double createTimeUs;
    double iterateTimeUs;
    int iterations;

    void print() const {
        std::cout << "  [" << name << "]\n";
        std::cout << "    Components: " << componentCount << " (" << totalBytes << " bytes)\n";
        std::cout << "    Entities:   " << entityCount << "\n";
        std::cout << "    Create:     " << (createTimeUs / iterations) << " us/call\n";
        std::cout << "    Iterate:    " << (iterateTimeUs / iterations) << " us/call";
        if (entityCount > 0) {
            double nsPerEntity = (iterateTimeUs / iterations / entityCount) * 1000.0;
            std::cout << "  (" << std::fixed << std::setprecision(2) << nsPerEntity << " ns/entity)";
        }
        std::cout << "\n\n";
    }
};

struct BenchmarkStats {
    std::vector<ViewTimings> viewTimings;
    double movementMs = 0;
    double aiMs = 0;
    double typeErasedCreateMs = 0;
    double typeErasedIterateMs = 0;
    double totalTimeMs = 0;
};

auto measureMicros = [](auto&& func) {
    auto start = Clock::now();
    func();
    auto end = Clock::now();
    return static_cast<double>(std::chrono::duration_cast<std::chrono::microseconds>(end - start).count());
};

// ============================================================================
// BENCHMARK FUNCTIONS
// ============================================================================

template<typename... Components>
ViewTimings benchmarkView(Registry& registry, const std::string& name, int iterations) {
    ViewTimings result;
    result.name = name;
    result.componentCount = sizeof...(Components);
    result.totalBytes = (sizeof(Components) + ...);
    result.iterations = iterations;

    double totalCreate = 0;
    double totalIterate = 0;
    size_t entityCount = 0;

    for (int i = 0; i < iterations; ++i) {
        // 1. Measure View Creation (Cache Lookup or Build)
        auto t0 = Clock::now();
        const auto& view = registry.view<Components...>();
        auto t1 = Clock::now();
        totalCreate += std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count();

        entityCount = view.size();

        // 2. Measure Iteration
        // Using std::apply to handle generic tuple unpacking without structured binding limitations
        auto t2 = Clock::now();
        for (auto tuple : view) {
            std::apply([](auto& entity, auto&... comps) {
                // Force read to prevent compiler optimization
                volatile auto id = entity.id();
                (void)id;
                // Fold expression to touch all components
                ((void)comps, ...);
            }, tuple);
        }
        auto t3 = Clock::now();
        totalIterate += std::chrono::duration_cast<std::chrono::microseconds>(t3 - t2).count();
    }

    result.createTimeUs = totalCreate;
    result.iterateTimeUs = totalIterate;
    result.entityCount = entityCount;
    return result;
}

void benchmarkTypeErasedViews(Registry& registry, BenchmarkStats& stats, int iterations) {
    auto& typeErased = registry.asTypeErased();

    // Register runtime types
    typeErased.registerType<Transform>();
    typeErased.registerType<Velocity>();

    const std::vector componentTypes = {
        typeid(Transform).hash_code(),
        typeid(Velocity).hash_code()
    };

    double totalCreate = 0;
    double totalIterate = 0;

    for (int i = 0; i < iterations; ++i) {
        // Measure Create
        auto t0 = Clock::now();
        auto& view = typeErased.view(componentTypes);
        auto t1 = Clock::now();
        totalCreate += std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count();

        // Measure Iterate
        auto t2 = Clock::now();
        for (size_t idx = 0; idx < view.size(); ++idx) {
            auto pack = view.at(idx);
            // Access pointers
            auto* t = static_cast<Transform*>(pack.components[0]);
            auto* v = static_cast<Velocity*>(pack.components[1]);
            volatile auto dummy1 = t;
            volatile auto dummy2 = v;
            (void)dummy1; (void)dummy2;
        }
        auto t3 = Clock::now();
        totalIterate += std::chrono::duration_cast<std::chrono::microseconds>(t3 - t2).count();
    }

    stats.typeErasedCreateMs = totalCreate / 1000.0;
    stats.typeErasedIterateMs = totalIterate / 1000.0;
}

// ============================================================================
// SYSTEMS (Simple baseline logic)
// ============================================================================

void movementSystem(Registry& registry) {
    // Only modify data fields, do not add/remove components here
    auto& view = registry.view<Transform, Velocity>();

    for (auto [entity, transform, velocity] : view) {
        transform.x += velocity.vx * 0.016f;
        transform.y += velocity.vy * 0.016f;
        transform.z += velocity.vz * 0.016f;
    }
}

void simpleAISystem(Registry& registry) {
    for (auto [entity, transform, ai, tag] : registry.view<Transform, AI, EnemyTag>()) {
        ai.stateTimer += 0.016f;
        if (ai.stateTimer > 1.0f) {
            ai.stateTimer = 0.0f;
            // Simple logic
        }
    }
}

// ============================================================================
// MAIN
// ============================================================================

int main() {
    Registry registry;
    BenchmarkStats stats;

    constexpr int entityCount = 50000;
    constexpr int viewIterations = 10000;
    constexpr int systemIterations = 100;

    std::cout << "==================================================\n";
    std::cout << " NEON ECS PERFORMANCE BENCHMARK\n";
    std::cout << "==================================================\n";
    std::cout << "Entities: " << entityCount << "\n";
    std::cout << "View Samples: " << viewIterations << "\n\n";

    std::cout << "1. Populating Registry...\n";
    std::vector<Entity> allEntities;
    allEntities.reserve(entityCount);

    // --- Safety Initialization ---
    // Ensure every component type has at least one entity to prevent empty view bugs
    {
        Entity e = registry.createEntity();
        registry.emplace<PlayerTag>(e);
        registry.emplace<EnemyTag>(e);
        registry.emplace<ProjectileTag>(e);
        registry.emplace<Transform>(e, 0.f,0.f,0.f,0.f,1.f);
        registry.emplace<Velocity>(e, 0.f,0.f,0.f);
        registry.emplace<Health>(e, 10,10,1.f);
        registry.emplace<CombatStats>(e, 1,1.f,0.f,0.f,0.f,1);
        registry.emplace<AI>(e, AI::State::Idle, 0, 0.f);
        registry.emplace<ExtendedStats>(e);
        registry.emplace<Damage>(e, 1, 1.f);
        registry.emplace<Lifetime>(e, 1.f);
        allEntities.push_back(e);
    }

    // --- Bulk Population ---
    for (int i = 0; i < entityCount; ++i) {
        Entity e = registry.createEntity();
        allEntities.push_back(e);

        // Everyone gets Transform
        registry.emplace<Transform>(e, rnd.range(-100, 100), rnd.range(-100, 100), 0.f, 0.f, 1.f);

        // 80% Move
        if (rnd.chance(0.80f)) {
            registry.emplace<Velocity>(e, rnd.range(-1, 1), rnd.range(-1, 1), 0.f);
        }

        // 50% AI Enemies
        if (rnd.chance(0.50f)) {
            registry.emplace<EnemyTag>(e);
            registry.emplace<AI>(e, AI::State::Idle, 0, 0.f);
            registry.emplace<Health>(e, 100, 100, 1.f);
            registry.emplace<CombatStats>(e, 10, 1.f, 0.f, 0.1f, 0.f, 1);
        }

        // 5% Heavy Stats
        if (rnd.chance(0.05f)) {
            registry.emplace<ExtendedStats>(e);
        }

        // 10% Sparse Projectiles
        if (rnd.chance(0.10f)) {
            registry.emplace<ProjectileTag>(e);
            registry.emplace<Damage>(e, 50, 5.f);
            registry.emplace<Lifetime>(e, 5.f);
        }
    }

    std::cout << "2. Benchmarking Templated Views...\n\n";

    // Tag (Empty)
    stats.viewTimings.push_back(benchmarkView<PlayerTag>(registry, "Tag Only (1 Comp)", viewIterations));

    // Small
    stats.viewTimings.push_back(benchmarkView<Health>(registry, "Small Struct (1 Comp, 12B)", viewIterations));

    // Medium
    stats.viewTimings.push_back(benchmarkView<Transform>(registry, "Medium Struct (1 Comp, 32B)", viewIterations));

    // Large
    stats.viewTimings.push_back(benchmarkView<AI>(registry, "Large Struct (1 Comp, 64B)", viewIterations));

    // Huge
    stats.viewTimings.push_back(benchmarkView<ExtendedStats>(registry, "Huge Struct (1 Comp, 128B)", viewIterations));

    // Combinations
    stats.viewTimings.push_back(benchmarkView<Transform, Velocity>(registry, "Common Pair (2 Comps)", viewIterations));
    stats.viewTimings.push_back(benchmarkView<Transform, AI, EnemyTag>(registry, "Complex (3 Comps)", viewIterations));
    stats.viewTimings.push_back(benchmarkView<Transform, Velocity, Health, CombatStats, EnemyTag>(registry, "Heavy (5 Comps)", viewIterations));

    // Sparse
    stats.viewTimings.push_back(benchmarkView<ProjectileTag, Damage, Lifetime>(registry, "Sparse View (~10% entities)", viewIterations));

    // Output View Results
    for (const auto& t : stats.viewTimings) t.print();

    std::cout << "3. Benchmarking Type Erased Views (Transform + Velocity)...\n";
    benchmarkTypeErasedViews(registry, stats, viewIterations);
    std::cout << "    Create:  " << (stats.typeErasedCreateMs * 1000.0 / viewIterations) << " us/call\n";
    std::cout << "    Iterate: " << (stats.typeErasedIterateMs * 1000.0 / viewIterations) << " us/call\n\n";


    std::cout << "4. Running Systems (" << systemIterations << " frames)...\n";
    auto totalStart = Clock::now();
    for(int i=0; i<systemIterations; ++i) {
        stats.movementMs += measureMicros([&](){ movementSystem(registry); }) / 1000.0;
        stats.aiMs += measureMicros([&](){ simpleAISystem(registry); }) / 1000.0;
    }
    auto totalEnd = Clock::now();
    stats.totalTimeMs = std::chrono::duration_cast<std::chrono::milliseconds>(totalEnd - totalStart).count();

    std::cout << "\n=== FINAL SUMMARY ===\n";
    std::cout << "Total Frames: " << systemIterations << "\n";
    std::cout << "Total Time:   " << stats.totalTimeMs << " ms\n";
    std::cout << "Avg Frame:    " << (stats.totalTimeMs / systemIterations) << " ms\n";
    std::cout << "Movement Sys: " << (stats.movementMs / systemIterations) << " ms/frame\n";
    std::cout << "AI System:    " << (stats.aiMs / systemIterations) << " ms/frame\n";

    // Cleanup
    for (auto e : allEntities) registry.destroy(e);

    return 0;
}