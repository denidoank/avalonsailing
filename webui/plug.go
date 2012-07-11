// Copyright 2012 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
//
// Avalon webui server.

package main

import (
	"bufio"
	"bytes"
	"fmt"
	"log"
	"net"
	"strings"
	"time"
)

// Client for the linebusd
func Plug(path_to_bus string) (rch <-chan string, sch chan<- string) {
	adr, err := net.ResolveUnixAddr("unix", path_to_bus)
	if err != nil {
		log.Print(err)
		return
	}

	sck, err := net.DialUnix("unix", nil, adr)
	if err != nil {
		log.Print(err)
		return
	}

	snd, rcv := make(chan string), make(chan string)

	go func() {
		r := bufio.NewReader(sck)
		for {
			line, err := r.ReadString('\n')
			if err != nil {
				break
			}
			rcv <- line
		}
		close(rcv)
	}()

	go func() {
		w := bufio.NewWriter(sck)
		for line := range snd {
			w.WriteString(line)
			w.WriteByte('\n')
			w.Flush()
		}
		sck.Close()
	}()

	return rcv, snd
}

type TokenBucket struct {
	Size int       // burst size
	Rate float64   // max per second
	val  int       // current value
	last time.Time // time of last 
}

func (tb *TokenBucket) Allow(now time.Time) bool {
	if tb.last.After(now) {
		tb.last = now
	}
	tb.val += int(now.Sub(tb.last).Seconds() * tb.Rate)
	if tb.val > tb.Size {
		tb.val = tb.Size
	}

	if tb.val <= 0 {
		return false
	}

	tb.val--
	tb.last = now
	return true
}

func findpfx(s string) int {
	for i, c := range s {
		if c == ':' {
			return i
		}
	}
	return 0
}

var zero time.Time

// ratelimit lines per prefix to hz
// prefix is line[0:i] where line[i] == ':' for the first i.
func SamplePlug(in <-chan string, hz float64) <-chan string {
	out := make(chan string)
	go func() {
		m := make(map[string]*TokenBucket)
		for line := range in {
			pfx := line[:findpfx(line)]
			tb, ok := m[pfx]
			if !ok {
				tb = &TokenBucket{2, hz, 2, zero}
				m[pfx] = tb
			}
			if tb.Allow(time.Now()) {
				out <- line
			}
		}
		close(out)
	}()
	return out
}

// turn lbus messages into json.  only valid json if all field values are numeric!
func line2json(s string) string {
	var buf bytes.Buffer
	i := findpfx(s)
	fmt.Fprintf(&buf, "{\"%s\": {", s[0:i])
	for _, f := range strings.Fields(s[i+1:]) {
		parts := strings.SplitN(f, ":", 2)
		if len(parts) == 2 {
			fmt.Fprintf(&buf, " \"%s\":%s,", parts[0], parts[1])
		}
	}
	buf.WriteString(" }}")
	return buf.String()
}

/*
func main() {
	r, s := Plug(os.Args[1])

	rr := SamplePlug(r, 0.25)

	go func() {
		for l := range rr {
			fmt.Println(line2json(l))
		}
		fmt.Println("done")
	}()
	
	i := 0
	for t := range time.Tick(3 * time.Second) {
		s <- fmt.Sprintf("time: %s", t)
		i++
		if i == 5 {
			close(s)
			break
		}
	}
	select{}
}
*/
