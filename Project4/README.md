# Project 4 : Dictionary Encoding

## Overview

### Introduction
The objective of this project was to implement a dictionary codec and demonstrate the speed advantages when querying against such a data structure, as well as the efficiency when storing such data structure on the disk. A dictionary codec works by encoding data into integers, rather than strings. For example, the word "computer" could be encoded as 1234. It therefore takes up much less space, as well as can be compared against other encoded words using integer comparisons, which are faster than a full string comparison.

### Encoding Structure
In order to implement the dictionary codec, a hash-table was used, to enable efficient O(1) lookups against the dictionary. In order to generate this data structure in a multi-threaded manner, a single lead thread reads data from the file into a buffer. Once the buffer is filled, the lead thread copy's it data into a queue shared by a number of worker threads. These worker threads then process the data into the data structure while the lead thread goes back to reading in data from the file into the buffer. Access to this shared queue is controlled by a semaphore, and workers threads take ownership of the queue, remove some data to work on, and then release for another worker to take data from. Assuming that IO is much slower than processing the data, parallelizing the reading in of data speeds up the process by a large amount.

Each worker thread takes it's data from the queue, and then assigns each string a unique integer. This unique integer is sourced from a shared atomic integer between all the threads, that is implemented. The integers are not guaranteed to be ordered, but are unique, which is all that is needed for this structure. (A operator exists that increments and returns the value of the integer in the same operation). After the worker threads assign the unique integer to the string value, the string value is entered in the hash table for usage. A custom hash algorithm is used to measure how effective SIMD operations would be at speeding up the hash table insertion, based on a rolling polynomial hash.

### File Structure
The file structure is a binary structure, where the raw data from the C++ variables are written in and then read out. This increases the compression of the file, and the speed at which it can be read. For integer compression, the FastPFor library was used, that was able to perform integer compression on an integer array representing the input file.

The format used was as such:
```
	 * lengthOfDict : int
	 * data:
	 * 	null terminated string : int
	 * lengthOfDataRaw : int
	 * lengthOfDataCompressed : int
	 * 	compressedIntegerStream
```

### Querying 
The querying strategy is quite simple. When loading the compressed file, we not only load the hash table containing our dict, but generate an additional hash table containing each entry, and what lines they map on to. As insertion into a hash table takes O(1), the time to load the file remains in the O(n) time class, and now we have a very quick way to look up data. All we have to do to find a single value is two lookups in hash tables. Once to find a value integer representation, and then a second to find the list of lines this occurs on. This gives us very fast lookup times, with a estimated constant time.

Querying with prefixes is slightly more complicated. We start by going through our dict, and finding not just the integer value of the prefix, but all strings that contain the prefix. We then then perform a search for the term using the previous strategy, and then combine those lists together to get all the lines the prefix occur on.

For the simple full query, SIMD is only used for the hashing, but for the prefix search, it is used for both the hashing and the string comparison.

### Usage
In an environment containing the files `main.cpp`, `dict.cpp` and `simd.cpp`, use the following command line.
Ensure you have the FastPFor library installed on your system

```
gcc main.cpp /usr/local/lib/libFastPFor.a -march natvie
```

Then run the program using the following usage.
```
./a.out <flags> <mode>
```
The available flags are
* -t <number> for threading
* -s to enable SIMD instructions
The modes are 
* compress <inputFile> <outputFile>
* readbin <compressedFile>
* readraw <originalFile>

Once in a mode, follow the prompts to search full terms and prefixes

### Testing Environment
* System : Thinkpad P15s Gen. 2
* OS: Ubuntu 22.04 x86_64
* Storage : SAMSUNG MZVLB512HBJQ-000L7
    * Reported Sequential Read : 3500 MB/s
    * Reported Sequential Write: 2900 MB/s
    * Random Read: 460K IOPS
    * Random Write : 500K IOPS

## Results

### Compression

| Flags           | Time (s) |
|-----------------|----------|
| Thread 1        | 124      |
| Thread 2        | 92       |
| Thread 8        | 78       |
| SIMD - Thread 8 | 72       |

*Compression was performed on the Column.txt file*

### File Size

| File               | Size (mB) |
|--------------------|-----------|
| Raw Column.txt     | 1069      |
| Encoded Column.bin | 397       |

### Querying - Full

| Mode                | Time (ms) |
|---------------------|-----------|
| Vanilla Search      | 3950      |
| Vanilla Search SIMD | 3780      |
| Encoded Search      | 0.0211    |
| Encoded Search SIMD | 0.0208    |

*All querying was performed on the Column.txt file*
*The difference between Encoded and Encoded SIMD is non-signifiant, and is within the variance when rerunning the trial*

### Querying - Prefix

| Mode                | Time (ms) |
|---------------------|-----------|
| Vanilla Prefix      | 4326      |
| Vanilla Prefix SIMD | 8613      |
| Encoded Prefix      | 130       |
| Encoded Prefix SIMD | 166       |

## Analysis

### Encoding / Compression
Encoding and compressing the file takes the longest of any operation, which is to be expected. Every single string in the file has to be loaded in, undergo a computationally expensive hash and then inserted into a data structure. The resulting dict is very fast to perform lookups, but requires a decent amount of time to generate. Further insertions can be done in minimal time, but the original generation takes a decent amount of time to generate.

Multi-threading is clearly working as intending, as the number of threads increases, the compression time decreases. We see a signifiant time saving over 1 worker thread, vs 4/8 threads. However, as the threads increase, we will start to see a signifiant decrease in time savings. This is because the worker threads start to consume the input from to file faster than it can be read. In addition, the non-parallelized section of the workload (compressing the data and outputting it) begins to dominate the time percentage, and increasing the thread count has no effect on it.

SIMD has a marginal impact on the performance of our compression, indicating that the default hash algorithm used for unordered map is already quite efficient when compared to our implementation. This could be that our algorithm does not have the ideal distribution across the map, or that the extra work to set up and extract from the SIMD instructions offsets the gain.

### File Size

File size is a clear savings. The compressed file is 37% the size of the original file. The downsize from the smaller size is much greater time to read and write to the file, and much harder to do so, rather than just being a plain text. These are trade-offs to consider when using dict encoding and integer compression.

### Querying

Doing a full search on the data file with a vanilla search versus an encoded search is very different. The encoded data structure is not significantly faster, but has a near constant time to search no matter the data size. Once the data is loaded in, it performs a single lookup against the data source, and returns the expected values.

The usage of SIMD instructions to speed up the full query has a non-signifiant speedup. When rerunning the trial, the difference between the two of them is small enough that small fluctuations are enough to cause flipping of which mode is faster. This is to be expected, a single hash calculation being converted to a SIMD, when we've already determined we don't gain much savings anyways, is not going to affect the program. There are no string comparisons in looking up in a hash table to compare against, so there's nothing else to save on.

The usage of SIMD instructions in vanilla speed up has much less of a effect that we expected. We would expect that the speed up of the string comparison would result in a speed increase, but it's only a slight speed up. This can tell us that the program is mostly dominated by other operations, for example the constant I/O from the file.

### Querying - Prefix

Similar to querying a full term, the prefix search is greatly reduced with the dict encoding. Our data structure does not lend it the best to this particular search, but means we only have to do string comparisons against a small subset of the total terms, then a few very cheap lookup operations for those terms.

However, we do see a trend where SIMD operations slow down this operation. Rather than just doing a single instruction for string comparisons, we need several to prep the sting for the SIMD operations. This additional time, especially combined that the vast majority of normal string comparisons are immediately going to exit when not equal, whereas SIMD will still take the time to setup for parallel operations, means SIMD slows down his process..

## Conclusion
Dictionary encoding can allow a number of major advantages when working with a data set. It is both much better at representing data in a smaller amount of disk space, and much faster at being querying against. The trade off for this advantage is the time required to generate this dictionary, and read/write it to disk. Your decision on how you should represent your data should take all of these factors into affect.
