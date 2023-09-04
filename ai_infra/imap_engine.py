#!/usr/bin/env python
# -*- encoding: utf-8 -*-
from lib import imap

class EngineIMAP():
    """ IMAP engine for python usages
    
    IO functions:
    read
    write
    
    Logic optimization functions:
    rewrite
    refactor
    balance
    lut_opt
    history
    
    Technology mapping functions:
    map_fpga
    
    Utils:
    add_sequence
    cleanup
    print_stats
    
    """
    def __init__(self, input_file, output_file):
        self.input_file = input_file
        self.output_file = output_file
        
        # command names
        self.srewrite = "rewrite"
        self.sbalance = "balance"
        self.srefactor = "refactor"
        self.slut_opt = "lut_opt"
        self.smap_fpga = "map_fpga"
        self.shistory = "history"

        # store the optimization sequence
        self.sequence = []
    
    # read the AIG file
    def read(self):
        """ Read the AIG file
        """
        imap.read_aiger(filename = self.input_file)
        
    # write the FPGA verilog file
    def write(self):
        """ Write the sequence file
        """
        with open(self.output_file, 'w') as file:
            for command in self.sequence:
                file.write(command + '; ')
    
    def add_sequence(self, str):
        """ Add the command into a sequence vector for output

        Args:
            str (_type_): The command with or without args.
        """
        self.sequence.append(str)
    
    def rewrite(self, priority_size = 10, cut_size = 4, level_preserve = True, zero_gain = False, verbose = False):
        """ Logic optimization by "rewrite" algorithm, mainly for area reduction

        Args:
            priority_size (int, optional): The max store size of the cut for a node. Range of [6,20]. Defaults to 10.
            cut_size (int, optional): The max input size of a cut. Range of [2,4]. Defaults to 4.
            level_preserve (bool, optional): Preserve depth after optimization. Defaults to True.
            zero_gain (bool, optional): Allow zero-cost-based local replacement. Defaults to False.
            verbose (bool, optional): Verbose report. Defaults to False.
        """
        imap.rewrite(priority_size = priority_size, cut_size = cut_size, level_preserve = level_preserve, zero_gain = zero_gain, verbose = verbose)
        
    def refactor(self, max_input_size = 10, max_cone_size = 16, level_preserve = True, zero_gain = False, verbose = False):
        """ Logic optimization by "refactor" algorithm, area/depth reduction

        Args:
            max_input_size (int, optional): The max input size of a reconvergence cone. Range of [6,12]. Defaults to 10.
            max_cone_size (int, optional): The max node size in a cone. Range of [10,20]. Defaults to 16.
            level_preserve (bool, optional): Preserve depth after optimization. Defaults to True.
            zero_gain (bool, optional): Allow zero-cost-based local replacement. Defaults to False.
            verbose (bool, optional): Verbose report. Defaults to False.
        """
        imap.refactor(max_input_size = max_input_size, max_cone_size = max_cone_size, level_preserve = level_preserve, zero_gain = zero_gain, verbose = verbose)
        
    def balance(self, verbose = False):
        """ Logic optimization by "balance" algorithm, mainly for depth reduction

        Args:
            verbose (bool, optional): Verbose report. Defaults to False.
        """
        imap.balance(verbose = verbose)
        
    def lut_opt(self, priority_size = 10, cut_size = 6, global_area_iterations = 1, local_area_iterations = 2, zero_gain = False, verbose = False):
        """ Logic optimization by "lut_opt" algorithm, area/depth reduction

        Args:
            priority_size (int, optional): The max store size of the cut for a node. Range of [6,20]. Defaults to 10.
            cut_size (int, optional): The max input size of a cut. Range of [2,6]. Defaults to 4.
            global_area_iterations (int, optional): Set the iteration numter for local area-based post-optimization. Range of [1,2]. Defaults to 1.
            local_area_iterations (int, optional): Set the iteration numter for local area-based post-optimization. Range of [1,3]. Defaults to 2.
            zero_gain (bool, optional): Allow zero-cost-based local replacement. Defaults to False.
            verbose (bool, optional): Verbose report. Defaults to False.
        """
        imap.lut_opt(priority_size = priority_size, cut_size = cut_size, global_area_iterations = global_area_iterations, local_area_iterations = local_area_iterations, zero_gain = zero_gain, verbose = verbose)

    def history(self, clear = False, size = False, add = False, replace = -1):
        """ Manipulation for the history stored AIG 

        Args:
            clear (bool, optional): Clear the history stored AIG. Defaults to False.
            size (bool, optional): Report the size of history stored AIG. Defaults to False.
            add (bool, optional): Add current AIG into the history AIG. Defaults to True.
            replace (int, optional): Replace the given indexed history AIG by current AIG. Defaults to -1.
        """
        imap.history(clear = clear, size = size, add = add, replace = replace)

    def map_fpga(self, priority_size = 10, cut_size = 6, global_area_iterations = 1, local_area_iterations = 1, type = 0, verbose = False):
        """ Technology mapping for FPGA

        Args:
            priority_size (int, optional): The max store size of the cut for a node. Range of [6,20]. Defaults to 10.
            cut_size (int, optional): The max input size of a cut. Range of [2,6]. Defaults to 6.
            global_area_iterations (int, optional): Set the iteration numter for local area-based post-optimization. Range of [1,2]. Defaults to 1.
            local_area_iterations (int, optional): Set the iteration numter for local area-based post-optimization. Range of [1,3]. Defaults to 2.
            type (int, optional): Mapping with/without history AIG, 0 means mapping without history AIG, 1 means mapping with history AIG. Defaults to 0.
            verbose (bool, optional): Verbose report. Defaults to False.
        """
        imap.map_fpga(priority_size = priority_size, cut_size = cut_size, global_area_iterations = global_area_iterations, local_area_iterations = local_area_iterations, type = type, verbose = verbose)   

    def clean_up(self, verbose = False):
        """ Clean up the dangling nodes

        Args:
            verbose (bool, optional): Verbose report. Defaults to False.
        """
        imap.cleanup(verbose = verbose)

    def print_stats(self, type = 0):
        """ Print the profile of a network

        Args:
            type (int, optional): 0 means print the stats of current AIG-netowork, while 1 means LUT-network. Range of [0,1]. Defaults to 0.
        """
        imap.print_stats(type = type)
