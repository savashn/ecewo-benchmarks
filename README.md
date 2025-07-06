## Hardware
CPU: 12th Gen Intel(R) Core(TM) i7-12700F, 2100 Mhz, 12 Core Processor

## Request Per Second

| Framework | Total Request | RPS   | Total Data Received | Average Receive Rate |
|-----------|---------------|-------|---------------------|----------------------|
| Ecewo     | 11,961        | 66.18 | 1.4 MB              | 7.8 kB/s             |
| Axum      | 11,957        | 66.13 | 1.6 MB              | 8.9 kB/s             |
| Go        | 11,956        | 66.09 | 1.6 MB              | 9.0 kB/s             |
| Hono      | 11,957        | 66.11 | 2.2 MB              | 12 kB/s              |
| Fastify   | 11,952        | 66.05 | 2.4 MB              | 13 kB/s              |
| Express   | 11,957        | 66.13 | 3.1 MB              | 17 kB/s              |

## HTTP Request Duration

| Framework | Average   | Median   | Max     | P90     | P95      |
|-----------|-----------|----------|---------|---------|----------|
| Ecewo     | 149.88µs  | 0s       | 4ms     | 510µs   | 582.5µs  |
| Axum      | 162.72µs  | 0s       | 10.51ms | 510.9µs | 576.01µs |
| Go        | 448.97µs  | 511µs    | 4.64ms  | 788.2µs | 1ms      |
| Hono      | 429.67µs  | 518.1µs  | 5.19ms  | 965.5µs | 1.06ms   |
| Fastify   | 567.14µs  | 527.59µs | 5.5ms   | 1.07ms  | 1.23ms   |
| Express   | 1.14ms    | 1.05ms   | 14.75ms | 2ms     | 2.35ms   |

## HTTP Request Waiting Time (Server Processing Time)

| Framework | Average   | Median   | Max     | P90      | P95      |
|-----------|-----------|----------|---------|----------|----------|
| Ecewo     | 120.11µs  | 0s       | 3.2ms   | 508.4µs  | 537.69µs |
| Axum      | 129.74µs  | 0s       | 10.51ms | 508.8µs  | 538.5µs  |
| Go        | 419.93µs  | 509.49µs | 4.64ms  | 764.59µs | 1ms      |
| Hono      | 397.62µs  | 509µs    | 5.03ms  | 875µs    | 1.04ms   |
| Fastify   | 527.96µs  | 523.59µs | 5.5ms   | 1.05ms   | 1.18ms   |
| Express   | 1.11ms    | 1.05ms   | 14.75ms | 1.99ms   | 2.31ms   |

## Run Benchmark
Run `k6 run benchmark.js` in the terminal
