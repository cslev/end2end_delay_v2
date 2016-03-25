/* Some things related to networking part (using interfaces, create 
 * packet and send it) was originally developed by austinmarton and 
 * adapted from https://gist.github.com/austinmarton/1922600
 */
 
#include <arpa/inet.h>
#include <linux/if_packet.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/ether.h>
#include <time.h>
#include <sys/time.h> 

#include <string>				//string
#include <iostream>			
#include <sstream>       //std::stringstream, std::hex
#include <iomanip>      // std::setw

#define MY_DEST_MAC0	0x01
#define MY_DEST_MAC1	0x02
#define MY_DEST_MAC2	0x03
#define MY_DEST_MAC3	0x04
#define MY_DEST_MAC4	0x05
#define MY_DEST_MAC5	0x06

#define DEFAULT_IF	"eth1"
#define BUF_SIZ		66
#define DEBUG 0

//define here the number of packets to send
#define NUM_PACKETS 32

using namespace std;

//this function converts an intger number to hex-string that has
//length hex digits
string convertInt2HexString(uint64_t num, int length)
{
	stringstream ss;
	ss << std::setfill('0') << setw(length) << hex << num;
	return ss.str();
}

int main(int argc, char *argv[])
{
	char ifName[IFNAMSIZ];
	int num_packets = NUM_PACKETS;

	//~ Get interface name and number of packets to send 
	if (argc == 2)
	{
		strcpy(ifName, argv[1]);
	}
	else if (argc == 3)
	{
		strcpy(ifName, argv[1]);
		num_packets = atoi(argv[2]);
		cout << "Number of packets is set to " << num_packets << endl;
	}
	else
	{
		strcpy(ifName, DEFAULT_IF);
		num_packets = NUM_PACKETS;
	}	
			
	//end of stream indicator number
	int end_of_stream = 0;
	for (int n=0; n < num_packets;n++)
	{
		if ((n+1) == num_packets)
			end_of_stream = 1;
		
		
		int sockfd;
		struct ifreq if_idx;
		struct ifreq if_mac;
		int tx_len = 0;
		char sendbuf[BUF_SIZ];
		struct ether_header *eh = (struct ether_header *) sendbuf;

		struct ifreq if_ip;
		struct sockaddr_ll socket_address;
	
		/* Open RAW socket to send on */
		if ((sockfd = socket(AF_PACKET, SOCK_RAW, IPPROTO_RAW)) == -1) {
				perror("socket");
		}

		/* Get the index of the interface to send on */
		memset(&if_idx, 0, sizeof(struct ifreq));
		strncpy(if_idx.ifr_name, ifName, IFNAMSIZ-1);
		if (ioctl(sockfd, SIOCGIFINDEX, &if_idx) < 0)
				perror("SIOCGIFINDEX");
		/* Get the MAC address of the interface to send on */
		memset(&if_mac, 0, sizeof(struct ifreq));
		strncpy(if_mac.ifr_name, ifName, IFNAMSIZ-1);
		if (ioctl(sockfd, SIOCGIFHWADDR, &if_mac) < 0)
				perror("SIOCGIFHWADDR");
				
	
		/* Construct the Ethernet header */
		memset(sendbuf, 0, BUF_SIZ);
		/* Ethernet header */
		eh->ether_shost[0] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[0];
		eh->ether_shost[1] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[1];
		eh->ether_shost[2] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[2];
		eh->ether_shost[3] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[3];
		eh->ether_shost[4] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[4];
		eh->ether_shost[5] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[5];
		eh->ether_dhost[0] = MY_DEST_MAC0;
		eh->ether_dhost[1] = MY_DEST_MAC1;
		eh->ether_dhost[2] = MY_DEST_MAC2;
		eh->ether_dhost[3] = MY_DEST_MAC3;
		eh->ether_dhost[4] = MY_DEST_MAC4;
		eh->ether_dhost[5] = MY_DEST_MAC5;
		/* Ethertype field */
		eh->ether_type = htons(ETH_P_IP);
		tx_len += sizeof(struct ether_header);
		
		//DATA should look like this
		//     DST MAC           SRC MAC           ETH_TYPE
		//Data:01:02:03:04:05:06:40:f2:e9:0e:39:16:08:00
		
	
	
		//in order to identify packets, we encode a counter into the payload
		//as well as 4 hex digits after ether type
		int pc = n; //packetcount
		string packetCount = convertInt2HexString(pc,8);
		if(DEBUG)
			cout << "packetCount: " << packetCount << endl;
			
		//create a char array of the packetcount in hex
		char pcArray[sizeof(packetCount)];
		strncpy(pcArray,packetCount.c_str(), sizeof(pcArray));
		// --- packetcount ----
		//Getting 2byte parts of the hex string and concatenate the 2byte
		//hex numbers to the packet data (sendbuf)
		for (int i=0; i<sizeof(pcArray); i=i+2)
		{	
			stringstream s;	 //create a stringstream for conversion
			s << "0x" << pcArray[i] << pcArray[i+1]; //concatenate 2 hex numbers
			int c; //int variable to store that hex number in int
			s >> hex >> c; //convert string hex to int hex
			sendbuf[tx_len++] = c; //append int hex to data (sendbuf)
			//DATA should look like this
			//     DST MAC          :SRC MAC          :ETH_TYPE:COUNT
			//Data:01:02:03:04:05:06:40:f2:e9:0e:39:16:08:00   :00:00:00:01
			//since strings are 8-byte longs	
		}

		//for timestamps
		struct timeval start;
		
		//if you want sleeps among packets, uncomment this line
		//~ struct timespec sleep;
		
		
		//get current precise timestamp
		gettimeofday(&start, NULL); 
		
		//If you want sleep among packets, define here how many secs and
		//nano!secs sleeps are required
		//~ sleep.tv_sec=1;
		//~ sleep.tv_nsec=0;
		//~ nanosleep(&sleep, NULL);

		
		//timestamp's seconds
		uint64_t sec = start.tv_sec;
		//timestamp's useconds
		uint64_t usec = start.tv_usec;
		//convert seconds and useconds to hex-string
		string secStr = convertInt2HexString(sec, 8);
		string usecStr = convertInt2HexString(usec, 8);
		if(DEBUG)
		{
			cout << "sec: " << sec << " in hex: " << secStr << endl;
			cout << "usec: " << usec << " in hex: " << usecStr << endl;
		}
		//we got timestamp's sec and usec in hex
		
		
		//-------- START TO APPEND TIMESTAMP TO PAYLOAD --------
		//create a char array of the sec in hex
		char secArray[sizeof(secStr)];
		strncpy(secArray,secStr.c_str(), sizeof(secArray));
		
		//create another char array of the usec in hex	
		char usecArray[sizeof(usecStr)];
		strncpy(usecArray,usecStr.c_str(), sizeof(usecArray));
		
		
		// --- SECs ----
		//Getting 2byte parts of the hex string and concatenate the 2byte
		//hex numbers to the packet data (sendbuf)
		for (int i=0; i<sizeof(secArray); i=i+2)
		{	
			stringstream s;	 //create a stringstream for conversion
			s << "0x" << secArray[i] << secArray[i+1]; //concatenate 2 hex numbers
			int c; //int variable to store that hex number in int
			s >> hex >> c; //convert string hex to int hex
			sendbuf[tx_len++] = c; //append int hex to data (sendbuf)
		}
		
		// --- uSECs ----
		//Getting 2byte parts of the hex string and concatenate the 2byte
		//hex numbers to the packet data (sendbuf)
		for (int i=0; i<sizeof(usecArray); i=i+2)
		{	
			stringstream s;	 //create a stringstream for conversion
			s << "0x" << usecArray[i] << usecArray[i+1]; //concatenate 2 hex numbers
			int c; //int variable to store that hex number in int
			s >> hex >> c; //convert string hex to int hex
			sendbuf[tx_len++] = c; //append int hex to data (sendbuf)
		}	
		
		// ---- END OF STREAM HANDLING ----
		// we send a simple integer in 8-byte hex (again) to indicate end of 
		// stream. If that integer is 1, then that packet was the last,
		// otherwise, it is not
		//create another char array of the end_of_stream in hex	
		string eosStr = convertInt2HexString(end_of_stream, 8);
		char eosArray[sizeof(eosStr)];
		strncpy(eosArray,eosStr.c_str(), sizeof(eosArray));
		for (int i=0; i<sizeof(eosArray); i=i+2)
		{	
			stringstream s;	 //create a stringstream for conversion
			s << "0x" << eosArray[i] << eosArray[i+1]; //concatenate 2 hex numbers
			int c; //int variable to store that hex number in int
			s >> hex >> c; //convert string hex to int hex
			sendbuf[tx_len++] = c; //append int hex to data (sendbuf)
		}	
		
		// =====================================================

		/* Index of the network device */
		socket_address.sll_ifindex = if_idx.ifr_ifindex;
		/* Address length*/
		socket_address.sll_halen = ETH_ALEN;
		/* Destination MAC */
		socket_address.sll_addr[0] = MY_DEST_MAC0;
		socket_address.sll_addr[1] = MY_DEST_MAC1;
		socket_address.sll_addr[2] = MY_DEST_MAC2;
		socket_address.sll_addr[3] = MY_DEST_MAC3;
		socket_address.sll_addr[4] = MY_DEST_MAC4;
		socket_address.sll_addr[5] = MY_DEST_MAC5;

		/* Send packet */
		if (sendto(sockfd, sendbuf, tx_len, 0, (struct sockaddr*)&socket_address, sizeof(struct sockaddr_ll)) < 0)
				printf("Send failed\n");
		else
			cout << ".";
	}
	cout << endl;
	
	return 0;
}
