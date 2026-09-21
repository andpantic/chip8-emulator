#include "internals.h"
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
#include <vector>

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
    constexpr auto LEGACY_SYSTEM{false};

} // namespace

Chip8::Chip8() {
    // populate fonts
    auto fontAddress{FONTS_ADDRESS};
    for (auto i = 0; i < fonts.size(); i++) {
        _memory[fontAddress++] = fonts.at(i);
    }
    _programCounter = PROGRAM_ADDRESS;
}

void Chip8::loadRom(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        std::cerr << "Error opening ROM file: " << filename << " | " << strerror(errno) << "\n";
        return;
    }
    file.unsetf(std::ios::skipws);

    std::streampos fileSize;

    file.seekg(0, std::ios::end);
    fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<uint8_t> bytes;
    bytes.reserve(fileSize);

    bytes.insert(bytes.begin(), std::istream_iterator<uint8_t>(file),
                 std::istream_iterator<uint8_t>());

    loadBytes(bytes);

    file.close();
}

void Chip8::loadBytes(std::span<const uint8_t> bytes) {
    assert(bytes.size() + PROGRAM_ADDRESS <= 4096);
    auto byteAddress{PROGRAM_ADDRESS};
    for (const auto byte : bytes) {
        _memory.at(byteAddress++) = byte;
    }
}

uint16_t Chip8::fetchNextInstruction() {
    uint16_t instruction{static_cast<uint16_t>((_memory.at(_programCounter) << 8) |
                                               _memory.at(_programCounter + 1))};
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
                _display = {};
            } else if (n == 0xE) { // 00EE
                _programCounter = _stack.top();
                _stack.pop();
            }
        }
        break;
    case 0x1: // 1NNN
        _programCounter = nnn;
        break;
    case 0x2: // 2NNN
        _stack.push(_programCounter);
        _programCounter = nnn;
        break;
    case 0x3: // 3XNN
        if (_registers.vRegisters.at(x) == nn)
            _programCounter += 2;
        break;
    case 0x4: // 4XNN
        if (_registers.vRegisters.at(x) != nn)
            _programCounter += 2;
        break;
    case 0x5: // 5XY0
        if (n == 0 && _registers.vRegisters.at(x) == _registers.vRegisters.at(y))
            _programCounter += 2;
        break;
    case 0x6: // 6XNN
        _registers.vRegisters.at(x) = nn;
        break;
    case 0x7: // 7XNN
        _registers.vRegisters.at(x) += nn;
        break;
    case 0x8:
        switch (n) {
        case 0x0: // 8XY0
            _registers.vRegisters.at(x) = _registers.vRegisters.at(y);
            break;
        case 0x1: // 8XY1
            _registers.vRegisters.at(x) |= _registers.vRegisters.at(y);
            break;
        case 0x2: // 8XY2
            _registers.vRegisters.at(x) &= _registers.vRegisters.at(y);
            break;
        case 0x3: // 8XY3
            _registers.vRegisters.at(x) ^= _registers.vRegisters.at(y);
            break;
        case 0x4: // 8XY4
        {
            const auto vx = _registers.vRegisters.at(x);
            const auto vy = _registers.vRegisters.at(y);
            _registers.vRegisters.at(x) = vx + vy;
            _registers.vRegisters.at(0xF) = (vx + vy > 255) ? 1 : 0;

        } break;
        case 0x5: // 8XY5
        {
            const auto vx = _registers.vRegisters.at(x);
            const auto vy = _registers.vRegisters.at(y);
            _registers.vRegisters.at(x) = vx - vy;
            _registers.vRegisters.at(0xF) = (vx >= vy) ? 1 : 0;

        } break;
        case 0x6: // 8XY6
        {
            if (LEGACY_SYSTEM) {
                _registers.vRegisters.at(x) = _registers.vRegisters.at(y);
            }
            const auto vx = _registers.vRegisters.at(x);
            const auto vy = _registers.vRegisters.at(y);
            _registers.vRegisters.at(x) = vx >> 1;
            _registers.vRegisters.at(0xF) = vx & 1;
        } break;
        case 0x7: // 8XY7
        {
            const auto vx = _registers.vRegisters.at(x);
            const auto vy = _registers.vRegisters.at(y);
            _registers.vRegisters.at(x) = vy - vx;
            _registers.vRegisters.at(0xF) = (vy >= vx) ? 1 : 0;

        } break;
        case 0xE: // 8XYE
        {
            if (LEGACY_SYSTEM) {
                _registers.vRegisters.at(x) = _registers.vRegisters.at(y);
            }
            const auto vx = _registers.vRegisters.at(x);
            const auto vy = _registers.vRegisters.at(y);
            _registers.vRegisters.at(x) = vx << 1;
            _registers.vRegisters.at(0xF) = vx >> 7;
        } break;
        default:
            break;
        }
        break;
    case 0x9: // 9XY0
        if (n == 0 && _registers.vRegisters.at(x) != _registers.vRegisters.at(y))
            _programCounter += 2;
        break;
    case 0xA: // ANNN
        _registers.indexRegister = nnn;
        break;
    case 0xD: // DXYN
    {
        const auto xCoord{_registers.vRegisters.at(x) & 63};
        const auto yCoord{_registers.vRegisters.at(y) & 31};
        _registers.vRegisters.at(0xF) = 0;
        const auto spriteLocation{_registers.indexRegister};
        for (auto i = xCoord; i < xCoord + 8; i++) {
            if (i > 0x3F)
                break;
            for (auto j = yCoord; j < yCoord + n; j++) {
                if (j > 0x1F)
                    break;
                std::bitset<8> spriteByte{_memory.at(spriteLocation + j - yCoord)};
                const auto startState{_display[j][i]};
                _display[j][i] = startState ^ spriteByte[7 - (i - xCoord)];
                if (startState == 1 && _display[j][i] == 0) {
                    _registers.vRegisters.at(0xF) = 1;
                }
            }
        }
    } break;
    default:
        std::cout << "Unknown instruction encountered: " << std::hex
                  << static_cast<int>(instruction) << "\n";
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
