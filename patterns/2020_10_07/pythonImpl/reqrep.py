import sys
import time
from datetime import datetime
from pynng import Req0, Rep0, Timeout


# print(str(datetime.now()))

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


def node1(url: str):
    with Req0(dial=url) as sock:
        print(f'NODE1: SENDING DATE REQUEST')
        sock.send('DATE'.encode())

        msg = sock.recv()
        print(f'NODE1: RECEIVED DATE {msg.decode()}')


if __name__ == "__main__":
    # print(sys.argv)
    if len(sys.argv) > 2 and sys.argv[1] == 'node0':
        node0(sys.argv[2])
    elif len(sys.argv) > 2 and sys.argv[1] == 'node1':
        node1(sys.argv[2])
    else:
        print(f"Usage: {sys.argv[0]} node0|node1 <URL> ...")
        sys.exit(1)
