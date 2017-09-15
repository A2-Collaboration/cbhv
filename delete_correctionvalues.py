#!/usr/bin/env python3

'''
This script is used to set the correction values for the CB HV boxes to 0
To make the output less verbose, set debug to False
'''

from sys import exit
from telnetlib import Telnet
import logging

debug = True
host_prefix = 'cbhv%02d'

level = logging.INFO
if debug:
    level = logging.DEBUG

logging.basicConfig(format='%(asctime)s [%(levelname)s]: %(message)s', datefmt='%Y-%m-%d %I:%M:%S', level=level)

def print_telnet(str):
    """remove the telnet prompt from the string and print the output using logging.DEBUG level"""
    cleaned = str.rstrip(b'\r\n>').decode('ascii').strip()
    if not cleaned:
        cleaned = 'empty'
    logging.debug('Telnet response: ' + cleaned)

logging.debug('start looping over the boxes')
for i in range(1, 19):
    host = host_prefix % i
    logging.info('Connecting to box ' + host)
    tn = Telnet(host)
    response = tn.read_until(b'>')
    print_telnet(response)
    logging.debug('Send eemem unprotect')
    tn.write(b"eemem unprotect\r\n")
    response = tn.read_until(b'>')
    print_telnet(response)
    first_card = i*5-4
    # loop over cards per box
    for j in range(5):
        card = first_card + j
        logging.debug('Handling card %d' % card)
        m_cmd = "eemem add M%d 0,0,0,0,0,0,0,0\r\n" % (j)
        n_cmd = "eemem add N%d 0,0,0,0,0,0,0,0\r\n" % (j)
        logging.debug('Issue command: ' + m_cmd.strip())
        tn.write(m_cmd.encode('ascii'))
        response = tn.read_until(b'>')
        print_telnet(response)
        logging.debug('Issue command: ' + n_cmd.strip())
        tn.write(n_cmd.encode('ascii'))
        response = tn.read_until(b'>')
        print_telnet(response)
        
    # deactivate correction loop
    logging.debug('Send eemem add REG off')
    tn.write(b"eemem add REG off\r\n")
    response = tn.read_until(b'>')
    print_telnet(response)
    
    # finished setting the values for the different channels per card, now store them
    logging.debug('Send eemem protect')
    tn.write(b"eemem protect\r\n")
    response = tn.read_until(b'>')
    print_telnet(response)
    logging.debug('Send read_config')
    tn.write(b"read_config\r\n")
    response = tn.read_until(b'>', 40)
    print_telnet(response)
    logging.debug('Send eemem print')
    tn.write(b"eemem print\r\n")
    logging.info('eemem print returned:\n' + tn.read_until(b'>').decode('ascii'))
    logging.debug('Closing telnet connection to box ' + host)
    tn.close()

logging.info('Done!')

