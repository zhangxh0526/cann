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
        this->blockIdx = AscendC::GetBlockIdx();
        this->blockNum = AscendC::GetBlockNum();

        uint32_t baseLength = length / this->blockNum;
        uint32_t tail = length % this->blockNum;
        this->coreStart = this->blockIdx * baseLength + (this->blockIdx < tail ? this->blockIdx : tail);
        this->coreLength = baseLength + (this->blockIdx < tail ? 1 : 0);

        xGm.SetGlobalBuffer((__gm__ DT_X *)x, length);
        yGm.SetGlobalBuffer((__gm__ DT_X *)y, length);

        pipe.InitBuffer(inQueueX, BUFFER_NUM, TILE_LENGTH * sizeof(DT_X));
        pipe.InitBuffer(outQueueY, BUFFER_NUM, TILE_LENGTH * sizeof(DT_X));
        pipe.InitBuffer(tmpBuffer1, TILE_LENGTH * sizeof(DT_X));
        pipe.InitBuffer(tmpBuffer2, TILE_LENGTH * sizeof(DT_X));
    }

    __aicore__ inline void Process() {
        for (uint32_t offset = 0; offset < this->coreLength; offset += TILE_LENGTH) {
            uint32_t currentLength = this->coreLength - offset;
            currentLength = currentLength > TILE_LENGTH ? TILE_LENGTH : currentLength;
            CopyIn(offset, currentLength);
            Compute(currentLength);
            CopyOut(offset, currentLength);
        }
    }

private:
    __aicore__ inline void CopyIn(uint32_t offset, uint32_t currentLength) {
        AscendC::LocalTensor<DT_X> xLocal = inQueueX.AllocTensor<DT_X>();
        AscendC::DataCopyExtParams copyParams = {
            1,
            static_cast<uint32_t>(currentLength * sizeof(DT_X)),
            0,
            0,
            0
        };
        AscendC::DataCopyPadExtParams<DT_X> padParams = {false, 0, 0, static_cast<DT_X>(0)};
        AscendC::DataCopyPad(xLocal, xGm[this->coreStart + offset], copyParams, padParams);
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

    __aicore__ inline void CopyOut(uint32_t offset, uint32_t currentLength) {
        AscendC::LocalTensor<DT_X> yLocal = outQueueY.DeQue<DT_X>();
        AscendC::DataCopyExtParams copyParams = {
            1,
            static_cast<uint32_t>(currentLength * sizeof(DT_X)),
            0,
            0,
            0
        };
        AscendC::DataCopyPad(yGm[this->coreStart + offset], yLocal, copyParams);
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
    uint32_t blockIdx;
    uint32_t blockNum;
    uint32_t coreStart;
    uint32_t coreLength;
};

template <typename DT_X>
__global__ __aicore__ void erf(GM_ADDR x, GM_ADDR y, GM_ADDR workspace, GM_ADDR tiling) {
    REGISTER_TILING_DEFAULT(ErfTilingData);
    GET_TILING_DATA_WITH_STRUCT(ErfTilingData, tiling_data, tiling);
    KernelErf<DT_X> op;
    op.Init(x, y, tiling_data.length);
    op.Process();
}
