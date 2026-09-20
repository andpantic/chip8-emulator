#include "internals.h"
#include <gtest/gtest.h>

namespace {
    void run(Chip8& chip8, std::span<const uint8_t> rom, int steps) {
        chip8.loadBytes(rom);
        for (int i = 0; i < steps; i++) {
            chip8.decodeAndExecute(chip8.fetchNextInstruction());
        }
    }
} // namespace

TEST(Instruction_00E0, SetsAllPixelsToZero) {
    Chip8 chip8;
    constexpr std::array<uint8_t, 7> rom{
        0xA2, 0x06, // I = 0x206, where sprite byte lives (0x200, 0x201)
        0xD0, 0x11, // draw 1 row at (V0=0, V1=0) (0x202, 0x203)
        0x00, 0xE0, // this instruction - clear screen (0x204, 0x205)
        0xFF,       // sprite: 11111111 (0x206)
    };
    run(chip8, rom, 2);
    EXPECT_TRUE(chip8.getDisplay()[0][0]);
    EXPECT_TRUE(chip8.getDisplay()[0][7]);
    chip8.decodeAndExecute(chip8.fetchNextInstruction());
    EXPECT_FALSE(chip8.getDisplay()[0][0]);
    EXPECT_FALSE(chip8.getDisplay()[0][7]);
}

TEST(Instruction_1NNN, JumpsPcToNNN) {
    Chip8 chip8;
    constexpr std::array<uint8_t, 2> rom{0x12, 0xAE};
    run(chip8, rom, 1);
    EXPECT_EQ(chip8.getProgramCounter(), 0x2AE);
}

TEST(Instruction_6XNN, SetRegisterVxToNN) {
    Chip8 chip8;
    constexpr std::array<uint8_t, 2> rom{0x6C, 0xAA};
    run(chip8, rom, 1);
    EXPECT_EQ(chip8.getRegisters().vRegisters.at(0xC), 0xAA);
}

TEST(Instruction_7XNN, AddNNtoRegisterVx) {
    Chip8 chip8;
    // clang-format off
    constexpr std::array<uint8_t, 4> rom{
        0x6C, 0x02, // 6C02, set register x to 02
        0x7C, 0x06, // 7C06, add 0x06 to register C
    };
    // clang-format on
    run(chip8, rom, 2);
    EXPECT_EQ(chip8.getRegisters().vRegisters.at(0xC), 0x08);
}

TEST(Instruction_7XNN, WrapsAround8Bits) {
    Chip8 chip8;
    // clang-format off
    constexpr std::array<uint8_t, 4> rom{
        0x60, 0x05, // 6005, V0 = 0x05
        0x70, 0xFF, // 70FF, V0 += 0xFF, should wrap to 0x04
    };
    // clang-format on
    run(chip8, rom, 2);
    EXPECT_EQ(chip8.getRegisters().vRegisters.at(0x0), 0x04);
}

TEST(Instruction_DXYN, DrawsMsbAsLeftmostPixel) {
    Chip8 chip8;
    // clang-format off
    constexpr std::array<uint8_t, 5> rom{
        0xA2, 0x04, // A204, I = 0x204
        0xD0, 0x11, // D011, draw 1 row at (V0=0, V1=0)
        0x80,       // sprite: 10000000 (0x204)
    };
    // clang-format on
    run(chip8, rom, 2);
    EXPECT_TRUE(chip8.getDisplay()[0][0]);
    EXPECT_FALSE(chip8.getDisplay()[0][7]);
}

TEST(Instruction_DXYN, SetsVfOnCollisionAndErasesPixel) {
    Chip8 chip8;
    // clang-format off
    constexpr std::array<uint8_t, 7> rom{
        0xA2, 0x06, // A206, I = 0x206
        0xD0, 0x11, // D011, draw once
        0xD0, 0x11, // D011, draw same sprite again, XOR erases
        0x80,       // sprite: 10000000 (0x206)
    };
    // clang-format on
    run(chip8, rom, 2);
    EXPECT_TRUE(chip8.getDisplay()[0][0]);
    EXPECT_EQ(chip8.getRegisters().vRegisters.at(0xF), 0);
    chip8.decodeAndExecute(chip8.fetchNextInstruction());
    EXPECT_FALSE(chip8.getDisplay()[0][0]);
    EXPECT_EQ(chip8.getRegisters().vRegisters.at(0xF), 1);
}

TEST(Instruction_ANNN, SetsIndexRegisterToNNN) {
    Chip8 chip8;
    constexpr std::array<uint8_t, 2> rom{0xAA, 0xBC};
    run(chip8, rom, 1);
    EXPECT_EQ(chip8.getRegisters().indexRegister, 0xABC);
}

