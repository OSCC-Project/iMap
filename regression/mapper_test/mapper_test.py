import os
import logging
import configparser
import shutil
import subprocess
import re
import time

def config_logger(workspace):
    logger = logging.getLogger('root')
    logger.setLevel(logging.INFO)
    fmt = logging.Formatter('[%(asctime)s]%(filename)s:%(lineno)d:[%(levelname)s]: %(message)s')
    file_handler  = logging.FileHandler(os.path.join(workspace, 'test.log'),  mode='w+')
    stream_handler = logging.StreamHandler()
    stream_handler.setFormatter(fmt)
    stream_handler.setLevel(logging.INFO)
    file_handler.setFormatter(fmt)
    logger.addHandler(stream_handler)
    logger.addHandler(file_handler)
    return logger


class Subprocess(object):
    # A wrapper of subprocess.Popen

    @staticmethod
    def run(cmd, logger):
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
            logger.error('Failed to run cmd "{}" with timeout'.format(cmd))
            stdout, stderr, runtime = '', '', 1

        if stderr:
            logger.error('Failed to run cmd "{}"'.format(cmd))
            logger.critical(stderr)
        else:
            logger.debug(stdout)

        return stdout, stderr, runtime


class Case(object):
    def __init__(self, source, workspace):
        self.source = os.path.abspath(source)
        self.workspace = os.path.abspath(workspace)

        self.case_name = os.path.split(os.path.splitext(self.source)[0])[1]
        self.case_dir = os.path.join(self.workspace, self.case_name)
        os.makedirs(self.case_dir, exist_ok=True)

        shutil.copy(self.source, self.case_dir)
        self.source = os.path.join(self.case_dir, os.path.basename(self.source))
        # logger
        logfile = os.path.join(self.workspace, self.case_name, self.case_name+'.log')
        self.logger = logging.getLogger('root.'+self.case_name)
        self.logger.setLevel(logging.DEBUG)
        fmt = logging.Formatter('[%(asctime)s]%(filename)s:%(lineno)d:[%(levelname)s]: %(message)s')
        file_handler = logging.FileHandler(logfile, mode='a')
        file_handler.setFormatter(fmt)
        self.logger.addHandler(file_handler)

        self.logger.info('run case "{}"'.format(self.case_name))
        # files
        #self.lut_lib = os.path.abspath(config.get('global', 'lut_lib'))
        #self.abc = os.path.abspath(config.get('global', 'bin') + '/abc')
        self.abc = 'abc'
        self.ifpga = 'ifpga'
        self.yosys = 'yosys'

    def run_abc(self):
        self.__run_abc_opt()
        return self.__run_abc_mapping()

    def __run_abc_opt(self):
        # abc flow
        fname = os.path.splitext(self.source)[0]
        self.opt_aig = fname + ".opt.aig"

        if config.getboolean('global', 'quick_run'):
            self.logger.info("skip ABC opt...")
            return
        self.logger.info("run ABC opt...")
        # abc cmd
        # run abc
        opt_cmd = config.get('global', 'opt_cmd').strip('"')
        abc_cmd = (f'&read {self.source}; '
                   f'{opt_cmd}; '
                   f'&write {self.opt_aig}; '
        )
        cmd = '{} -c "{}"'.format(self.abc, abc_cmd)
        cwd = os.getcwd()
        os.chdir(self.case_dir)
        stdout, _, _ = Subprocess.run(cmd, self.logger)
        os.chdir(cwd)
        return stdout

    def __run_abc_mapping(self):
        fname = os.path.splitext(self.source)[0]
        self.abc_out = fname + ".abc.v"
        lut_input = config.getint('global', 'K', fallback=6)
        lut_lib = os.path.abspath(config.get('global', 'lut_lib'))

        self.logger.info("run abc mapping...")
        abc_cmd = (f'&read {self.opt_aig}; '
                   f'read_lut {lut_lib}; '
                   #f'balance; rewrite; rewrite -z; balance;'
                   f'&if -v -K {lut_input}; &put; sweep;'
                   f'write_verilog {self.abc_out}; '
                   f'print_gates; print_level')

        # run abc
        cmd = '{} -c "{}"'.format(self.abc, abc_cmd)
        stdout, _, runtime = Subprocess.run(cmd, self.logger)

        return self.__extract_abc_report(stdout) + [runtime]

    def __extract_abc_report(self, rpt):
        lut_nums, area = self.__lut_statistics()

        level_pat = r'Level\s*=\s*(\d+)'
        level = 0
        for m in re.finditer(level_pat, rpt):
            level = max(int(m.group(1)), level)
        self.logger.info('area: {} level: {}'.format(area, level))

        mem_pat = r'Peak memory: (\d+) bytes'
        m = re.search(mem_pat, rpt)
        memory = m.group(1)

        time_pat = r'Total time =\s*([\d\.]+)'
        m = re.search(time_pat, rpt)
        runtime = m.group(1)

        return [lut_nums, str(area), str(level), memory, runtime]

    def __lut_statistics(self):
        with open(self.abc_out) as f:
            lines = f.readlines()
        lut_input = config.getint('global', 'K', fallback=6)
        lut_nums = [0] * lut_input

        simple_id = r'[a-zA-Z_][\w\$]*'
        escaped_id = r'\\.+?\s'
        identifier = re.compile(simple_id + '|' + escaped_id)
        for line in lines:
            if 'assign' not in line: continue
            # remove const
            if "1'b" in line: continue
            expr = line.split('=')[-1]
            inputs = re.findall(identifier, expr)
            input_num = len(set(inputs))
            # remove buffer
            if input_num == 1 and '~' not in expr: continue
            lut_nums[input_num - 1] += 1

        area = sum(lut_nums)
        if lut_input == 7:
            area += lut_nums[-1]
        return [[str(i) for i in lut_nums], area]

    def run_ifpga(self):
        #
        fname = os.path.splitext(self.source)[0]
        self.ifpga_out = fname + ".ifpga.v"
        self.ifpga_lut = fname + ".ifpga.lut.v"
        self.ifpga_config = config.get('global', 'config_file')

        self.logger.info("run iFPGA flow...")
        #cmd = f'ifpga -i "{self.opt_aig}" -c "{self.ifpga_config}" -v "{self.ifpga_out}"'
        cmd = f'ifpga -i "{self.case_dir}" -c "{self.ifpga_config}" -v "{self.ifpga_out}"'
        stdout, stderr, runtime = Subprocess.run(cmd, self.logger)
        if stderr:
            return []
        return self.__extract_ifpga_report(stdout) + [runtime]

    def __extract_ifpga_report(self, rpt):
        lut_input = config.getint('global', 'K', fallback=6)
        lut_nums = [0] * lut_input
        for i in range(lut_input):
            lut_pat = r'LUT fanins:{}\s*numbers\s*:(\d+)'.format(i + 1)
            lut_m = re.search(lut_pat, rpt)
            if lut_m:
                lut_nums[i] = int(lut_m.group(1))

        level_pat = r'max delay\s*:\s*(\d+)'
        m = re.search(level_pat, rpt)
        if m:
            level = m.group(1)
        else:
            self.logger.error("Failed to run ifpga")
            return '0', '0'
        area = sum(lut_nums)
        if lut_input == 7:
            area += lut_nums[-1]
        self.logger.info('area: {} level: {}'.format(area, level))

        mem_pat = r'Peak memory: (\d+) bytes'
        m = re.search(mem_pat, rpt)
        mem = m.group(1)

        time_pat = r'Mapping time: ([\d\.\-e]+)'
        m = re.search(time_pat, rpt)
        runtime = m.group(1)
        return [[str(i) for i in lut_nums], str(area), level, mem, runtime]


def main(config_file):
    case_dir = config.get('global', 'cases')
    workspace = config.get('global', 'workspace')
    os.makedirs(workspace, exist_ok=True)
    shutil.copy(config_file, workspace)

    logger = config_logger(workspace)
    results = []
    worse_cases = []
    better_cases = []
    for root, _, files in os.walk(case_dir):
        for f in files:
            if not f.endswith('.aig'):
                continue
            source = os.path.join(root, f)
            case = Case(source, workspace)
            result = [case.case_name]
            #if case.case_name in ('vga_lcd_comb', 'hyp'): continue
            #run abc
            lut_num, area, level, memory, maptime, runtime = case.run_abc()
            if area == '0': continue
            result.extend(lut_num + [area, level, maptime, str(runtime), str(memory)])

            # run ifpga
            metrics = case.run_ifpga()
            if not metrics:
                continue
            lut_num, gate, delay, peak, imaptime, time = metrics
            #peak = f'{float(peak) / 1024 /1024:.2f}' # to MB
            result.extend(lut_num + [gate, delay, imaptime, str(time), str(peak)])
            time_score = float(runtime) / float(time) * 100
            maptime_score = float(maptime) / float(imaptime) * 100
            memory_score = float(memory) / float(peak) * 100
            result.append(f'{maptime_score:.2f}')
            result.append(f'{time_score:.2f}')
            result.append(f'{memory_score:.2f}')

            # score = abc_area / pcl_area * 100 * 0.4 + abc_level / pcl_level * 100 * 0.6
            score = 30.0 * int(area) / int(gate) + 70.0 * int(level) / int(delay)

            # score2 = (abc_area * abc_level) / (pcl_area * pcl_level)
            score2 = 100 * int(area) * int(level) / (int(gate) * int(delay))
            result.append(f'{score:.2f}')
            result.append(f'{score2:.2f}')
            results.append(result)
            if int(gate) > int(area) and int(delay) > int(level):
                worse_cases.append(case.case_name)
            elif int(gate) <= int(area) and int(delay) <= int(level):
                better_cases.append(case.case_name)

    lut_input = config.getint('global', 'K')
    header = ['case_name']
    header.extend([f'abc_lut{i+1}' for i in range(lut_input)])
    header.extend(['abc_area', 'abc_level', 'abc_maptime', 'abc_time', 'abc_mem'])
    header.extend([f'ifpga_lut{i+1}' for i in range(lut_input)])
    header.extend(['ifpga_area', 'ifpga_level', 'ifpga_maptime','ifpga_time', 'ifpga_mem'])
    header.extend(['maptime_score', 'time_score', 'mem_score', 'score', 'score2'])
    #header = ['case_name', 'abc_area', 'abc_level', 'ifpga_area', 'ifpga_level']

    results.sort(key=lambda x: float(x[-2]))
    # score
    # < 1000
    small_memory_score = 0
    small_time_score   = 0
    valid_small_cases  = 0
    small_time2_score  = 0
    small_cases = [i for i in results if int(i[5]) < 1000]
    if len(small_cases) != 0:
        small_memory_score = sum([float(i[-3]) for i in small_cases]) / len(small_cases)
        small_time_score = sum([float(i[-4]) for i in small_cases]) / len(small_cases)
        valid_small_cases = [float(i[-5]) for i in small_cases if float(i[-5]) > 0]
        small_time2_score = sum(valid_small_cases) / len(valid_small_cases)

    # 1000 ~ 5000
    med_memory_score = 0
    med_time_score   = 0
    valid_med_cases  = 0
    med_time2_score  = 0
    med_cases = [i for i in results if int(i[5]) >= 1000 and int(i[5]) <= 5000]
    if len(med_cases) != 0:
        med_memory_score = sum([float(i[-3]) for i in med_cases]) / len(med_cases)
        med_time_score = sum([float(i[-4]) for i in med_cases]) / len(med_cases)
        valid_med_cases = [float(i[-5]) for i in med_cases if float(i[-5]) > 0]
        med_time2_score = sum(valid_med_cases) / len(valid_med_cases)
    # > 5000
    large_memory_score = 0
    large_time_score   = 0
    valid_large_cases  = 0
    large_time2_score  = 0
    large_cases = [i for i in results if int(i[5]) > 5000]
    if len(large_cases) != 0:
        large_memory_score = sum([float(i[-3]) for i in large_cases]) / len(large_cases)
        large_time_score = sum([float(i[-4]) for i in large_cases]) / len(large_cases)
        valid_large_cases = [float(i[-5]) for i in large_cases if float(i[-5]) > 0]
        large_time2_score = sum(valid_large_cases) / len(valid_large_cases)

    area_score = sum([float(i[lut_input+1]) / float(i[-10]) * 100 for i in results]) / len(results)
    level_score = sum([float(i[lut_input+2]) / float(i[-9]) * 100 for i in results]) / len(results)
    #score = sum([float(i[-2]) for i in results]) / len(results)
    score2 = sum([float(i[-1]) for i in results]) / len(results)

    results.insert(0, header)
    max_list = [len(max(i, key=len)) for i in zip(*results)]
    format_str = ' '.join(['{{:<{}}}'.format(i) for i in max_list])
    result_str = '\n'.join([format_str.format(*i) for i in results])


    logger.info('case results:\n' + result_str)
    logger.info(f'worse cases: {", ".join(worse_cases)}')
    logger.info(f'worse rate: {len(worse_cases) / (len(results)  - 1)}')
    logger.info(f'better cases: {", ".join(better_cases)}')
    logger.info(f'better rate: {len(better_cases) / (len(results)  - 1)}')
    logger.info(f'small maptime score: {small_time2_score:.2f}')
    logger.info(f'medium maptime score: {med_time2_score:.2f}')
    logger.info(f'large maptime score: {large_time2_score:.2f}')
    logger.info(f'small time score: {small_time_score:.2f}')
    logger.info(f'medium time score: {med_time_score:.2f}')
    logger.info(f'large time score: {large_time_score:.2f}')
    logger.info(f'small memory score: {small_memory_score:.2f}')
    logger.info(f'medium memory score: {med_memory_score:.2f}')
    logger.info(f'large memory score: {large_memory_score:.2f}')
    logger.info(f'area score: {area_score:.2f}')
    logger.info(f'level score: {level_score:.2f}')
    #logger.info(f'QoR(weight) score: {score:.2f}')
    #logger.info(f'QoR(product) score: {score2:.2f}')
    logger.info(f'QoR score: {score2:.2f}')

    # write to .csv
    csv_file = os.path.join(workspace, 'test.csv')
    with open(csv_file, 'w') as f:
        f.write('\n'.join([','.join(l) for l in results]))
        f.write('\n'+ ' '.join([f'{small_time2_score}', f'{med_time2_score}', f'{large_time_score}',
                            f'{small_time_score}', f'{med_time_score}',
                            f'{large_time_score:.2f}', f'{small_memory_score:.2f}',
                            f'{med_memory_score:.2f}', f'{large_memory_score:.2f}',
                            f'{score:.2f}', f'{score2:.2f}']))
    logger.info(f'Generating result file: {csv_file}')

    logger.info(f"Finished to run all cases, see '{workspace}' for details.")

if __name__ == '__main__':
    import sys
    config = configparser.ConfigParser()
    config.read(sys.argv[1])

    main(sys.argv[1])
