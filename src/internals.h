#pragma once

#include <array>
#include <cstdint>
#include <stack>

struct Registers final {
    std::array<uint8_t, 16> vRegisters{};
    uint16_t indexRegister{};
};

struct Chip8 final {
  public:
    Chip8(const Chip8&) = delete;
    Chip8& operator=(const Chip8&) = delete;

    static Chip8& getInstance() {
        static Chip8 _instance;
        return _instance;
    }

    uint16_t fetchNextInstruction();
    void decodeAndExecute(uint16_t instruction);
    inline uint16_t getInstructionCount() {
        return _instructionCount;
    }
    void printDisplay();

  private:
    std::array<uint8_t, 4096> _memory{};
    std::array<std::array<bool, 32>, 64> _display{}; // 64*32, on or off | white or black
    std::stack<uint16_t> _stack{};
    Registers _registers{};
    uint16_t _programCounter{};
    uint8_t _delayTimer{};
    uint8_t _soundTimer{};
    uint16_t _instructionCount{};

    Chip8();
    ~Chip8() = default;

    void loadRom(const std::string& file);
};

