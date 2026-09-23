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
    constexpr std::array<uint8_t, 6> rom{
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
    constexpr std::array<uint8_t, 6> rom{
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
