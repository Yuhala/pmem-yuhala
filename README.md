# pmem-yuhala

This repository contains two folders with programs based on [PMDK]( https://github.com/pmem/pmdk). The programs use the libpmemobj library to implement fault resilient algorithms. This readme is a quick guide on how to easily test the programs.

### Prerequisites
In order to follow this tutorial with minimal hiccups, clone the [PMDK]( https://github.com/pmem/pmdk) from github; This will facilitate the build process of the programs as it contains all the libraries we used.
Use the following command to clone the PMDK to your local environment

```
git clone https://github.com/pmem/pmdk.git

```

## How to test the programs 
Start by cloning this project to your machine 
```
git clone https://github.com/Yuhala/pmem-yuhala.git

```
Place the two folders : _queue-array_ and _queue-list_ in the _pmdk/src/examples/libpmemobj_ directory. This is a quick and dirty way to ease compilation later. This directory should be your main working directory.

### Compiling and Testing the queue-array program
#### Quick intro
This program creates two persistent queues q1 and q2 based on arrays. It continuously enqueues and dequeues an item from both lists alternatively until the _ctrl-c_ signal is sent to the process. The program then tells us whether the item is present in either queues.
#### Compiling 
To compile the queue-array program cd into the queue-array directory, assuming your current working directory is _pmdk/src/examples/libpmemobj_ . Then run the make file

```
cd queue-array
make
```
#### Testing
After a successful compilation, the executable file  _queue_ is created. Start by creating a poolfile in the _/dev/shm_ directory

```
touch /dev/shm/poolfile
```

The program takes two arguments : the path to the poolfile and the item (use any character). The following command runs the program on the poolfile created above and 'a' as the content of the node item.

```
./queue /dev/shm/poolfile a
```
The program continuously prints statements indicating whether the item has been enqueued or deqHiueued from either queues.
Hit _ctrl-c_ to start the signal handler in the program. The signal handler stops the infinite loop and tells us whether our program is fault resilient or not.

#### Troubleshooting
In case you get an _Aborted (core dumped)_ error after running the program, simply delete the poolfile and create a new one.

### Compiling and Testing the queue-llist program
#### Quick intro
This program is similar to the one above. It creates two persistent queues q1 and q2 based on arrays. It then continuously enqueues and dequeues an item from both lists alternatively until the _ctrl-c_ signal is sent to the process. The program then tells us whether the item is present in either queues.
#### Compiling 
To compile the queue-llist program cd into the queue-llist directory, assuming your current working directory is _pmdk/src/examples/libpmemobj_ . Then run the make file

```
cd queue-llist
make
```
#### Testing
After a successful compilation, the executable file  _fifo_ is created. You don't need to manually create the poolfile here; it is created in the program.

The program takes two arguments : the path to the poolfile and the item (use any character). The following command runs the program on a poolfile and 'a' as the content of the node item. If the poolfile exists, the program uses it. Else, it creates a poolfile with the same name. 

```
./fifo /dev/shm/poolfile a
```
The program continuously prints statements indicating whether the item has been enqueued or deqHiueued from either queues.
Hit _ctrl-c_ to start the signal handler in the program. The signal handler stops the infinite loop and tells us whether our program is fault resilient or not.

#### Troubleshooting
In case you get an _Aborted (core dumped)_ error after running the program, simply delete the poolfile and create a new one.




## Authors

* **Peterson Yuhala** 


## Acknowledgments

* PMDK Source code : https://github.com/pmem/pmdk
