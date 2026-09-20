#include "internals.h"
#include "raylib.h"
#include <chrono>
#include <thread>

int main() {
    constexpr int screenWidth{1280};
    constexpr int screenHeight{640};
    constexpr auto INSTRUCTIONS_PER_SECOND{800};
    constexpr auto WAIT_NS = 1'000'000'000 / INSTRUCTIONS_PER_SECOND;

    InitWindow(screenWidth, screenHeight, "Chip-8");

    SetTargetFPS(60);

    Chip8& chip8 = Chip8::getInstance();
    const auto instructionCount{chip8.getInstructionCount()};

    auto i{0};
    while (!WindowShouldClose()) {
        BeginDrawing();

        ClearBackground(BLACK);

        if (i < instructionCount) {
            uint16_t instruction = chip8.fetchNextInstruction();
            chip8.decodeAndExecute(instruction);
            std::this_thread::sleep_for(std::chrono::nanoseconds(WAIT_NS));
            i++;
        }
        EndDrawing();
    }

    CloseWindow();

    return 0;
}

