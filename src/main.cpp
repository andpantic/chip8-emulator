#include "internals.h"
#include "raylib.h"

int main() {
    const int screenWidth = 800;
    const int screenHeight = 450;

    InitWindow(screenWidth, screenHeight, "Chip-8");

    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        BeginDrawing();

        ClearBackground(BLACK);

        DrawText("Hello Chip-8!", 190, 200, 52, WHITE);

        EndDrawing();
    }

    CloseWindow();

    return 0;
}

