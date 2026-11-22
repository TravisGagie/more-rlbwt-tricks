## Counters of the search (user space only; the 100-character run subtracted)

| row | individuals | algorithm | mode | cycles | IPC | stalled, any reason | on any memory access | on an L3 miss | L3-miss loads | page walks |
|---|---:|---|---|---:|---:|---:|---:|---:|---:|---:|
| plain, move structure | 16 | BML | without huge pages | 8.738e+10 | 0.38 | 76.4% | 74.2% | 56.5% | 1.448e+08 | 18.2% |
| plain, move structure | 16 | BML | with huge pages | 7.261e+10 | 0.46 | 72.3% | 69.8% | 64.3% | 1.430e+08 | 0.1% |
| plain, move structure | 16 | FB | without huge pages | 8.525e+10 | 0.38 | 76.5% | 74.3% | 56.9% | 1.413e+08 | 17.9% |
| plain, move structure | 16 | FB | with huge pages | 7.087e+10 | 0.46 | 72.5% | 69.9% | 64.5% | 1.349e+08 | 0.1% |
| plain, move structure | 32 | BML | without huge pages | 9.652e+10 | 0.38 | 76.6% | 74.4% | 56.5% | 1.560e+08 | 17.6% |
| plain, move structure | 32 | BML | with huge pages | 8.019e+10 | 0.45 | 72.6% | 70.1% | 64.5% | 1.540e+08 | 0.1% |
| plain, move structure | 32 | FB | without huge pages | 9.527e+10 | 0.38 | 76.6% | 74.4% | 56.5% | 1.573e+08 | 17.3% |
| plain, move structure | 32 | FB | with huge pages | 7.911e+10 | 0.45 | 72.6% | 70.2% | 64.6% | 1.517e+08 | 0.1% |
| plain, move structure | 64 | BML | without huge pages | 1.038e+11 | 0.37 | 76.7% | 74.6% | 56.4% | 1.670e+08 | 17.9% |
| plain, move structure | 64 | BML | with huge pages | 8.605e+10 | 0.45 | 72.6% | 70.1% | 64.5% | 1.652e+08 | 0.1% |
| plain, move structure | 64 | FB | without huge pages | 1.027e+11 | 0.37 | 76.8% | 74.6% | 56.4% | 1.667e+08 | 17.7% |
| plain, move structure | 64 | FB | with huge pages | 8.523e+10 | 0.45 | 72.7% | 70.3% | 64.6% | 1.620e+08 | 0.1% |
| heavy paths θ = 90%, with the first-run table | 16 | BML | without huge pages | 1.322e+10 | 1.56 | 38.4% | 32.2% | 21.5% | 1.310e+07 | 15.5% |
| heavy paths θ = 90%, with the first-run table | 16 | BML | with huge pages | 1.216e+10 | 1.70 | 33.9% | 27.6% | 19.6% | 1.237e+07 | 1.6% |
| heavy paths θ = 90%, with the first-run table | 16 | FB | without huge pages | 1.318e+10 | 1.57 | 38.6% | 32.2% | 21.2% | 1.306e+07 | 15.1% |
| heavy paths θ = 90%, with the first-run table | 16 | FB | with huge pages | 1.219e+10 | 1.69 | 34.4% | 28.0% | 19.9% | 1.227e+07 | 1.6% |
| heavy paths θ = 90%, with the first-run table | 32 | BML | without huge pages | 1.289e+10 | 1.58 | 37.5% | 31.1% | 20.2% | 1.226e+07 | 15.8% |
| heavy paths θ = 90%, with the first-run table | 32 | BML | with huge pages | 1.189e+10 | 1.72 | 33.1% | 26.6% | 18.3% | 1.180e+07 | 1.4% |
| heavy paths θ = 90%, with the first-run table | 32 | FB | without huge pages | 1.265e+10 | 1.60 | 37.2% | 30.8% | 19.8% | 1.179e+07 | 15.5% |
| heavy paths θ = 90%, with the first-run table | 32 | FB | with huge pages | 1.168e+10 | 1.73 | 32.9% | 26.3% | 17.9% | 1.109e+07 | 1.3% |
| heavy paths θ = 90%, with the first-run table | 64 | BML | without huge pages | 1.321e+10 | 1.56 | 38.0% | 31.6% | 20.6% | 1.298e+07 | 16.4% |
| heavy paths θ = 90%, with the first-run table | 64 | BML | with huge pages | 1.221e+10 | 1.69 | 33.9% | 27.2% | 18.8% | 1.221e+07 | 1.1% |
| heavy paths θ = 90%, with the first-run table | 64 | FB | without huge pages | 1.289e+10 | 1.57 | 37.8% | 31.3% | 20.3% | 1.228e+07 | 16.3% |
| heavy paths θ = 90%, with the first-run table | 64 | FB | with huge pages | 1.190e+10 | 1.70 | 33.5% | 26.9% | 18.4% | 1.172e+07 | 1.3% |

## Bounds on plain search with the move structure

Times are the final timing session's medians (`final/runs.tsv`). Bound L3 = time x (1 - share stalled on an L3 miss); bound mem = time x (1 - share stalled on any memory access); best = the fastest two-level row in the same mode, algorithm and size; ratio = bound / best.

| individuals | algorithm | mode | time | bound L3 | bound mem | best two-level row | best | bound L3 / best | bound mem / best | heavy θ = 90% + table | bound mem / it |
|---:|---|---|---:|---:|---:|---|---:|---:|---:|---:|---:|
| 16 | BML | without huge pages | 19.25 | 8.38 | 4.97 | heavy08+table | 3.10 | 2.70 | 1.60 | 3.12 | 1.59 |
| 16 | BML | with huge pages | 15.99 | 5.71 | 4.83 | heavy08+table | 2.75 | 2.08 | 1.76 | 2.75 | 1.76 |
| 16 | FB | without huge pages | 18.79 | 8.10 | 4.83 | heavy08+table | 3.09 | 2.62 | 1.56 | 3.10 | 1.56 |
| 16 | FB | with huge pages | 15.62 | 5.55 | 4.70 | heavy08+table | 2.73 | 2.03 | 1.72 | 2.74 | 1.72 |
| 32 | BML | without huge pages | 21.37 | 9.30 | 5.47 | heavy08+table | 3.01 | 3.09 | 1.82 | 3.03 | 1.81 |
| 32 | BML | with huge pages | 17.70 | 6.28 | 5.30 | heavy08+table | 2.65 | 2.36 | 1.99 | 2.68 | 1.98 |
| 32 | FB | without huge pages | 21.07 | 9.18 | 5.39 | heavy08+table | 2.95 | 3.11 | 1.83 | 2.98 | 1.81 |
| 32 | FB | with huge pages | 17.45 | 6.17 | 5.21 | heavy08+table | 2.62 | 2.36 | 1.99 | 2.63 | 1.98 |
| 64 | BML | without huge pages | 22.98 | 10.02 | 5.84 | heavy08+table | 3.08 | 3.26 | 1.90 | 3.10 | 1.89 |
| 64 | BML | with huge pages | 18.98 | 6.74 | 5.67 | heavy08+table | 2.69 | 2.50 | 2.11 | 2.75 | 2.06 |
| 64 | FB | without huge pages | 22.74 | 9.91 | 5.77 | heavy08+table | 2.97 | 3.34 | 1.94 | 3.03 | 1.90 |
| 64 | FB | with huge pages | 18.77 | 6.64 | 5.59 | heavy08+table | 2.63 | 2.53 | 2.13 | 2.68 | 2.09 |
