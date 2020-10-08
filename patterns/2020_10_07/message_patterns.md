[TOC]

## 以nng为例的消息通信模式

消息通信是有一些模式存在的,在`nanomsg`以及其继任`nng`中称之为"scalability protocols",其提供了如下模式:

- PAIR - 简单的1对1双向通信
- BUS - 简单的多对多双向通信
- REQREP - 请求回复式双向通信
- PUBSUB - 发布订阅式单向通信
- PIPELINE - 单向管道通信
- SURVEY - 1对多双向通信

这些消息通信模式能够涵盖绝大多数应用场景,同时`nng`也提供了不同的通信方式支持:

- INPROC - 进程间通信
- IPC - 跨进程通信
- TCP - 通过TCP通信 
- WS  - 通过基于TCP的websocket通信

以下将基于`nng`来阐述上述消息通信模式,以及C、Go、C++、Python实现样例.

### 说明

实例均来自于[`nng`官方文档](https://nanomsg.org/gettingstarted/nng/index.html),各个语言及库信息如下:

- C 

  github上仓库名为 [nng](https://github.com/nanomsg/nng),通过CMake可以拉取使用.

- Go 

  github上仓库名为[mangos](https://github.com/nanomsg/mangos),使用Go的第三方库使用方式即可,例如`import("go.nanomsg.org/mangos/v3")`

- C++ 

  github上仓库名为[nngpp](https://github.com/cwzx/nngpp),通过CMake可以拉取使用,由于是对`nng`的包装,必须同步拉取`nng`,并对其`CMakeLists.txt`做一些调整.

- Python 

  github上仓库名为[pynng](https://github.com/codypiersall/pynng),可以通过`pip install pynng`安装使用.

示例运行方式除了Python外均与官方文档示例运行方式一致.

### Pipeline(A One-Way Pipe)

![pipeline](https://nanomsg.org/gettingstarted/pipeline.png)

该模式对于解决生产者/消费者问题非常有用,包含负载均衡.消息从**push**端流动到**pull**端.如果有多个peer连接,该模式试图公平分配.

以`ipc:///tmp/pipeline.ipc  `为通信端口,启动`node0`之后,再启动`node1`发送消息到`node0`,脚本如下:

```bash
pipeline.exe node0 ipc:///tmp/pipeline.ipc
pipeline.exe node1 ipc:///tmp/pipeline.ipc "Hello, World!"
pipeline.exe node1 ipc:///tmp/pipeline.ipc "Goodbye."
```

则输出为:

```bash
NODE1: SENDING "Hello, World!"
NODE0: RECEIVED "Hello, World!"
NODE1: SENDING "Goodbye."
NODE0: RECEIVED "Goodbye."
```

那么示例结构包含三部分:

1. node0
2. node1
3. 应用程序入口

下面来看一看各个语言的实现.

#### 采用C实现Pipeline样例

首先引入`pipeline`协议:

```C++
#include "nng/nng.h"
#include "nng/protocol/pipeline0/pull.h"
#include "nng/protocol/pipeline0/push.h"
```

提供简易的错误处理:

```c++
void fatal(const char *func, int rv)
{
    fprintf(stderr, "%s: %s\n", func, nng_strerror(rv));
    exit(1);
}
```

然后看一下**pull**端`node0`的实现逻辑:

1. 打开socket
2. 监听socket
3. 循环从socket中读取消息

```C++
int node0(const char *url)
{
    nng_socket sock;
    int rv;

    if ((rv = nng_pull_open(&sock)) != 0)
    {
        fatal("nng_pull_open", rv);
    }
    if ((rv = nng_listen(sock, url, nullptr, 0)) != 0)
    {
        fatal("nng_listen", rv);
    }
    for (;;)
    {
        char *buf = NULL;
        size_t sz;
        uint64_t val;
        if ((rv = nng_recv(sock, &buf, &sz, NNG_FLAG_ALLOC)) != 0)
        {
            fatal("nng_recv", rv);
        }
        printf("NODE0:RECEIVED \"%s\"\n", buf);
        nng_free(buf, sz);
    }
}
```

然后是**push**端`node1`的实现逻辑:

1. 打开socket
2. socket拨号
3. 发送消息
4. 等待消息发送完成(这里使用了C++的线程sleep方法)

```C++
#include <chrono>
#include <thread>
int node1(const char *url, const char *msg)
{
    int sz_msg = strlen(msg) + 1; //'\0'
    nng_socket sock;
    int rv;
    size_t sz;
    if ((rv = nng_push_open(&sock)) != 0)
    {
        fatal("nng_push_open", rv);
    }
    if ((rv = nng_dial(sock, url, NULL, 0)) != 0)
    {
        fatal("nng_dial", rv);
    }
    printf("NODE1: SENDING \"%s\"\n", msg);
    if ((rv = nng_send(sock, (void *)msg, sz_msg, 0)) != 0)
    {
        fatal("nng_send", rv);
    }

    using namespace std::chrono_literals;
    std::this_thread::sleep_for(2s);

    nng_close(sock);
    return 0;
}
```

最后是应用程序入口:

```C++
#define NODE0 "node0"
#define NODE1 "node1"
int main(const int argc, const char **argv)
{
    if ((argc > 1) && (strcmp(NODE0, argv[1]) == 0))
        return (node0(argv[2]));
    if ((argc > 2) && (strcmp(NODE1, argv[1]) == 0))
        return (node1(argv[2], argv[3]));
    fprintf(stderr, "Usage: pipelineImpl %s|%s <URL> <ARG> ...\n", NODE0, NODE1);
    return 1;
}
```

#### 采用Go实现Pipeline样例

首先引入库和pipeline协议,并注册通信方式实现:

```go
import (
	"go.nanomsg.org/mangos/v3"
	"go.nanomsg.org/mangos/v3/protocol/pull"
	"go.nanomsg.org/mangos/v3/protocol/push"

	// register transports
	_ "go.nanomsg.org/mangos/v3/transport/all"
)
```

提供简易的错误处理支持:

```go
func die(format string, v ...interface{}) {
	fmt.Fprintln(os.Stderr, fmt.Sprintf(format, v...))
	os.Exit(1)
}
```

之后是**pull**端`node0`的实现:

```go
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
```

然后是**push**端`node1`的实现:

```go
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
```

最后是应用程序入口:

```go
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
```

可以看到,Go不亏是新的C,除了库使用方便、具有GC不需要管理内存,实现及代码量与C语言版本没有多大差异.下面看一下C++实现.

#### 采用C++实现Pipeline样例

C++的包装库`nngpp`采用的异常进行错误处理,内存等资源管理使用的是RAII,实现较为简单.

首先引入头文件:

```C++
#include <cstdio>
#include "nngpp/nngpp.h"
#include "nngpp/protocol/pull0.h"
#include "nngpp/protocol/push0.h"
```

之后是**pull**端`node0`的实现:

```C++
using namespace nng;

int node0(const char *url)
{
    try
    {
        socket sock = pull::open();
        sock.listen(url);
        while (true)
        {
            auto msg = sock.recv();
            printf("NODE0:RECEIVED \"%s\"\n", msg.data<char>());
        }
    }
    catch (const nng::exception &e)
    {
        printf("%s: %s\n", e.who(), e.what());
        return 1;
    }
}
```

可以看到接收的数据直接使用了`auto msg`来存储,这里的`msg`类型是`nng::buffer`,会自行管理生命周期,访问内部使用使用`data<T>()`即可.

然后是**push**端`node1`的实现:

```C++
#include <chrono>
#include <thread>
int node1(const char *url, const char *msg)
{
    try
    {
        socket sock = push::open();
        sock.dial(url);

        printf("NODE1: SENDING \"%s\"\n", msg);
        sock.send(view{msg, strlen(msg) + 1});

        using namespace std::chrono_literals;
        std::this_thread::sleep_for(2s);
        return 1;
    }
    catch (const nng::exception &e)
    {
        printf("%s: %s\n", e.who(), e.what());
        return 1;
    }
}
```

注意`send`使用的是`view`,可以发送已存在的数据而不需要关心内存处理.

应用程序入口与C语言版本一致,不再赘述.

可以看到,通过C++对C接口的简易封装之后,使用起来还是非常容易的,RAII和异常确实香.下面看一下Python实现是否体验更好.

#### 采用Python实现Pipeline样例

首先导入库:

```python
import time
from pynng import Pull0, Push0, Timeout
```

之后是**pull**端`node0`的实现:

```python
def node0(url: str):
    with Pull0(listen=url, recv_timeout=100) as sock:
        while True:
            try:
                msg = sock.recv()
                print(f"NODE0: RECEIVED \"{msg.decode()} \"")
            except Timeout:
                pass
            time.sleep(0.5)
```

注意与别的语言版本不同的是这里设置了超时,因为别的语言实现可以直接CTRL-C中断,而python脚本不设置超时是不会响应键盘时间的.

然后是**push**端`node1`的实现:

```python
def node1(url: str, msg: str):
    with Push0(dial=url) as sock:
        print(f"NODE1: SENDING \"{msg}\"")
        sock.send(msg.encode())
        time.sleep(1)
```

最后是应用程序入口实现:

```python
import sys
if __name__ == "__main__":
    # print(sys.argv)
    if len(sys.argv) > 2 and sys.argv[1] == 'node0':
        node0(sys.argv[2])
    elif len(sys.argv) > 3 and sys.argv[1] == 'node1':
        node1(sys.argv[2], sys.argv[3])
    else:
        print(f"Usage: {sys.argv[0]} node0|node1 <URL> <ARGS> ...")
        sys.exit(1)
```



### Request/Reply (I ask, you answer)

![reqrep.png](https://nanomsg.org/gettingstarted/reqrep.png)

请求/回复用在同步通信中,每一个问题都会被反馈一个答案,例如远程过程调用.与Pipeline一样也可以用来实现负载均衡.这是`nng`中提供的唯一具有可靠性的消息模式,当请求没有得到响应时会自动重试.

以`ipc:///tmp/reqrep.ipc  `为通信端口,启动`node0`之后,再启动`node1`发送日期请求到`node0`并获取反馈,脚本如下:

```bash
reqrep.exe node0 ipc:///tmp/reqrep.ipc
reqrep.exe node1 ipc:///tmp/reqrep.ipc
```

输出类似如下:

```bash
NODE1: SENDING DATE REQUEST DATE
NODE0: RECEIVED DATE REQUEST
NODE0: SENDING DATE Tue Jan  9 09:17:02 2018
NODE1: RECEIVED DATE Tue Jan  9 09:17:02 2018
```

示例结构包含四部分:

1. 日期信息获取
2. node0
3. node1
4. 应用程序入口

下面来看一看各个语言的实现.

#### 采用C实现Request/Reply样例

首先引入头文件,错误处理函数与之前一致:

```C++
#include "nng/nng.h"
#include "nng/protocol/reqrep0/req.h"
#include "nng/protocol/reqrep0/rep.h"
```

之后是日期获取函数:

```C++
#include <time.h>
char *date(void)
{
    time_t now = time(&now);
    struct tm *info = localtime(&now);
    char *text = asctime(info);
    text[strlen(text) - 1] = '\0'; // remove '\n'
    return (text);
}
```

然后是**Reply**端node0实现逻辑:

1. 打开socket
2. socket监听
3. 循环接收请求,如果是日期请求则发送响应

```C++
#define DATE "DATE"
int node0(const char *url)
{
    int sz_date = strlen(DATE) + 1; //'\0'
    nng_socket sock;
    int rv;
    if ((rv = nng_rep_open(&sock)) != 0)
    {
        fatal("nng_rep_open", rv);
    }
    if ((rv = nng_listen(sock, url, NULL, 0)) != 0)
    {
        fatal("nng_listen", rv);
    }
    for (;;)
    {
        char *buf = NULL;
        size_t sz;
        uint64_t val;
        if ((rv = nng_recv(sock, &buf, &sz, NNG_FLAG_ALLOC)) != 0)
        {
            fatal("nng_recv", rv);
        }
        if ((sz == sz_date) && (strcmp(DATE, buf) == 0))
        {
            printf("NODE0: RECEIVED DATE REQUEST\n");
            char *now = date();
            int sz_now = strlen(now) + 1;
            printf("NODE0: SENDING DATE %s\n", now);
            if ((rv = nng_send(sock, now, sz_now, 0)) != 0)
            {
                fatal("nng_send", rv);
            }
        }
        nng_free(buf, sz);
    }
}
```

然后是**Request**端node1实现逻辑:

1. 打开socket
2. socket拨号
3. 发送日期请求
4. 获取响应

```C++
int node1(const char *url)
{
    int sz_date = strlen(DATE) + 1; //'\0'
    nng_socket sock;
    int rv;
    char *buf = NULL;
    size_t sz = -1;

    if ((rv = nng_req_open(&sock)) != 0)
    {
        fatal("nng_req_open", rv);
    }
    if ((rv = nng_dial(sock, url, NULL, 0)) != 0)
    {
        fatal("nng_dial", rv);
    }
    printf("NODE1: SENDING DATE REQUEST %s\n", DATE);
    if ((rv = nng_send(sock, DATE, sz_date, 0)) != 0)
    {
        fatal("nng_send", rv);
    }
    if ((rv = nng_recv(sock, &buf, &sz, NNG_FLAG_ALLOC)) != 0)
    {
        fatal("nng_recv", rv);
    }
    printf("NODE1: RECEIVED DATE %s\n", buf);
    nng_free(buf, sz);
    return nng_close(sock);
}
```

之后是应用程序入口:

```C++
int main(const int argc, const char **argv)
{
    if ((argc > 1) && (strcmp(NODE0, argv[1]) == 0))
        return (node0(argv[2]));

    if ((argc > 1) && (strcmp(NODE1, argv[1]) == 0))
        return (node1(argv[2]));

    fprintf(stderr, "Usage: reqrepImpl %s|%s <URL> ...\n", NODE0, NODE1);
    return (1);
}
```



#### 采用Go实现Request/Reply样例

首先导入库,错误处理与之前一致:

```go
import (
	"go.nanomsg.org/mangos/v3"
	"go.nanomsg.org/mangos/v3/protocol/rep"
	"go.nanomsg.org/mangos/v3/protocol/req"

	// register transports
	_ "go.nanomsg.org/mangos/v3/transport/all"
)
```

之后是日期获取函数:

```go
func date() string {
	return time.Now().Format(time.ANSIC)
}
```

然后是**Reply**端node0实现:

```go
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
```

然后是**Request**端node1实现:

```go
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
```

应用程序入口如下:

```go
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
```

#### 采用C++实现Request/Reply样例

为了简便,应用程序入口和日期获取函数都采用C实现的版本,首先引入头文件:

```C++
#include "nngpp/nngpp.h"
#include "nngpp/protocol/req0.h"
#include "nngpp/protocol/rep0.h"
```

然后是**Reply**端node0实现:

```C++
using namespace nng;
int node0(const char *url)
{
    try
    {
        auto sock = rep::open();
        sock.listen(url);
        while (true)
        {
            auto msg = sock.recv();
            if (msg == "DATE")
            {
                printf("NODE0: RECEIVED DATE REQUEST\n");
                char *now = date();
                printf("NODE0: SENDING DATE %s\n", now);
                sock.send(view{now, strlen(now) + 1});
            }
        }
    }
    catch (const nng::exception &e)
    {
        printf("%s: %s\n", e.who(), e.what());
        return 1;
    }
}
```

然后是**Request**端node1实现:

```C++
int node1(const char *url)
{
    try
    {
        auto sock = req::open();
        sock.dial(url);
        printf("NODE1: SENDING DATE REQUEST %s\n", DATE);
        sock.send("DATE");
        auto msg = sock.recv();
        printf("NODE1: RECEIVED DATE %s\n", msg.data<char>());
    }
    catch (const nng::exception &e)
    {
        printf("%s: %s\n", e.who(), e.what());
        return 1;
    }
    return 1;
}
```

#### 采用Python实现Request/Reply样例

鉴于日期获取非常容易,不在单独提供实现,首先导入库:

```python
from datetime import datetime
from pynng import Req0, Rep0, Timeout
```

然后是**Reply**端node0实现:

```python
def node0(url: str):
    with Rep0(listen=url, recv_timeout=100) as sock:
        while True:
            try:
                msg = sock.recv()
                if str(msg.decode()) == 'DATE':
                    print(f'NODE0: RECEIVED DATE REQUEST')
                    data = str(datetime.now())
                    print(f'NODE0: SENDING DATE {data}')
                    sock.send(data.encode())
            except Timeout:
                pass
            time.sleep(0.5)
```

然后是**Request**端node1实现:

```python
def node1(url: str):
    with Req0(dial=url) as sock:
        print(f'NODE1: SENDING DATE REQUEST')
        sock.send('DATE'.encode())

        msg = sock.recv()
        print(f'NODE1: RECEIVED DATE {msg.decode()}')
```

应用程序入口如下:

```python
if __name__ == "__main__":
    if len(sys.argv) > 2 and sys.argv[1] == 'node0':
        node0(sys.argv[2])
    elif len(sys.argv) > 2 and sys.argv[1] == 'node1':
        node1(sys.argv[2])
    else:
        print(f"Usage: {sys.argv[0]} node0|node1 <URL> ...")
        sys.exit(1)
```

### Pair (Two Way Radio)

![pair](https://nanomsg.org/gettingstarted/pair.png)

pair模式应用于1对1的对等关系场景下,在同一时刻只能有一个连接到另外一个,但是双方通信是随意的.

这里以`ipc:///tmp/pair.ipc`为通信端口,先启动node0,然后再启动node1,双方互相发送自己的名称到对方,脚本如下:

```bash
pair.exe node0 ipc:///tmp/pair.ipc
pair.exe node1 ipc:///tmp/pair.ipc
```

输出类似如下:

```bash
node0: SENDING "node0"
node1: RECEIVED "node0"
node1: SENDING "node1"
node0: SENDING "node0"
node0: RECEIVED "node1"
node1: RECEIVED "node0"
```

示例结构分为四部分:

1. 接收与发送消息
2. node0
3. node1
4. 应用程序入口

注意虽然是对等关系,这里也有所区分,因为node0和node1的部分实现是不一样的.

#### 采用C实现Pair样例

引入头文件:

```c++
#include "nng/nng.h"
#include "nng/protocol/pair0/pair.h"
```

接收与发送名称的实现:

```C++
int send_name(nng_socket sock, const char *name)
{
    int rv;
    printf("%s: SENDING \"%s\"\n", name, name);
    size_t sz = strlen(name) + 1; //'\0'
    if ((rv = nng_send(sock, (void *)name, sz, 0)) != 0)
    {
        fatal("nng_send", rv);
    }
    return rv;
}

int recv_name(nng_socket sock, const char *name)
{
    char *buf = NULL;
    size_t sz = -1;
    int rv;

    if ((rv = nng_recv(sock, &buf, &sz, NNG_FLAG_ALLOC)) == 0)
    {
        printf("%s: RECEIVED \"%s\"\n", name, buf);
        nng_free(buf, sz);
    }
    return rv;
}
int send_recv(nng_socket sock, const char *name)
{
    int rv;
    if ((rv = nng_setopt_ms(sock, NNG_OPT_RECVTIMEO, 3000)) != 0)
    {
        fatal("nng_setopt", rv);
    }
    for (;;)
    {
        recv_name(sock, name);

        using namespace std::chrono_literals;
        std::this_thread::sleep_for(2s);

        send_name(sock, name);
    }
}
```

这时的node0实现逻辑为:

1. 打开socket
2. socket监听
3. 循环发送接收

```C++
int node0(const char *url)
{
    nng_socket sock;
    int rv;
    if ((rv = nng_pair_open(&sock)) != 0)
    {
        fatal("nng_pair_open", rv);
    }
    if ((rv = nng_listen(sock, url, NULL, 0)) != 0)
    {
        fatal("nng_listen", rv);
    }
    return send_recv(sock, NODE0);
}
```

node1实现逻辑为:

1. 打开socket
2. socket拨号
3. 循环发送接收

```C++
int node1(const char *url)
{
    nng_socket sock;
    int rv;
    if ((rv = nng_pair_open(&sock)) != 0)
    {
        fatal("nng_pair_open", rv);
    }
    if ((rv = nng_dial(sock, url, NULL, 0)) != 0)
    {
        fatal("nng_dial", rv);
    }
    return send_recv(sock, NODE1);
}
```

最后是应用程序入口:

```C++
int main(const int argc, const char **argv)
{
    if ((argc > 1) && (strcmp(NODE0, argv[1]) == 0))
        return (node0(argv[2]));

    if ((argc > 1) && (strcmp(NODE1, argv[1]) == 0))
        return (node1(argv[2]));

    fprintf(stderr, "Usage: pairImpl %s|%s <URL> ...\n", NODE0, NODE1);
    return (1);
}
```



#### 采用Go实现Pair样例

首先导入库:

```go
import (
	"go.nanomsg.org/mangos/v3"
	"go.nanomsg.org/mangos/v3/protocol/pair"

	// register transports
	_ "go.nanomsg.org/mangos/v3/transport/all"
)
```

然后是接收发送实现:

```go
func sendName(sock mangos.Socket, name string) {
	fmt.Printf("%s: SENDING \"%s\"\n", name, name)
	if err := sock.Send([]byte(name)); err != nil {
		die("failed sending:%s", err)
	}
}

func recvName(sock mangos.Socket, name string) {
	var msg []byte
	var err error
	if msg, err = sock.Recv(); err == nil {
		fmt.Printf("%s: RECEIVED: \"%s\"\n", name, string(msg))
	}
}

func sendRecv(sock mangos.Socket, name string) {
	for {
		sock.SetOption(mangos.OptionRecvDeadline, 100*time.Millisecond)
		recvName(sock, name)
		time.Sleep(time.Second)
		sendName(sock, name)
	}
}
```

node0的实现如下:

```go
func node0(url string) {
	var sock mangos.Socket
	var err error
	if sock, err = pair.NewSocket(); err != nil {
		die("can't get new pair socket: %s", err)
	}
	if err = sock.Listen(url); err != nil {
		die("can't listen on pair socket:%s", err.Error())
	}
	sendRecv(sock, "node0")
}
```

node1的实现如下:

```go
func node1(url string) {
	var sock mangos.Socket
	var err error
	if sock, err = pair.NewSocket(); err != nil {
		die("can't get new pair socket: %s", err)
	}
	if err = sock.Dial(url); err != nil {
		die("can't dial on pair socket:%s", err.Error())
	}
	sendRecv(sock, "node1")
}
```

应用程序入口:

```go
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
		"Usage: pair node0|node1 <URL> <ARG> ...\n")
	os.Exit(1)
}
```

#### 采用C++实现Pair样例

首先引入头文件:

```C++
#include "nngpp/nngpp.h"
#include "nngpp/protocol/pair1.h"
```

然后是发送接收实现:

```C++
using namespace nng;

int send_recv(nng::socket_view sock, const char *name)
{
    nng::set_opt_recv_timeout(sock, 3000);
    while (true)
    {
        printf("%s: SENDING \"%s\"\n", name, name);
        sock.send(view{name, strlen(name) + 1});

        using namespace std::chrono_literals;
        std::this_thread::sleep_for(2s);

        auto msg = sock.recv();
        printf("%s: RECEIVED \"%s\"\n", name, msg.data<char>());
    }
}
```

注意nngpp采用了RAII封装socket对象,因而如果想要传递socket需要使用`nng::socket_view`.

node0实现如下:

```C++
int node0(const char *url)
{
    try
    {
        auto sock = pair::open();
        sock.listen(url);
        return send_recv(sock, "node0");
    }
    catch (const nng::exception &e)
    {
        printf("%s: %s\n", e.who(), e.what());
        return 1;
    }
}
```

node1实现如下:

```C++
int node1(const char *url)
{
    try
    {
        auto sock = pair::open();
        sock.dial(url);
        return send_recv(sock, "node1");
    }
    catch (const nng::exception &e)
    {
        printf("%s: %s\n", e.who(), e.what());
        return 1;
    }
}
```

应用程序入口与C版本一致.

#### 采用Python实现Pair样例

首先是导入库:

```python
from pynng import Pair0, Timeout
```

然后是接收发送实现:

```python
def send_recv(sock: Pair0, name: str):
    while True:
        try:
            msg = sock.recv()
            print(f'{name}: RECEIVED "{msg.decode()}"')
        except Timeout:
            pass

        time.sleep(1)

        try:
            print(f'{name}: SENDING "{name}"')
            sock.send(name.encode())
        except Timeout:
            pass
```

node0实现如下:

```python
def node0(url: str):
    with Pair0(listen=url, recv_timeout=100, send_timeout=100) as sock:
        send_recv(sock, 'NODE0')
```

node1实现如下:

```python
def node1(url: str):
    with Pair0(dial=url, recv_timeout=100, send_timeout=100) as sock:
        send_recv(sock, 'NODE1')
```

应用程序入口为:

```python
if __name__ == "__main__":
    if len(sys.argv) > 2 and sys.argv[1] == 'node0':
        node0(sys.argv[2])
    elif len(sys.argv) > 2 and sys.argv[1] == 'node1':
        node1(sys.argv[2])
    else:
        print(f"Usage: {sys.argv[0]} node0|node1 <URL> ...")
        sys.exit(1)
```



### Pub/Sub (Topics & Broadcast)

![pubsub](https://nanomsg.org/gettingstarted/pubsub.png)

该模式允许单个广播员发送信息到多个订阅者,订阅者可以选择限制其能够接收到的消息.

这里以`ipc:///tmp/pubsub.ipc`为通信地址,首先启动server,然后可以启动多个client,client能够接收到server推送来的日期信息.脚本如下:

```bash
pubsub.exe server ipc:///tmp/pubsub.ipc
pubsub.exe client ipc:///tmp/pubsub.ipc client0
pubsub.exe client ipc:///tmp/pubsub.ipc client1
```

输出类似于:

```bash
SERVER: PUBLISHING DATE Tue Jan  9 08:21:31 2018
SERVER: PUBLISHING DATE Tue Jan  9 08:21:32 2018
CLIENT (client1): RECEIVED Tue Jan  9 08:21:32 2018
CLIENT (client0): RECEIVED Tue Jan  9 08:21:32 2018
SERVER: PUBLISHING DATE Tue Jan  9 08:21:33 2018
CLIENT (client0): RECEIVED Tue Jan  9 08:21:33 2018
```

示例结构为:

1. server
2. client
3. 应用程序入口

#### 采用C实现Pub/Sub样例

首先导入头文件:

```C++
#include "nng/nng.h"
#include "nng/protocol/pubsub0/pub.h"
#include "nng/protocol/pubsub0/sub.h"
```

然后是server实现逻辑:

1. 打开socket
2. socket监听
3. 循环发送日期信息

```C++
int server(const char *url)
{
    nng_socket sock;
    int rv;
    if ((rv = nng_pub_open(&sock)) != 0)
    {
        fatal("nng_pub_open", rv);
    }
    if ((rv = nng_listen(sock, url, NULL, 0)) != 0)
    {
        fatal("nng_listen", rv);
    }
    for (;;)
    {
        char *now = date();
        int sz_now = strlen(now) + 1;
        printf("SERVER: PUBLISHING DATE %s\n", now);
        if ((rv = nng_send(sock, now, sz_now, 0)) != 0)
        {
            fatal("nng_send", rv);
        }

        using namespace std::chrono_literals;
        std::this_thread::sleep_for(1s);
    }
}
```

之后是client实现逻辑:

1. 打开socket
2. 设置socket订阅
3. socket拨号
4. 循环接收消息

```C++
int client(const char *url, const char *name)
{
    nng_socket sock;
    int rv;

    if ((rv = nng_sub_open(&sock)) != 0)
    {
        fatal("nng_sub_open", rv);
    }
    if ((rv = nng_setopt(sock, NNG_OPT_SUB_SUBSCRIBE, "", 0)) != 0)
    {
        fatal("nng_setopt", rv);
    }
    if ((rv = nng_dial(sock, url, NULL, 0)) != 0)
    {
        fatal("nng_dial", rv);
    }
    for (;;)
    {
        char *buf = NULL;
        size_t sz = -1;
        if ((rv = nng_recv(sock, &buf, &sz, NNG_FLAG_ALLOC)) != 0)
        {
            fatal("nng_recv", rv);
        }
        printf("CLIENT (%s): RECEIVED %s\n", name, buf);
        nng_free(buf, sz);
    }
}
```

最后是应用程序入口:

```C++
int main(const int argc, const char **argv)
{
    if ((argc >= 2) && (strcmp(SERVER, argv[1]) == 0))
        return (server(argv[2]));

    if ((argc >= 3) && (strcmp(CLIENT, argv[1]) == 0))
        return (client(argv[2], argv[3]));

    fprintf(stderr, "Usage: pubsubImpl %s|%s <URL> ...\n", SERVER, CLIENT);
    return (1);
}
```

#### 采用Go实现Pub/Sub样例

首先导入库:

```go
import (
	"go.nanomsg.org/mangos/v3"
	"go.nanomsg.org/mangos/v3/protocol/pub"
	"go.nanomsg.org/mangos/v3/protocol/sub"

	// register transports
	_ "go.nanomsg.org/mangos/v3/transport/all"
)
```

然后是server实现:

```go
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
```

之后是client实现:

```go
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
```

最后是应用程序入口:

```go
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
```

#### 采用C++实现Pub/Sub样例

首先导入头文件:

```c++
#include "nngpp/nngpp.h"
#include "nngpp/protocol/pub0.h"
#include "nngpp/protocol/sub0.h"
```

然后是server实现:

```C++
using namespace nng;
int server(const char *url)
{
    try
    {
        auto sock = pub::open();
        sock.listen(url);
        while (true)
        {
            char *now = date();
            printf("SERVER: PUBLISHING DATE %s\n", now);
            sock.send(view{now, strlen(now) + 1});

            using namespace std::chrono_literals;
            std::this_thread::sleep_for(1s);
        }
    }
    catch (const nng::exception &e)
    {
        printf("%s: %s\n", e.who(), e.what());
        return 1;
    }
}
```

之后是client实现:

```C++
int client(const char *url, const char *name)
{
    try
    {
        auto sock = sub::open();
        nng::sub::set_opt_subscribe(sock, "");
        sock.dial(url);
        while (true)
        {
            auto msg = sock.recv();
            printf("CLIENT (%s): RECEIVED %s\n", name, msg.data<char>());
        }
    }
    catch (const nng::exception &e)
    {
        printf("%s: %s\n", e.who(), e.what());
        return 1;
    }
}
```

#### 采用Python实现Pub/Sub样例

首先导入库:

```python
from pynng import Pub0, Sub0, Timeout
```

然后是server实现:

```python
def server(url: str):
    with Pub0(listen=url, send_timeout=100) as sock:
        while True:
            data = str(datetime.now())
            print(f'SERVER: PUBLISHING DATE {data}')
            sock.send(data.encode())
            time.sleep(1)
```

之后是client实现:

```python
def client(url: str, name: str):
    with Sub0(topics='', dial=url, recv_timeout=100) as sock:
        while True:
            try:
                msg = sock.recv()
                print(f'CLIENT ({name}): RECEIVED {msg.decode()}')
            except Timeout:
                pass
            time.sleep(0.5)
```

最后是应用程序入口:

```python
if __name__ == "__main__":
    if len(sys.argv) > 2 and sys.argv[1] == 'server':
        server(sys.argv[2])
    elif len(sys.argv) > 3 and sys.argv[1] == 'client':
        client(sys.argv[2], sys.argv[3])
    else:
        print(f"Usage: {sys.argv[0]} server|client <URL> <ARGS> ...")
        sys.exit(1)
```

### Survey (Everybody Votes)

![survey](https://nanomsg.org/gettingstarted/survey.png)

调查员模式可以发布一个定时调查,反馈将单独返回直到调查到期.在实现服务发现和投票算法是非常有用.

这里使用`ipc:///tmp/survey.ipc`为通信端口,首先启动server,然后可以启动多个client接收server的请求并发送反馈,脚本如下:

```bash
survey.exe server ipc:///tmp/survey.ipc
survey.exe client ipc:///tmp/survey.ipc client0 
survey.exe client ipc:///tmp/survey.ipc client1
survey.exe client ipc:///tmp/survey.ipc client2
```

输出类似如下:

```bash
SERVER: SENDING DATE SURVEY REQUEST
SERVER: SURVEY COMPLETE
SERVER: SENDING DATE SURVEY REQUEST
CLIENT (client2): RECEIVED "DATE" SURVEY REQUEST
CLIENT (client0): RECEIVED "DATE" SURVEY REQUEST
CLIENT (client1): RECEIVED "DATE" SURVEY REQUEST
CLIENT (client2): SENDING DATE SURVEY RESPONSE
CLIENT (client1): SENDING DATE SURVEY RESPONSE
CLIENT (client0): SENDING DATE SURVEY RESPONSE
SERVER: RECEIVED "Tue Jan  9 11:33:40 2018" SURVEY RESPONSE
SERVER: RECEIVED "Tue Jan  9 11:33:40 2018" SURVEY RESPONSE
SERVER: RECEIVED "Tue Jan  9 11:33:40 2018" SURVEY RESPONSE
SERVER: SURVEY COMPLETE
```

示例结构分为:

1. server
2. client
3. 应用程序入口

#### 采用C实现Survey样例

首先是引入头文件:

```C++
#include "nng/nng.h"
#include "nng/protocol/survey0/survey.h"
#include "nng/protocol/survey0/respond.h"
```

server实现逻辑为:

1. 打开socket
2. socket监听
3. 循环发送调查请求
4. 获取响应,如果超时则跳出循环

```C++
int server(const char *url)
{
    nng_socket sock;
    int rv;

    if ((rv = nng_surveyor_open(&sock)) != 0)
    {
        fatal("nng_surveyor_open", rv);
    }
    if ((rv = nng_listen(sock, url, NULL, 0)) != 0)
    {
        fatal("nng_listen", rv);
    }
    for (;;)
    {
        printf("SERVER: SENDING DATE SURVEY REQUEST\n");
        if ((rv = nng_send(sock, DATE, strlen(DATE) + 1, 0)) != 0)
        {
            fatal("nng_send", rv);
        }

        for (;;)
        {
            char *buf = NULL;
            size_t sz;
            rv = nng_recv(sock, &buf, &sz, NNG_FLAG_ALLOC);
            if (rv == NNG_ETIMEDOUT)
            {
                break;
            }
            if (rv != 0)
            {
                fatal("nng_recv", rv);
            }
            printf("SERVER: RECEIVED \"%s\" SURVEY RESPONSE\n", buf);
            nng_free(buf, sz);
        }
        printf("SERVER:SURVERY COMPLETE\n");
    }
}
```

client实现逻辑为:

1. 打开socket
2. socket拨号
3. 循环接收请求,并发送日期信息

```C++
int client(const char *url, const char *name)
{
    nng_socket sock;
    int rv;

    if ((rv = nng_respondent_open(&sock)) != 0)
    {
        fatal("nng_respondent_open", rv);
    }
    if ((rv = nng_dial(sock, url, NULL, NNG_FLAG_NONBLOCK)) != 0)
    {
        fatal("nng_dial", rv);
    }
    for (;;)
    {
        char *buf = NULL;
        size_t sz;
        if ((rv = nng_recv(sock, &buf, &sz, NNG_FLAG_ALLOC)) == 0)
        {
            printf("CLIENT (%s): RECEIVED \"%s\" SURVERY REQUEST\n", name, buf);
            nng_free(buf, sz);

            char *now = date();
            printf("CLIENT (%s): SENDING DATE SURVEY RESPONSE\n", name);
            if ((rv = nng_send(sock, now, strlen(now) + 1, 0)) != 0)
            {
                fatal("nng_send", rv);
            }
        }
    }
}
```

应用程序入口为:

```C++
int main(const int argc, const char **argv)
{
    if ((argc >= 2) && (strcmp(SERVER, argv[1]) == 0))
        return (server(argv[2]));

    if ((argc >= 3) && (strcmp(CLIENT, argv[1]) == 0))
        return (client(argv[2], argv[3]));

    fprintf(stderr, "Usage: surveyImpl %s|%s <URL> <ARG>...\n", SERVER, CLIENT);
    return (1);
}
```

#### 采用Go实现Survey样例

首先是引入库:

```go
import (
	"go.nanomsg.org/mangos/v3"
	"go.nanomsg.org/mangos/v3/protocol/respondent"
	"go.nanomsg.org/mangos/v3/protocol/surveyor"

	// register transports
	_ "go.nanomsg.org/mangos/v3/transport/all"
)
```

server实现如下:

```go
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
```

client实现如下:

```go
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
```

应用程序入口为:

```go
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
```

#### 采用C++实现Survey样例

首先是引入头文件:

```C++
#include "nngpp/nngpp.h"
#include "nngpp/protocol/survey0.h"
#include "nngpp/protocol/respond0.h"
```

server实现如下:

```C++
using namespace nng;
int server(const char *url)
{
    try
    {
        auto sock = survey::open();
        survey::set_opt_survey_time(sock, 1000);
        sock.listen(url);

        while (true)
        {
            printf("SERVER: SENDING DATE SURVEY REQUEST\n");
            sock.send("DATE");

            while (true)
            {
                try
                {
                    auto msg = sock.recv();
                    printf("SERVER: RECEIVED \"%s\" SURVEY RESPONSE\n", msg.data<char>());
                }
                catch (const nng::exception &e)
                {
                    if (e.get_error() == nng::error::timedout)
                        break;
                    else
                    {
                        printf("%s: %s\n", e.who(), e.what());
                        return 1;
                    }
                }
            }
            printf("SERVER:SURVERY COMPLETE\n");
        }
    }
    catch (const nng::exception &e)
    {
        printf("%s: %s\n", e.who(), e.what());
        return 1;
    }
}
```

client实现如下:

```C++
int client(const char *url, const char *name)
{
    try
    {
        auto sock = respond::open();
        sock.dial(url);
        while (true)
        {
            auto msg = sock.recv();
            printf("CLIENT (%s): RECEIVED \"%s\" SURVERY REQUEST\n", name, msg.data<char>());
            char *now = date();
            printf("CLIENT (%s): SENDING DATE SURVEY RESPONSE\n", name);
            sock.send(view{now, strlen(now) + 1});
        }
    }
    catch (const nng::exception &e)
    {
        printf("%s: %s\n", e.who(), e.what());
        return 1;
    }
}
```

#### 采用Python实现Survey样例

首先是引入库:

```python
from pynng import Surveyor0, Respondent0, Timeout
```

server实现如下:

```python
def server(url: str):
    with Surveyor0(survey_time=100, listen=url) as sock:
        while True:
            print(f'SERVER: SENDING DATE SURVEY REQUEST')
            sock.send('DATE'.encode())

            while True:
                try:
                    msg = sock.recv()
                    print(f'SERVER: RECEIVED "{msg.decode()}" SURVEY RESPONSE')
                except Timeout:
                    break

            print(f'SERVER: SURVEY COMPLETE')
```

client实现如下:

```python
def client(url: str, name: str):
    with Respondent0(dial=url, block_on_dial=False, recv_timeout=100) as sock:
        while True:
            try:
                msg = sock.recv()

                if str(msg.decode()) == 'DATE':
                    print(
                        f'CLIENT ({name}): RECEIVED "{msg.decode()}" SURVEY REQUEST')
                    print(f'CLIENT ({name}): SENDING DATE SURVEY RESPONSE')
                    data = str(datetime.now())
                    sock.send(data.encode())
            except Timeout:
                pass
```

应用程序入口为:

```python
if __name__ == "__main__":
    if len(sys.argv) > 2 and sys.argv[1] == 'server':
        server(sys.argv[2])
    elif len(sys.argv) > 3 and sys.argv[1] == 'client':
        client(sys.argv[2], sys.argv[3])
    else:
        print(f"Usage: {sys.argv[0]} server|client <URL> <ARGS> ...")
        sys.exit(1)
```

### Bus (Routing)

![bus](https://nanomsg.org/gettingstarted/bus.png)

总线模式应用于路由程序,或者建立完全互联的网状网络.在这个模式中消息被直接发送到每个直接连接的对等方.

这里为每个node分配对应的通信端口,并附带其连接的通信端口,脚本如下:

```bash
bus.exe node0 ipc:///tmp/node0.ipc ipc:///tmp/node1.ipc
bus.exe node1 ipc:///tmp/node1.ipc
```

输出类似如下:

```bash
node0: SENDING 'node0' ONTO BUS
node1: SENDING 'node1' ONTO BUS
node0: RECEIVED 'node1' FROM BUS
node1: RECEIVED 'node0' FROM BUS
```

示例结构为:

1. node
2. 应用程序入口

#### 采用C实现Bus样例

首先引入头文件:

```C++
#include "nng/nng.h"
#include "nng/protocol/bus0/bus.h"
```

node的实现逻辑为:

1. 打开socket
2. 监听socket
3. socket拨号
4. 发送自身名称到总线上
5. 循环接收消息

```C++
int node(int argc, char **argv)
{
    nng_socket sock;
    int rv;
    size_t sz;

    if ((rv = nng_bus_open(&sock)) != 0)
    {
        fatal("nng_bus_open", rv);
    }

    if ((rv = nng_listen(sock, argv[2], NULL, 0)) != 0)
    {
        fatal("nng_listen", rv);
    }

    using namespace std::chrono_literals;

    std::this_thread::sleep_for(3s);

    if (argc >= 3)
    {
        for (int i = 3; i < argc; i++)
        {
            if ((rv = nng_dial(sock, argv[i], NULL, 0)) != 0)
            {
                fatal("nng_dial", rv);
            }
        }
    }

    std::this_thread::sleep_for(3s);

    sz = strlen(argv[1]) + 1;
    printf("%s:SENDING '%s' ONTO BUS\n", argv[1], argv[1]);
    if ((rv = nng_send(sock, argv[1], sz, 0)) != 0)
    {
        fatal("nng_send", rv);
    }

    for (;;)
    {
        char *buf = NULL;
        size_t sz;
        if ((rv = nng_recv(sock, &buf, &sz, NNG_FLAG_ALLOC)) != 0)
        {
            if (rv == NNG_ETIMEDOUT)
            {
                fatal("nng_recv", rv);
            }
        }
        printf("%s: RECEIVED '%s' FROM BUS\n", argv[1], buf);
        nng_free(buf, sz);
    }
    return nng_close(sock);
}
```

应用程序实现为:

```C++
int main(int argc, char **argv)
{
    if (argc >= 3)
    {
        return node(argc, argv);
    }
    fprintf(stderr, "Usage: busImpl <NODE_NAME> <URL> <URL> ...\n");
    return 1;
}
```



#### 采用Go实现Bus样例

首先引入库:

```go
import (
	"go.nanomsg.org/mangos/v3"
	"go.nanomsg.org/mangos/v3/protocol/bus"

	// register transports
	_ "go.nanomsg.org/mangos/v3/transport/all"
)
```

node的实现如下:

```go
func node(args []string) {
	var sock mangos.Socket
	var err error
	var msg []byte
	var x int

	if sock, err = bus.NewSocket(); err != nil {
		die("bus.NewSocket:%s", err.Error())
	}
	if err = sock.Listen(args[2]); err != nil {
		die("sock.Listen: %s", err.Error())
	}

	time.Sleep(3 * time.Second)
	for x = 3; x < len(args); x++ {
		if err = sock.Dial(args[x]); err != nil {
			die("sock.Dial: %s", err.Error())
		}
	}

	time.Sleep(time.Second)

	fmt.Printf("%s: SENDING '%s' ONTO BUS\n", args[1], args[1])
	if err = sock.Send([]byte(args[1])); err != nil {
		die("sock.Send: %s", err.Error())
	}

	for {
		if msg, err = sock.Recv(); err != nil {
			die("sock.Recv: %s", err.Error())
		}
		fmt.Printf("%s: RECEIVED '%s' FROM BUS\n", args[1], string(msg))
	}
}
```

应用程序实现为:

```go
func main() {
	if len(os.Args) > 2 {
		node(os.Args)
		os.Exit(0)
	}
	fmt.Fprintf(os.Stderr,
		"Usage: bus <NODE_NAME> <URL> <URL> ...\n")
	os.Exit(1)
}
```

#### 采用C++实现Bus样例

首先引入头文件:

```C++
#include "nngpp/nngpp.h"
#include "nngpp/protocol/bus0.h"
```

node的实现如下:

```C++
using namespace nng;

int node(int argc, char **argv)
{
    try
    {
        auto sock = bus::open();
        sock.listen(argv[2]);

        using namespace std::chrono_literals;
        std::this_thread::sleep_for(3s);

        if (argc >= 3)
        {
            for (int i = 3; i < argc; i++)
            {
                sock.dial(argv[i]);
            }
        }

        std::this_thread::sleep_for(3s);

        printf("%s:SENDING '%s' ONTO BUS\n", argv[1], argv[1]);
        sock.send(view{argv[1], strlen(argv[1]) + 1});

        while (true)
        {
            auto msg = sock.recv();
            printf("%s: RECEIVED '%s' FROM BUS\n", argv[1], msg.data<char>());
        }
    }
    catch (const nng::exception &e)
    {
        printf("%s: %s\n", e.who(), e.what());
        return 1;
    }
}

```

#### 采用Python实现Bus样例

首先引入库:

```python
from pynng import Bus0, Timeout
```

node的实现如下:

```python
def node():
    url = sys.argv[2]
    with Bus0(listen=url, recv_timeout=100) as sock:

        time.sleep(1)

        if len(sys.argv) > 3:
            for n in sys.argv[3:]:
                sock.dial(n)

        time.sleep(1)

        node = sys.argv[1]
        print(f'{node}: SENDING \'{node}\' ONTO BUS')
        sock.send(node.encode())

        while True:
            try:
                msg = sock.recv()
                print(f'{node}: RECEIVED \'{msg.decode()}\' FROM BUS')
            except Timeout:
                sys.exit(1)
```

应用程序实现为:

```python
if __name__ == "__main__":
    if len(sys.argv) > 2:
        node()
    else:
        print(f"Usage: {sys.argv[0]} <NODE_NAME> <URL> <URL> ...")
        sys.exit(1)
```

### 总结

通过`nng`提供的这些消息通信模式示例,可以看到,在真实应用中通信模式大多数属于其中几种的拼接组合,可以为我们提供消息通信层面的整体视角、设计模式,帮助我们更好地理解和设计应用程序.你可以将其和真实的应用建立下映射关系,譬如git、http服务器、聊天室等等.

同时,通过`nng`可以看到实现能够做到简洁清晰,易于理解和使用,在API设计层面也能够参考.通过对C、Go、C++、Python四种语言样例的实现,能够感觉到C和C++确实为开发者提供了更接近底层的能力;C和Go采用的错误码处理错误/异常在实现时非常繁琐,异常还是有一定优势的;RAII这种模式能够明显降低开发者心智负担;Python确实适合做简单的任务或者原型验证;C++比较强大,却也最麻烦,想要写得好得多学习多了解.