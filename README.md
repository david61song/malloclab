
# Malloc lab Originated From CMU CS:APP malloc lab (64-bit version)

## Contributor : david61song



```
#####################################################################
# CS:APP Malloc Lab
# Handout files for students
#
# Copyright (c) 2002, R. Bryant and D. O'Hallaron, All rights reserved.
# May not be used, modified, or copied without permission.
#
######################################################################

***********
Main Files:
***********

mm.{c,h}
	Your solution malloc package. mm.c is the file that you
	will be handing in, and is the only file you should modify.

mdriver.c
	The malloc driver that tests your mm.c file

short{1,2}-bal.rep
	Two tiny tracefiles to help you get started.

Makefile
	Builds the driver

**********************************
Other support files for the driver
**********************************

config.h	Configures the malloc lab driver
fsecs.{c,h}	Wrapper function for the different timer packages
clock.{c,h}	Routines for accessing the Pentium and Alpha cycle counters
fcyc.{c,h}	Timer functions based on cycle counters
ftimer.{c,h}	Timer functions based on interval timers and gettimeofday()
memlib.{c,h}	Models the heap and sbrk function

*******************************
Building and running the driver
*******************************
To build the driver, type "make" to the shell.

To run the driver on a tiny test trace:

	unix> mdriver -V -f short1-bal.rep

The -V option prints out helpful tracing and summary information.

To get a list of the driver flags:

	unix> mdriver -h
```

# PERF index

## implicit_list

```text
Team Name:david61song
Member 1 :david song:none@none.none
Using default tracefiles in traces/
Measuring performance with gettimeofday().

Results for mm malloc:
trace  valid  util     ops      secs  Kops
 0       yes   98%    5694  0.010454   545
 1       yes   99%    5848  0.009309   628
 2       yes   99%    6648  0.017281   385
 3       yes   99%    5380  0.013588   396
 4       yes   66%   14400  0.000284 50633
 5       yes   92%    4800  0.006823   704
 6       yes   91%    4800  0.006235   770
 7       yes   54%   12000  0.173194    69
 8       yes   47%   24000  0.233843   103
 9       yes   27%   14401  0.058287   247
10       yes   34%   14401  0.002562  5621
Total          73%  112372  0.531860   211

Perf index = 44 (util) + 14 (thru) = 58/100


```
