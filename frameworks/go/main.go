package main

import (
	"encoding/json"
	"flag"
	"fmt"
	"net"
	"net/http"
	"runtime"
)

type Response struct {
	Message string `json:"message"`
}

func handler(w http.ResponseWriter, r *http.Request) {
	w.Header().Set("Content-Type", "application/json")
	resp := Response{Message: "Hello, World!"}
	if err := json.NewEncoder(w).Encode(resp); err != nil {
		http.Error(w, err.Error(), http.StatusInternalServerError)
	}
}

func main() {
	bindHost := flag.String("bind", ":3000", "set bind host")
	prefork := flag.Bool("prefork", false, "use prefork")
	child := flag.Bool("child", false, "is child proc")
	flag.Parse()

	var listener net.Listener
	if *prefork {
		listener = doPrefork(*child, *bindHost)
	} else {
		runtime.GOMAXPROCS(runtime.NumCPU())
	}

	http.HandleFunc("/", handler)

	if *prefork {
		fmt.Printf("Server running on http://localhost%s (prefork mode with %d workers)\n", *bindHost, runtime.NumCPU())
		http.Serve(listener, nil)
	} else {
		fmt.Printf("Server running on http://localhost%s\n", *bindHost)
		http.ListenAndServe(*bindHost, nil)
	}
}
