#include "kernel_operator.h"

#include "erf_tiling.h"
#include "tiling_key_erf.h"

static constexpr uint32_t BUFFER_NUM = 2;
static constexpr uint32_t TILE_LENGTH = 2048;

static constexpr float ERF_CLAMP = 2.75f;
static constexpr float ERF_C0 = 1.12837500608f;
static constexpr float ERF_C1 = -0.376028255931f;
static constexpr float ERF_C2 = 0.112454472053f;
static constexpr float ERF_C3 = -0.0262818582733f;
static constexpr float ERF_C4 = 0.00476769821608f;
static constexpr float ERF_C5 = -0.000646623345734f;
static constexpr float ERF_C6 = 0.0000606508755252f;
static constexpr float ERF_C7 = -0.00000344208037461f;
static constexpr float ERF_C8 = 0.0000000877506744302f;

template <class DT_X>
class KernelErf {
public:
    __aicore__ inline KernelErf() {}

    __aicore__ inline void Init(GM_ADDR x, GM_ADDR y, uint32_t length) {
        this->length = length;
        this->blockIdx = AscendC::GetBlockIdx();
        this->blockNum = AscendC::GetBlockNum();

        xGm.SetGlobalBuffer((__gm__ DT_X *)x, length);
        yGm.SetGlobalBuffer((__gm__ DT_X *)y, length);

        pipe.InitBuffer(inQueueX, BUFFER_NUM, TILE_LENGTH * sizeof(DT_X));
        pipe.InitBuffer(outQueueY, BUFFER_NUM, TILE_LENGTH * sizeof(DT_X));
        pipe.InitBuffer(tmpBuffer1, TILE_LENGTH * sizeof(DT_X));
        pipe.InitBuffer(tmpBuffer2, TILE_LENGTH * sizeof(DT_X));
    }

    __aicore__ inline void Process() {
        uint32_t tileCount = (this->length + TILE_LENGTH - 1) / TILE_LENGTH;
        for (uint32_t tileIdx = this->blockIdx; tileIdx < tileCount; tileIdx += this->blockNum) {
            uint32_t offset = tileIdx * TILE_LENGTH;
            uint32_t currentLength = this->length - offset;
            currentLength = currentLength > TILE_LENGTH ? TILE_LENGTH : currentLength;
            bool alignedTile = currentLength == TILE_LENGTH;
            CopyIn(offset, currentLength, alignedTile);
            Compute(currentLength);
            CopyOut(offset, currentLength, alignedTile);
        }
    }

private:
    __aicore__ inline void CopyIn(uint32_t offset, uint32_t currentLength, bool alignedTile) {
        AscendC::LocalTensor<DT_X> xLocal = inQueueX.AllocTensor<DT_X>();
        if (alignedTile) {
            AscendC::DataCopy(xLocal, xGm[offset], TILE_LENGTH);
            inQueueX.EnQue(xLocal);
            return;
        }

        AscendC::DataCopyExtParams copyParams = {
            1,
            static_cast<uint32_t>(currentLength * sizeof(DT_X)),
            0,
            0,
            0
        };
        AscendC::DataCopyPadExtParams<DT_X> padParams = {false, 0, 0, static_cast<DT_X>(0)};
        AscendC::DataCopyPad(xLocal, xGm[offset], copyParams, padParams);
        inQueueX.EnQue(xLocal);
    }

    __aicore__ inline void Compute(uint32_t currentLength) {
        AscendC::LocalTensor<DT_X> xLocal = inQueueX.DeQue<DT_X>();
        AscendC::LocalTensor<DT_X> yLocal = outQueueY.AllocTensor<DT_X>();
        AscendC::LocalTensor<DT_X> x2Local = tmpBuffer1.Get<DT_X>();
        AscendC::LocalTensor<DT_X> polyLocal = tmpBuffer2.Get<DT_X>();

        AscendC::Maxs(yLocal, xLocal, static_cast<DT_X>(-ERF_CLAMP), currentLength);
        AscendC::Mins(yLocal, yLocal, static_cast<DT_X>(ERF_CLAMP), currentLength);
        AscendC::Mul(x2Local, yLocal, yLocal, currentLength);

        AscendC::Muls(polyLocal, x2Local, static_cast<DT_X>(ERF_C8), currentLength);
        AscendC::Adds(polyLocal, polyLocal, static_cast<DT_X>(ERF_C7), currentLength);
        AscendC::Mul(polyLocal, polyLocal, x2Local, currentLength);
        AscendC::Adds(polyLocal, polyLocal, static_cast<DT_X>(ERF_C6), currentLength);
        AscendC::Mul(polyLocal, polyLocal, x2Local, currentLength);
        AscendC::Adds(polyLocal, polyLocal, static_cast<DT_X>(ERF_C5), currentLength);
        AscendC::Mul(polyLocal, polyLocal, x2Local, currentLength);
        AscendC::Adds(polyLocal, polyLocal, static_cast<DT_X>(ERF_C4), currentLength);
        AscendC::Mul(polyLocal, polyLocal, x2Local, currentLength);
        AscendC::Adds(polyLocal, polyLocal, static_cast<DT_X>(ERF_C3), currentLength);
        AscendC::Mul(polyLocal, polyLocal, x2Local, currentLength);
        AscendC::Adds(polyLocal, polyLocal, static_cast<DT_X>(ERF_C2), currentLength);
        AscendC::Mul(polyLocal, polyLocal, x2Local, currentLength);
        AscendC::Adds(polyLocal, polyLocal, static_cast<DT_X>(ERF_C1), currentLength);
        AscendC::Mul(polyLocal, polyLocal, x2Local, currentLength);
        AscendC::Adds(polyLocal, polyLocal, static_cast<DT_X>(ERF_C0), currentLength);
        AscendC::Mul(yLocal, yLocal, polyLocal, currentLength);
        outQueueY.EnQue(yLocal);
        inQueueX.FreeTensor(xLocal);
    }

    __aicore__ inline void CopyOut(uint32_t offset, uint32_t currentLength, bool alignedTile) {
        AscendC::LocalTensor<DT_X> yLocal = outQueueY.DeQue<DT_X>();
        if (alignedTile) {
            AscendC::DataCopy(yGm[offset], yLocal, TILE_LENGTH);
            outQueueY.FreeTensor(yLocal);
            return;
        }

        AscendC::DataCopyExtParams copyParams = {
            1,
            static_cast<uint32_t>(currentLength * sizeof(DT_X)),
            0,
            0,
            0
        };
        AscendC::DataCopyPad(yGm[offset], yLocal, copyParams);
        outQueueY.FreeTensor(yLocal);
    }

private:
    AscendC::TPipe pipe;
    AscendC::TQue<AscendC::TPosition::VECIN, BUFFER_NUM> inQueueX;
    AscendC::TQue<AscendC::TPosition::VECOUT, BUFFER_NUM> outQueueY;
    AscendC::TBuf<AscendC::TPosition::VECCALC> tmpBuffer1;
    AscendC::TBuf<AscendC::TPosition::VECCALC> tmpBuffer2;
    AscendC::GlobalTensor<DT_X> xGm;
    AscendC::GlobalTensor<DT_X> yGm;
    uint32_t length;
    uint32_t blockIdx;
    uint32_t blockNum;
};

template <typename DT_X>
__global__ __aicore__ void erf(GM_ADDR x, GM_ADDR y, GM_ADDR workspace, GM_ADDR tiling) {
    REGISTER_TILING_DEFAULT(ErfTilingData);
    GET_TILING_DATA_WITH_STRUCT(ErfTilingData, tiling_data, tiling);
    KernelErf<DT_X> op;
    op.Init(x, y, tiling_data.length);
    op.Process();
}
