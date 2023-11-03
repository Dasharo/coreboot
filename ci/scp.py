#!/usr/bin/env python3

from argparse import ArgumentParser
import paramiko

def main():
    parser = ArgumentParser()
    parser.add_argument("host")
    parser.add_argument("local")
    parser.add_argument("remote")
    args = parser.parse_args()

    client = paramiko.client.SSHClient()
    client.set_missing_host_key_policy(paramiko.AutoAddPolicy())
    client.connect(args.host, username="root", password="meta-rte")

    ftp = client.open_sftp()
    ftp.put(args.local, args.remote)
    ftp.close()

if __name__ == '__main__':
    main()
