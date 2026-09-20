#include "internals.h"
#include <array>
#include <cassert>
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
    romFile.close();
}

Chip8::Chip8() {
    // populate fonts
    auto fontAddress{FONTS_ADDRESS};
    for (auto i = 0; i < fonts.size(); i++) {
        _memory[fontAddress++] = fonts.at(i);
    }
    _programCounter = PROGRAM_ADDRESS;
}

uint16_t Chip8::fetchNextInstruction() {
    uint16_t instruction{
        static_cast<uint16_t>((_memory[_programCounter] << 8) | _memory[_programCounter + 1])};
    _programCounter += 2;
    return instruction;
}

void Chip8::decodeAndExecute(uint16_t instruction) {
    assert(_programCounter <= 0xFFF);
    const auto instructionCategory{(instruction & 0xF000) >> 12};
    const auto x{(instruction & 0x0F00) >> 8};
    const auto y{(instruction & 0x00F0) >> 4};
    const auto n{instruction & 0x000F};
    const auto nn{instruction & 0x00FF};
    const auto nnn{instruction & 0x0FFF};
    switch (instructionCategory) {
    case 0x0:
        if (y == 0xE) {
            if (n == 0) { // 00E0
                _display = {0};
            }
        }
        break;
    case 0x1: // 1NNN
        _programCounter = nnn;
        break;
    case 0x6: // 6XNN
        _registers.vRegisters[x] = nn;
        break;
    case 0x7: // 7XNN
        _registers.vRegisters[x] += nn;
        break;
    case 0xA: // ANNN
        _registers.indexRegister = nnn;
        break;
    case 0xD: // DXYN
    {
        const auto xCoord{_registers.vRegisters[x] & 63};
        const auto yCoord{_registers.vRegisters[y] & 31};
        _registers.vRegisters[0xF] = 0;
        const auto spriteLocation{_registers.indexRegister};
        for (auto i = xCoord; i < xCoord + 8; i++) {
            if (i > 0x3F)
                break;
            for (auto j = yCoord; j < yCoord + n; j++) {
                if (j > 0x1F)
                    break;
                std::bitset<8> spriteByte{_memory[spriteLocation + j - yCoord]};
                const auto startState{_display[i][j]};
                _display[i][j] = startState ^ spriteByte[7 - (i - xCoord)];
                if (startState == 1 && _display[i][j] == 0) {
                    _registers.vRegisters[0xF] = 1;
                }
            }
        }
    } break;
    default:
        break;
    }
}

void Chip8::printDisplay() {
    for (int i = 0; i < _display.size(); i++) {
        for (int j = 0; j < _display[0].size(); j++) {
            std::cout << _display[i][j] << " ";
        }
        std::cout << "\n";
    }
}
