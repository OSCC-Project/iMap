import sys
import os
import time

from imap_engine import EngineIMAP

class Demo(object):
    '''
    A very simple demo.
    '''
    def __init__(self, input_file) -> None:
        self.input_file = input_file
        self.engine = EngineIMAP(input_file, input_file+'.seq')

    def _opt_size(self):
        self.engine.rewrite()
        self.engine.add_sequence('rewrite')
        self.engine.refactor(zero_gain=True)
        self.engine.add_sequence('refactor -z')

    def _opt_depth(self):
        self.engine.balance()
        self.engine.add_sequence('balance')

    def _opt_lut(self):
        self.engine.lut_opt()
        self.engine.add_sequence('lut_opt')

    def run(self):
        self.engine.read()
        opt_round = 5
        while opt_round > 0:
            self._opt_size()
            self._opt_depth()
            self.engine.print_stats()
            opt_round -= 1

        self.engine.map_fpga()
        self.engine.add_sequence('map_fpga')
        self.engine.print_stats(type=1)

        self.engine.write()


class HistoryDemo(Demo):
    '''
    A demo with history and choice mapping.
    '''
    def __init__(self, input_file, output_file) -> None:
        super().__init__(input_file, output_file)

    def _history_empty(self):
        return self.engine.history(size=True) == 0

    def _history_full(self):
        return self.engine.history(size=True) == 5

    def _history_add(self):
        self.engine.history(add=True)
        self.engine.add_sequence('history -a')

    def _history_replace(self, idx):
        self.engine.history(replace=idx)
        self.engine.add_sequence(f'history -r {idx}')

    def run(self):
        self.engine.read()
        opt_round = 10
        current_size  = self.engine.get_aig_size()
        current_depth = self.engine.get_aig_depth()
        while opt_round > 0:
            self._opt_size()
            self._opt_lut()
            self._opt_depth()
            size = self.engine.get_aig_size()
            depth = self.engine.get_aig_depth()
            if depth < current_depth:
                if self._history_full():
                    self._history_replace(-1)
                else:
                    self._history_add()
            elif depth == current_depth and size < current_size:
                if self._history_empty():
                    self._history_add()

            current_size = size
            current_depth = depth
            opt_round -= 1

        self.engine.map_fpga(type=1)
        self.engine.add_sequence('map_fpga')

        self.engine.write()


if __name__ == '__main__':
    d = Demo(sys.argv[1])
    d.run()
