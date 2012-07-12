// Copyright 2012 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.

// Generate a triangular plan, starting from current position and course
// Usage: plug -o -f imu /var/run/helmsman | ./gentriangleplan > /var/run/plan.txt
package main

import (
	"bufio"
	"flag"
	"fmt"
	"log"
	"math"
	"os"
	"strconv"
	"strings"

	"code.google.com/p/avalonsailing/trunk/go/wgs84"
)

func rad(deg float64) float64 { return deg * (math.Pi / 180.0) }
func deg(rad float64) float64 { return rad * (180.0 / math.Pi) }

var (
	left = flag.Bool("l", false, "make left turn (default rigth)")
	dist = flag.Float64("d", 500, "distance along each edge")
)

func main() {

	flag.Parse()

	// read stdin for first imu: line, extract lat/long/yaw
	r := bufio.NewReader(os.Stdin)
	var parts []string
	for {
		line, err := r.ReadString('\n')
		if err != nil {
			log.Fatal(err)
		}
		parts = strings.Split(line, " ")
		if parts[0] == "imu:" {
			break
		}
	}
	var yaw, lat, lng float64 = math.NaN(), math.NaN(), math.NaN()
	for _, p := range parts {
		kv := strings.SplitN(p, ":", 2)
		if len(kv) != 2 {
			continue
		}
		val, err := strconv.ParseFloat(kv[1], 64)
		if err != nil {
			continue
		}
		if kv[0] == "yaw_deg" {
			yaw = val
		}
		if kv[0] == "lat_deg" {
			lat = val
		}
		if kv[0] == "lng_deg" {
			lng = val
		}
	}

	if math.IsNaN(yaw) || math.IsNaN(lat) || math.IsNaN(lng) {
		log.Fatal("could not parse imu line:", parts)
	}

	turn := -rad(60)
	if *left {
		turn = -turn
	}

	lat2, lng2, azi2 := wgs84.Forward(rad(lat), rad(lng), rad(yaw), *dist)
	lat3, lng3, _ := wgs84.Forward(lat2, lng2, azi2+turn, *dist)
	fmt.Printf("%.7f,%.7f\n", lat, lng)
	fmt.Printf("%.7f,%.7f\n", deg(lat2), deg(lng2))
	fmt.Printf("%.7f,%.7f\n", deg(lat3), deg(lng3))
}
