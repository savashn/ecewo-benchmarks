package main

import (
	"fmt"
	"net/http"
)

var jsonResponse = []byte(`{"message":"Hello World!"}`)

func main() {
	http.HandleFunc("/", func(w http.ResponseWriter, r *http.Request) {
		w.Header().Set("Content-Type", "application/json")
		w.Write(jsonResponse)
	})

	fmt.Println("Server is running at http://localhost:3000")
	err := http.ListenAndServe(":3000", nil)
	if err != nil {
		panic(err)
	}
}

// go build -ldflags="-s -w" -gcflags="-B -N" -o server.exe main.go
// ./server.exe
