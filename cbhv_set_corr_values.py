#!/usr/bin/env python3

'''
This script is used to set the correction values for the CB HV boxes
The values are read from a file specified in the beginning of the script
To make the output less verbose, set debug to False
'''

from sys import exit
from time import sleep
from telnetlib import Telnet
import logging

debug = True
gains_file = 'HV_gains_offsets.txt'
host_prefix = 'cbhv%02d'

level = logging.INFO
if debug:
    level = logging.DEBUG

logging.basicConfig(format='%(asctime)s [%(levelname)s]: %(message)s', datefmt='%Y-%m-%d %I:%M:%S', level=level)

def print_telnet(string):
    """remove the telnet prompt from the string and print the output using logging.DEBUG level"""
    cleaned = string.rstrip(b'\r\n>').decode('ascii').strip()
    if not cleaned:
        cleaned = 'empty'
    logging.debug('Telnet response: ' + cleaned)

def send_telnet_command(tn, cmd):
    logging.debug('Send ' + cmd.rstrip('\r\n'))
    if not cmd.endswith('\r\n'):
        cmd += '\r\n'
    tn.write(cmd.encode('ascii'))
    try:
        response = tn.read_until(b'>')
    except EOFError:
        logging.error('Telnet connection closed while trying to send command ' + cmd.rstrip('\r\n'))
        return False
    print_telnet(response)
    return True

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
    # telnet needs some time...
    sleep(1)
    response = tn.read_until(b'>')
    print_telnet(response)
    if not send_telnet_command(tn, 'eemem unprotect'):
        logging.warning('Box %s may be dead, continue with next one' % host)
        continue
    first_card = i*5-4
    logging.info('Start setting correction values, this may take 2 minutes or longer')
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
        if not send_telnet_command(tn, m_cmd):
            logging.warning('Box %s may be dead, continue with next one' % host)
            continue
        if not send_telnet_command(tn, n_cmd):
            logging.warning('Box %s may be dead, continue with next one' % host)
            continue
        
    # activate correction loop
    if not send_telnet_command(tn, 'eemem add REG on'):
        logging.warning('Box %s may be dead, continue with next one' % host)
        continue
    
    # finished setting the values for the different channels per card, now store them
    if not send_telnet_command(tn, 'eemem protect'):
        logging.warning('Box %s may be dead, continue with next one' % host)
        continue
    if not send_telnet_command(tn, 'read_config'):
        logging.warning('Box %s may be dead, continue with next one' % host)
        continue
    logging.debug('Send eemem print')
    tn.write(b"eemem print\r\n")
    try:
        logging.info('eemem print returned:\n' + tn.read_until(b'>').rstrip(b'\r\n>').decode('ascii'))
    except EOFError:
        logging.warning("Box %s didn't respond after sending eemem print, go to next box" % host)
        continue
    logging.debug('Closing telnet connection to box ' + host)
    tn.close()

logging.info('Done!')

