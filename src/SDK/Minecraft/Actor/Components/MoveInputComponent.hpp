//
// Created by vastrakai on 6/30/2024.
//
// [1.26] ПЕРЕПИСАНО ПОД 1.26 (было: хардкод оффсетов 0x20/0x26/0x27/0x2C..0x2F/0x48/0x8A)
// Источник — заголовки LeviLamina 26.51 (это реальные символы/лейауты BDS, не угадайка):
//   src/mc/entity/components/MoveInputComponent.h
//   src/mc/input/MoveInputState.h   (там же enum Flag с индексами битов)
//
// Что изменилось в 1.26:
//   * флаги ввода (sneak/jump/sprint/WASD) больше НЕ отдельные байты —
//     это биты в brstd::bitset<27,uint> внутри двух структур MoveInputState;
//   * аналоговый вектор движения лежит в mMove (Vec2) по смещению 0x24, а не 0x48;
//   * размер структуры 0x64 (100 байт) вместо 136.
//
// Старый API (поля mIsSneakDown/mIsJumping/mForward/...) сохранён как методы,
// чтобы вызывающий код правился механически. См. tools/port_moveinput.py.
//
#pragma once

#include <cstddef>
#include <cstdint>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

// brstd::bitset<N, uint> в BDS — это просто uint32, где используются младшие N бит.
// Точные индексы взяты из MoveInputState::Flag (src/mc/input/MoveInputState.h).
enum class MoveInputFlag : int {
    SneakDown               = 0,
    SneakToggleDown         = 1,
    WantDownSlow            = 2,
    WantUpSlow              = 3,
    BlockSelectDown         = 4,
    AscendBlock             = 5,
    DescendBlock            = 6,
    JumpDown                = 7,
    SprintDown              = 8,
    UpLeft                  = 9,
    UpRight                 = 10,
    DownLeft                = 11,
    DownRight               = 12,
    Up                      = 13,   // W
    Down                    = 14,   // S
    Left                    = 15,   // A
    Right                   = 16,   // D
    Ascend                  = 17,
    Descend                 = 18,
    ChangeHeight            = 19,
    LookCenter              = 20,
    SneakInputCurrentlyDown = 21,
    SneakInputWasReleased   = 22,
    SneakInputWasPressed    = 23,
    JumpInputWasReleased    = 24,
    JumpInputWasPressed     = 25,
    JumpInputCurrentlyDown  = 26,
    Count                   = 27,
};

// src/mc/input/MoveInputState.h — расчётный размер 0x10
struct MoveInputState {
    uint32_t   mFlagValues;          // 0x00  bitset<27, uint>
    glm::vec2  mAnalogMoveVector;    // 0x04
    uint8_t    mLookSlightDirField;  // 0x0C
    uint8_t    mLookNormalDirField;  // 0x0D
    uint8_t    mLookSmoothDirField;  // 0x0E
                                     // 0x0F — padding до 0x10 (align 4)
};

// src/mc/entity/components/MoveInputComponent.h — расчётный размер 0x64
struct MoveInputComponent {
    MoveInputState mInputState;                 // 0x00
    MoveInputState mRawInputState;              // 0x10
    uint8_t        mHoldAutoJumpInWaterTicks;   // 0x20
    // padding 0x21..0x23 (align 4)
    glm::vec2      mMove;                       // 0x24
    glm::vec2      mLookDelta;                  // 0x2C
    glm::vec2      mInteractDir;                // 0x34
    glm::vec3      mDisplacement;               // 0x3C
    glm::vec3      mDisplacementDelta;          // 0x48
    glm::vec3      mCameraOrientation;          // 0x54
    uint16_t       mFlagValues;                 // 0x60  bitset<11, ushort>
    bool           mIsPaddling[2];              // 0x62  std::array<bool,2>

    // ── работа с битами ────────────────────────────────────────────────────
    // raw = false → mInputState (обработанный ввод), true → mRawInputState (сырой)
    bool getFlag(MoveInputFlag flag, bool raw = false) const {
        uint32_t const bits = raw ? mRawInputState.mFlagValues : mInputState.mFlagValues;
        return (bits >> static_cast<int>(flag)) & 1u;
    }
    void setFlag(MoveInputFlag flag, bool value, bool raw = false) {
        uint32_t& bits = raw ? mRawInputState.mFlagValues : mInputState.mFlagValues;
        uint32_t const mask = 1u << static_cast<int>(flag);
        bits = value ? (bits | mask) : (bits & ~mask);
    }

    // ── совместимость со старым API (были полями) ──────────────────────────
    bool isSneakDown() const { return getFlag(MoveInputFlag::SneakDown); }
    void setSneakDown(bool v) { setFlag(MoveInputFlag::SneakDown, v); }

    bool isJumping() const { return getFlag(MoveInputFlag::JumpDown); }
    void setJumping(bool v) {
        setFlag(MoveInputFlag::JumpDown, v);
        setFlag(MoveInputFlag::JumpInputCurrentlyDown, v);
    }

    // старый mIsJumping2 — «джамп нажат прямо сейчас»
    bool isJumpingCurrentlyDown() const { return getFlag(MoveInputFlag::JumpInputCurrentlyDown); }
    void setJumpingCurrentlyDown(bool v) { setFlag(MoveInputFlag::JumpInputCurrentlyDown, v); }

    bool isSprinting() const { return getFlag(MoveInputFlag::SprintDown); }
    void setSprinting(bool v) { setFlag(MoveInputFlag::SprintDown, v); }

    bool isForward() const { return getFlag(MoveInputFlag::Up); }
    bool isBackward() const { return getFlag(MoveInputFlag::Down); }
    bool isLeft() const { return getFlag(MoveInputFlag::Left); }
    bool isRight() const { return getFlag(MoveInputFlag::Right); }
    void setForward(bool v) { setFlag(MoveInputFlag::Up, v); }
    void setBackward(bool v) { setFlag(MoveInputFlag::Down, v); }
    void setLeft(bool v) { setFlag(MoveInputFlag::Left, v); }
    void setRight(bool v) { setFlag(MoveInputFlag::Right, v); }

    glm::vec2 const& moveVector() const { return mMove; }
    void setMoveVector(glm::vec2 const& v) { mMove = v; }

    // [1.26] TODO: «move input locked» в 1.26 живёт не в этом компоненте
    // (кандидаты: ClientInputLockComponent / SetMovingFlagRequestComponent).
    // Оставлено как заглушка, чтобы код компилировался; подтвердить в IDA.
    bool isMoveLocked() const { return false; }
    void setMoveLocked(bool) {}

    void reset(bool lockMove = false, bool resetMove = true) {
        setMoveLocked(lockMove);
        setSneakDown(false);
        setJumping(false);
        setJumpingCurrentlyDown(false);
        setSprinting(false);
        if (resetMove) {
            setForward(false);
            setBackward(false);
            setLeft(false);
            setRight(false);
        }
        mMove = glm::vec2(0, 0);
    }
};

// В 1.21.44 это был отдельный ECS-компонент; в 1.26 «сырой» ввод — это
// mRawInputState того же компонента. Структура оставлена отдельной,
// но методы читают/пишут биты mRawInputState.
struct RawMoveInputComponent : MoveInputComponent {
    bool isSneakDown() const { return getFlag(MoveInputFlag::SneakDown, true); }
    void setSneakDown(bool v) { setFlag(MoveInputFlag::SneakDown, v, true); }
    bool isJumping() const { return getFlag(MoveInputFlag::JumpDown, true); }
    void setJumping(bool v) { setFlag(MoveInputFlag::JumpDown, v, true); }
    bool isSprinting() const { return getFlag(MoveInputFlag::SprintDown, true); }
    void setSprinting(bool v) { setFlag(MoveInputFlag::SprintDown, v, true); }
    bool isForward() const { return getFlag(MoveInputFlag::Up, true); }
    bool isBackward() const { return getFlag(MoveInputFlag::Down, true); }
    bool isLeft() const { return getFlag(MoveInputFlag::Left, true); }
    bool isRight() const { return getFlag(MoveInputFlag::Right, true); }
};

// [1.26] размер структуры 0x64 (100), в 1.21.44 было 136
static_assert(sizeof(MoveInputState) == 16, "MoveInputState size is not 16 bytes!");
static_assert(sizeof(MoveInputComponent) == 100, "MoveInputComponent size is not 100 bytes!");
static_assert(offsetof(MoveInputComponent, mInputState) == 0x00);
static_assert(offsetof(MoveInputComponent, mRawInputState) == 0x10);
static_assert(offsetof(MoveInputComponent, mMove) == 0x24);
static_assert(offsetof(MoveInputComponent, mDisplacement) == 0x3C);
static_assert(offsetof(MoveInputComponent, mCameraOrientation) == 0x54);
