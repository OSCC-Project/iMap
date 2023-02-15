
"""Here are utilities for daily regression."""

import subprocess
import os
import logging
import sys

def config_logger(logfile):
    logger = logging.getLogger('root')
    logger.setLevel(logging.DEBUG)
    fmt = logging.Formatter('[%(asctime)s]%(filename)s:%(lineno)d:[%(levelname)s]: %(message)s')

    stdout_handler = logging.StreamHandler(sys.stdout)
    stdout_handler.setLevel(logging.INFO)
    stdout_handler.addFilter(lambda x: x.levelno <= logging.INFO)
    stdout_handler.setFormatter(fmt)

    stderr_handler = logging.StreamHandler(sys.stderr)
    stderr_handler.setLevel(logging.ERROR)
    stderr_handler.setFormatter(fmt)

    file_handler = logging.FileHandler(logfile)
    file_handler.setFormatter(fmt)

    logger.addHandler(stdout_handler)
    logger.addHandler(stderr_handler)
    logger.addHandler(file_handler)
    return logger

def run_subprocess(arg):
    """ A wrapper of subprocess.Popen """
    cmd, logger = arg
    p = subprocess.Popen(cmd, shell=True, 
                        stdout=subprocess.PIPE,
                        stderr=subprocess.PIPE)
    stdout, stderr = p.communicate()
    stdout, stderr = stdout.decode(), stderr.decode()
    if logger:
        if stderr:
            logger.error(stderr)
            logger.error(f'Failed to run cmd: {cmd}')
        else:
            logger.debug(stdout)
            logger.info(f'Finished to run cmd: {cmd}')
    else:
        print(stdout)
        print(stderr)
    return p.returncode 
