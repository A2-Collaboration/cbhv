#!/usr/bin/env python3

'''
This script is used to set the correction values for the CB HV boxes
The values are read from a file specified in the beginning of the script
To make the output less verbose, set debug to False
'''

from sys import exit
from telnetlib import Telnet
import logging

debug = True
gains_file = 'HV_gains_offsets.txt'
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

logging.debug('Try to read file ' + gains_file)
with open(gains_file, 'r') as gains:
    hv_gains = gains.readlines()
if not len(hv_gains):
    logging.error('No lines read from file %s, please check the provided file' % gains_file)
    exit(1)
logging.debug('Successfully read %d lines from file' % len(hv_gains))

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
        m_values = []
        n_values = []
        # loop over channels per card
        for channel in range(8):
            line = next((i for i in hv_gains if i.startswith("%d,%d" % (card, channel))), None)
            if not line:
                log.error("No values found for card %d, channel %d" % (card, channel))
                continue
            vals = line.strip().split(',')[-2:]
            m_values.append(vals[0])
            n_values.append(vals[1])

        if len(m_values) is not 8 or len(n_values) is not 8:
            log.error("Card %d problem parsing values!" % card)
            continue
        m_cmd = "eemem add M%d %s\r\n" % (j, ','.join(m_values))
        n_cmd = "eemem add N%d %s\r\n" % (j, ','.join(n_values))
        logging.debug('Issue command: ' + m_cmd.strip())
        tn.write(m_cmd.encode('ascii'))
        response = tn.read_until(b'>')
        print_telnet(response)
        logging.debug('Issue command: ' + n_cmd.strip())
        tn.write(n_cmd.encode('ascii'))
        response = tn.read_until(b'>')
        print_telnet(response)
        
    # activate correction loop
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

