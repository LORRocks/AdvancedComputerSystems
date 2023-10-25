# Project 3 : SSD Performance Testing

## Overview

### Introduction
The goal of this project is to profile the performance of a modern SSD, and demonstrate the difference in performance when adjusting multiple parameters. The parameters we are varying are the following, (1) data access size, (2) read vs. write intensity, (3) IO/queue depth. While varying these parameters, we are measuring the SSD latency and throughput. Based on our queuing theory, we should a trade off between latency and throughput.

### Testing Procedure

In order to obtain a space for the `fio` utility to test the performance of the drive, the test environment was booted up on a external drive, the existing portion shrunk, and new portion allocated in the newly freed space. 

After the necessary space to test was obtained, a script was written. This script directly calls the `fio` utility, while looping through the desired variations of the parameters, and then save that output in to a file for later review. This prevented having to run the `fio` utility many times by hand.

### Testing Environment
* System : Thinkpad P15s Gen. 2
* OS: Ubuntu 22.04 x86_64
* Storage : SAMSUNG MZVLB512HBJQ-000L7
    * Reported Sequential Read : 3500 MB/s
    * Reported Sequential Write: 2900 MB/s
    * Random Read: 460K IOPS
    * Random Write : 500K IOPS

## Results

| Block Size | Read-Write % | IODepth |   | latency       | bw (mb / s) | iops   |
|------------|--------------|---------|---|---------------|-------------|--------|
| 4K         | 100          | 1024    |   | 7295.22 usec  | 2193.75     | 561600 |
| 4K         | 100          | 512     |   | 3639 usec     | 2200        | 563220 |
| 4K         | 70           | 1024    |   | 7295.22 usec  | 2193.75     | 561600 |
| 4K         | 70           | 512     |   | 3612 usec     | 2217.92     | 567786 |
| 4K         | 0            | 1024    |   | 7215.86 usec  | 2207        | 565162 |
| 4K         | 0            | 512     |   | 3776 usec     | 2120        | 542764 |
| 32K        | 0            | 1024    |   | 38836 usec    | 3292        | 105373 |
| 32K        | 0            | 512     |   | 19365.17 usec | 3309        | 105914 |
| 32K        | 70           | 1024    |   | 38725 usec    | 3302        | 105680 |
| 32K        | 70           | 512     |   | 20012 usec    | 3198        | 102361 |
| 32K        | 0            | 1024    |   | 38495 usec    | 3322        | 106305 |
| 32K        | 0            | 512     |   | 20027 usec    | 3198.24     | 102323 |
| 128K       | 0            | 1024    |   | 154 msec      | 3307        | 26462  |
| 128K       | 0            | 512     |   | 76 msec       | 3337        | 26702  |
| 128K       | 70           | 1024    |   | 152 msec      | 3340        | 26725  |
| 128K       | 70           | 512     |   | 74 msec       | 3242        | 25946  |

*Read-Write % refers to the percentage of write operations, so 0 refers to 0 write, 100% reads.*

## Analysis
### General Analysis

* In general, increasing the block size ( the data access granularity) causes the IOPS to go down, and the latency to go up. This is expected, as a larger operation means more time per operation, therefore less io operations per unit time, as well as a larger latency. However, we also see a larger bandwidth at this larger block sizes, showing the trade-off between latency and bandwidth.
* Adjusting the read write percentage does not have to a significant effect on any of the measured attributes, which is contradictory to the drive's reported performance from the manufactures website.
* Adjusting the ioDepth did appear to change latency, and a slight increase in bandwidth. This also makes sense, having a larger resource utilization should cause a higher latency, and a higher throughput.
* Overall, we see the general queuing theory we expect to see, that as we increase the data access size, we get a clear trade-off between latency and bandwidth.

### Comparison to Intel Enterprise-Grade SSD
The Intel Enterprise Grade Data Center NVMe SSD D7-P5600 (1.6TB) has the following reported test.

* random write-only with 130K IOPS at 4KB data access

The SSD we are using has a reported IOPS of 516,000. Initially this seems to be contradictory, as the general ( and cheaper) SSD should perform worse, but is performing several times faster according to IOPS. This can make sense when we consider that the results in our testing and the role of enterprise drives. We showed in our testing we have a tradeoff between bandwidth and latency. The enterprise drive may be designed for a different access pattern. Possibly it is designed for larger batch jobs, with less IOPS, and a greater bandwidth. It could also be specialized for read mostly over read-write jobs. These would all explain the why the IOPS of the general SSD are better, IOPS are not everything when it comes to a drive's performance.
## Conclusion
In order to maximise the performance of various storage solutions, we need to understand that we can't simply look at a simple metric such as IOPS or Latency. Bandwidth is a combination of the access pattern we are using combined with the drive's performance factors. We need to structure how we access data in combination with the general performance of the storage solution we are using to maximise overall performance.
