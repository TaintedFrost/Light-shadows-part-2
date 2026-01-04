#pragma once
#include <vector>
#include <glm.hpp>

enum class SimonShape { BLUE, RED, GREEN, YELLOW };




enum class GameState
{
    WORLD,
    SIMON_SHOW,
    SIMON_INPUT,
    SIMON_SUCCESS,
    SIMON_FAIL
};

enum class SimonShape
{
    BLUE,
    RED,
    GREEN,
    YELLOW
};

struct SimonButton
{
    SimonShape shape;
    glm::vec2 pos;
    float size;
};

extern GameState gameState;
extern bool task2Completed;

void startSimonGame();
void updateSimon(float deltaTime);
void handleSimonClick(double mx, double my);
void drawSimonUI();
