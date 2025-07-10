## Hardware
CPU: 12th Gen Intel(R) Core(TM) i7-12700F, 2100 Mhz, 12 Core Processor

## Request Per Second

| Framework | Total Request | RPS   | Total Data Received | Average Receive Rate |
|-----------|---------------|-------|---------------------|----------------------|
| Ecewo     | 11,984        | 66.43 | 1.4 MB              | 7.8 kB/s             |
| Axum      | 11,956        | 66.13 | 1.6 MB              | 8.9 kB/s             |
| Go        | 11,952        | 66.08 | 1.6 MB              | 9.0 kB/s             |
| Hono      | 11,954        | 66.10 | 2.2 MB              | 12 kB/s              |
| Fastify   | 11,957        | 66.08 | 2.4 MB              | 13 kB/s              |
| Express   | 11,951        | 66.05 | 3.1 MB              | 17 kB/s              |

## HTTP Request Duration

| Framework | Average   | Median   | Max     | P90     | P95      |
|-----------|-----------|----------|---------|---------|----------|
| Ecewo     | 71.43µs   | 0s       | 4.97ms  | 503.4µs | 511.1µs  |
| Axum      | 98.35µs   | 0s       | 3.52ms  | 508µs   | 526.9µs  |
| Go        | 366.14µs  | 504.4µs  | 4.51ms  | 726µs   | 862.77µs |
| Hono      | 364.72µs  | 504.9µs  | 4.5ms   | 755.3µs | 1ms      |
| Fastify   | 490.55µs  | 525.5µs  | 5.99ms  | 896.2µs | 1.06ms   |
| Express   | 1.11ms    | 1.05ms   | 5.98ms  | 2ms     | 2.36ms   |

## HTTP Request Waiting Time (Server Processing Time)

| Framework | Average   | Median   | Max     | P90      | P95      |
|-----------|-----------|----------|---------|----------|----------|
| Ecewo     | 51.63µs   | 0s       | 4.97ms  | 138.1µs  | 506.9µs  |
| Axum      | 75.29µs   | 0s       | 3.5ms   | 504.65µs | 510.72µs |
| Go        | 337.63µs  | 383.7µs  | 3.48ms  | 710.3µs  | 836.09µs |
| Hono      | 328.01µs  | 315.55µs | 4.5ms   | 678.41µs | 1ms      |
| Fastify   | 453.91µs  | 521.1µs  | 4.5ms   | 804.7µs  | 1.04ms   |
| Express   | 1.08ms    | 1.04ms   | 5.98ms  | 1.98ms   | 2.34ms   |

## Run Benchmark
Run `k6 run benchmark.js` in the terminal
