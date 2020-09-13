package main

import (
	"fmt"
	"io"
	"os"
	"strings"
)

type drawable interface {
	draw(os io.Writer, position uint)
}

func draw(document []drawable, os io.Writer, position uint) {
	prefix := strings.Repeat(" ", int(position))
	fmt.Fprintf(os, "%v<document>\n", prefix)
	for _, e := range document {
		e.draw(os, position+2)
	}
	fmt.Fprintf(os, "%v</document>\n", prefix)
}

func main() {
	var document []drawable
	document = append(document, MyClass{})
	draw(document, os.Stdout, 0)
}

//MyClass demo used
type MyClass struct {
}

func (v MyClass) draw(os io.Writer, position uint) {
	prefix := strings.Repeat(" ", int(position))
	fmt.Fprintf(os, "%vMyClass\n", prefix)
}
