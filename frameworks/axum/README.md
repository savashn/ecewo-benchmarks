### Run Benchmark

```shell
cargo build --release
./target/release/server
wrk -t8 -c100 -d40s http://localhost:3000
```
