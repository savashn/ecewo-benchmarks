## Test

- Step 1: Ramp up to 1000 virtual users over 2 minute
- Step 2: Stay at 1000 users for 2 minute
- Step 3: Ramp down to 0 users over 2 minute

## Hardware
CPU: 12th Gen Intel(R) Core(TM) i7-12700F, 2100 Mhz, 12 Core Processor

## Request Per Second

| Framework | Total Request | RPS   | Total Data Received | Average Receive Rate |
|-----------|---------------|-------|---------------------|----------------------|
| Ecewo     | 238,833       | 661.9 | 28 MB               | 78 kB/s              |
| Axum      | 238,790       | 661.8 | 32 MB               | 89 kB/s              |
| Go        | 238,723       | 661.4 | 33 MB               | 90 kB/s              |
| Hono      | 238,765       | 661.7 | 43 MB               | 120 kB/s             |
| Express   | 238,710       | 661.5 | 62 MB               | 173 kB/s             |

## HTTP Request Duration

| Framework | Average   | Median   | Max     | P90      | P95     |
|-----------|-----------|----------|---------|----------|---------|
| Ecewo     | 166.24µs  | 0s       | 19.52ms | 545.29µs | 922µs   |
| Axum      | 185.67µs  | 0s       | 35.9ms  | 549.79µs | 971.5µs |
| Go        | 720.42µs  | 598.19µs | 22.15ms | 1.46ms   | 1.93ms  |
| Hono      | 393.05µs  | 341.8µs  | 23.08ms | 1ms      | 1.12ms  |
| Express   | 1.38ms    | 1.08ms   | 18.83ms | 2.83ms   | 3.7ms   |

## HTTP Request Waiting Time (Server Processing Time)

| Framework | Average   | Median   | Max     | P90      | P95     |
|-----------|-----------|----------|---------|----------|---------|
| Ecewo     | 143.7µs   | 0s       | 19.52ms | 523.8µs  | 868.5µs |
| Axum      | 157.84µs  | 0s       | 34ms    | 522.4µs  | 889.4µs |
| Go        | 700.04µs  | 586.09µs | 22.15ms | 1.44ms   | 1.91ms  |
| Hono      | 365.81µs  | 134µs    | 21.08ms | 1ms      | 1.09ms  |
| Express   | 1.36ms    | 1.07ms   | 18.83ms | 2.81ms   | 3.67ms  |

## Run Benchmark
Run `k6 run benchmark.js` in the terminal
