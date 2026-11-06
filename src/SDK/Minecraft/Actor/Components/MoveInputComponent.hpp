//
// Created by vastrakai on 6/30/2024.
//

#pragma once
#include <array>
#include <bitset>

struct MoveInputState
{
    enum class Flag : int {
        SneakDown = 0,
        SneakToggleDown = 1,
        WantDownSlow = 2,
        WantUpSlow = 3,
        BlockSelectDown = 4,
        AscendBlock = 5,
        DescendBlock = 6,
        JumpDown = 7,
        SprintDown = 8,
        UpLeft = 9,
        UpRight = 10,
        DownLeft = 11,
        DownRight = 12,
        Up = 13,
        Down = 14,
        Left = 15,
        Right = 16,
        Ascend = 17,
        Descend = 18,
        ChangeHeight = 19,
        LookCenter = 20,
        SneakInputCurrentlyDown = 21,
        SneakInputWasReleased = 22,
        SneakInputWasPressed = 23,
        JumpInputWasReleased = 24,
        JumpInputWasPressed = 25,
        JumpInputCurrentlyDown = 26,
        Count = 27,
    };

    std::bitset<27> FlagValues;
    glm::vec2 AnalogMoveVector;
    unsigned char LookSlightDirField;
    unsigned char LookNormalDirField;
    unsigned char LookSmoothDirField;
};

struct MoveInputComponent {
public:
    enum class Flag : int {
        Sneaking = 0,
        Sprinting = 1,
        WantUp = 2,
        WantDown = 3,
        Jumping = 4,
        AutoJumpingInWater = 5,
        MoveInputStateLocked = 6,
        PersistSneak = 7,
        AutoJumpEnabled = 8,
        IsCameraRelativeMovementEnabled = 9,
        IsRotControlledByMoveDirection = 10,
        Count = 11,
    };

    MoveInputState mInputState;
    MoveInputState mRawInputState;
    unsigned char mHoldAutoJumpInWaterTicks;
    glm::vec2 mMove;
    glm::vec2 mLookDelta;
    glm::vec2 mInteractDir;
    glm::vec3 mDisplacement;
    glm::vec3 mDisplacementDelta;
    glm::vec3 mCameraOrientation;
    std::bitset<8> mFlagValues;
    std::array<bool, 2> mIsPaddling;

    bool isUp() const {
        return mInputState.FlagValues.test((size_t)MoveInputState::Flag::Up);
    }
    void setUp(bool val) {
        mInputState.FlagValues.set((size_t)MoveInputState::Flag::Up, val);
    }

    bool isDown() const {
        return mInputState.FlagValues.test((size_t)MoveInputState::Flag::Down);
    }
    void setDown(bool val) {
        mInputState.FlagValues.set((size_t)MoveInputState::Flag::Down, val);
    }

    bool isLeft() const {
        return mInputState.FlagValues.test((size_t)MoveInputState::Flag::Left);
    }
    void setLeft(bool val) {
        mInputState.FlagValues.set((size_t)MoveInputState::Flag::Left, val);
    }

    bool isRight() const {
        return mInputState.FlagValues.test((size_t)MoveInputState::Flag::Right);
    }
    void setRight(bool val) {
        mInputState.FlagValues.set((size_t)MoveInputState::Flag::Right, val);
    }

    bool isJumping() const {
        return mInputState.FlagValues.test((size_t)MoveInputState::Flag::JumpDown);
    }
    void setJumping(bool val) {
        mInputState.FlagValues.set((size_t)MoveInputState::Flag::JumpDown, val);
        mRawInputState.FlagValues.set((size_t)MoveInputState::Flag::JumpDown, val);
    }

    bool isSneaking() const {
        return mInputState.FlagValues.test((size_t)MoveInputState::Flag::SneakDown);
    }
    void setSneaking(bool val) {
        mInputState.FlagValues.set((size_t)MoveInputState::Flag::SneakDown, val);
        mRawInputState.FlagValues.set((size_t)MoveInputState::Flag::SneakDown, val);
    }

    bool isSprinting() const {
        return mInputState.FlagValues.test((size_t)MoveInputState::Flag::SprintDown);
    }
    void setSprinting(bool val) {
        mInputState.FlagValues.set((size_t)MoveInputState::Flag::SprintDown, val);
        mRawInputState.FlagValues.set((size_t)MoveInputState::Flag::SprintDown, val);
    }
    void setmIsSprinting(bool val) {
        setSprinting(val);
    }

    bool isMoveLocked() const {
        return mFlagValues.test((size_t)Flag::MoveInputStateLocked);
    }
    void setMoveLocked(bool val) {
        mFlagValues.set((size_t)Flag::MoveInputStateLocked, val);
    }

    glm::vec2 getMoveVector() const { return mMove; }
    void setMoveVector(glm::vec2 val) { mMove = val; }

    void reset(bool lockMove = false, bool resetMove = true) {
        setMoveLocked(lockMove);
        setSneaking(false);
        setJumping(false);
        setSprinting(false);
        if (resetMove)
        {
            setUp(false);
            setDown(false);
            setLeft(false);
            setRight(false);
        }
        mMove = glm::vec2(0, 0);
        mInputState.AnalogMoveVector = glm::vec2(0, 0);
        mRawInputState.AnalogMoveVector = glm::vec2(0, 0);
    }

    // Zero-size property aliases for 1.21.44 module compatibility
    __declspec(property(get = isUp, put = setUp)) bool mForward;
    __declspec(property(get = isDown, put = setDown)) bool mBackward;
    __declspec(property(get = isLeft, put = setLeft)) bool mLeft;
    __declspec(property(get = isRight, put = setRight)) bool mRight;
    __declspec(property(get = isJumping, put = setJumping)) bool mIsJumping;
    __declspec(property(get = isSneaking, put = setSneaking)) bool mIsSneakDown;
    __declspec(property(get = isSprinting, put = setSprinting)) bool mIsSprinting;
    __declspec(property(get = isMoveLocked, put = setMoveLocked)) bool mIsMoveLocked;
    __declspec(property(get = getMoveVector, put = setMoveVector)) glm::vec2 mMoveVector;
};

struct RawMoveInputComponent {
public:
    glm::vec2 mRawMove;

    bool isJumping() const { return false; }
    void setJumping(bool) {}
    bool isSneaking() const { return false; }
    void setSneaking(bool) {}

    __declspec(property(get = isJumping, put = setJumping)) bool mIsJumping;
    __declspec(property(get = isSneaking, put = setSneaking)) bool mIsSneakDown;
};
