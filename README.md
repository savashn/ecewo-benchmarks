## Hardware
12th Gen Intel(R) Core(TM) i7-12700F, 2100 Mhz, 12 Core Processor

## Benchmark Results
| Framework | Latency.Avg | Latency.Max | Request.Total | Request.Req/Sec | Transfer.Total | Transfer.Rate |
|---|---|---|---|---|---|---|
|ecewo|62.53µs|3.89ms|11,987|66.41|1.4 MB|7.8 kB/s|
|axum|87.68µs|5.98ms|11,959|66.15|1.6 MB|8.9 kB/s|
|go|261.69µs|4.38ms|11,961|66.16|1.6 MB|8.9 kB/s|
|hono|426.24µs|6.46ms|11,965|66.21|2.2 MB|12 kB/s|
|fastify|489.87µs|7.62ms|11,964|66.22|2.4 MB|13 kB/s|
|express|1.04ms|7.2ms|11,964|66.18|3.1 MB|17 kB/s

The performance of Ecewo is:

- ~1.4x faster than Axum, but they are almost same.
- ~4.2x faster than Go net/http
- ~6.8x faster than Hono
- ~7.8x faster than Fastify
- ~16.6x faster than Express

## Run Benchmark
Run `k6 run benchmark.js` in the terminal
