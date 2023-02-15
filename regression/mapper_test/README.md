# A mapping QoR test Tool 

A test tool for PCL lut-mapping QoR.  
QoR metrics(area & level):  
The area and level of LUT1~LUT6 are 1;
The area of LUT7 is 2 while the level is 1.

## **Directory**
```
> mapper_test/lutlib/                       // lut lib for area and level computation
> mapper_test/abc_diff                      // refer to the following specification
> mapper_test/mapper_test.py                // the test script
> mapper_test/consumer_config.yaml          // ifpga flow's configurauton

specification about  abc_diff:
Before use, an modified ABC binary is needed for peak memory counting.
You can find ABC source in https://github.com/berkeley-abc/abc;  
Then merge abc_diff to the ABC source and rebuild.
But for now, we suggest you build the abc under our tools directory which also contain the aigs extracted from &dch. And that is need for our choice-flow test.

```

## **Usage**
1. open config.ini and write your configuration
2. run the mapper scripts:
```
$ python3 mapper_test.py config.ini
```

## **Result Description**
It will create a directory /mapper_test/choice_output/ which contain the mapping result of our flow; \
And a .csv file also count which is in the result direcotry named "test.csv", which logs the area & level, together with the number of LUT1~LUT6 for all cases.