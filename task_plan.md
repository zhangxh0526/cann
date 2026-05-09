# Task Plan: ERF Challenge Preparation

## Goal
Analyze the ERF challenge, understand its scoring and constraints, and develop the fastest correct implementation in this cloned codebase.

## Current Phase
Phase 4

## Phases

### Phase 1: Requirements & Discovery
- [x] Inspect the challenge page and capture the rules, scoring model, and constraints
- [x] Inspect the cloned codebase structure and current baseline implementation
- [x] Identify what is allowed to change and what must remain compatible
- **Status:** complete

### Phase 2: Planning & Benchmarking
- [x] Define the optimization strategy based on the observed bottlenecks
- [x] Establish a baseline build and runtime measurement workflow
- [x] Document technical decisions
- **Status:** complete

### Phase 3: Implementation
- [x] Make the smallest changes that improve throughput or latency
- [x] Keep correctness intact while iterating on hot paths
- [x] Preserve compatibility with the judge interface
- **Status:** complete

### Phase 4: Testing & Verification
- [ ] Build and run the solution locally
- [ ] Benchmark changes against the baseline
- [ ] Fix any regressions found during verification
- **Status:** in_progress, blocked on local ASC/CANN package

### Phase 5: Delivery
- [ ] Summarize findings, changes, and remaining risks
- [ ] Give the user a clear next step for submission
- **Status:** pending

## Key Questions
1. What exact scoring or ranking signal does the judge use?
2. What baseline performance does the current code achieve on the judge?
3. Which parts of the host/kernel contract are fixed versus editable?

## Decisions Made
| Decision | Rationale |
|----------|-----------|
| Use a discovery-first workflow | The competition details and hot spots are not yet known. |
| Keep changes narrowly focused on performance | The goal is ranking, so unrelated refactors add risk without value. |
| Start with a conservative vectorized rational-approximation kernel | It targets correctness and a submit-ready baseline before per-testcase leaderboard tuning. |
| Switch first implementation to an odd polynomial approximation | The page's displayed rational formula did not meet `1e-4` in local sampled checks, while the odd polynomial does and avoids division/exp. |
| Keep 2048-element tiles after v4 | The 1024-tile experiment improved only a couple of tiny cases and regressed large cases badly. |
| Keep the v5 degree-7 polynomial as the active baseline | CANNJudge score improved from v2 `62.42` to v5 `66.07`, with 13/15 testcase timings faster than v2. |

## Errors Encountered
| Error | Attempt | Resolution |
|-------|---------|------------|
| Playwright wrapper failed under WSL1 | 1 | Used `npx --package @playwright/cli playwright-cli` from PowerShell. |
| PowerShell cleanup script variables were expanded by the outer shell | 1-2 | Re-ran with single-quote escaping and removed `.playwright-cli/`. |

## Notes
- Re-read this plan before major decisions.
- Log all findings immediately so they survive context resets.
- User will log in to the CANNJudge platform and submit builds after code is prepared.
- User is applying for Huawei-provided resources; until then, verification happens through website submissions and user-provided per-testcase results.
- Active baseline as of 2026-05-09 16:58 Asia/Shanghai: v5 degree-7 polynomial, 2048-element tiles, score `66.07`.
