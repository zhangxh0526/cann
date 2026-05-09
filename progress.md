# Progress Log

## Session: 2026-05-09

### Phase 1: Requirements & Discovery
- **Status:** complete
- **Started:** 2026-05-09 14:53:45
- Actions taken:
  - Read workspace instructions and relevant workflow skills.
  - Inspected the repository root and confirmed the presence of `op_host`, `op_kernel`, and `CMakeLists.txt`.
  - Created initial planning files to persist requirements and findings.
  - Fetched and inspected the SPA routing and problem-detail frontend modules.
  - Rendered the actual ERF problem page with Playwright and captured the challenge constraints.
  - Rendered the ranking tab and captured first-place score and testcase time distribution at 2026-05-09 15:05 Asia/Shanghai.
  - Read local host/kernel/CMake files and confirmed the kernel implementation is empty.
  - Checked local git/toolchain state: repo is at `600a985 baseline template`, CMake exists, no `ASCEND*` env vars were present.
  - Confirmed the submission workflow with the user: I prepare code, the user submits on CANNJudge, then returns detailed per-testcase timings for iteration.
- Files created/modified:
  - `task_plan.md` (created)
  - `findings.md` (created)
  - `progress.md` (created)

### Phase 2: Planning & Benchmarking
- **Status:** complete
- Actions taken:
  - User clarified that implementation should start immediately and website submissions will provide feedback until Huawei resources are available.
  - Selected first-submission strategy: vectorized odd polynomial approximation with `DataCopyPad` for non-aligned transfers.
- Files created/modified:
  - `task_plan.md`
  - `findings.md`
  - `progress.md`

### Phase 3: Implementation
- **Status:** complete
- Actions taken:
  - Added `tools/check_erf_kernel.py` as a local numerical guard for kernel constants.
  - Implemented `op_kernel/erf.cpp` with `DataCopyPad`, per-core contiguous splitting, clamp to `[-2.75, 2.75]`, and Horner polynomial evaluation.
  - Updated `op_host/erf.cpp` to set block dim from input length instead of always launching all AIV cores.
  - Rewrote `op_host/erf.cpp` with ASCII comments/formatting to avoid the original encoding-corrupted line layout around `length_x`.
- Files created/modified:
  - `tools/check_erf_kernel.py`
  - `op_kernel/erf.cpp`
  - `op_host/erf.cpp`

### Phase 4: Website Feedback Iteration 1
- **Status:** in_progress
- Actions taken:
  - User submitted first implementation to CANNJudge.
  - Received result: rank 115, score `60.33`, testcase times `[3.98us, 6.76us, 1.76ms, 3.56us, 6.98us, 1.76ms, 4.80us, 8.12us, 2.30ms, 3.58us, 7.98us, 2.30ms, 4.34us, 1.62ms, 5.47ms]`.
  - Fetched current leaderboard data; first place is score `87.62` with times `[1.86us, 4.28us, 1.597ms, 1.94us, 4.64us, 1.598ms, 5.16us, 5.98us, 2.098ms, 4.74us, 6.32us, 2.105ms, 3.32us, 1.469ms, 5.051ms]`.
  - Added a transfer-strategy check to `tools/check_erf_kernel.py`; it failed on v1 because no aligned `DataCopy` fast path existed.
  - Implemented v2 kernel transfer strategy: grid-stride 2048-element tiles, aligned `DataCopy` for full tiles, and `DataCopyPad` only for the tail.
- Files created/modified:
  - `progress.md`
  - `findings.md`
  - `tools/check_erf_kernel.py`
  - `op_kernel/erf.cpp`

### Phase 4: Website Feedback Iteration 2
- **Status:** complete
- Actions taken:
  - Rejected v3 based on website score regression from `62.42` to `57.06`.
  - Restored the v2 hand-written polynomial route.
  - Prepared v4 as a single-variable experiment: kernel and host tile size changed from 2048 to 1024 elements.
  - Extended `tools/check_erf_kernel.py` to enforce 1024-element host/kernel tile agreement and reject `AscendC::Erf`.
  - User submitted v4; result regressed to score `53.28`, so 1024-element tiles were rejected.
- Files created/modified:
  - `progress.md`
  - `findings.md`
  - `tools/check_erf_kernel.py`
  - `op_kernel/erf.cpp`
  - `op_host/erf.cpp`

### Phase 4: Website Feedback Iteration 3
- **Status:** complete
- Actions taken:
  - Restored 2048-element tile behavior from v2.
  - Searched lower-degree odd polynomial candidates locally and selected a degree-7 approximation with sampled max abs/rel error around `5.36e-5`.
  - Implemented v5 by removing the `ERF_C8` term and one Horner multiply/add stage.
  - Strengthened `tools/check_erf_kernel.py` to enforce 2048-element tiles, reject the official `AscendC::Erf` route, reject `ERF_C8`, and sample accuracy more densely.
  - User submitted v5 to CANNJudge at 2026-05-09 16:58 Asia/Shanghai; result score `66.07`, timings `[3.60us, 6.44us, 1.70ms, 3.82us, 6.20us, 1.68ms, 3.42us, 7.46us, 2.20ms, 4.34us, 7.28us, 2.20ms, 3.54us, 1.56ms, 5.31ms]`.
  - v5 improved over v2 by `+3.65` score points and 13/15 testcase timings, so degree-7 polynomial is the active baseline for future work.
- Files created/modified:
  - `progress.md`
  - `findings.md`
  - `tools/check_erf_kernel.py`
  - `op_kernel/erf.cpp`

## Test Results
| Test | Input | Expected | Actual | Status |
|------|-------|----------|--------|--------|
| Planning files created | N/A | Files exist in repo root | Created | Pass |
| Numeric polynomial guard | `rtk python tools/check_erf_kernel.py` | Sampled float32 error under `1e-4` | `erf kernel constants pass sampled accuracy checks` | Pass |
| Local CMake configure | `rtk cmake -S . -B build` | Configure if ASC package exists | Failed: missing `ASCConfig.cmake`/`asc-config.cmake` | Blocked |
| Website submission v1 | User submitted current code | Pass and useful timing data | Pass, score `60.33`, rank 115 | Needs optimization |
| v2 local guard | `rtk python tools/check_erf_kernel.py` | Numeric and transfer-strategy checks pass | `erf kernel constants pass sampled accuracy checks` | Pass |
| Website submission v2 | User submitted grid-stride/DataCopy version | Improve over v1 | Pass, score `62.42`, rank 113, 13/15 faster | Useful, keep |
| v3 local guard | `rtk python tools/check_erf_kernel.py` | Uses official Erf API, keeps transfer strategy, no source/destination overlap | `erf kernel structure checks passed` | Pass |
| Website submission v3 | User submitted official `AscendC::Erf` version | Beat v2 if API is optimized | Pass, score `57.06`; slower than v2 | Reject v3 route |
| v4 local guard | `rtk python tools/check_erf_kernel.py` | Restore polynomial route and enforce 1024-element host/kernel tiles | `erf kernel constants and transfer strategy checks passed` | Pass |
| Website submission v4 | User submitted 1024-tile version | Improve small/medium cases | Pass, score `53.28`; large cases much slower | Reject v4, restore 2048 |
| v5 local guard | `rtk python tools/check_erf_kernel.py` | Degree-7 polynomial, 2048 tiles, transfer strategy intact | `erf kernel constants and transfer strategy checks passed` | Pass |
| Website submission v5 | User submitted degree-7 polynomial version | Improve over v2 by reducing arithmetic chain | Pass, score `66.07`, 13/15 timings faster than v2 | Active baseline |

## Error Log
| Timestamp | Error | Attempt | Resolution |
|-----------|-------|---------|------------|
| 2026-05-09 15:05 | Playwright wrapper failed under WSL1 | 1 | Used direct `npx --package @playwright/cli playwright-cli` from PowerShell. |
| 2026-05-09 15:10 | PowerShell cleanup script lost variables due outer quoting | 1-2 | Used single-quote escaping and safely deleted `.playwright-cli/`. |
| 2026-05-09 15:40 | Directly interpreted page rational formula failed accuracy guard | 1 | Replaced with fitted odd polynomial approximation. |
| 2026-05-09 15:45 | Local CMake configure failed because ASC package is unavailable | 1 | Cleaned `build/`; website submission is the next compiler/runtime check. |

## 5-Question Reboot Check
| Question | Answer |
|----------|--------|
| Where am I? | Phase 4 website-feedback iteration; active baseline is v5 degree-7 polynomial with score `66.07`. |
| Where am I going? | Continue toward first place by testing lower arithmetic cost, small-input fast paths, and length-specific tiling once more timing data or Huawei resources are available. |
| What's the goal? | Build the fastest correct ERF solution for the judge |
| What have I learned? | See findings.md |
| What have I done? | Implemented and evaluated v1-v5; rejected official `AscendC::Erf` and 1024-tile route; kept 2048-tile degree-7 polynomial as the best known version. |
