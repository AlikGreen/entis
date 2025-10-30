#include <iostream>
#include <vector>

#include "../src/registry.h"
#include "../src/entity.h"

struct Position
{
    float x, y;
};

struct Velocity
{
    float dx, dy;
};

struct Renderable
{
    char glyph;
};

struct PlayerTag
{
};

struct EnemyAI
{

};

int main()
{
    using namespace Neon::ECS;


    Registry registry;

    auto player = registry.createEntity();
    registry.emplace<Position>(player, 10.f, 5.f);
    registry.emplace<Velocity>(player, 0.f, 0.f);
    registry.emplace<Renderable>(player, '@');
    registry.emplace<PlayerTag>(player);

    std::vector<Entity> enemies;
    for (int i = 0; i < 3; ++i)
    {
        auto enemy = registry.createEntity();
        registry.emplace<Position>(enemy, 5.f * i, 10.f);
        registry.emplace<Velocity>(enemy, -0.1f, 0.f);
        registry.emplace<Renderable>(enemy, 'E');
        registry.emplace<EnemyAI>(enemy);
        enemies.push_back(enemy);
    }


    auto wall = registry.createEntity();
    registry.emplace<Position>(wall, 20.f, 20.f);
    registry.emplace<Renderable>(wall, '#');

    for (int frame = 1; frame <= 2; ++frame)
    {
        std::cout << "\n[ Frame " << frame << " ]\n";

        {
            auto movables = registry.view<Position, Velocity>();
            for (auto [entity, pos, vel] : movables)
            {
                pos.x += vel.dx;
                pos.y += vel.dy;
            }
        }

        {
            std::cout << "  > Render System:\n";
            auto renderObjects = registry.view<Position, Renderable>();
            for (const auto& [entity, pos, render] : renderObjects)
            {
                std::cout << "    - Drawing '" << render.glyph << "' at (" << pos.x << ", " << pos.y << ")\n";
            }
        }
    }

    Entity firstEnemy = enemies[0];

    if (registry.has<EnemyAI>(firstEnemy))
    {
        std::cout << "  - Confirmed: Entity has EnemyAI component.\n";
    }

    auto& enemyPos = registry.get<Position>(firstEnemy);
    enemyPos.x = 99.f;
    std::cout << "  - Teleported enemy to x=" << enemyPos.x << "\n";

    registry.remove<Velocity>(firstEnemy);

    registry.destroy(firstEnemy);

    return 0;
}