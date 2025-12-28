### Run Benchmark

```shell
node cluster.js
wrk -t8 -c100 -d40s http://localhost:3000
```