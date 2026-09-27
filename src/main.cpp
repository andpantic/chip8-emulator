#include "internals.h"
#include "raylib.h"
#include <chrono>
#include <iostream>
#include <thread>

void RedrawDisplay(Chip8&);
std::array<bool, 16> GetPressedKeys();

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Missing ROM file argument, Usage: ./chip8_emulator <ROM file>\n";
        return 1;
    }
    constexpr auto screenWidth{1280};
    constexpr auto screenHeight{640};
    constexpr auto INSTRUCTIONS_PER_SECOND{780};
    constexpr auto DISPLAY_INTERRUPT_THRESHOLD{INSTRUCTIONS_PER_SECOND / 60};
    constexpr auto WAIT_NS = 1'000'000'000 / INSTRUCTIONS_PER_SECOND;

    InitWindow(screenWidth, screenHeight, "Chip-8");
    SetTargetFPS(60);
    InitAudioDevice();
    Sound sound = LoadSound("sound/sound.wav");

    Chip8 chip8;
    chip8.loadRom(argv[1]);

    auto displayInterruptCounter{0};
    while (!WindowShouldClose()) {

        chip8.setPressedKeys(GetPressedKeys());
        chip8.decodeAndExecute(chip8.fetchNextInstruction());
        ++displayInterruptCounter;
        if (displayInterruptCounter == DISPLAY_INTERRUPT_THRESHOLD) {
            chip8.tickTimers();
            BeginDrawing();
            RedrawDisplay(chip8);
            EndDrawing();
            displayInterruptCounter = 0;
        }
        if (chip8.getSoundTimerValue() > 0) {
            PlaySound(sound);
        }
        std::this_thread::sleep_for(std::chrono::nanoseconds(WAIT_NS));
    }
    UnloadSound(sound);

    CloseWindow();
    CloseAudioDevice();
    return 0;
}

void RedrawDisplay(Chip8& chip8) {
    const auto display{chip8.getDisplay()};
    for (auto y = 0; y < display.size(); y++) {
        for (auto x = 0; x < display[0].size(); x++) {
            DrawRectangle(x * 20, y * 20, 20, 20, display[y][x] ? WHITE : BLACK);
        }
    }
}

// Keyboard

/**
 * MAPPING HEX KEYBOARD KEYS
 * 0x1 -> 1, 0x2 -> 2, 0x3 -> 3, 0xC -> 4,
 * 0x4 -> Q, 0x5 -> W, 0x6 -> E, 0xD -> R,
 * 0x7 -> A, 0x8 -> S, 0x9 -> D, 0xE -> F,
 * 0xA -> Z, 0x0 -> X, 0xB -> C, 0xF -> V,
 */

// clang-format off
constexpr std::array<int, 16> KEY_MAP{
    KEY_X, KEY_ONE, KEY_TWO, KEY_THREE,
    KEY_Q, KEY_W,   KEY_E,   KEY_A,
    KEY_S, KEY_D,   KEY_Z,   KEY_C,
    KEY_FOUR, KEY_R, KEY_F,  KEY_V,
};
// clang-format on

std::array<bool, 16> GetPressedKeys() {
    std::array<bool, 16> keys{};
    for (int i = 0; i < 16; i++) {
        keys[i] = IsKeyDown(KEY_MAP[i]);
    }
    return keys;
}
