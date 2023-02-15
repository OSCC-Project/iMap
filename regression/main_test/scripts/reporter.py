
import re
import os

class Status(object):
    """ enum values for status """
    PASS = 0
    ABNORMAL_EXIT = 1
    NONEQUIVALENT = 2
    WORSE_QOR = 3
    WORSE_PERFORMANCE = 4
    POST_CHECK_FAILED = 5

    TOTAL = 6

    @classmethod
    def to_string(cls, val):
        for k, v in vars(cls).items():
            if v == val:
                return k

class Reporter(object):
    """ the reporter of regression """
    def __init__(self, cases, workspace):
        self.cases = cases
        self.workspace = workspace

    def report(self):
        results = [[] for _ in range(int(Status.TOTAL))]
        for case in self.cases:
            case_name = os.path.split(case)[1]
            logfile = os.path.join(self.workspace, case_name, case_name+'.log')
            with open(logfile) as f:
                content = f.read()
            status = None
            if 'Finished to run case' in content:
                status = Status.PASS
            elif 'exit code' in content:
                status = Status.ABNORMAL_EXIT
            elif 'nonequivalent' in content:
                status = Status.NONEQUIVALENT
            elif 'post check failure' in content:
                status = Status.POST_CHECK_FAILED
            elif 'worse QoR' in content:
                status = Status.WORSE_QOR
            elif 'worse performance' in content:
                status = Status.WORSE_PERFORMANCE
            results[status].append(os.path.join(self.workspace, case_name))

        total = len(self.cases)
        failed = total - len(results[0])

        rpt_lines = ['Summary:',
                     f'Total:  {total}',
                     f'Failed: {failed}',
                     f'Pass Rate: {(100.0 * len(results[0]) / total):.2f}%']
        for s, cases in enumerate(results[1:]):
            if len(cases):
                rpt_lines.append(f'{Status.to_string(s+1)}:')
                rpt_lines.extend([f'    {i}' for i in cases])

        rpt_str = '\n'.join(rpt_lines)

        rpt_file = os.path.join(self.workspace, 'regression.rpt')
        with open(rpt_file, 'w') as f:
            f.write(rpt_str)

        return rpt_str





