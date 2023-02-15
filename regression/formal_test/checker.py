import os
import logging
import configparser
import shutil
import subprocess
import re
import time
import pathlib

class Subprocess(object):
    # A wrapper of subprocess.Popen

    @staticmethod
    def run(cmd):
        proc = subprocess.Popen(cmd, shell=True,
                                stdout=subprocess.PIPE,
                                stderr=subprocess.PIPE)
        try:
            start_time = time.time()
            stdout, stderr = proc.communicate(timeout=7200)
            stdout, stderr = stdout.decode(), stderr.decode()
            runtime = int((time.time() - start_time) * 1000000) # us
        except subprocess.TimeoutExpired:
            # kill subprocess with it's desendants
            ptree = subprocess.Popen(f'pstree -p {proc.pid}', shell=True,
                                     stdout=subprocess.PIPE,
                                     stderr=subprocess.PIPE).communicate()[0].decode()
            for m in re.finditer(r'\d+', ptree):
                cpid = int(m.group())
                try:
                    os.kill(cpid, 9)
                except:
                    pass
            print('Failed to run cmd "{}" with timeout'.format(cmd))
            stdout, stderr, runtime = '', '', 1

        # if stderr:
        #     print('Failed to run cmd "{}"'.format(cmd))
        #     print(stderr)
        # else:
        #     print(stdout)

        return stdout, stderr, runtime


class Case(object):
    def __init__(self, source, workspace):
        self.source = os.path.abspath(source)
        self.workspace = os.path.abspath(workspace)
        self.case_name = os.path.splitext(self.source)[0]
        self.ifpga = 'ifpga'


    def run_ifpga(self): 
        path = self.source.split('/');
        fname = path[ len(path)-1 ]
        choice_dir = pathlib.Path(self.source).parent;
        
        print("run case at " + self.source );
        
        self.ifpga_out = self.workspace + fname + ".ifpga.v"
        self.ifpga_lut = self.workspace + fname + ".ifpga.lut.v"
        self.ifpga_config = config.get('global', 'config_file')
        
        cmd = f'ifpga -i "{choice_dir}" -v "{self.ifpga_out}" -l "{self.ifpga_lut}" -c "{self.ifpga_config}"'
        stdout, stderr, runtime = Subprocess.run(cmd)
        
        if "unequivalent" in stdout:
            print(stdout)
            return choice_dir
        else:
            print("equivalent")
            return ""


def main(config_file):
    case_dir = config.get('global', 'cases')
    workspace = config.get('global', 'workspace')

    unequal_cases = []
    for root, _, files in os.walk(case_dir):
        for f in files:
            if not f.endswith('.aig'):
                continue
            if "original" not in f:
                continue
            source = os.path.join(root, f)            
            case = Case(source, workspace)
            equal_dir = case.run_ifpga()
            if equal_dir != "":
                unequal_cases.append(equal_dir);
    print("UNEQUIVALENT CASES ARE:")
    print(unequal_cases)
    
if __name__ == '__main__':
    import sys
    config = configparser.ConfigParser()
    config.read(sys.argv[1])

    main(sys.argv[1])
