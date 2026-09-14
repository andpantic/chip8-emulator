#include "internals.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sys/_types/_u_char.h>

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
    Chip8::loadRom("roms/IBM Logo.ch8");
}

