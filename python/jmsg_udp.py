"""
    Copyright 2022 bitValence, Inc.
    All Rights Reserved.

    This program is free software; you can modify and/or redistribute it
    under the terms of the GNU Affero General Public License
    as published by the Free Software Foundation; version 3 with
    attribution addendums as found in the LICENSE.txt.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    Purpose:
      TODO
    
    Notes:
      None

"""

import configparser
import socket
import threading
import time

def tx_thread():
    
    for i in range(1,2):
        floati = float(i)
        jmsg = 'basecamp/test:{"int32": %i,"float": %f}' % (i, floati*1.3)
        print(f'>>> Sending message {jmsg}')
        sock.sendto(jmsg.encode('ASCII'), (cfs_ip_addr, cfs_app_port))
        time.sleep(10)
        jmsg = 'basecamp/rpi/demo:{"rpi-demo":{"rate-x": %f, "rate-y": %f, "rate-z": %f, "lux": %i}}' % (floati*1.0,floati*2.0,floati*3.0,i)
        print(f'>>> Sending message {jmsg}')
        sock.sendto(jmsg.encode('ASCII'), (cfs_ip_addr, cfs_app_port))
        
def rx_thread():

    rx_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    rx_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    rx_socket.bind((cfs_ip_addr, py_app_port))
    rx_socket.setblocking(False)
    rx_socket.settimeout(1000)

    while True:    
        try:
            while True:
                datagram, host = rx_socket.recvfrom(1024)
                datastr = datagram.decode('utf-8')
                print(f'Received from {host} jmsg size={len(datastr)}: {datastr}')
        except socket.timeout:
            pass
        time.sleep(2)

    
if __name__ == "__main__":

    config = configparser.ConfigParser()
    config.read('jmsg_udp.ini')

    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    cfs_ip_addr  = config.get('NETWORK','CFS_IP_ADDR')
    cfs_app_port = config.getint('NETWORK','CFS_APP_PORT')
    py_app_port = config.getint('NETWORK','PY_APP_PORT')

    rx = threading.Thread(target=rx_thread)
    rx.start()
   
    #tx = threading.Thread(target=tx_thread)
    #tx.start()
        
