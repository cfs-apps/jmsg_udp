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
    
    for i in range(1,5):
        jmsg = 'basecamp/test:{"int32": %i,"float": %f}' % (i, float(i)*1.3)
        print(f'>>> Sending message {jmsg}')
        sock.sendto(jmsg.encode('ASCII'), (cfs_ip_addr, cfs_app_port))
        time.sleep(2)

def rx_thread():
    time.sleep(2)

    
if __name__ == "__main__":

    config = configparser.ConfigParser()
    config.read('jmsg_udp.ini')

    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    cfs_ip_addr  = config.get('NETWORK','CFS_IP_ADDR')
    cfs_app_port = config.getint('NETWORK','CFS_APP_PORT')
    
    tx = threading.Thread(target=tx_thread)
    tx.start()
        
