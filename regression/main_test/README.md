# A Daily Regression Script

A demo of daily regression for PCL yosys or mapper(ifpga)

## **Directory**
```
> main_test/ifpga_cases/        // cases for ifpga
> main_test/scripts/            // test scripts
> main_test/simple_cases/       // cases for PCL yosys
> main_test/config.ini          // test configuration
```
## **Usage**
1. open config.ini and write your configuration
2. run the regression script:
```
$ python3 scripts/run_test.py config.ini
```

## **Add a test case!**
we assume you will add a test case which names decode_32_2.aig, so you will make it when you follows the instructions below:

```
$ cd ifpga_cases
$ mkdir decode_32_2
$ cp path/decode_32_2.aig decode_32_2.aig
$ touch testinfo.ini

specification about testinfo.ini's template:
    [flow]
    # this is an ifpga case or yosys case, default is yosys
    flow = ifpga
    # skip equivalence check, default is false
    disable_verification = true
    [qor]
    # lut number > 11 will cause a QoR failure
    lut   = 11
    area  = 1023
    level = 51
```



## **Result Description**

The test result is like below:
```
[2021-02-24 14:33:24,568]run_test.py:45:[INFO]: Summary:
Total:  2
Failed: 1
NONEQUIVALENT:
/home/chfeng/fpga-map-tool/test/anlogic_test/output/210224-143324/set_64_1
[2021-02-24 14:33:24,568]run_test.py:48:[INFO]: Finished to run all test, see "/home/chfeng/fpga-map-tool/test/anlogic_test/output/210224-143324/regression.rpt" for report.
```
There are report of total and failed numbers, together with a list of failure cases.  

Currently, the failure reasons are:  
- ABNORMAL_EXIT: yosys crash or other tool errors  
- NONEQUIVALENT: case failed to pass formality  
- WORSE_QOR    : worse QoR, lut number is greater than that configured in testinfo.ini

