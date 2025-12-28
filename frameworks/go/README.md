### Run Benchmark

```shell
go build -ldflags="-s -w" -o server main.go prefork.go
./server -prefork
wrk -t8 -c100 -d40s http://localhost:3000
```