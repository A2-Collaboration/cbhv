from telnetlib import Telnet
from time import sleep
import logging

debug = True
gains_file = 'HV_gains_offsets.txt'
host_prefix = 'cbhv%02d'

level = logging.INFO
if debug:
    level = logging.DEBUG

logging.basicConfig(format='%(asctime)s [%(levelname)s]: %(message)s', datefmt='%Y-%m-%d %I:%M:%S', level=level)

def print_telnet(str):
    cleaned = str.rstrip(b'\r\n>').decode('ascii').strip()
    if not cleaned:
        cleaned = 'empty'
    logging.debug('Telnet response: ' + cleaned)

logging.debug('Try to read file ' + gains_file)
gains = open(gains_file, 'r')
hv_gains = gains.readlines()
gains.close()
logging.debug('Successfully read %d lines from file' % len(hv_gains))

logging.debug('start loop')
for i in range(1, 19):
    host = host_prefix % i
    logging.info('Connecting to box ' + host)
    tn = Telnet(host)
    sleep(1)
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
        
    logging.debug('Send eemem protect')
    tn.write(b"eemem protect\r\n")
    response = tn.read_until(b'>')
    print_telnet(response)
    logging.debug('Send read_config')
    tn.write(b"read_config\r\n")
    response = tn.read_until(b'>', 30)
    print_telnet(response)
    logging.debug('Send eemem print')
    tn.write(b"eemem print\r\n")
    logging.info('eemem print returned:\n' + tn.read_until(b'>').decode('ascii'))
    logging.debug('Closing telnet connection to box ' + host)
    tn.close()

logging.info('Done!')

