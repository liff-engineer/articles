import sys
import time
from datetime import datetime
from pynng import Surveyor0, Respondent0, Timeout


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


if __name__ == "__main__":
    # print(sys.argv)
    if len(sys.argv) > 2 and sys.argv[1] == 'server':
        server(sys.argv[2])
    elif len(sys.argv) > 3 and sys.argv[1] == 'client':
        client(sys.argv[2], sys.argv[3])
    else:
        print(f"Usage: {sys.argv[0]} server|client <URL> <ARGS> ...")
        sys.exit(1)
