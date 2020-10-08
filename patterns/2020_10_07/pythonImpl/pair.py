import sys
import time
from datetime import datetime
from pynng import Pair0, Timeout


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


def node0(url: str):
    with Pair0(listen=url, recv_timeout=100, send_timeout=100) as sock:
        send_recv(sock, 'NODE0')


def node1(url: str):
    with Pair0(dial=url, recv_timeout=100, send_timeout=100) as sock:
        send_recv(sock, 'NODE1')


if __name__ == "__main__":
    # print(sys.argv)
    if len(sys.argv) > 2 and sys.argv[1] == 'node0':
        node0(sys.argv[2])
    elif len(sys.argv) > 2 and sys.argv[1] == 'node1':
        node1(sys.argv[2])
    else:
        print(f"Usage: {sys.argv[0]} node0|node1 <URL> ...")
        sys.exit(1)
