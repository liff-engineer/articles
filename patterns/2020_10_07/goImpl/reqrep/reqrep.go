package main

import (
	"fmt"
	"os"
	"time"

	"go.nanomsg.org/mangos/v3"
	"go.nanomsg.org/mangos/v3/protocol/rep"
	"go.nanomsg.org/mangos/v3/protocol/req"

	// register transports
	_ "go.nanomsg.org/mangos/v3/transport/all"
)

func die(format string, v ...interface{}) {
	fmt.Fprintln(os.Stderr, fmt.Sprintf(format, v...))
	os.Exit(1)
}

func date() string {
	return time.Now().Format(time.ANSIC)
}

func node0(url string) {
	var sock mangos.Socket
	var err error
	var msg []byte

	if sock, err = rep.NewSocket(); err != nil {
		die("can't get new rep socket:%s", err)
	}
	if err = sock.Listen(url); err != nil {
		die("can't listen on rep socket:%s", err.Error())
	}

	for {
		msg, err = sock.Recv()
		if err != nil {
			die("can't receive on req socket:%s", err.Error())
		}

		if string(msg) == "DATE" {
			fmt.Println("NODE0: RECEIVED DATE REQUEST")
			d := date()
			fmt.Printf("NODE0: SENDING DATE %s\n", d)
			err = sock.Send([]byte(d))
			if err != nil {
				die("can't send reply: %s", err.Error())
			}
		}
	}
}

func node1(url string) {
	var sock mangos.Socket
	var err error
	var msg []byte

	if sock, err = req.NewSocket(); err != nil {
		die("can't get new req socket: %s", err.Error())
	}
	if err = sock.Dial(url); err != nil {
		die("can't dial on req socket:%s", err.Error())
	}

	fmt.Printf("NODE1: SENDING DATE REQUEST %s\n", "DATE")
	if err = sock.Send([]byte("DATE")); err != nil {
		die("can't send message on push socket: %s", err.Error())
	}
	if msg, err = sock.Recv(); err != nil {
		die("can't receive date: %s", err.Error())
	}
	fmt.Printf("NODE1: RECEIVED DATE %s", string(msg))
	sock.Close()
}

func main() {
	if len(os.Args) > 2 && os.Args[1] == "node0" {
		node0(os.Args[2])
		os.Exit(0)
	}
	if len(os.Args) > 2 && os.Args[1] == "node1" {
		node1(os.Args[2])
		os.Exit(0)
	}

	fmt.Fprintf(os.Stderr,
		"Usage: reqrep node0|node1 <URL> <ARG> ...\n")
	os.Exit(1)
}
