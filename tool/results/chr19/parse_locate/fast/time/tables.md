## Without locating against the paper's (huge pages), BML / FB, s

| row | 16: this session (spread) / paper | 32: this session (spread) / paper | 64: this session (spread) / paper |
|---|---|---|---|
| two-level | 5.04 (4.0%) / 5.00; 5.55 (1.5%) / 5.54 | 5.47 (1.8%) / 5.49; 6.01 (1.9%) / 5.92 | 6.14 (1.4%) / 6.20; 6.64 (2.8%) / 6.66 |
| with the table and lookahead | 3.44 (1.6%) / 3.40; 3.73 (0.8%) / 3.71 | 3.69 (0.6%) / 3.63; 3.99 (0.7%) / 3.95 | 4.01 (0.3%) / 4.00; 4.35 (1.1%) / 4.33 |
| ▷, jump, tables and lookahead | 2.51 (1.0%) / 2.50; 2.67 (1.5%) / 2.67 | 2.68 (0.3%) / 2.67; 2.86 (0.5%) / 2.86 | 2.93 (0.6%) / 2.94; 3.14 (1.2%) / 3.15 |
| heavy paths θ = 80%, with the table | 2.26 (2.5%) / 2.26; 2.37 (0.3%) / 2.37 | 2.28 (1.5%) / 2.25; 2.38 (0.6%) / 2.38 | 2.32 (0.5%) / 2.32; 2.46 (0.2%) / 2.47 |

## Added time, s = 64 (median without → with: added), s

| row | alg | 16 | 32 | 64 |
|---|---|---|---|---|
| two-level | BML | 5.036 → 5.225: **+0.189** | 5.470 → 5.652: **+0.182** | 6.144 → 6.320: **+0.176** |
| two-level | FB | 5.553 → 5.747: **+0.194** | 6.015 → 6.100: **+0.085** | 6.638 → 6.802: **+0.164** |
| with the table and lookahead | BML | 3.440 → 3.569: **+0.129** | 3.685 → 3.797: **+0.112** | 4.012 → 4.189: **+0.177** |
| with the table and lookahead | FB | 3.726 → 3.912: **+0.186** | 3.992 → 4.117: **+0.125** | 4.350 → 4.468: **+0.118** |
| ▷, jump, tables and lookahead | BML | 2.509 → 2.643: **+0.134** | 2.678 → 2.789: **+0.111** | 2.933 → 3.058: **+0.125** |
| ▷, jump, tables and lookahead | FB | 2.666 → 2.803: **+0.137** | 2.856 → 2.981: **+0.125** | 3.143 → 3.281: **+0.138** |
| heavy paths θ = 80%, with the table | BML | 2.261 → 2.402: **+0.141** | 2.283 → 2.379: **+0.096** | 2.323 → 2.456: **+0.133** |
| heavy paths θ = 80%, with the table | FB | 2.371 → 2.530: **+0.159** | 2.384 → 2.525: **+0.141** | 2.458 → 2.602: **+0.144** |

## Configurations on the rows tl and tlh08+table: added time against off (median), s

| row | configuration | alg | 16 | 32 | 64 |
|---|---|---|---|---|---|
| two-level | 8912148 (bin/katka-ploc0): one row at a time, after each MEM | BML | +0.413 (spread 1.6%) | +0.362 (spread 1.8%) | +0.342 (spread 0.8%) |
| two-level | 8912148 (bin/katka-ploc0): one row at a time, after each MEM | FB | +0.410 (spread 0.9%) | +0.352 (spread 3.9%) | +0.338 (spread 1.3%) |
| two-level | one row at a time, per segment | BML | +0.365 (spread 1.8%) | +0.332 (spread 2.1%) | +0.380 (spread 0.6%) |
| two-level | one row at a time, per segment | FB | +0.414 (spread 1.1%) | +0.319 (spread 1.2%) | +0.362 (spread 0.4%) |
| two-level | 12 staged lanes, a row at a time | BML | +0.264 (spread 1.0%) | +0.232 (spread 1.4%) | +0.270 (spread 1.3%) |
| two-level | 12 staged lanes, a row at a time | FB | +0.207 (spread 2.3%) | +0.176 (spread 1.8%) | +0.278 (spread 1.8%) |
| two-level | 12 staged lanes, a run of rows at a time (final), s = 64 | BML | +0.189 (spread 2.4%) | +0.182 (spread 1.6%) | +0.176 (spread 0.3%) |
| two-level | 12 staged lanes, a run of rows at a time (final), s = 64 | FB | +0.194 (spread 4.6%) | +0.085 (spread 1.0%) | +0.164 (spread 2.5%) |
| two-level | final, s = 32 | BML | +0.067 (spread 0.6%) | +0.052 (spread 1.9%) | +0.060 (spread 1.5%) |
| two-level | final, s = 32 | FB | +0.112 (spread 0.9%) | +0.022 (spread 1.4%) | +0.104 (spread 0.1%) |
| two-level | final, s = 16 | BML | +0.072 (spread 1.5%) | +0.052 (spread 1.6%) | +0.047 (spread 1.9%) |
| two-level | final, s = 16 | FB | +0.058 (spread 2.9%) | +0.057 (spread 0.7%) | +0.070 (spread 0.4%) |
| two-level | rows collected, not walked | BML | -0.009 (spread 0.8%) | +0.070 (spread 2.4%) | -0.001 (spread 0.2%) |
| two-level | rows collected, not walked | FB | +0.049 (spread 0.7%) | +0.003 (spread 0.2%) | +0.027 (spread 1.0%) |
| heavy paths θ = 80%, with the table | 12 staged lanes, a run of rows at a time (final), s = 64 | BML | +0.141 (spread 0.0%) | +0.096 (spread 1.0%) | +0.133 (spread 0.8%) |
| heavy paths θ = 80%, with the table | 12 staged lanes, a run of rows at a time (final), s = 64 | FB | +0.159 (spread 0.9%) | +0.141 (spread 0.0%) | +0.144 (spread 0.3%) |
| heavy paths θ = 80%, with the table | final, s = 32 | BML | +0.084 (spread 0.3%) | +0.037 (spread 0.3%) | +0.072 (spread 0.1%) |
| heavy paths θ = 80%, with the table | final, s = 32 | FB | +0.086 (spread 0.2%) | +0.087 (spread 0.6%) | +0.085 (spread 0.2%) |
| heavy paths θ = 80%, with the table | final, s = 16 | BML | +0.071 (spread 0.7%) | +0.015 (spread 0.4%) | +0.050 (spread 0.6%) |
| heavy paths θ = 80%, with the table | final, s = 16 | FB | +0.059 (spread 1.5%) | +0.082 (spread 1.0%) | +0.066 (spread 0.9%) |

## Peak resident size (MB, largest of the rounds)

| row | configuration | 16 | 32 | 64 |
|---|---|---|---|---|
| two-level | off | 674 | 728 | 826 |
| two-level | on | 702 | 774 | 907 |
| two-level | s32 | 718 | 807 | 975 |
| two-level | s16 | 750 | 870 | 1105 |
| with the table and lookahead | off | 739 | 801 | 910 |
| with the table and lookahead | on | 767 | 846 | 991 |
| ▷, jump, tables and lookahead | off | 792 | 871 | 1009 |
| ▷, jump, tables and lookahead | on | 818 | 916 | 1091 |
| heavy paths θ = 80%, with the table | off | 932 | 1018 | 1165 |
| heavy paths θ = 80%, with the table | on | 989 | 1091 | 1274 |
| heavy paths θ = 80%, with the table | s32 | 1005 | 1125 | 1342 |
| heavy paths θ = 80%, with the table | s16 | 1037 | 1187 | 1473 |

Largest spread of any configuration: 4.6%
