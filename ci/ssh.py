#!/usr/bin/env python3

from argparse import ArgumentParser, REMAINDER
from select import select
import paramiko
import sys
import os

def main():
    parser = ArgumentParser()
    parser.add_argument("host")
    parser.add_argument("command")
    parser.add_argument("args", nargs=REMAINDER)
    args = parser.parse_args()

    client = paramiko.client.SSHClient()
    client.set_missing_host_key_policy(paramiko.AutoAddPolicy())
    client.connect(args.host, username="root", password="meta-rte")

    cmd = f"{args.command} "
    for arg in args.args:
        cmd += f'\'{arg}\' '

    transport = client.get_transport()
    channel = transport.open_session()
    channel.exec_command(cmd)

    while True:
        if channel.exit_status_ready():
            break
        rl, _wl, _xl = select([channel], [], [])
        if len(rl) > 0:
            if channel.recv_ready():
                data = channel.recv(1024)
                os.write(sys.stdout.fileno(), data)

            if channel.recv_stderr_ready():
                data = channel.recv_stderr(1024)
                os.write(sys.stderr.fileno(), data)

if __name__ == '__main__':
    main()
