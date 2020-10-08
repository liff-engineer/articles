package main

import (
	"fmt"
	"os"
	"time"

	"go.nanomsg.org/mangos/v3"
	"go.nanomsg.org/mangos/v3/protocol/respondent"
	"go.nanomsg.org/mangos/v3/protocol/surveyor"

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
	var msg []byte

	if sock, err = surveyor.NewSocket(); err != nil {
		die("can't get new surveyor socket: %s", err.Error())
	}
	if err = sock.Listen(url); err != nil {
		die("can't listen on surveyor socket:%s", err.Error())
	}

	err = sock.SetOption(mangos.OptionSurveyTime, time.Second/2)
	if err != nil {
		die("SetOption faild: %s", err.Error())
	}
	for {
		fmt.Printf("SERVER: SENDING DATE SURVEY REQUEST\n")

		if err = sock.Send([]byte("DATE")); err != nil {
			die("failed send DATE SURVEY:%s", err.Error())
		}

		for {
			if msg, err = sock.Recv(); err != nil {
				break
			}

			fmt.Printf("SERVER: RECEIVED \"%s\" SURVERY RESPONSE\n", string(msg))
		}
		fmt.Println("SERVER: SURVEY COMPLETE")
	}
}

func client(url string, name string) {
	var sock mangos.Socket
	var err error
	var msg []byte

	if sock, err = respondent.NewSocket(); err != nil {
		die("can't get new respondent socket: %s", err.Error())
	}
	if err = sock.Dial(url); err != nil {
		die("can't dial on respondent socket:%s", err.Error())
	}

	for {
		if msg, err = sock.Recv(); err != nil {
			die("can't recv:%s", err.Error())
		}

		fmt.Printf("CLIENT (%s): RECEIVED \"%s\" SURVEY REQUEST\n", name, string(msg))

		d := date()
		fmt.Printf("CLIENT (%s): SENDING DATE SURVEY RESPONSE\n", name)

		if err = sock.Send([]byte(d)); err != nil {
			die("can't send: %s", err.Error())
		}
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
		"Usage: survey server|client <URL> <ARG> ...\n")
	os.Exit(1)
}
