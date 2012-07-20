// tool to help analyze lbus logs: read lbus log on stdin
// print fields listed on the commandline in format src:fld
// if multiple sources are listed, for each source line all 
// last values of the other sources are printed
// for every line with a timestamp_ms field, a field 'timestamp_s' is 
// synthesized with the time in seconds since the first log entry.
package main

import (
	"bufio"
	"fmt"
	"io"
	"math"
	"log"
	"os"
	"strconv"
	"strings"
)

var firsttimestamp_ms float64 = math.NaN()

func parseLbusLine(line string,  vals map[string]map[string]float64) string {
	srcval := strings.SplitN(line, ":", 2)
	if len(srcval) != 2 {
		return ""
	}
	parts := strings.Split(srcval[1], " ")
	if len(parts) == 0 {
		return ""
	}
	
	m, ok := vals[srcval[0]]
	if !ok {
		m = make(map[string]float64, len(parts))
		vals[srcval[0]] = m
	}

	for _, p := range parts {
		kv := strings.SplitN(p, ":", 2)
		if len(kv) != 2 {
			continue
		}
		v, err := strconv.ParseFloat(kv[1], 64)
		if err != nil {
			log.Print(err)
			return ""
		}
		if strings.HasSuffix(kv[0], "_deg") {
			for v < 180  { v += 360 }
			for v >= 180 { v -= 360 }
		}

		m[kv[0]] = v

		if kv[0] == "timestamp_ms" {
			if math.IsNaN(firsttimestamp_ms) {
				firsttimestamp_ms = v
			}

			m["timestamp_s"] = (v - firsttimestamp_ms) * 0.001
		}
	}
	return srcval[0]
}



func main() {
	srcs := make(map[string]bool)
	var flds [][]string
	for _, f := range os.Args[1:] {
		srcfld := strings.SplitN(f, ":", 2)
		if len(srcfld) != 2 {
			log.Fatal("can't parse ", f, " as src:fld")
		}
		srcs[srcfld[0]] = true
		flds = append(flds, srcfld)
	}
	
	if flds == nil {
		log.Fatal("usage: loga src:fld ...")
	}

	vals := make(map[string]map[string]float64)
	
	r := bufio.NewReader(os.Stdin)
	for l := 0; ; l++ {
		line, _, err := r.ReadLine()
		if err == io.EOF {
			break
		}
		if err != nil {
			log.Fatal(err)
		}

		src := parseLbusLine(string(line), vals)
		if !srcs[src] {
			continue
		}
		
		for _, f := range flds {
			v := math.NaN()
			r := vals[f[0]]
			if r != nil {
				v = r[f[1]]
			}
			fmt.Printf("%f ", v)
		}
		fmt.Printf("\n")
	}
}