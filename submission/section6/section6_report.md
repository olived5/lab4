# Lab 4 — Section 6: FRFCFS_THRESHOLD

Threshold: 4

## Files
- `system/FRFCFS_THRESHOLD/RequestScheduler_FRFCFS_THRESHOLD.h`: written
- `system/FRFCFS_THRESHOLD/FRFCFS_THRESHOLD.ini`: written
- `src/SchedulerRegister.h`: written

## Checks
- model: PASS: threshold boundary cases 0, 1, 2 and configured value
- registration: PASS
- configuration: PASS
- source_contract: PASS
- build: PASS
- smoke: PASS
- threshold_behavior_runtime: NOT VERIFIED: simulator smoke tests do not prove the per-bank starvation bound; use a controlled same-bank hit/conflict trace and inspect request order

## Design
Uses the original FR-FCFS request and command scheduling configuration. For each bank, tracks the oldest pending request and the number of successfully scheduled younger row hits that bypass it. Once the configured limit is reached, younger hits to that bank cannot bypass that oldest request. The count resets when the oldest request changes. No counter is charged on a failed schedule.

## Limitations
The provided lab does not specify a numeric HIT Threshold; the value is an explicit script parameter (default 4). A successful compile and ordinary simulation do not establish full behavioral correctness. The queue structure is assumed to match the provided FRFCFS.ini.

