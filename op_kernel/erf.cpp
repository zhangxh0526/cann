// Kernel侧核函数实现
#include "kernel_operator.h"

#include "erf_tiling.h"
#include "tiling_key_erf.h"

template <class DT_X>
class KernelErf {
public:
    __aicore__ inline KernelErf() {}
    __aicore__ inline void Init(GM_ADDR x, GM_ADDR y, uint32_t length) {

    }
    __aicore__ inline void Process() {

    }
private:

};

template <typename DT_X>
 __global__ __aicore__ void erf(GM_ADDR x, GM_ADDR y, GM_ADDR workspace, GM_ADDR tiling) {
    REGISTER_TILING_DEFAULT(ErfTilingData);
    GET_TILING_DATA_WITH_STRUCT(ErfTilingData, tiling_data, tiling);
    KernelErf<DT_X> op;
    op.Init(x, y, tiling_data.length);
    op.Process();
}

