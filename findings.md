# Findings & Decisions

## Requirements
- Inspect the competition page at `https://cannjudge.cn/public/op_challenge_jiangshan_prelim/erf`
- Work from the cloned code in `F:\cann-erf\code`
- Optimize for first place using the judge's microsecond-level timing signal
- The challenge is `erf` in `2026年CANN算子挑战赛_江山赛区（初赛）`.
- Submission workflow: after code is written, the user will log in to the platform, submit, and paste back detailed per-testcase timing/score results for iteration.

## Research Findings
- The repository root contains `op_host`, `op_kernel`, and a top-level `CMakeLists.txt`.
- `op_host` and `op_kernel` each have their own `CMakeLists.txt`, suggesting a host/kernel split.
- The user has not yet provided the contents of the competition page, so the scoring details still need to be confirmed.
- The competition URL currently serves a single-page shell with `app.js` and `app.css`; the visible HTML itself does not include the challenge details.
- The page title is `CANNJudge`, which suggests the challenge details are loaded client-side.
- `js/pages/open.js` shows the public experience for ongoing contests: a problem list, ranking tab, and submission tab.
- Public contest ranking sorts by `score` by default and only switches to `passCount` when `contest.ranking_mode === 'num'`.
- The open problem list aggregates problems from ongoing public contests and displays pass rate, pass count, and attempt count from `/api/submissions/contest/{id}/stats`.
- `js/pages/problem/detail.js` shows the actual problem detail page: title, description, ranking tab, and a submission list.
- Problem ranking comes from `/api/problems/{problemId}/ranking` and includes per-testcase times, total score, and baseline comparison metadata.
- A problem can expose baseline timings for each testcase; when present, the page renders a baseline row and colors testcase times against that baseline.
- The problem page also shows the user's last submissions and a "开始答题" entry point.
- `js/foundation.js` is just a composition layer; the useful formatting helpers live in `js/core/*`, `js/services/*`, and `js/ui/*`.
- `js/ui/shared.js` is where the detailed presentation helpers live, including problem metadata, status chips, and likely the testcase timing labels used by the ranking table.
- `testcaseTimeLabel` formats values below 1000 as microseconds and switches to milliseconds at 1000+, so the judge's displayed hot-path metric is microsecond-scale.
- `compareAgainstBaselineTone` marks a testcase as better when the submitted time is less than or equal to the baseline time for that testcase.
- Rendered problem metadata: chip type `910B`, project type `自定义算子工程`, pass rate `53%`, pass users `134`, attempts `251`.
- Problem requirements: implement `y = erf(x)` against PyTorch `torch.erf`; input/output are float32 ND tensors with identical shape and type.
- Accuracy requirement: float32 relative error < `1e-4` and absolute error < `1e-4`.
- Compatibility requirement: support arbitrary dimensions and non-32-byte-aligned/non-32-multiple shapes.
- Test coverage stated on the page: 1D/2D/3D/4D/high-dimensional tensors, non-aligned cases, values in `[-3, 3]`, saturated ranges `[-10, -3]` and `[3, 10]`, and near-zero range `[-0.1, 0.1]`.
- Ranking page has 15 testcase columns. Current first place is score `87.21`, with testcase times roughly `1.86us, 4.28us, 1.60ms, 1.94us, 4.64us, 1.60ms, 5.16us, 5.98us, 2.10ms, 4.74us, 6.32us, 2.11ms, 3.32us, 1.47ms, 5.05ms`.
- The rendered page made API calls to public group `69c8ffcea2bcbfa3591f481e`, contest `69e7798bbaa77b2077c513ce`, and problem `69e782b3baa77b2077c515f4`.
- Problem JSON fields: `code_template=custom_template`, `score_mode=1`, `use_baseline=false`, tag `vector`, testcase IDs `476` through `490`.
- All problem testcase baseline fields are `null`, so the visible leaderboard does not expose official baseline timings.
- The official page gives a suggested rational approximation: clamp `x` to `[-3.92, 3.92]`, compute a degree-11 odd numerator over a degree-10 denominator using `x2 = x * x`.
- Converted coefficients from the suggested approximation:
  - Numerator: `0.053443748819`, `7.5517016694`, `101.62808918`, `1393.8061484`, `5063.791506`, `29638.38468`
  - Denominator: `1.0`, `31.212858877`, `398.56963806`, `3023.124815`, `13243.365831`, `26672.24157`
- API ranking top row confirms first place submission `10247` has score `87.21`, status `Pass`, and `precision_ratio=1` for all 15 testcases.
- Local kernel file `op_kernel/erf.cpp` is currently an empty template: `Init` and `Process` have no implementation.
- Local host file `op_host/erf.cpp` currently records only input length in tiling data, sets block dim to all AIV cores, and registers float32 ND input/output.
- Local `ErfTilingData` currently contains only `uint32_t length`, so any optimized tiling needs either derive from block index in kernel or extend this struct.
- `cmake` exists locally (`4.0.2`), but no `ASCEND*` environment variables are set in the current PowerShell environment.
- Official Ascend C samples use `GlobalTensor::SetGlobalBuffer`, `TPipe`, `TQue`, `DataCopy`, `EnQue`, `DeQue`, vector APIs, and `DataCopy` back to GM for vector operators.
- Official docs state ordinary `DataCopy` length and UB start must be 32-byte aligned in non-aligned scenarios.
- Official docs state `DataCopyPad` supports Atlas A2 training/inference series, which covers Ascend 910B, and can handle non-32B-aligned GM<->UB transfers with padding.
- `DataCopyPad` GM->Local uses `DataCopyExtParams{blockCount, blockLenBytes, srcStride, dstStride, rsv}` plus `DataCopyPadExtParams<T>{isPad, leftPadding, rightPadding, paddingValue}`.
- First implementation uses `erf(x) ~= clamp(x, -2.75, 2.75) * P(clamp(x)^2)` with degree-8 polynomial in `x^2`, avoiding `Div`, `Exp`, and sign-selection logic.
- Local sampled polynomial check simulates float32 operations and passes `1e-4` absolute/relative threshold over the tested coverage range.
- Host tiling now sets `blockDim = min(num_cores_aiv, ceil(length / 2048))`, so tiny inputs do not launch all AIV cores.
- First website result for odd-polynomial version: score `60.33`, rank 115, times `[3.98us, 6.76us, 1.76ms, 3.56us, 6.98us, 1.76ms, 4.80us, 8.12us, 2.30ms, 3.58us, 7.98us, 2.30ms, 4.34us, 1.62ms, 5.47ms]`.
- Current first place after fetching leaderboard is score `87.62`; same top submission times are `[1.86us, 4.28us, 1.597ms, 1.94us, 4.64us, 1.598ms, 5.16us, 5.98us, 2.098ms, 4.74us, 6.32us, 2.105ms, 3.32us, 1.469ms, 5.051ms]`.
- Pattern from v1: small microsecond tests 1/2/4/5 are much slower than top, large tests are around 8-12% slower, while tests 7 and 10 are already near or faster than the current top row.
- Direct testcase API access such as `/api/testcases/69e782b3baa77b2077c515f8` returns 403, so testcase shapes/lengths are not publicly exposed.
- Optimization hypothesis for v2: v1 uses `DataCopyPad` for every tile and per-core contiguous slices can create non-32B-aligned starts/lengths; grid-stride 2048-element tiles allow full tiles to use aligned `DataCopy` and reserve `DataCopyPad` for the final tail.
- Website result for v2: score `62.42`, rank `113`, times `[3.58us, 6.60us, 1.72ms, 4.36us, 6.28us, 1.74ms, 3.58us, 7.96us, 2.26ms, 4.64us, 7.74us, 2.25ms, 3.34us, 1.61ms, 5.36ms]`.
- v2 vs v1: improved 13/15 testcases and raised score by `+2.09`, so the aligned `DataCopy`/grid-stride change is useful and should be kept as a baseline. Regressions were testcase 4 (`+0.80us`) and testcase 10 (`+1.06us`), likely small-input/noise or workload-specific scheduling effects.
- v2 is still far from first place: small tests are still roughly 45-125% slower than top where comparable, and large tests are still roughly 7-10% slower, so next optimization should target arithmetic chain length and small-input path rather than only transfer.

## Technical Decisions
| Decision | Rationale |
|----------|-----------|
| Treat the site rules as the source of truth | The exact evaluation model may be competition-specific and time-sensitive. |
| Start with structure and baseline inspection before code changes | Performance work needs a known baseline and a clear contract. |
| Implement now and use website submissions for feedback | User clarified that website submission is available now; local/remote Huawei resources will come later. |
| First submission should prioritize correctness over extreme specialization | A passing baseline is needed before optimizing around per-testcase timing. |

## Issues Encountered
| Issue | Resolution |
|-------|------------|
| Challenge page contents not yet captured | Will fetch and inspect the page next. |
| Playwright wrapper script failed under WSL1 | Used `npx --package @playwright/cli playwright-cli` directly from PowerShell. |
| Local CANN toolchain not confirmed | Treat judge submission as primary verification unless CANN/ASC environment becomes available. |
| Initial local search found no useful Ascend C examples in the cloned repo | Used official HiAscend documentation and public Ascend/samples snippets for API shape. |
| PowerShell command lost `$_.Name` in a directory scan due quoting | Do not reuse that exact outer-double-quoted pattern; use single quotes or simpler commands. |
| Page rational formula failed local accuracy guard | Switched first implementation to a fitted odd polynomial approximation that passes sampled accuracy. |
| `cmake -S . -B build` failed locally | Missing `ASCConfig.cmake`/`asc-config.cmake`; cleaned generated `build/` directory and will rely on website compile feedback. |

## Resources
- Competition page: https://cannjudge.cn/public/op_challenge_jiangshan_prelim/erf
- Repo root: `F:\cann-erf\code`

## Visual/Browser Findings
- The page at `https://cannjudge.cn/public/op_challenge_jiangshan_prelim/erf` is a SPA shell with an empty `#app` container.
- The raw HTML only references `/app.css` and `/app.js`, so the actual rules and scoreboard data likely come from JavaScript or API calls.
- `https://cannjudge.cn/js/pages/open.js` confirms the public contest page is data-driven and that ranking and submission data are fetched from `/api/submissions/...` endpoints.
- `https://cannjudge.cn/js/pages/problem/detail.js` confirms the judging surface shows per-testcase times and a baseline comparison row, which means optimization will need to focus on measured runtime per testcase rather than only pass/fail.
- The detailed time-formatting and baseline-color helpers are not in the first chunk of `ui/shared.js`; they need one more targeted search in the same file or its `core` dependency.
- The time display logic confirms that sub-1000 values are the important regime for leaderboard gains, so reducing per-testcase microseconds is the relevant target.
- Real browser rendering successfully loaded the ERF problem and ranking pages.
- The visible top leaderboard confirms the first-place target score is at least `87.21` as of 2026-05-09 15:05 Asia/Shanghai.
