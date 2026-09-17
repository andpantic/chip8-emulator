#include "internals.h"
#include "raylib.h"

int main() {
    const int screenWidth = 800;
    const int screenHeight = 450;

    InitWindow(screenWidth, screenHeight, "Chip-8");

    SetTargetFPS(60);

    // test
    Chip8& chip8 = Chip8::getInstance();
    const auto instructionCount{chip8.getInstructionCount()};
    for (int i = 0; i < instructionCount; i++) {
        uint16_t instruction = chip8.fetchNextInstruction();
        chip8.decodeAndExecute(instruction);
    }

    while (!WindowShouldClose()) {
        BeginDrawing();

        ClearBackground(BLACK);

        DrawText("Hello Chip-8!", 190, 200, 52, WHITE);

        EndDrawing();
    }

    CloseWindow();

    return 0;
}

