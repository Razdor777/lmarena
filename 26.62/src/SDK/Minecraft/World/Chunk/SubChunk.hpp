#pragma once
#include "SubChunkStorage.hpp"

class SubChunk {
public:
    PAD(0x1c);
    bool                                     mHasMaxSkyLight;                // this+0x0020
    bool                                     mNeedsInitLighting;              // this+0x0021
    bool                                     mNeedsClientLighting;                    // this+0x0022
    std::unique_ptr< ::SubChunkStorage< ::Block> > mBlocks[2];                           // this+0x0028
    class SubChunkBlockStorage* mBlocksReadPtr[2];                     // this+0x0038
    PAD(0x18);                     // this+0x0040
    uint64_t mHash;
    bool mHashDirty;
    signed char mAbsoluteIndex;
    bool mIsReplacementSubChunk;
    unsigned char mRenderChunkTrackingVersionNumber;
    bool mIsInitialized;

    // ==== ported from the 1.21.44 master ====
    // В 1.26.x поля переименованы:
    //   blockReadPtr  -> mBlocksReadPtr[0]           (this+0x38)
    //   subchunkIndex -> mAbsoluteIndex              (this+0x59)
    //   blocks        -> (нет прямого аналога, mBlocks[2])
    // Портированный код (OreMiner*, BlockESP, SmartXRay, InfiniteChestAura, BlockUtils)
    // обращается к ним как к обычным полям, поэтому здесь объявлены поля-адаптеры:
    // в MSVC это делает __declspec(property).
    //
    // mBlocksReadPtr[0] — указатель для чтения, [1] — для записи.
    [[nodiscard]] SubChunkBlockStorage* getBlockReadPtr() const { return mBlocksReadPtr[0]; }
    void setBlockReadPtr(SubChunkBlockStorage* p) { mBlocksReadPtr[0] = p; }
    __declspec(property(get = getBlockReadPtr, put = setBlockReadPtr)) SubChunkBlockStorage* blockReadPtr;

    [[nodiscard]] signed char getSubchunkIndex() const { return mAbsoluteIndex; }
    __declspec(property(get = getSubchunkIndex)) signed char subchunkIndex;
    // ==== end of ported block ====
};
