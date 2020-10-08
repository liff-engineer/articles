package main

import (
	"fmt"
	"os"
	"time"

	"go.nanomsg.org/mangos/v3"
	"go.nanomsg.org/mangos/v3/protocol/pull"
	"go.nanomsg.org/mangos/v3/protocol/push"

	// register transports
	_ "go.nanomsg.org/mangos/v3/transport/all"
)

func die(format string, v ...interface{}) {
	fmt.Fprintln(os.Stderr, fmt.Sprintf(format, v...))
	os.Exit(1)
}

func node0(url string) {
	var sock mangos.Socket
	var err error
	var msg []byte

	if sock, err = pull.NewSocket(); err != nil {
		die("can't get new pull socket: %s", err)
	}
	if err = sock.Listen(url); err != nil {
		die("can't listen on pull socket: %s", err.Error())
	}

	for {
		msg, err = sock.Recv()
		if err != nil {
			die("can't receive from mangos Socket: %s", err.Error())
		}

		fmt.Printf("NODE0: RECEIVED \"%s\"\n", msg)

		if string(msg) == "STOP" {
			fmt.Println("NODE0: STOPING")
			return
		}
	}
}

func node1(url string, msg string) {
	var sock mangos.Socket
	var err error

	if sock, err = push.NewSocket(); err != nil {
		die("can't get new push socket: %s", err.Error())
	}
	if err = sock.Dial(url); err != nil {
		die("can't dial on push socket: %s", err.Error())
	}
	fmt.Printf("NODE1: SENDING \"%s\"\n", msg)
	if err = sock.Send([]byte(msg)); err != nil {
		die("can't send message on push socket: %s", err.Error())
	}

	time.Sleep(time.Second / 10)
	sock.Close()
}

func main() {
	if len(os.Args) > 2 && os.Args[1] == "node0" {
		node0(os.Args[2])
		os.Exit(0)
	}
	if len(os.Args) > 3 && os.Args[1] == "node1" {
		node1(os.Args[2], os.Args[3])
		os.Exit(0)
	}

	fmt.Fprintf(os.Stderr,
		"Usage: pipeline node0|node1 <URL> <ARG> ...\n")
	os.Exit(1)
}
