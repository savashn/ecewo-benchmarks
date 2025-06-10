### Compile and Run Go

```shell
go build -ldflags="-s -w" -gcflags="-B -N" -o server.exe main.go
./server.exe
```