| row | algorithm | mode | cycles | instructions | IPC | stalled, any reason | on any memory access | on an L3 miss | L3-miss loads | page walks |
|---|---|---|---:|---:|---:|---:|---:|---:|---:|---:|
| unchanged | BML | without | 8.738e+10 | 3.331e+10 | 0.38 | 76.4% | 74.2% | 56.5% | 1.448e+08 | 18.2% |
| unchanged | BML | with | 7.261e+10 | 3.331e+10 | 0.46 | 72.3% | 69.8% | 64.3% | 1.430e+08 | 0.1% |
| unchanged | FB | without | 8.525e+10 | 3.235e+10 | 0.38 | 76.5% | 74.3% | 56.9% | 1.413e+08 | 17.9% |
| unchanged | FB | with | 7.087e+10 | 3.235e+10 | 0.46 | 72.5% | 69.9% | 64.5% | 1.349e+08 | 0.1% |
| interleaved, K = 4 | BML | without | 4.015e+10 | 3.877e+10 | 0.97 | 50.5% | 46.5% | 7.5% | 7.100e+06 | 40.6% |
| interleaved, K = 4 | BML | with | 3.198e+10 | 3.877e+10 | 1.21 | 40.6% | 35.8% | 8.3% | 7.515e+06 | 0.2% |
| interleaved, K = 4 | FB | without | 3.860e+10 | 3.757e+10 | 0.97 | 50.8% | 46.4% | 8.5% | 8.095e+06 | 38.4% |
| interleaved, K = 4 | FB | with | 3.215e+10 | 3.757e+10 | 1.17 | 43.0% | 38.2% | 9.2% | 8.053e+06 | 0.3% |
| interleaved, K = 8 | BML | without | 4.229e+10 | 3.875e+10 | 0.92 | 51.9% | 47.8% | 10.0% | 9.387e+06 | 37.5% |
| interleaved, K = 8 | BML | with | 2.890e+10 | 3.875e+10 | 1.34 | 37.6% | 32.1% | 11.6% | 1.013e+07 | 0.2% |
| interleaved, K = 8 | FB | without | 4.120e+10 | 3.755e+10 | 0.91 | 52.0% | 47.8% | 10.4% | 9.843e+06 | 37.3% |
| interleaved, K = 8 | FB | with | 2.799e+10 | 3.755e+10 | 1.34 | 36.6% | 31.0% | 12.2% | 1.060e+07 | 0.3% |
