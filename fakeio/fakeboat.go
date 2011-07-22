/*
 *  fakeboat: open 3 server sockets: imud, wind and rudderd, and pretend to be a boat on them
 *
 *  todo, noise and more realism
 */

package main

import (
	"bufio"
	"bytes"
	"flag"
	"fmt"
	"math"
	"net"
	"os"
	"strings"
	"strconv"
	"time"
)

func crash(msg ...interface{}) {
	fmt.Fprintln(os.Stderr, msg...)
	os.Exit(1)
}

var workingDir *string = flag.String("C", "/tmp", "Directory to create sockets in.")

type KeyValPairs map[string]interface{}

func (kvp KeyValPairs) Bytes() []byte {
	var b bytes.Buffer
	for k, v := range kvp {
		if b.Len() > 0 {
			b.WriteRune(' ')
		}
		fmt.Fprint(&b, k, ":", v)
	}
	b.WriteRune('\n')
	return b.Bytes()
}

func (kvp KeyValPairs) Scan(state fmt.ScanState, verb int) os.Error {
	for {
		var kv string
		_, err := fmt.Fscanf(state, "%s", &kv)
		if err == os.EOF {
			break
		}
		if err != nil {
			return err
		}

		parts := strings.SplitN(kv, ":", 2)
		if len(parts) < 2 {
			return fmt.Errorf("no colon:%s", kv)
		}
		val, err := strconv.Atof64(parts[1])
		if err != nil {
			return err
		}
		kvp[parts[0]] = val
	}
	return nil
}

// Like the unix program io/gulp
func gulp(path string) (chan<- KeyValPairs, <-chan string) {
	os.Remove(path)
	lst, err := net.ListenUnix("unix", &net.UnixAddr{path, "unix"})
	if err != nil {
		crash("Error listening on ", path, ": ", err)
	}

	cc := make(chan *net.UnixConn)
	cin := make(chan string)

	go func() {
		for {
			c, err := lst.AcceptUnix()
			if err != nil {
				crash("Error accepting from ", path, ": ", err)
			}
			cc <- c
			go func() {
				var line []byte
				for buf, err := bufio.NewReaderSize(c, 8192); err == nil; line, _, err = buf.ReadLine() {
					if len(line) > 0 {
						cin <- string(line)
					}
				}
				fmt.Fprintln(os.Stderr, "Closing (r) connection", c, ": ", err)
				c.Close()
			}()
		}
	}()

	pipe := make(chan KeyValPairs)

	go func() {

		var conn []*net.UnixConn
		for {
			select {
			case c := <-cc:
				fmt.Fprintln(os.Stderr, "Accepted connection", c)
				conn = append(conn, c)
			case kvp := <-pipe:
				b := kvp.Bytes()
				ii := 0
				for i, c := range conn {
					_, err := c.Write(b)
					if err != nil {
						fmt.Fprintln(os.Stderr, "Closing (w) connection", c, ": ", err)
						c.Close()
						conn[i] = nil
					} else {
						conn[ii] = c
						ii++
					}
				}
				conn = conn[:ii]
			}
		}
	}()

	return pipe, cin
}

func now_ms() int64 { return time.Nanoseconds() / 1E6 }

// Fake Rudderd

const rudderSpeed_deg_ms = float64(0.01) // 10 degrees/second = .01 degrees/millisecond
const sailSpeed_deg_ms = float64(0.02)   // 20 degrees/second = .02 degrees/millisecond

type AngleState struct {
	speed_deg_ms     float64
	time_ms          int64
	angle_deg        float64
	target_angle_deg float64
}

func (rs *AngleState) SetTarget(d float64) { rs.target_angle_deg = d }

func (rs *AngleState) Get(time_ms int64) float64 {
	switch {
	case rs.target_angle_deg < rs.angle_deg:
		rs.angle_deg -= float64(time_ms-rs.time_ms) * rudderSpeed_deg_ms
		if rs.target_angle_deg > rs.angle_deg {
			rs.angle_deg = rs.target_angle_deg
		}
	case rs.target_angle_deg > rs.angle_deg:
		rs.angle_deg += float64(time_ms-rs.time_ms) * rudderSpeed_deg_ms
		if rs.target_angle_deg < rs.angle_deg {
			rs.angle_deg = rs.target_angle_deg
		}
	}
	rs.time_ms = time_ms
	return rs.angle_deg
}

// Fake wind:  3 m/s from compass direction 42.  account for yaw and sail angle
const (
	kWindAngle_deg   = float64(42)
	kWindSpeed_m_s   = float64(3)
	kTurnRadius_m    = float64(25)
	kWindFudgeFactor = float64(.1) // units: 1/m
	kDragFactor_x    = float64(.5)
	kDragFactor_y    = float64(.9)
)

// Fake IMU, assume immediate limit speed, given force given wind
// rate of turn is proportional to speed and rudder angle

type IMUState struct {
	time_ms     int64
	lat_deg     float64
	lng_deg     float64
	speed_x_m_s float64
	speed_y_m_s float64 // x is forward, y is left, z is up
	course_deg  float64 // also yaw
	gyr_z_deg_s float64
}

func rad(deg float64) float64 { return deg * math.Pi / 180.0 }

//
func (s *IMUState) Update(time_ms int64, rudder_deg, sail_deg float64) {
	deltat_s := float64(time_ms-s.time_ms) / 1000.0

	wind_acc_m_s2 := kWindSpeed_m_s * kWindSpeed_m_s * kWindFudgeFactor * math.Cos(rad(sail_deg))
	s.speed_x_m_s += deltat_s * (wind_acc_m_s2*math.Sin(rad(sail_deg)) - kDragFactor_x*s.speed_x_m_s)
	s.speed_y_m_s += deltat_s * (wind_acc_m_s2*math.Cos(rad(sail_deg)) - kDragFactor_y*s.speed_y_m_s)

	speed_m_s := math.Sqrt(s.speed_x_m_s*s.speed_x_m_s + s.speed_y_m_s*s.speed_y_m_s)
	s.gyr_z_deg_s x= rudder_deg * speed_m_s / kTurnRadius_m
	s.course_deg += s.gyr_z_deg_s * deltat_s

	dist_m := deltat_s * speed_m_s
	s.lat_deg += dist_m * math.Cos(rad(s.course_deg)) / (1852.0 * 60.0)
	s.lng_deg += dist_m * math.Sin(rad(s.course_deg)) / (1852.0 * 60.0) / math.Cos(s.lat_deg)
	s.time_ms = time_ms
}

func main() {

	flag.Parse()

	if err := os.Chdir(*workingDir); err != nil {
		crash("Could not chdir to ", *workingDir, ": ", err)
	}

	imud, _ := gulp("imud")
	wind, _ := gulp("wind")
	rudd, cmd := gulp("rudderd")

	var rudder_l = AngleState{rudderSpeed_deg_ms, now_ms(), 0, 0}
	var rudder_r = AngleState{rudderSpeed_deg_ms, now_ms(), 0, 0}
	var sail = AngleState{sailSpeed_deg_ms, now_ms(), 0, 0}

	var imu = IMUState{now_ms(), 32.694866, -22.016602, .1, .1, 90, 0}

	tick := time.Tick(5E8)
	for {

		kvp := make(KeyValPairs)

		select {
		case l := <-cmd:
			if _, err := fmt.Sscan(l, &kvp); err != nil {
				fmt.Fprintln(os.Stderr, "Error parsing command: ", l, ": ", err, ": ", kvp)
			}

			if v, ok := kvp["rudder_l_deg"]; ok {
				rudder_l.SetTarget(v.(float64))
			}
			if v, ok := kvp["rudder_r_deg"]; ok {
				rudder_r.SetTarget(v.(float64))
			}
			if v, ok := kvp["sail_deg"]; ok {
				sail.SetTarget(v.(float64))
			}

		case <-tick:
		}

		now := now_ms()

		imu.Update(now, 0.5*(rudder_l.Get(now)+rudder_r.Get(now)), sail.Get(now))

		imud <- KeyValPairs{
			"timestamp_ms": imu.time_ms,
			"acc_x_m_s2":   .1,
			"acc_y_m_s2":   .1,
			"gyr_z_rad_s":  imu.gyr_z_deg_s * math.Pi / 180.0,
			"yaw_deg":      imu.course_deg,
			"lat_deg":      imu.lat_deg,
			"lng_deg":      imu.lng_deg,
			"vel_x_m_s":    imu.speed_x_m_s,
			"vel_y_m_s":    imu.speed_y_m_s,
		}
		wind <- KeyValPairs{
			"timestamp_ms": now,
			"angle_deg":    kWindAngle_deg - sail.Get(now) - imu.course_deg,
			"speed_m_s":    kWindSpeed_m_s,
			"valid":        1}
		rudd <- KeyValPairs{
			"timestamp_ms": now,
			"rudder_l_deg": rudder_l.Get(now),
			"rudder_r_deg": rudder_r.Get(now),
			"sail_deg":     sail.Get(now)}

	}

}
