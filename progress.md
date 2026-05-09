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
  - Rendered the ranking tab and captured current first-place score and testcase time distribution.
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

## Test Results
| Test | Input | Expected | Actual | Status |
|------|-------|----------|--------|--------|
| Planning files created | N/A | Files exist in repo root | Created | Pass |
| Numeric polynomial guard | `rtk python tools/check_erf_kernel.py` | Sampled float32 error under `1e-4` | `erf kernel constants pass sampled accuracy checks` | Pass |
| Local CMake configure | `rtk cmake -S . -B build` | Configure if ASC package exists | Failed: missing `ASCConfig.cmake`/`asc-config.cmake` | Blocked |

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
| Where am I? | Paused after Phase 1 discovery |
| Where am I going? | Phase 2/3: prepare a submit-ready ERF implementation now, then iterate from website results |
| What's the goal? | Build the fastest correct ERF solution for the judge |
| What have I learned? | See findings.md |
| What have I done? | Read instructions, inspected repo root, created planning files |
