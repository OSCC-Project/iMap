import sys
import os
import time

from imap_engine import EngineIMAP

class TestEngineIMAP:
    def __init__(self, folder):
        self.folder = folder

    def log_case_time(self, file_aig, file_seq):
        engine = EngineIMAP(file_aig, file_seq)
        
        curr_time = time.time()
        print("read", end=' ')
        engine.read()
        end_time = time.time()
        time_read = end_time - curr_time
        
        curr_time = time.time()
        print("rewrite", end=' ')
        engine.rewrite()
        end_time = time.time()
        time_rewrite = end_time - curr_time
        
        curr_time = time.time()
        # print("refactor", end=' ')
        # engine.refactor()
        end_time = time.time()
        time_refactor = end_time - curr_time
        
        curr_time = time.time()
        print("balance", end=' ')
        engine.balance()
        end_time = time.time()
        time_balance = end_time - curr_time
        
        curr_time = time.time()
        print("lut_opt", end=' ')
        engine.lut_opt()
        end_time = time.time()
        time_lut_opt= end_time - curr_time
        
        curr_time = time.time()
        print("map_fpga")
        engine.map_fpga()
        end_time = time.time()
        time_map_fpga = end_time - curr_time
        
        print(file_aig, time_read, time_rewrite, time_refactor, time_balance, time_lut_opt, time_map_fpga)
        
    def run(self):
        aig_files = []
        for root, dirs, files in os.walk(self.folder):
            for file in files:
                if file.endswith(".aig"):
                    aig_files.append(os.path.join(root, file))

        for file in aig_files:
            print("Process at:" + file)
            self.log_case_time(file, "tmp.seq")
            
# Usage: python test_time.py <folder>
folder = sys.argv[1]
TestEngineIMAP(folder).run()