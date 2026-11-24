# Corrected MB for the rows of the posted paper built on the plain `c<N>.lvl` (stage 2, part C0)

*Commit hashes in this README refer to the private development history of this code; those commits are not in this repository.*

MB = 10^6 bytes, 16 / 32 / 64 individuals. "Published" is recomputed by the paper's rule from the same files and agrees, to within 1 MB of rounding in the first-run tables (which the logs give to 1 kB), with the paper. "Corrected" is the same sum with `c<N>.lvl` replaced by the file without its pairs (a real file, written by `bin/strip-pairs`; searches on it give byte-identical output), and for rows with the table, without the pairs' first-run table.

| table | row | published | corrected | difference |
|---|---|---|---|---|
| Table 2 (tab:twolevel) | RLCSA | 979 / 1,660 / 1,873 | 937 / 1,605 / 1,794 | −42 (4.3%) / −56 (3.3%) / −79 (4.2%) |
| Table 2 (tab:twolevel) | move structure | 1,068 / 1,183 / 1,362 | 1,026 / 1,128 / 1,283 | −42 (3.9%) / −56 (4.7%) / −79 (5.8%) |
| tab:fun | two-level | 1,068 / 1,183 / 1,362 | 1,026 / 1,128 / 1,283 | −42 (3.9%) / −56 (4.7%) / −79 (5.8%) |
| tab:fun | almost-funnels, g = 4 | 1,087 / 1,206 / 1,391 | 1,044 / 1,150 / 1,312 | −42 (3.9%) / −56 (4.6%) / −79 (5.7%) |
| tab:fun | almost-funnels, g = 16 | 1,075 / 1,192 / 1,375 | 1,033 / 1,137 / 1,295 | −42 (3.9%) / −56 (4.7%) / −79 (5.8%) |
| tab:fun | almost-funnels, g = 64 | 1,071 / 1,187 / 1,367 | 1,029 / 1,132 / 1,288 | −42 (3.9%) / −56 (4.7%) / −79 (5.8%) |
| tab:heavy | two-level | 1,068 / 1,183 / 1,362 | 1,026 / 1,128 / 1,283 | −42 (3.9%) / −56 (4.7%) / −79 (5.8%) |
| tab:heavy | θ = 80% | 1,217 / 1,356 / 1,574 | 1,175 / 1,300 / 1,495 | −42 (3.5%) / −56 (4.1%) / −79 (5.0%) |
| tab:heavy | θ = 90% | 1,207 / 1,339 / 1,545 | 1,165 / 1,283 / 1,466 | −42 (3.5%) / −56 (4.2%) / −79 (5.1%) |
| tab:heavy | θ = 95% | 1,199 / 1,330 / 1,531 | 1,157 / 1,275 / 1,452 | −42 (3.5%) / −56 (4.2%) / −79 (5.2%) |
| tab:encodings | two-level | 1,068 / 1,183 / 1,362 | 1,026 / 1,128 / 1,283 | −42 (3.9%) / −56 (4.7%) / −79 (5.8%) |
| tab:latency | two-level | 1,068 / 1,183 / 1,362 | 1,026 / 1,128 / 1,283 | −42 (3.9%) / −56 (4.7%) / −79 (5.8%) |
| tab:latency | with the table | 1,146 / 1,274 / 1,471 | 1,094 / 1,204 / 1,371 | −52 (4.5%) / −70 (5.5%) / −100 (6.8%) |
| tab:latency | with the table and lookahead | 1,146 / 1,274 / 1,471 | 1,094 / 1,204 / 1,371 | −52 (4.5%) / −70 (5.5%) / −100 (6.8%) |
| tab:latency | with lookahead alone | 1,068 / 1,183 / 1,362 | 1,026 / 1,128 / 1,283 | −42 (3.9%) / −56 (4.7%) / −79 (5.8%) |
| tab:latency | heavy paths, θ = 80% | 1,217 / 1,356 / 1,574 | 1,175 / 1,300 / 1,495 | −42 (3.5%) / −56 (4.1%) / −79 (5.0%) |
| tab:latency | heavy paths, θ = 80%, with the table | 1,295 / 1,446 / 1,683 | 1,243 / 1,377 / 1,583 | −52 (4.0%) / −70 (4.8%) / −100 (5.9%) |
| tab:latency | heavy paths, θ = 90% | 1,207 / 1,339 / 1,545 | 1,165 / 1,283 / 1,466 | −42 (3.5%) / −56 (4.2%) / −79 (5.1%) |
| tab:latency | heavy paths, θ = 90%, with the table | 1,285 / 1,429 / 1,654 | 1,233 / 1,360 / 1,554 | −52 (4.0%) / −70 (4.9%) / −100 (6.0%) |
| tab:latency | heavy paths, θ = 95% | 1,199 / 1,330 / 1,531 | 1,157 / 1,275 / 1,452 | −42 (3.5%) / −56 (4.2%) / −79 (5.2%) |
| tab:latency | heavy paths, θ = 95%, with the table | 1,277 / 1,421 / 1,640 | 1,225 / 1,351 / 1,540 | −52 (4.1%) / −70 (4.9%) / −100 (6.1%) |
| tab:summary | two-level (move), plain | 1,068 / 1,183 / 1,362 | 1,026 / 1,128 / 1,283 | −42 (3.9%) / −56 (4.7%) / −79 (5.8%) |
| tab:summary | table | 1,146 / 1,274 / 1,471 | 1,094 / 1,204 / 1,371 | −52 (4.5%) / −70 (5.5%) / −100 (6.8%) |
| tab:summary | table and lookahead | 1,146 / 1,274 / 1,471 | 1,094 / 1,204 / 1,371 | −52 (4.5%) / −70 (5.5%) / −100 (6.8%) |
| tab:summary | lookahead alone | 1,068 / 1,183 / 1,362 | 1,026 / 1,128 / 1,283 | −42 (3.9%) / −56 (4.7%) / −79 (5.8%) |
| tab:summary | STPD of the parse | 2,660 / 4,273 / 6,303 | 2,618 / 4,218 / 6,224 | −42 (1.6%) / −56 (1.3%) / −79 (1.3%) |
| tab:summary | funnels, g = 64 | 1,071 / 1,187 / 1,367 | 1,029 / 1,132 / 1,288 | −42 (3.9%) / −56 (4.7%) / −79 (5.8%) |
| tab:summary | funnels, g = 16 | 1,075 / 1,192 / 1,375 | 1,033 / 1,137 / 1,295 | −42 (3.9%) / −56 (4.7%) / −79 (5.8%) |
| tab:summary | funnels, g = 4 | 1,087 / 1,206 / 1,391 | 1,044 / 1,150 / 1,312 | −42 (3.9%) / −56 (4.6%) / −79 (5.7%) |
| tab:summary | heavy paths, θ = 95% | 1,199 / 1,330 / 1,531 | 1,157 / 1,275 / 1,452 | −42 (3.5%) / −56 (4.2%) / −79 (5.2%) |
| tab:summary | heavy paths, θ = 95%, with the table | 1,277 / 1,421 / 1,640 | 1,225 / 1,351 / 1,540 | −52 (4.1%) / −70 (4.9%) / −100 (6.1%) |
| tab:summary | heavy paths, θ = 90% | 1,207 / 1,339 / 1,545 | 1,165 / 1,283 / 1,466 | −42 (3.5%) / −56 (4.2%) / −79 (5.1%) |
| tab:summary | heavy paths, θ = 90%, with the table | 1,285 / 1,429 / 1,654 | 1,233 / 1,360 / 1,554 | −52 (4.0%) / −70 (4.9%) / −100 (6.0%) |
| tab:summary | heavy paths, θ = 80% | 1,217 / 1,356 / 1,574 | 1,175 / 1,300 / 1,495 | −42 (3.5%) / −56 (4.1%) / −79 (5.0%) |
| tab:summary | heavy paths, θ = 80%, with the table | 1,295 / 1,446 / 1,683 | 1,243 / 1,377 / 1,583 | −52 (4.0%) / −70 (4.8%) / −100 (5.9%) |

The ▷ rows (tab:encodings "with encoding", tab:latency and tab:summary's encoding rows) read their pairs: unchanged. The rows without the parse (RLCSA, the move structure, almost-funnels on the text, ropebwt3, Movi 2, the STPD-index): unchanged.
