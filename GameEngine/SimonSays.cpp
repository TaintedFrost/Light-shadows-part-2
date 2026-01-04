#include "SimonSays.h"
#include <cstdlib>
#include <iostream>

GameState gameState = GameState::WORLD;
bool task2Completed = false;

static std::vector<SimonShape> sequence;
static std::vector<SimonShape> input;

static float timer = 0.0f;
static int showIndex = 0;

static std::vector<SimonButton> buttons =
{
    { SimonShape::BLUE,   glm::vec2(250.0f, 350.0f), 70.0f },
    { SimonShape::RED,    glm::vec2(450.0f, 350.0f), 70.0f },
    { SimonShape::GREEN,  glm::vec2(250.0f, 550.0f), 70.0f },
    { SimonShape::YELLOW, glm::vec2(450.0f, 550.0f), 70.0f }
};


static SimonShape randomShape()
{
    return static_cast<SimonShape>(rand() % 4);
}

void startSimonGame()
{
    sequence.clear();
    input.clear();

    for (int i = 0; i < 1; i++)
        sequence.push_back(randomShape());

    timer = 0.0f;
    showIndex = 0;
    gameState = GameState::SIMON_SHOW;

    std::cout << "Simon Says started\n";
}

void updateSimon(float deltaTime)
{
    if (gameState == GameState::SIMON_SHOW)
    {
        timer += deltaTime;
        if (timer > 0.8f)
        {
            timer = 0.0f;
            showIndex++;

            if (showIndex >= (int)sequence.size())
            {
                gameState = GameState::SIMON_INPUT;
                std::cout << "[SIMON] INPUT PHASE\n";
            }

        }
    }
}

static bool inside(const SimonButton& b, double x, double y)
{
    return x > b.pos.x - b.size &&
        x < b.pos.x + b.size &&
        y > b.pos.y - b.size &&
        y < b.pos.y + b.size;
}

void handleSimonClick(double mx, double my)
{
    if (gameState != GameState::SIMON_INPUT)
        return;

    for (auto& b : buttons)
    {
        if (inside(b, mx, my))
        {
            input.push_back(b.shape);

            int idx = input.size() - 1;
            if (input[idx] != sequence[idx])
            {
                gameState = GameState::SIMON_FAIL;
                std::cout << "Simon FAIL\n";
                startSimonGame();
                return;
            }

            if (input.size() == sequence.size())
            {
                gameState = GameState::SIMON_SUCCESS;
                task2Completed = true;
                std::cout << "Simon SUCCESS\n";
                gameState = GameState::WORLD;
            }
        }
    }
}

void drawSimonUI()
{
    if (gameState == GameState::SIMON_SHOW)
        std::cout << "[SIMON] Showing sequence...\n";

    if (gameState == GameState::SIMON_INPUT)
        std::cout << "[SIMON] Waiting for input...\n";
}

