package main

import (
	"fmt"
	"os"
	"time"

	"go.nanomsg.org/mangos/v3"
	"go.nanomsg.org/mangos/v3/protocol/pub"
	"go.nanomsg.org/mangos/v3/protocol/sub"

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

func server(url string) {
	var sock mangos.Socket
	var err error

	if sock, err = pub.NewSocket(); err != nil {
		die("can't get new pub socket: %s", err.Error())
	}

	if err = sock.Listen(url); err != nil {
		die("can't listen on pub socket:%s", err.Error())
	}

	for {
		d := date()
		fmt.Printf("SERVER: PUBLISHING DATE %s\n", d)
		if err = sock.Send([]byte(d)); err != nil {
			die("Failed publishing: %s", err.Error())
		}
		time.Sleep(time.Second)
	}
}

func client(url string, name string) {
	var sock mangos.Socket
	var err error
	var msg []byte

	if sock, err = sub.NewSocket(); err != nil {
		die("can't get new sub socket: %s", err.Error())
	}

	if err = sock.Dial(url); err != nil {
		die("can't dial on sub socket:%s", err.Error())
	}

	err = sock.SetOption(mangos.OptionSubscribe, []byte(""))
	if err != nil {
		die("can't subscribe: %s", err.Error())
	}

	for {
		if msg, err = sock.Recv(); err != nil {
			die("can't recv : %s", err.Error())
		}
		fmt.Printf("CLIENT (%s): RECEIVED %s\n", name, string(msg))
	}
}

func main() {
	if len(os.Args) > 2 && os.Args[1] == "server" {
		server(os.Args[2])
		os.Exit(0)
	}
	if len(os.Args) > 3 && os.Args[1] == "client" {
		client(os.Args[2], os.Args[3])
		os.Exit(0)
	}

	fmt.Fprintf(os.Stderr,
		"Usage: pubsub server|client <URL> <ARG> ...\n")
	os.Exit(1)
}
