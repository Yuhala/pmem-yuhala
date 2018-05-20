# pmem-yuhala

This repository contains two folders with programs based on [PMDK]( https://github.com/pmem/pmdk). The programs use the libpmemobj library to implement fault resilient algorithms. This readme is a quick guide on how to easily test the programs.

### Prerequisites
In order to follow this tutorial with minimal hiccups, clone the [PMDK]( https://github.com/pmem/pmdk) from github; This will facilitate the build process of the programs as it contains all the libraries we used.
Use the following command to clone the PMDK to your local linux environment

```
git clone https://github.com/pmem/pmdk.git

```
Install required packages for a linux system

```
apt-get install autoconf
apt-get install pkg-config

```

Build the pmdk library on your system; cd into your `pmdk` folder and type the following commands

```
make 
#as root
make install 
ldconfig

```

## How to test the programs 
Start by cloning this project to your machine 
```
git clone https://github.com/Yuhala/pmem-yuhala.git

```
Place the two folders : `queue-array` and `queue-llist` in the `pmdk/src/examples/libpmemobj` directory. This is a quick and dirty way to ease compilation later. This directory should be your main working directory.

### Compiling and Testing the queue-array program
#### Quick intro
This program creates two persistent queues q1 and q2 based on arrays. It continuously enqueues and dequeues an item from both lists alternatively until the `ctrl-c` signal is sent to the process. The program then tells us whether the item is present in either queues.
#### Compiling 
To compile the queue-array program cd into the queue-array directory, assuming your current working directory is `pmdk/src/examples/libpmemobj` . Then run the make file

```
cd queue-array
make
```
#### Testing
After a successful compilation, the executable file  `queue` is created. Start by creating a poolfile in the `/dev/shm` directory

```
pmempool create obj /dev/shm/poolfile --layout queue

```

The program takes two arguments : the path to the poolfile and the item (use any character). The following command runs the program on the poolfile created above and 'a' as the content of the node item.

```
./queue /dev/shm/poolfile a
```
The program continuously prints statements indicating whether the item has been enqueued or dequeued from either queues.
Hit `ctrl-c` to start the signal handler in the program. The signal handler stops the infinite loop and tells us whether our program is fault resilient or not.

#### Troubleshooting
In case you get an `Aborted (core dumped)` error after running the program, simply delete the poolfile and create a new one.




### Compiling and Testing the queue-llist program
#### Quick intro
This program is similar to the one above. It creates two persistent queues q1 and q2 based on arrays. It then continuously enqueues and dequeues an item from both lists alternatively until the `ctrl-c` signal is sent to the process. The program then tells us whether the item is present in either queues.
#### Compiling 
To compile the queue-llist program cd into the queue-llist directory, assuming your current working directory is `pmdk/src/examples/libpmemobj` . Then run the make file

```
cd queue-llist
make
```
#### Testing
After a successful compilation, the executable file  `fifo` is created. You don't need to manually create the poolfile here; it is created in the program.

The program takes two arguments : the path to a poolfile and the item (use any character). The following command runs the program on a poolfile and 'a' as the content of the node item. If the poolfile exists, the program uses it. Else, it creates a poolfile with the same name. Be careful not to use the same poolfile used in the preceding program.

```
./fifo /dev/shm/poolfile a
```
The program continuously prints statements indicating whether the item has been enqueued or dequeued from either queues.
Hit `ctrl-` to start the signal handler in the program. The signal handler stops the infinite loop and tells us whether our program is fault resilient or not.

#### Troubleshooting
In case you get an `Aborted (core dumped)` error after running the program, simply delete the poolfile and create a new one.





## Author

* **Peterson Yuhala** 


## Acknowledgments

* PMDK Source code : https://github.com/pmem/pmdk
