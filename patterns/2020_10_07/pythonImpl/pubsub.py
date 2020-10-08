import sys
import time
from datetime import datetime
from pynng import Pub0, Sub0, Timeout


def server(url: str):
    with Pub0(listen=url, send_timeout=100) as sock:
        while True:
            data = str(datetime.now())
            print(f'SERVER: PUBLISHING DATE {data}')
            sock.send(data.encode())
            time.sleep(1)


def client(url: str, name: str):
    with Sub0(topics='', dial=url, recv_timeout=100) as sock:
        while True:
            try:
                msg = sock.recv()
                print(f'CLIENT ({name}): RECEIVED {msg.decode()}')
            except Timeout:
                pass
            time.sleep(0.5)


if __name__ == "__main__":
    # print(sys.argv)
    if len(sys.argv) > 2 and sys.argv[1] == 'server':
        server(sys.argv[2])
    elif len(sys.argv) > 3 and sys.argv[1] == 'client':
        client(sys.argv[2], sys.argv[3])
    else:
        print(f"Usage: {sys.argv[0]} server|client <URL> <ARGS> ...")
        sys.exit(1)
