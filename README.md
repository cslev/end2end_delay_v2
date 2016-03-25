# end2end_delay_v2
Completely rewritten version in C/C++ of my python based end2end_delay found under https://github.com/cslev/end2end_delay

Works similarly to end2end_delay, but it has some new features.

# How it works
It assembles a pure ethernet packet with a payload of the timestamp of the sending and a 
packet counter, and after receiving the packet on the other side, this app calculates the difference.
Packetcounter is used for avoiding possible packet reordering and wrong calculations afterwards.

Since it is written in C/C++, it is much more precise than the python based tool.
It uses gettimeofday() function for getting timestamps, which is (at least according to my 
experiments) are 10-50 useconds (micro!) precise compared to the couple of hundreds of msec (milli) 
precision of python's timestamp.

#Note
In a real-time OS, this application probably works more precise

#Compiling
There is no special library needed, so simply type:

$ make

This will produce two binaries send_eth, and recv_eth, however, you don't have to use
them manually. See below for more details.

#USAGE with display manager
$ sudo start.sh iface_sender iface_reciever number_of_packets number_of_executions

The number_of_packets denotes how many packets wanted to be sent out, while
number_of_executions marks how many times the whole sending and receiving process should be
started/repeated to get more precise average results.

Again, do not run send_eth, and recv_eth  manually, start.sh does the job for you.

It will bring up the interfaces in promiscuous mode, and the measurement then will be started
in two xterms!
In the server window, you will only notice average delays. If you want to read more details,
change debug variable to 1 at line 14 in start.sh.
If you want more and more details, change DEBUG variable in both source codes (send_eth.cc,recv_eth.cc)
and compile them again via excecuting make again.

There is no calculation at the end again, so only the average results of one execution are shown,
the user himself/herself should conclude the results of the multiple executions (since there 
could be outlier results, some jitters due to some background process, etc.).


#USAGE without display manager
If you have no GUI/display manager, then modify start.sh accordingly, or set up your interfaces
manually, first start the reciever side:

$ sudo ./recv_eth server_iface number_of_packets


recv_eth should know in advance how many packets will be received in order to preallocate memory
for them and shorten the processing time

Then, in another terminal, start the sender side:

$ sudo ./send_eth client_iface number_of_packets




