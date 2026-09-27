#include "internals.h"
#include <cstdint>
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

TEST(Instruction_2NNN, CallsSubroutineAtMemoryLocationNNN) {
    Chip8 chip8;
    constexpr std::array<uint8_t, 2> rom{0x2A, 0xBC};
    EXPECT_EQ(chip8.getProgramCounter(), 0x200);
    run(chip8, rom, 1);
    EXPECT_EQ(chip8.getProgramCounter(), 0xABC);
    EXPECT_EQ(chip8.getStack().top(), 0x202);
}

TEST(Instruction_00EE, ReturnsFromSubroutine) {
    Chip8 chip8;
    // clang-format off
    constexpr std::array<uint8_t, 6> rom{
        0x22, 0x04, // 2204
        0x00, 0xE0, // 00E0
        0x00, 0xEE, // 00EE
    };
    // clang-format on
    EXPECT_EQ(chip8.getProgramCounter(), 0x200);
    run(chip8, rom, 2);
    EXPECT_TRUE(chip8.getStack().empty());
    EXPECT_EQ(chip8.getProgramCounter(), 0x202);
}

TEST(Instruction_3XNN, SkipsInstructionIfVxEqualsNN) {
    Chip8 chip8;
    // clang-format off
    constexpr std::array<uint8_t, 4> rom{
        0x61, 0x25, // 6125, set v1 to 25
        0x31, 0x25, // 3125, skip if v1 == 25
    };
    // clang-format on
    run(chip8, rom, 2);
    EXPECT_EQ(chip8.getRegisters().vRegisters.at(0x1), 0x25);
    EXPECT_EQ(chip8.getProgramCounter(), 0x206);
}

TEST(Instruction_4XNN, SkipsInstructionIfVxNotEqualsNN) {
    Chip8 chip8;
    // clang-format off
    constexpr std::array<uint8_t, 6> rom{
        0x61, 0x25, // 6125, set v1 to 25
        0x41, 0x25, // 4125, skip if v1 != 25
        0x41, 0xAB, // 41AB, skip if v1 != AB
    };
    // clang-format on
    run(chip8, rom, 2);
    EXPECT_EQ(chip8.getRegisters().vRegisters.at(0x1), 0x25);
    EXPECT_EQ(chip8.getProgramCounter(), 0x204);
    chip8.decodeAndExecute(chip8.fetchNextInstruction());
    EXPECT_EQ(chip8.getProgramCounter(), 0x208);
}

TEST(Instruction_5XY0, SkipsInstructionIfVxEqualsVY) {
    Chip8 chip8;
    // clang-format off
    constexpr std::array<uint8_t, 10> rom{
        0x61, 0x25, // 6125, set v1 to 25
        0x62, 0x26, // 6225, set v2 to 25
        0x51, 0x20, // 5120, skip if v1 == v2
        0x62, 0x25, // 6225, set v2 to 25
        0x51, 0x20, // 5120, skip if v1 == v2
    };
    // clang-format on
    run(chip8, rom, 2);
    EXPECT_NE(chip8.getRegisters().vRegisters.at(0x1), chip8.getRegisters().vRegisters.at(0x2));
    EXPECT_EQ(chip8.getProgramCounter(), 0x204);
    chip8.decodeAndExecute(chip8.fetchNextInstruction());
    EXPECT_EQ(chip8.getProgramCounter(), 0x206);
    chip8.decodeAndExecute(chip8.fetchNextInstruction());
    chip8.decodeAndExecute(chip8.fetchNextInstruction());
    EXPECT_EQ(chip8.getRegisters().vRegisters.at(0x1), chip8.getRegisters().vRegisters.at(0x2));
    EXPECT_EQ(chip8.getProgramCounter(), 0x20C);
}

TEST(Instruction_9XY0, SkipsInstructionIfVxNotEqualsVY) {
    Chip8 chip8;
    // clang-format off
    constexpr std::array<uint8_t, 10> rom{
        0x61, 0x25, // 6125, set v1 to 25
        0x62, 0x25, // 6225, set v2 to 25
        0x91, 0x20, // 9120, skip if v1 != v2
        0x61, 0xAB, // 61AB, set v1 to AB
        0x91, 0x20, // 9120, skip if v1 != v2
    };
    // clang-format on
    run(chip8, rom, 2);
    EXPECT_EQ(chip8.getRegisters().vRegisters.at(0x1), chip8.getRegisters().vRegisters.at(0x2));
    EXPECT_EQ(chip8.getProgramCounter(), 0x204);
    chip8.decodeAndExecute(chip8.fetchNextInstruction());
    EXPECT_EQ(chip8.getProgramCounter(), 0x206);
    chip8.decodeAndExecute(chip8.fetchNextInstruction());
    chip8.decodeAndExecute(chip8.fetchNextInstruction());
    EXPECT_NE(chip8.getRegisters().vRegisters.at(0x1), chip8.getRegisters().vRegisters.at(0x2));
    EXPECT_EQ(chip8.getProgramCounter(), 0x20C);
}

TEST(Instruction_8XY0, VxSetToVy) {
    Chip8 chip8;
    // clang-format off
    constexpr std::array<uint8_t, 4> rom{
        0x61, 0xAB, // 61AB, set v1 to AB
        0x80, 0x10, // 8010, set value of v0 to v1
    };
    // clang-format on
    run(chip8, rom, 1);
    EXPECT_EQ(chip8.getRegisters().vRegisters.at(0), 0);
    chip8.decodeAndExecute(chip8.fetchNextInstruction());
    EXPECT_EQ(chip8.getRegisters().vRegisters.at(0), chip8.getRegisters().vRegisters.at(0x1));
}

TEST(Instruction_8XY1, VxSetToBinaryORwithVy) {
    Chip8 chip8;
    // clang-format off
    constexpr std::array<uint8_t, 6> rom{
        0x60, 0x47, // 6047, set v0 to 47
        0x61, 0xCD, // 61CD, set v1 to CD
        0x80, 0x11, // 8011, set value of v0 to (v0 | v1)
    };
    // clang-format on
    run(chip8, rom, 3);
    const auto result = 0x47 | 0xCD;
    EXPECT_EQ(chip8.getRegisters().vRegisters.at(0), result);
    EXPECT_EQ(chip8.getRegisters().vRegisters.at(0x1), 0xCD);
}

TEST(Instruction_8XY2, VxSetToBinaryANDwithVy) {
    Chip8 chip8;
    // clang-format off
    constexpr std::array<uint8_t, 6> rom{
        0x60, 0x47, // 6047, set v0 to 47
        0x61, 0xCD, // 61CD, set v1 to CD
        0x80, 0x12, // 8012, set value of v0 to (v0 & v1)
    };
    // clang-format on
    run(chip8, rom, 3);
    const auto result = 0x47 & 0xCD;
    EXPECT_EQ(chip8.getRegisters().vRegisters.at(0), result);
    EXPECT_EQ(chip8.getRegisters().vRegisters.at(0x1), 0xCD);
}

TEST(Instruction_8XY3, VxSetToBinaryXORwithVy) {
    Chip8 chip8;
    // clang-format off
    constexpr std::array<uint8_t, 6> rom{
        0x60, 0x47, // 6047, set v0 to 47
        0x61, 0xCD, // 61CD, set v1 to CD
        0x80, 0x13, // 8013, set value of v0 to (v0 ^ v1)
    };
    // clang-format on
    run(chip8, rom, 3);
    const auto result = 0x47 ^ 0xCD;
    EXPECT_EQ(chip8.getRegisters().vRegisters.at(0), result);
    EXPECT_EQ(chip8.getRegisters().vRegisters.at(0x1), 0xCD);
}

TEST(Instruction_8XY4, VxSetToVxPlusVyNoOverflow) {
    Chip8 chip8;
    // clang-format off
    constexpr std::array<uint8_t, 6> rom{
        0x60, 0x12, // 6012, set v0 to 12
        0x61, 0x23, // 6123 set v1 to 23
        0x80, 0x14, // 8014, set v0 to v0+v1
    };
    // clang-format on
    run(chip8, rom, 3);
    const auto result = 0x12 + 0x23;
    EXPECT_EQ(chip8.getRegisters().vRegisters.at(0), result);
    EXPECT_EQ(chip8.getRegisters().vRegisters.at(0xF), 0);
}

TEST(Instruction_8XY4, VxSetToVxPlusVyWithOverflow) {
    Chip8 chip8;
    // clang-format off
    constexpr std::array<uint8_t, 6> rom{
        0x60, 0xCD, // 60CD, set v0 to CD
        0x61, 0xAF, // 61AF set v1 to AF
        0x80, 0x14, // 8014, set v0 to v0+v1
    };
    // clang-format on
    run(chip8, rom, 3);
    const auto result = static_cast<uint8_t>(0xCD + 0xAF);
    EXPECT_EQ(chip8.getRegisters().vRegisters.at(0), result);
    EXPECT_EQ(chip8.getRegisters().vRegisters.at(0xF), 1);
}

TEST(Instruction_8XY4, VxSetToVxPlusVFasOperandNoOverflow) {
    Chip8 chip8;
    // clang-format off
    constexpr std::array<uint8_t, 6> rom{
        0x60, 0x05, // 6005, set v0 to 05
        0x6F, 0x03, // 6F03 set vF to 03
        0x80, 0xF4, // 80F4, set v0 to v0+vf
    };
    // clang-format on
    run(chip8, rom, 3);
    const auto result = 0x05 + 0x03;
    EXPECT_EQ(chip8.getRegisters().vRegisters.at(0), result);
    EXPECT_EQ(chip8.getRegisters().vRegisters.at(0xF), 0);
}

TEST(Instruction_8XY4, VxSetToVxPlusVFasOperandWithOverflow) {
    Chip8 chip8;
    // clang-format off
    constexpr std::array<uint8_t, 6> rom{
        0x60, 0xAB, // 60AB, set v0 to AB
        0x6F, 0xDF, // 6FDF set vF to DF
        0x80, 0xF4, // 80F4, set v0 to v0+vf
    };
    // clang-format on
    run(chip8, rom, 3);
    const auto result = static_cast<uint8_t>(0xAB + 0xDF);
    EXPECT_EQ(chip8.getRegisters().vRegisters.at(0), result);
    EXPECT_EQ(chip8.getRegisters().vRegisters.at(0xF), 1);
}

TEST(Instruction_8XY5, VxSetToVxMinusVyNoUnderflow) {
    Chip8 chip8;
    // clang-format off
    constexpr std::array<uint8_t, 6> rom{
        0x60, 0xCD, // 60CD, set v0 to CD
        0x61, 0x12, // 6112 set v1 to 12
        0x80, 0x15, // 8015, set v0 to v0-v1
    };
    // clang-format on
    run(chip8, rom, 3);
    const auto result = 0xCD - 0x12;
    EXPECT_EQ(chip8.getRegisters().vRegisters.at(0), result);
    EXPECT_EQ(chip8.getRegisters().vRegisters.at(0xF), 1);
}

TEST(Instruction_8XY5, VxSetToVxMinusVyWithUnderflow) {
    Chip8 chip8;
    // clang-format off
    constexpr std::array<uint8_t, 6> rom{
        0x60, 0x12, // 6012, set v0 to 12
        0x61, 0xCD, // 61CD set v1 to CD
        0x80, 0x15, // 8015, set v0 to v0-v1
    };
    // clang-format on
    run(chip8, rom, 3);
    const auto result = static_cast<uint8_t>(0x12 - 0xCD);
    EXPECT_EQ(chip8.getRegisters().vRegisters.at(0), result);
    EXPECT_EQ(chip8.getRegisters().vRegisters.at(0xF), 0);
}

TEST(Instruction_8XY6, VxShiftsRightOneBit) {
    Chip8 chip8;
    // clang-format off
    constexpr std::array<uint8_t, 4> rom{
        0x60, 0xFB, // 60FB, set v0 to 22
        0x80, 0x16, // 8016, set v0 to v0>>1
    };
    // clang-format on
    run(chip8, rom, 2);
    const auto result = static_cast<uint8_t>(0xFB >> 1);
    EXPECT_EQ(chip8.getRegisters().vRegisters.at(0), result);
    EXPECT_EQ(chip8.getRegisters().vRegisters.at(0xF), 1);
}

TEST(Instruction_8XY7, VxSetToVyMinusVxNoUnderflow) {
    Chip8 chip8;
    // clang-format off
    constexpr std::array<uint8_t, 6> rom{
        0x60, 0x22, // 6022, set v0 to 22
        0x61, 0xAB, // 61AB set v1 to AB
        0x80, 0x17, // 8015, set v0 to v0-v1
    };
    // clang-format on
    run(chip8, rom, 3);
    const auto result = 0xAB - 0x22;
    EXPECT_EQ(chip8.getRegisters().vRegisters.at(0), result);
    EXPECT_EQ(chip8.getRegisters().vRegisters.at(0xF), 1);
}

TEST(Instruction_8XY7, VxSetToVyMinusVxWithUnderflow) {
    Chip8 chip8;
    // clang-format off
    constexpr std::array<uint8_t, 6> rom{
        0x60, 0xAB, // 6022, set v0 to 22
        0x61, 0x22, // 61AB set v1 to AB
        0x80, 0x17, // 8015, set v0 to v0-v1
    };
    // clang-format on
    run(chip8, rom, 3);
    const auto result = static_cast<uint8_t>(0x22 - 0xAB);
    EXPECT_EQ(chip8.getRegisters().vRegisters.at(0), result);
    EXPECT_EQ(chip8.getRegisters().vRegisters.at(0xF), 0);
}

TEST(Instruction_8XYE, VxShiftsLeftOneBit) {
    Chip8 chip8;
    // clang-format off
    constexpr std::array<uint8_t, 4> rom{
        0x60, 0xFB, // 60FB, set v0 to 22
        0x80, 0x1E, // 801E, set v0 to v0<<1
    };
    // clang-format on
    run(chip8, rom, 2);
    const auto result = static_cast<uint8_t>(0xFB << 1);
    EXPECT_EQ(chip8.getRegisters().vRegisters.at(0), result);
    EXPECT_EQ(chip8.getRegisters().vRegisters.at(0xF), 1);
}

TEST(Instruction_BNNN, JumpsPcToVxPlusNNN) {
    Chip8 chip8;
    // clang-format off
    constexpr std::array<uint8_t, 6> rom{
        0x60, 0x43, // 6043, set v0 to 43
        0xB0, 0x12, // B012, set pc to v0 + nnn (012)
    };
    // clang-format on
    run(chip8, rom, 2);
    const auto result = 0x43 + 0x12;
    EXPECT_EQ(chip8.getProgramCounter(), result);
}

TEST(Instruction_FX1E, IndexRegisterIncrementsByVx) {
    Chip8 chip8;
    // clang-format off
    constexpr std::array<uint8_t, 6> rom{
        0x60, 0x43, // 6043, set v0 to 0x43
        0xA0, 0x12, // A012, set index register to nnn (0x12)
        0xF0, 0x1E, // F01E, index register += v0
    };
    // clang-format on
    run(chip8, rom, 2);
    EXPECT_EQ(chip8.getRegisters().vRegisters.at(0), 0x43);
    EXPECT_EQ(chip8.getRegisters().indexRegister, 0x12);
    chip8.decodeAndExecute(chip8.fetchNextInstruction());
    const auto result = 0x43 + 0x12;
    EXPECT_EQ(chip8.getRegisters().indexRegister, result);
}

TEST(Instruction_FX07, SetsVxToDelayTimerValue) {
    Chip8 chip8;
    // clang-format off
    constexpr std::array<uint8_t, 6> rom{
        0x60, 0x3C, // 603C, set v0 to 0x3C (decimal 60)
        0xF0, 0x15, // F015, set delay timer value to v0
        0xF1, 0x07, // F107, set v1 to delay timer value
    };
    // clang-format on
    run(chip8, rom, 2);
    EXPECT_EQ(chip8.getDelayTimerValue(), 0x3C);
    for (int i = 0; i < 30; i++)
        chip8.tickTimers();
    chip8.decodeAndExecute(chip8.fetchNextInstruction());
    EXPECT_EQ(chip8.getDelayTimerValue(), 30);
    EXPECT_EQ(chip8.getRegisters().vRegisters.at(1), 30);
}

TEST(Instruction_FX15, SetsDelayTimerToVx) {
    Chip8 chip8;
    // clang-format off
    constexpr std::array<uint8_t, 4> rom{
        0x60, 0x3C, // 603C, set v0 to 0x3C (decimal 60)
        0xF0, 0x15, // F015, set delay timer value to v0
    };
    // clang-format on
    run(chip8, rom, 2);
    EXPECT_EQ(chip8.getDelayTimerValue(), 0x3C);
    for (int i = 0; i < 100; i++)
        chip8.tickTimers();
    EXPECT_EQ(chip8.getDelayTimerValue(), 0);
}

TEST(Instruction_FX18, SetsSoundTimerToVx) {
    Chip8 chip8;
    // clang-format off
    constexpr std::array<uint8_t, 4> rom{
        0x60, 0x3C, // 603C, set v0 to 0x3C (decimal 60)
        0xF0, 0x18, // F018, set sound timer value to v0
    };
    // clang-format on
    run(chip8, rom, 2);
    EXPECT_EQ(chip8.getSoundTimerValue(), 0x3C);
    for (int i = 0; i < 100; i++)
        chip8.tickTimers();
    EXPECT_EQ(chip8.getSoundTimerValue(), 0);
}

TEST(Instruction_EX9E, SkipsIfKeyIsPressed) {
    Chip8 chip8;
    std::array<bool, 16> pressedKeys{};
    pressedKeys[1] = true;
    chip8.setPressedKeys(pressedKeys);
    // clang-format off
    constexpr std::array<uint8_t, 6> rom{
        0x60, 0x01, // set v0 to 0x1
        0xE6, 0x9E, // E69E, skip instruction if key in v6 is pressed
        0xE0, 0x9E, // E09E, skip instruction if key in v0 is pressed
    };
    // clang-format on
    EXPECT_EQ(chip8.getProgramCounter(), 0x200);
    run(chip8, rom, 2);
    EXPECT_EQ(chip8.getProgramCounter(), 0x204);
    chip8.decodeAndExecute(chip8.fetchNextInstruction());
    EXPECT_EQ(chip8.getProgramCounter(), 0x208);
}

TEST(Instruction_EXA1, SkipsIfKeyNotPressed) {
    Chip8 chip8;
    std::array<bool, 16> pressedKeys{};
    pressedKeys[1] = true;
    chip8.setPressedKeys(pressedKeys);
    // clang-format off
    constexpr std::array<uint8_t, 6> rom{
        0x60, 0x01, // set v0 to 0x1
        0xE0, 0xA1, // E0A1, skip instruction if key in v0 is NOT pressed
        0xE6, 0xA1, // E6A1, skip instruction if key in v6 is NOT pressed
    };
    // clang-format on
    EXPECT_EQ(chip8.getProgramCounter(), 0x200);
    run(chip8, rom, 2);
    EXPECT_EQ(chip8.getProgramCounter(), 0x204);
    chip8.decodeAndExecute(chip8.fetchNextInstruction());
    EXPECT_EQ(chip8.getProgramCounter(), 0x208);
}

TEST(Instruction_FX0A, BlocksInstructionUntilKeyPressed) {
    Chip8 chip8;
    std::array<bool, 16> pressedKeys{};
    chip8.setPressedKeys(pressedKeys);
    // clang-format off
    constexpr std::array<uint8_t, 2> rom{
        0xF0, 0x0A, // blocks execution until key is pressed
    };
    // clang-format on
    EXPECT_EQ(chip8.getProgramCounter(), 0x200);
    run(chip8, rom, 1);
    EXPECT_EQ(chip8.getProgramCounter(), 0x200);
    chip8.decodeAndExecute(chip8.fetchNextInstruction());
    EXPECT_EQ(chip8.getProgramCounter(), 0x200);
    EXPECT_EQ(chip8.getRegisters().vRegisters.at(0), 0);
    pressedKeys.at(0xB) = true;
    chip8.setPressedKeys(pressedKeys);
    chip8.decodeAndExecute(chip8.fetchNextInstruction());
    EXPECT_EQ(chip8.getProgramCounter(), 0x202);
    EXPECT_EQ(chip8.getRegisters().vRegisters.at(0), 0xB);
}

TEST(Instruction_FX29, SetsIndexRegisterToFontAddressInVx) {
    Chip8 chip8;
    // clang-format off
    constexpr std::array<uint8_t, 4> rom{
        0x60, 0x0C, // 600C, set v0 to 0xC
        0xF0, 0x29, // F029, set index register to hex character in v0
    };
    // clang-format on
    run(chip8, rom, 2);
    const auto charAddress = 0x50 + 0xC * 5;
    EXPECT_EQ(chip8.getRegisters().indexRegister, charAddress);
}

TEST(Instruction_FX33, StoresDecimalDigitsOfVxIntoMemory) {
    Chip8 chip8;
    // clang-format off
    constexpr std::array<uint8_t, 4> rom{
        0x60, 0x7F, // 607F, set v0 to 0x7F (decimal 127)
        0xF0, 0x33, // F033, set memory starting with index register to decimal digits of v0
    };
    // clang-format on
    run(chip8, rom, 2);
    const auto i = chip8.getRegisters().indexRegister;
    EXPECT_EQ(chip8.getMemory().at(i), 1);
    EXPECT_EQ(chip8.getMemory().at(i + 1), 2);
    EXPECT_EQ(chip8.getMemory().at(i + 2), 7);
}

TEST(Instruction_FX55, StoreFromV0toVxInMemory) {
    Chip8 chip8;
    // clang-format off
    constexpr std::array<uint8_t, 14> rom{
        0xAA, 0xBC, // AABC, set index register to 0xABC
        0x60, 0x1C, // 601C, set v0 to 0x1C
        0x61, 0xAF, // 61AF, set v1 to 0xAF
        0x62, 0x02, // 6202, set v2 to 0x02
        0x63, 0x72, // 6372, set v3 to 0x72
        0x64, 0x23, // 6423, set v4 to 0x23
        0xF4, 0x55, // F055, set memory at index register to values from v0 to vx (inclusive)
    };
    // clang-format on
    run(chip8, rom, 7);
    const auto i = chip8.getRegisters().indexRegister;
    EXPECT_EQ(chip8.getMemory().at(i), 0x1C);
    EXPECT_EQ(chip8.getMemory().at(i + 1), 0xAF);
    EXPECT_EQ(chip8.getMemory().at(i + 2), 0x02);
    EXPECT_EQ(chip8.getMemory().at(i + 3), 0x72);
    EXPECT_EQ(chip8.getMemory().at(i + 4), 0x23);
}

TEST(Instruction_FX65, StoreFromMemoryIntoV0toVx) {
    Chip8 chip8;
    // clang-format off
    constexpr std::array<uint8_t, 4> rom{
        0xA0, 0x50, // A050, set index register to 0x50 (fonts start)
        0xF4, 0x65, // F065, set registers from v0 to v4 (inclusive) to memory at index register
    };
    // clang-format on
    run(chip8, rom, 2);
    const auto i = chip8.getRegisters().indexRegister;
    // font for sprite 0 in memory
    // 0xF0, 0x90, 0x90, 0x90, 0xF0
    EXPECT_EQ(chip8.getMemory().at(i), 0xF0);
    EXPECT_EQ(chip8.getMemory().at(i + 1), 0x90);
    EXPECT_EQ(chip8.getMemory().at(i + 2), 0x90);
    EXPECT_EQ(chip8.getMemory().at(i + 3), 0x90);
    EXPECT_EQ(chip8.getMemory().at(i + 4), 0xF0);
    // stored in registers?
    EXPECT_EQ(chip8.getRegisters().vRegisters.at(0), 0xF0);
    EXPECT_EQ(chip8.getRegisters().vRegisters.at(1), 0x90);
    EXPECT_EQ(chip8.getRegisters().vRegisters.at(2), 0x90);
    EXPECT_EQ(chip8.getRegisters().vRegisters.at(3), 0x90);
    EXPECT_EQ(chip8.getRegisters().vRegisters.at(4), 0xF0);
}

