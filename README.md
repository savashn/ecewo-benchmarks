## Test

- Step 1: Ramp up to 100 virtual users over 10 seconds
- Step 2: Stay at 100 users for 40 seconds
- Step 3: Ramp down to 0 users over 10 seconds

## Hardware
CPU: 12th Gen Intel(R) Core(TM) i7-12700F, 2100 Mhz, 12 Core Processor

## Request Per Second

Higher is better.

| Framework  | Total Request | RPS   | Total Data Received | Average Receive Rate |
|------------|---------------|-------|---------------------|----------------------|
| Ecewo      | 5010          | 82.5  | 777 kB              | 13 kB/s              |
| Axum       | 5007          | 82.4  | 671 kB              | 11 kB/s              |
| Go         | 5007          | 82.4  | 681 kB              | 11 kB/s              |
| Express.js | 5000          | 82.3  | 1.3 MB              | 22 kB/s              |

## HTTP Request Duration

Lower is better.

| Framework  | Average   | Median   | Max     | P90      | P95     |
|------------|-----------|----------|---------|----------|---------|
| Ecewo      | 0.387ms   | 0.152ms  | 7.23ms  | 0.99ms   | 1.09ms  |
| Axum       | 0.442ms   | 0.505ms  | 5.61ms  | 1.01ms   | 1.21ms  |
| Go         | 0.958ms   | 0.725ms  | 12.62ms | 1.97ms   | 2.48ms  |
| Express.js | 1.85ms    | 1.58ms   | 11.05ms | 3.48ms   | 4.27ms  |

## HTTP Request Waiting Time (Server Processing Time)

Lower is better.

| Framework  | Average   | Median   | Max     | P90      | P95     |
|------------|-----------|----------|---------|----------|---------|
| Ecewo      | 0.313ms   | 0s       | 7.23ms  | 0.98ms   | 1.02ms  |
| Axum       | 0.362ms   | 0.089ms  | 4.59ms  | 1.00ms   | 1.04ms  |
| Go         | 0.891ms   | 0.698ms  | 12.62ms | 1.80ms   | 2.34ms  |
| Express.js | 1.78ms    | 1.52ms   | 11.05ms | 3.39ms   | 4.12ms  |

## Run Benchmark

Run `k6 run benchmark.js` in the terminal
