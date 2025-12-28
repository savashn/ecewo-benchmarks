# ecewo benchmarks

This is the benchmark test repo for [ecewo](https://github.com/savashn/ecewo).

### System

**Machine:** 12th Gen Intel Core i7-12700F x 20, 32GB RAM, SSD
**OS:** Fedora Workstation 43
**Method:** `wrk -t8 -c100 -d40s http://localhost:3000` * 2, taking the second results.

### Versions

Node: v22.20.0
Rust: v1.91.1
Go: v1.25.5
GCC: v15.2.1
CMake: v3.31.6

### Results

| Framework | Req/Sec   | Transfer/Sec |
|-----------|-----------|--------------|
| ecewo     | 1,208,226 | 178.60 MB    |
| axum      | 1,192,785 | 168.35 MB    |
| go        | 893,248   | 115.85 MB    |
| express   | 93,214    | 23.20 MB     |
