#!/usr/bin/env python3
# vim: set ai ts=4 sw=4 sts=4 noet fileencoding=utf-8 ft=python

'''
This Python script is intended to set gain correction values for
the CBHV boxes. The values to be set can be either given by file
or just setting all values to 0.
'''

import sys
import os
import errno
import argparse
from os.path import abspath, dirname, join as pjoin
import logging
# import own modules
# helper for colored output
from modules.color import print_color, print_error, ColoredLogger
# small helper class for telnet connections
from modules.telnet_manager import TelnetManager


# check if the installed Python version is at least 3.6
# the usage of Telnet as a context manager was added in 3.6, which is used in this script
if sys.hexversion < 0x3060000:
    print_error('At least Python 3.6 is required to run this script')
    sys.exit(1)

def check_path(path, create=False, write=True):
    """Check if given path exists and is readable as well as writable if specified;
    if create is true and the path doesn't exist, the directories will be created if possible"""
    path = os.path.expanduser(path)
    exist = os.path.isdir(path)
    if not exist and create:
        print("Directory '%s' does not exist, it will be created now" % path)
        # try to create the directory; if it should exist for whatever reason,
        # ignore it, otherwise report the error
        try:
            os.makedirs(path)
        except OSError as exception:
            if exception.errno == errno.EACCES:
                print_error("[ERROR] You don't have the permission to create directories in '%s'"
                            % dirname(path))
                return False
            elif exception.errno != errno.EEXIST:
                raise
        return True
    elif not exist:
        print_error("[ERROR] Directory '%s' does not exist" % path)
        return False
    else:
        if not is_readable(path):
            print_error("[ERROR] Directory '%s' is not readable" % path)
            return False
        if write and not is_writable(path):
            print_error("[ERROR] Directory '%s' is not writable" % path)
            return False
    return True

def check_file(path, file):
    """Check if a file in a certain path exists and is readable;
    if the file argument is omitted only path is checked, it's recommended to use
    check_path instead for this case"""
    path = os.path.expanduser(path)
    if file is None:
        if not os.path.isfile(path):
            print_error("[ERROR] The file '%s' does not exist!" % (path))
            return False
        else:
            if not is_readable(path):
                print_error("[ERROR] The file '%s' is not readable!" % (path))
                return False
            return True
    path = get_path(path, file)
    if not os.path.isfile(path):
        print_error("[ERROR] The file '%s' does not exist!" % path)
        return False
    else:
        if not is_readable(path):
            print_error("[ERROR] The file '%s' is not readable!" % (path))
            return False
        return True

def check_permission(path, permission):
    """Check if the given permission is allowed for path"""
    if os.path.exists(path):
        return os.access(path, permission)
    return False

def is_readable(path):
    """Check if path is readable"""
    return check_permission(path, os.R_OK)

def is_writable(path):
    """Check if path is writable"""
    return check_permission(path, os.W_OK)

def is_executable(path):
    """Check if path is executable"""
    return check_permission(path, os.X_OK)

def get_path(path, file=None):
    """Get the absolute path of a given path and an optional file"""
    if not file:
        return abspath(os.path.expanduser(path))
    return abspath(os.path.expanduser(pjoin(path, file)))


def check_directory(path, force, verbose, relative=None, write=True):
    """Check if a given (relative) directory exists and is writable, update settings accordingly"""
    # check if the given output directory exists
    if verbose:
        print('Check the specified directory %s' % path)
    if relative is not None:
        path = get_path(relative, path)
    else:
        path = get_path(path)
    if not check_path(path, force, write):
        if verbose and not force:
            print('        Please make sure the specified directory exists.')
        return False
    if verbose:
        print('Path found:', path)

    return True


def set_values(logger, host_prefix, hv_gains=[], reset=False):
    """
    This method is used to either reset the HV boxes HV gains to zero
    or write calibrated values to them, given as a list of lines from a file provided earlier
    """
    if not hv_gains and not reset:
        logger.error("No HV gains given and no reset of values specified")
        return False

    # start connecting to the boxes
    for i in range(1, 19):
        host = host_prefix % i
        logger.info('Connecting to box ' + host)
        with TelnetManager(host, logger=logger) as tnm:
            if not tnm.send_command('eemem unprotect'):
                logger.warning('Box %s may be dead, continue with next one' % host)
                continue
            first_card = i*5-4
            logger.info('Start setting correction values, this may take 2 minutes or longer')
            # loop over cards per box
            for j in range(5):
                card = first_card + j
                logger.debug('Handling card %d' % card)
                m_vals, n_vals = [], []
                m_cmd, n_cmd = '', ''
                # loop over channels per card and read the values if they should not be set to 0
                if not reset:
                    for channel in range(8):
                        line = next((i for i in hv_gains if i.startswith("%d,%d" % (card, channel))), None)
                        if not line:
                            logger.error("No values found for card %d, channel %d" % (card, channel))
                            continue
                        vals = line.strip().split(',')[-2:]
                        m_vals.append(vals[0])
                        n_vals.append(vals[1])

                    if len(m_vals) is not 8 or len(n_vals) is not 8:
                        logger.error("Card %d problem parsing values!" % card)
                        continue
                    m_cmd = "eemem add M%d %s\r\n" % (j, ','.join(m_vals))
                    n_cmd = "eemem add N%d %s\r\n" % (j, ','.join(n_vals))
                else:
                    m_cmd = "eemem add M%d %s\r\n" % (j, ','.join('0'*8))
                    n_cmd = "eemem add N%d %s\r\n" % (j, ','.join('0'*8))

                if not tnm.send_command(m_cmd):
                    logger.warning('Box %s may be dead, continue with next one' % host)
                    continue
                if not tnm.send_command(n_cmd):
                    logger.warning('Box %s may be dead, continue with next one' % host)
                    continue

            if reset:
                # deactivate correction loop while setting zeros
                if not tnm.send_command('eemem add REG off'):
                    logger.warning('Box %s may be dead, continue with next one' % host)
                    continue
            else:
                # activate correction loop
                if not tnm.send_command('eemem add REG on'):
                    logger.warning('Box %s may be dead, continue with next one' % host)
                    continue

            # finished setting the values for the different channels per card, now store them
            if not tnm.send_command('eemem protect'):
                logger.warning('Box %s may be dead, continue with next one' % host)
                continue
            if not tnm.send_command('read_config'):
                logger.warning('Box %s may be dead, continue with next one' % host)
                continue
            logger.debug('Send eemem print')
            logger.info('eemem print returned the following:')
            if not tnm.send_command("eemem print", print_info=True):
                logger.warning("Box %s didn't respond after sending eemem print, go to next box" % host)
                continue
            logger.debug('Closing telnet connection to box ' + host)
        logger.debug('Telnet connection closed')

    logger.info('Done')

    return True

def measure_values(logger, host_prefix, output):
    """
    This method performs a measurement of the CB HV correction values
    and stores the results in a separate file per card
    """
    if not output:
        logger.error("No output given")
        return False


def is_valid_file(parser, arg):
    """Helper function for argparse to check if a file exists"""
    if not os.path.isfile(arg):
        parser.error('The file %s does not exist!' % arg)
    else:
        #return open(arg, 'r')
        return arg

def is_valid_dir(parser, arg):
    """Helper function for argparse to check if a directory exists"""
    if not os.path.isdir(os.path.expanduser(arg)):
        parser.error('The directory %s does not exist!' % arg)
    else:
        return os.path.expanduser(arg)


def main():
    """Main function: process all information, prepare simulation process, submit jobs"""
    parser = argparse.ArgumentParser(description='Set correction values for CBHV boxes '
                                     'via a given file, by default HV_gains_offsets.txt')
    parser.add_argument('-i', '--input', nargs=1, metavar='corr_file',
                        dest='corr_file',# required=True,
                        type=lambda x: is_valid_file(parser, x),
                        help='Optional: Specify a custom correction file; '
                        'default will be HV_gains_offsets.txt in the current directory')
    parser.add_argument('-o', '--output', nargs=1, metavar='output_directory',
                        #type=lambda x: is_valid_dir(parser, x),
                        type=str, help='Optional: Custom output directory')
    parser.add_argument('-p', '--prefix', nargs=1, type=str,
                        dest='host_prefix', metavar='"host prefix"',
                        help='Specify a different hostname scheme for the CBHV boxes, '
                        'including formatting for digits, like "cbhv%%02d"')
    parser.add_argument('-r', '--reset', action='store_true',
                        help='Reset correction values to 0')
    parser.add_argument('-c', '--calibrate', action='store_true',
                        help='Measure correction values')
    parser.add_argument('-n', '--name', nargs=1, type=str,
                        dest='out_file', metavar='"output file name"',
                        help='Specify the output file name for the correction values '
                        'if -c/--calibrate is used including formatting, like "karte%%04d.txt". '
                        'Output path can be changed with -o/--output.')
    parser.add_argument('-f', '--force', action='store_true',
                        help='Force creation of directories or other minor errors '
                        'which otherwise terminate the program')
    parser.set_defaults(reset=False)
    parser.set_defaults(calibrate=False)
    parser.set_defaults(force=False)
    parser.add_argument('-v', '--verbose', action='store_true',
                        help='Print additional output')

    logging.setLoggerClass(ColoredLogger)
    logger = logging.getLogger('CBHV')

    args = parser.parse_args()
    verbose = args.verbose
    reset = args.reset
    calibrate = args.calibrate
    if verbose:
        logger.setLevel(logging.DEBUG)
    gains_file = 'HV_gains_offsets.txt'
    hv_gains = []
    host_prefix = 'cbhv%02d'
    output = '.'
    out_file = 'karte%04d.txt'
    force = args.force

    if args.host_prefix:
        host_prefix = args.host_prefix[0]
        logger.info('Set custom host prefix to "%s"', host_prefix)

    if not calibrate:
        logger.info('Checking arguments for setting CB HV values . . .')
        if reset and args.corr_file:
            logger.warning('Reset issued and custom correction file given, '
                           'correction values from file will be ignored!')

        if args.corr_file:
            gains_file = args.corr_file[0]
            logger.info('Set custom gain correction file to %s', gains_file)

        if not reset:
            # read values from given file
            logger.debug('Try to read file ' + gains_file)
            with open(gains_file, 'r') as gains:
                hv_gains = gains.readlines()
            if not hv_gains:
                logger.error('No lines read from file %s, please check the provided file', gains_file)
                sys.exit(1)
            logger.info('Successfully read %d lines from file %s', len(hv_gains), gains_file)
    else:
        logger.info('Preparing measurement of CB HV correction values . . .')
        # check if the output path exists and is writable
        if args.output:
            if not check_path(args.output[0], force, write=True):
                sys.exit('The output directory %s cannot be used' % args.output[0])
            output = get_path(args.output[0])
            logger.info('Setting custom output directory: %s' % output)
        if args.out_file:
            out_file = args.out_file[0]
            logger.info('Set custom output file formatting to "%s"', out_file)
        # create full output path string
        output = get_path(output, out_file)
        logger.info('The following path and naming scheme will be used for the files: %s' % output)


    print_color('Start connecting to the CBHV boxes', 'GREEN')

    if not calibrate:
        if not set_values(logger, host_prefix, hv_gains, reset):
            sys.exit('Failed setting CB HV values')
    else:
        if not measure_values(logger, host_prefix, output):
            sys.exit('Failed measuring CB HV correction values')

    print_color('Done!', 'GREEN')


if __name__ == '__main__':
    try:
        main()
    except KeyboardInterrupt:
        print('\nCtrl+C detected, terminating program')
        sys.exit(0)
    except Exception as e:
        print_error('An error occured during execution:')
        print(e)
        if '-v' or '--verbose' in sys.argv:
            raise e
        sys.exit(1)
