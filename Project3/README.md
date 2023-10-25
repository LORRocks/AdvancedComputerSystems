# Project 3 : SSD Performance Testing

## Overview

### Introduction
    The goal of this project is to profile the performance of a modern SSD, and demonstrate the difference in performance when adjusting multiple parameters. The parameters we are varying are the following, (1) data access size, (2) read vs. write intensity, (3) IO/queue depth. While varying these parameters, we are measuring the SSD latency and throughput. Based on our queuing theory, we should a trade off between latency and throughput.

### Testing Procedure

After the necessary space to test was obtained, a script was written. This script directly calls the `fio` utility, while looping through the desired variations of the parameters, and then save that output in to a file for later review. This prevented having to run the `fio` utility 27 times by hand.

## Results

## Analysis
### General Analysis

### Comparison to Intel Enterprise-Grade SSD

## Conclusion

