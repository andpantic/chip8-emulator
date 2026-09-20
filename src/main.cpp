#include "internals.h"
#include "raylib.h"
#include <chrono>
#include <thread>

void RedrawDisplay(Chip8&);

int main() {
    constexpr int screenWidth{1280};
    constexpr int screenHeight{640};
    constexpr auto INSTRUCTIONS_PER_SECOND{800};
    constexpr auto WAIT_NS = 1'000'000'000 / INSTRUCTIONS_PER_SECOND;

    InitWindow(screenWidth, screenHeight, "Chip-8");
    SetTargetFPS(60);

    Chip8 chip8;
    chip8.loadRom("roms/IBM Logo.ch8");

    while (!WindowShouldClose()) {

        uint16_t instruction = chip8.fetchNextInstruction();
        chip8.decodeAndExecute(instruction);
        std::this_thread::sleep_for(std::chrono::nanoseconds(WAIT_NS));

        BeginDrawing();
        RedrawDisplay(chip8);
        EndDrawing();
    }

    CloseWindow();

    return 0;
}

void RedrawDisplay(Chip8& chip8) {
    const auto display{chip8.getDisplay()};
    for (auto i = 0; i < display.size(); i++) {
        for (auto j = 0; j < display[0].size(); j++) {
            DrawRectangle(i * 20, j * 20, 20, 20, (display[i][j]) ? WHITE : BLACK);
        }
    }
}

