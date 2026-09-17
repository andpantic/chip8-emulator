#include "internals.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>

namespace {
    // each font is 4 pixels wide, 5 pixels tall
    constexpr auto fonts{std::to_array<uint8_t>({
        0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
        0x20, 0x60, 0x20, 0x20, 0x70, // 1
        0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
        0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
        0x90, 0x90, 0xF0, 0x10, 0x10, // 4
        0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
        0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
        0xF0, 0x10, 0x20, 0x40, 0x40, // 7
        0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
        0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
        0xF0, 0x90, 0xF0, 0x90, 0x90, // A
        0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
        0xF0, 0x80, 0x80, 0x80, 0xF0, // C
        0xE0, 0x90, 0x90, 0x90, 0xE0, // D
        0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
        0xF0, 0x80, 0xF0, 0x80, 0x80  // F
    })};

    constexpr auto FONTS_ADDRESS{0x50};
    constexpr auto PROGRAM_ADDRESS{0x200};

} // namespace

void Chip8::loadRom(const std::string& file) {
    std::ifstream romFile(file, std::ios::in | std::ios::binary);
    if (!romFile) {
        std::cerr << "Error opening ROM file: " << file << " | " << strerror(errno) << "\n";
        return;
    }
    auto byteAddress{PROGRAM_ADDRESS};
    while (!romFile.eof()) {
        char byte{};
        romFile.get(byte);
        _memory[byteAddress++] = byte;
    }
    _instructionCount = (byteAddress - PROGRAM_ADDRESS) / 2;
    romFile.close();
}

Chip8::Chip8() {
    // populate fonts
    auto fontAddress{FONTS_ADDRESS};
    for (auto i = 0; i < fonts.size(); i++) {
        _memory[fontAddress++] = fonts.at(i);
    }
    _programCounter = PROGRAM_ADDRESS;
    Chip8::loadRom("roms/IBM Logo.ch8");
}

uint16_t Chip8::fetchNextInstruction() {
    uint16_t instruction{
        static_cast<uint16_t>((_memory[_programCounter] << 8) | _memory[_programCounter + 1])};
    _programCounter += 2;
    return instruction;
}

void Chip8::decodeAndExecute(uint16_t instruction) {
    const auto instructionCategory{(instruction & 0xF000) >> 12};
    const auto x{(instruction & 0x0F00) >> 8};
    const auto y{(instruction & 0x00F0) >> 4};
    const auto n{instruction & 0x000F};
    const auto nn{instruction & 0x00FF};
    const auto nnn{instruction & 0x0FFF};
    std::cout << "============================================\n";
    std::cout << "Instruction: " << std::hex << static_cast<int>(instruction) << "\n";
    std::cout << "Instruction category (first nibble): " << std::hex << instructionCategory << "\n";
    std::cout << "X (second nibble): " << std::hex << x << "\n";
    std::cout << "Y (third nibble): " << std::hex << y << "\n";
    std::cout << "N (fourth nibble): " << std::hex << n << "\n";
    std::cout << "NN (second byte): " << std::hex << nn << "\n";
    std::cout << "NNN (second, third, fourth nibble): " << std::hex << nnn << "\n";
    std::cout << "============================================\n";
    switch (instructionCategory) {
    case 0x0:
        break;
    case 0x1:
        break;
    case 0x6:
        break;
    case 0x7:
        break;
    case 0xA:
        break;
    case 0xD:
        break;
    default:
        break;
    }
}

