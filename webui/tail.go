package main

import (
	"io"
	"os"
	"time"
//	"log"
)

func Tail(path string, w io.WriteCloser) chan<- bool {
	stop := make(chan bool)
	go func() {
		tick := time.Tick(2*time.Second)
		var now time.Time
		var lastsize int64 = -1

	outer:
		for {

			select {
			case  _ = <-stop:
				break outer
			case now = <-tick:
			}

			f, err := os.Open(path)
			if err != nil {
				continue outer
			}
//			log.Print("opened ", path)

			for {			
				fi, err := f.Stat()
				if  err != nil {
					continue outer
				}
//				log.Print(fi.Size())
				if lastsize == -1 {
					lastsize = fi.Size()
				}
				if lastsize > fi.Size() {
					lastsize = 0
				}

				if fi.Size() > lastsize {
					f.Seek(lastsize, 0)
					io.Copy(w,f)
					lastsize = fi.Size()
				} else if now.Sub(fi.ModTime()) > 5 * time.Second {
					break
				}

				select {
				case  _ = <-stop:
					break outer
				case now = <-tick:
				}
			}
			f.Close()
		}
		w.Close()
//		log.Print("tail end")
	}()

	return stop
}
/*
func main() {
	stop := Tail(os.Args[1], os.Stdout)
	<-time.After(60*time.Second)
	close(stop)
}
*/