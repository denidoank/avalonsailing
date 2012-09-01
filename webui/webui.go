// Copyright 2012 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
//
// Avalon webui server.

package main

import (
	"bufio"
	"code.google.com/p/go.net/websocket"
	"fmt"
	"flag"
	"log"
	"net/http"
)

var (
	webroot = flag.String("webroot", "./s", "path to dir with static webpages")
	lbus = flag.String("lbus", "/var/run/lbus", "path to lbus socket")
	slog = flag.String("log", "/var/log/messages", "path to syslog output")
	port = flag.String("http", ":1969", "port to serve http on")
)

func PlugServer(ws *websocket.Conn) {
	r, s := Plug(*lbus)
	if r == nil || s == nil {
		ws.Close()
		return
	}
	log.Print("plugged into ", *lbus)
	rr := SamplePlug(r, 2)
	go func() {
		for l := range rr {
			fmt.Fprint(ws, line2json(l))
		}
		log.Print("plug pulled from ", *lbus)
		ws.Close()
	}()

	b := bufio.NewReader(ws)
	for {
		line, err := b.ReadString('\n')
		if err != nil {
			break
		}
		s <- line
	}
	log.Print("websocket closed ", *lbus)
	close(s)
}

func TailServer(ws *websocket.Conn) {
	stop := Tail(*slog, ws)
	b := bufio.NewReader(ws)
	b.ReadString('\n')
	close(stop)
}

func main() {
	flag.Parse()
	http.Handle("/", http.RedirectHandler("/s/control.html", 301))
	http.Handle("/s/", http.StripPrefix("/s/", http.FileServer(http.Dir(*webroot))))
	http.Handle("/lbus", websocket.Handler(PlugServer))
	http.Handle("/syslog", websocket.Handler(TailServer))
	if err := http.ListenAndServe(*port, nil); err != nil {
		log.Fatal(err)
	}
	select {}
}
