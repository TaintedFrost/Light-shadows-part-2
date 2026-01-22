# Light & Shadows: The Knight's Quest

**Computer Graphics - Final Project**

## 📖 Story
A brave Knight stands before his Princess, recounting the tale of how he saved the Kingdom from the clutches of the Dark Warlock. To win her hand in marriage, he had to overcome five deadly trials, solving puzzles and battling ancient evils.

## 🎮 Controls
*   **W / A / S / D**: Move Character
*   **Mouse**: Look Camera
*   **E**: Interact / Attack / Advance Dialogue
*   **Esc**: Exit Game

## ⚔️ Walkthrough & Tasks

### Task 1: The Apprentice
The Warlock's apprentice guards the entrance.
*   **Goal**: Approach the enemy and press **E** to strike him down.

### Task 2: The Memory Puzzle
The Warlock scatters four mystical shapes into the forest.
*   **Goal**: 
    1.  Memorize the order of the shapes shown on the pedestal.
    2.  Find the scattered shapes hidden in the grove.
    3.  Collect them in the **exact same order** to break the seal.

### Task 3: The Lunar Blade
An ancient weapon is required to defeat the darkness.
*   **Goal**: Find the floating Lunar Blade on the pedestal and press **E** to claim it.

### Task 4: The Shadow Wizard (Boss Fight 1)
A shadow guardian blocks your path. You must break his defenses.
*   **Mechanic**: A timing mini-game.
*   **Goal**: Press **E** when the moving bar is inside the **Green Safe Zone**.
*   **Difficulty**: 
    *   You need **3 Successful Hits** to win.
    *   After each hit, the **Speed Increases** and the **Safe Zone Shrinks**.
    *   The Safe Zone moves to a **Random Position** each round.

### Task 5: The Dark Warlock (Final Boss)
The master of shadows appears!
*   **Mechanic**: Boss Cycle (Attack vs Rest).
*   **Goal**: Deplete the Boss's Health (10 HP).
*   **Strategy**:
    *   **ATTACK Phase (10s)**: The Warlock shoots projectiles. **Hide behind trees!**
    *   **REST Phase (5s)**: The Warlock is tired. **Run in and attack (Press E)!**

## ✨ Features
*   **Story Mode**: Cinematic intro and outro with the Princess.
*   **Dynamic Dialogues**: The Warlock taunts you after every victory.
*   **Visual Effects**: 
    *   Dynamic Day/Night cycle (Dark sky during boss fights).
    *   Particle/Skybox animations.
    *   Custom shaders for lighting and feedback.
*   **Multiple Gameplay Styles**: Exploration, Puzzle Solving, Timing Reflexes, and Action Combat.

## 🛠️ Installation & Build
1.  Open the project in **Visual Studio**.
2.  Ensure `glm`, `glew`, `glfw`, and `assimp` are properly linked.
3.  Build and Run in **Release** or **Debug** mode.
4.  Make sure the `Resources/` folder is in the working directory.
