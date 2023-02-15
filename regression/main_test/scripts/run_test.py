#!/usr/bin/env python3
import os
import configparser
from datetime import datetime
from multiprocessing import Pool

from utilities import *
from reporter import Reporter

class Tester(object):
    def __init__(self, config):
        self.current_time = datetime.now().strftime("%y%m%d-%H%M%S")
        self.config = os.path.abspath(config)

    def run_test(self, config):
        # read configuration and cases
        workspace = os.path.join(config.get('global', 'results'), self.current_time)
        workspace = os.path.abspath(workspace)

        os.makedirs(workspace, exist_ok=True)
        self.logger = config_logger(os.path.join(workspace, 'regression.log'))
        self.logger.info(f'create workspace: {workspace}')

        case_dir = os.path.abspath(config.get('global', 'case_dir'))

        tasks = []
        cases = []
        for f in os.listdir(case_dir):
            source = os.path.join(case_dir, f)
            if os.path.isdir(source):
                cases.append(source)
                cmd = f'python3 {os.path.dirname(os.path.realpath(__file__))}/case.py {source} {workspace} {self.config}'
                tasks.append([cmd, None])
        
        # run cases
        p = Pool()
        p.map(run_subprocess, tasks)

        # report
        self.reporter = Reporter(cases, workspace)
        rpt_str = self.reporter.report()
        self.logger.info(rpt_str)

        rpt_file = os.path.join(workspace, 'regression.rpt')
        self.logger.info(f'Finished to run all test, see "{rpt_file}" for report.')

if __name__ == '__main__':
    import sys
    config = configparser.ConfigParser()
    config.read(sys.argv[1])

    tester = Tester(sys.argv[1])
    tester.run_test(config)
