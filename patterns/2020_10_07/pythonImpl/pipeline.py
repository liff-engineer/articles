import sys
import time
from pynng import Pull0, Push0, Timeout


def node0(url: str):
    with Pull0(listen=url, recv_timeout=100) as sock:
        while True:
            try:
                msg = sock.recv()
                print(f"NODE0: RECEIVED \"{msg.decode()} \"")
            except Timeout:
                pass
            time.sleep(0.5)


def node1(url: str, msg: str):
    with Push0(dial=url) as sock:
        print(f"NODE1: SENDING \"{msg}\"")
        sock.send(msg.encode())
        time.sleep(1)


if __name__ == "__main__":
    # print(sys.argv)
    if len(sys.argv) > 2 and sys.argv[1] == 'node0':
        node0(sys.argv[2])
    elif len(sys.argv) > 3 and sys.argv[1] == 'node1':
        node1(sys.argv[2], sys.argv[3])
    else:
        print(f"Usage: {sys.argv[0]} node0|node1 <URL> <ARGS> ...")
        sys.exit(1)
