#pragma once

#include <array>
#include <cstdint>
#include <random>
#include <span>
#include <stack>

struct Registers final {
    std::array<uint8_t, 16> vRegisters{};
    uint16_t indexRegister{};
};

struct Chip8 final {
  public:
    uint16_t fetchNextInstruction();
    void decodeAndExecute(uint16_t instruction);
    void loadRom(const std::string& filename);
    void loadBytes(std::span<const uint8_t> bytes);

    const std::array<std::array<bool, 64>, 32>& getDisplay() const {
        return _display;
    }
    const Registers& getRegisters() const {
        return _registers;
    }
    const std::stack<uint16_t>& getStack() const {
        return _stack;
    }
    uint16_t getProgramCounter() const {
        return _programCounter;
    }

    void printDisplay();

    Chip8();
    ~Chip8() = default;

  private:
    std::array<uint8_t, 4096> _memory{};
    std::array<std::array<bool, 64>, 32> _display{}; // 64*32, on or off | white or black
    std::stack<uint16_t> _stack{};
    Registers _registers{};
    std::mt19937 _rng{std::random_device{}()};
    uint16_t _programCounter{};
    uint8_t _delayTimer{};
    uint8_t _soundTimer{};
};

