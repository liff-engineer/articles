import sys
import time
from pynng import Bus0, Timeout


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


if __name__ == "__main__":
    # print(sys.argv)
    if len(sys.argv) > 2:
        node()
    else:
        print(f"Usage: {sys.argv[0]} <NODE_NAME> <URL> <URL> ...")
        sys.exit(1)
