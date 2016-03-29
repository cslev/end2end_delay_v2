/* Some things related to networking part (using interfaces, create 
 * packet and send it) was originally developed by austinmarton and 
 * adapted from https://gist.github.com/austinmarton/1922600
 */

#include <arpa/inet.h>
#include <linux/if_packet.h>
#include <linux/ip.h>
#include <linux/udp.h>
#include <stdio.h>
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


#define DEST_MAC0	0x01
#define DEST_MAC1	0x02
#define DEST_MAC2	0x03
#define DEST_MAC3	0x04
#define DEST_MAC4	0x05
#define DEST_MAC5	0x06

#define ETHER_TYPE	0x0800

#define DEFAULT_IF	"eth0"
#define BUF_SIZ		66
#define DEBUG 1

//define here the number of packets to send
#define NUM_PACKETS 32

using namespace std;

// --------------- FUNCTIONS -----------------

//this function converts payload data to integer numbers
//first it requires the 8-byte long payload segments in 4 2-byte segments
//then converts it to char array, and then to int via stringstream
uint64_t createIntFromPayload(uint8_t first2byte,
														  uint8_t second2byte,
														  uint8_t third2byte,
														  uint8_t fourth2byte)
{
	//create a char array for the 2-byte segments
	char payloadArray[8];
	//load them to the char array
	sprintf(payloadArray, "%02x%02x%02x%02x", 
					first2byte,
					second2byte,
					third2byte,
					fourth2byte);
					
	//create temporary stringstream for latter conversion
	stringstream plStream;
	plStream << "0x";
	for (int i=0; i < sizeof(payloadArray); i++)
	{
		plStream << payloadArray[i]; 
	}
	//now, we have the 8-byte payload as one hex string

	uint64_t retval; //payload finally in integer type as pure integer number
	plStream >> hex >> retval;
	
	return retval;
}



//int arrays for timestamp data
//first, we use these simple arrays in order to fasten the processing
//time, then we will use GHashtable to easily calculate the differences
uint64_t* sent_seconds_array;
uint64_t* sent_useconds_array;
uint64_t* recv_seconds_array;
uint64_t* recv_useconds_array;

int num_packets = NUM_PACKETS;

//this function iterates the seconds and useconds int arrays 
void get_delay_data()
{
	//declare variables and lists to calculate min, max and avg delays
	uint64_t min_usec = 999999;
	uint64_t max_usec = 0;
	double avg_usec = 0.0;
	
	//these variable are only used if once more than 1 sec was the delay
	bool more_than_one_sec_delay_realized = false;
	double min_diff = 999999.9;
	double max_diff = 0.0;
	double avg_diff = 0.0;
	
	
	
	for (int i=0; i < num_packets;i++)
	{
		if(DEBUG)
		{
			cout << "sent sec: " << sent_seconds_array[i] << endl;
			cout << "sent usec: " << sent_useconds_array[i] << endl;
			cout << "recv sec: " << recv_seconds_array[i] << endl;
			cout << "recv usec: " << recv_useconds_array[i] << endl;
		}
		
		//calculate min usec
		uint64_t usec_diff = recv_useconds_array[i] - sent_useconds_array[i];
		if (usec_diff < min_usec)
			min_usec = usec_diff;
			
		//calculate max usec
		if (usec_diff > max_usec)
			max_usec = usec_diff;
		
		//sum diffs to variable avg_usec	
		avg_usec += usec_diff;
			
			
		uint64_t sec_diff = recv_seconds_array[i] - sent_seconds_array[i];
		//if delay was more than 1 sec (probably not happens very often), 
		//then we need to handle this case as well
		//WHY separate? WHY NOT using this block only? If usecs and secs
		//are hanndled differently, then since they are just big integers
		//floating point precision issues shall not arise.
		if(sec_diff != 0)
		{
			//we just set this var to true in order to indicate that at the
			//end of this function, we will use and print diff variables
			//instead of usec_diff variables
			more_than_one_sec_delay_realized = true;
		}
		
		//now we handle a big double number (sec.usec)
		double diff = sec_diff + usec_diff;
		//calculate min diff
		if(diff < min_diff)
			diff = min_diff;
		//calculate max sec
		if(diff > max_diff)
			max_diff = diff;
		
		//sum diffs to variable avg_usec	
		avg_diff += diff;
	}
	
	//calculate avg_usec by dividing it with num_packets
	avg_usec /= num_packets;
	//calculate avg_diff by dividing it with num_packets
	avg_diff /= num_packets;
	cout << endl;
	if(more_than_one_sec_delay_realized)
	{
		
		cout << "Delay was more than 1 sec at least once!" << endl;
		cout << "Min diff delay: " << min_diff << endl;
		cout << "Avg diff delay: " << avg_diff << endl;
		cout << "Max diff delay: " << max_diff << endl;
		
	}
	else
	{
		cout << "All delay data were below 1 sec" << endl;
		cout << "Min usec delay: " << min_usec << endl;
		cout << "Avg usec delay: " << avg_usec << endl;
		cout << "Max usec delay: " << max_usec << endl;
	}
}
// ------------ END FUNCTIONS -----------------



// ------------============== MAIN ===============--------------
int main(int argc, char *argv[])
{
	
	char sender[INET6_ADDRSTRLEN];
	int sockfd, ret, i;
	int sockopt;
	ssize_t numbytes;
	struct ifreq ifopts;	/* set promiscuous mode */
	struct ifreq if_ip;	/* get ip addr */
	struct sockaddr_storage their_addr;
	uint8_t buf[BUF_SIZ];
	char ifName[IFNAMSIZ];
	

	
	//~ Get interface name and number of packets to receive 
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
	

	//malloc memory for the int arrays of timestamps
	sent_seconds_array = new uint64_t[num_packets];
	sent_useconds_array = new uint64_t[num_packets];
	recv_seconds_array = new uint64_t[num_packets];
	recv_useconds_array = new uint64_t[num_packets];
	

	/* Header structures */
	struct ether_header *eh = (struct ether_header *) buf;
	struct iphdr *iph = (struct iphdr *) (buf + sizeof(struct ether_header));
	struct udphdr *udph = (struct udphdr *) (buf + sizeof(struct iphdr) + sizeof(struct ether_header));

	memset(&if_ip, 0, sizeof(struct ifreq));

	/* Open PF_PACKET socket, listening for EtherType ETHER_TYPE */
	if ((sockfd = socket(PF_PACKET, SOCK_RAW, htons(ETHER_TYPE))) == -1) {
		perror("listener: socket");	
		return -1;
	}

	/* Set interface to promiscuous mode - do we need to do this every time? */
	strncpy(ifopts.ifr_name, ifName, IFNAMSIZ-1);
	ioctl(sockfd, SIOCGIFFLAGS, &ifopts);
	ifopts.ifr_flags |= IFF_PROMISC;
	ioctl(sockfd, SIOCSIFFLAGS, &ifopts);
	/* Allow the socket to be reused - incase connection is closed prematurely */
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &sockopt, sizeof sockopt) == -1) {
		perror("setsockopt");
		shutdown(sockfd,1);
		exit(EXIT_FAILURE);
	}
	/* Bind to device */
	if (setsockopt(sockfd, SOL_SOCKET, SO_BINDTODEVICE, ifName, IFNAMSIZ-1) == -1)	{
		perror("SO_BINDTODEVICE");
		shutdown(sockfd, 1);
		exit(EXIT_FAILURE);
	}

	//to indicate sending is in progress (according to received packets,
	//this could be changed) -> check code around line 181
  bool recv_progress = 1;

	cout << "listener: Waiting to recvfrom..." << endl;
	while (recv_progress == 1)
	{
		numbytes = recvfrom(sockfd, buf, BUF_SIZ, 0, NULL, NULL);
			//for timestamps
			struct timeval recv_time;		
			//get current precise timestamp
			gettimeofday(&recv_time, NULL);
			
		/* Check the packet is for me */
		if (eh->ether_dhost[0] == DEST_MAC0 &&
				eh->ether_dhost[1] == DEST_MAC1 &&
				eh->ether_dhost[2] == DEST_MAC2 &&
				eh->ether_dhost[3] == DEST_MAC3 &&
				eh->ether_dhost[4] == DEST_MAC4 &&
				eh->ether_dhost[5] == DEST_MAC5) 
				{
				//Correct destination MAC address
				//parsing remaining data
				
				//the next 8 digits represent the senders MAC, we don't care 
				//about this ether_dhost[6-7-8-9-10-11]
				//the next 4 digits represent ether type, we don't care 
				//about this ether_dhost[12-13]
				
				//now, comes the packet counter in 8 digits, convert all 2-digit
				//parts to string and then concatenate them
				
				int packetCount = createIntFromPayload(buf[14],
																							 buf[15],
																							 buf[16],
																							 buf[17]);
				if(DEBUG)
					cout << "packetcount: " << packetCount << endl;
				
				int seconds = createIntFromPayload(buf[18],
																					 buf[19],
																					 buf[20],
																					 buf[21]);
				if(DEBUG)
					cout << "seconds: " << seconds << endl;
																					 
				int useconds = createIntFromPayload(buf[22],
																						buf[23],
																						buf[24],
																						buf[25]);
				if(DEBUG)
					cout << "useconds: " << useconds << endl;	
				
				int end_of_stream = createIntFromPayload(buf[26],
																								 buf[27],
																								 buf[28],
																								 buf[29]);	
				if(DEBUG)
				{	
					cout << "end_of_stream ?: " << end_of_stream << endl;
					cout << "packetcount: " << packetCount << endl;
				
					/* Print packet */
					cout << "\tData:";
					for (i=0; i<numbytes; i++) 
					{
						//this C style was easier to print out hex
						printf("%02x:", buf[i]); 
					}
					cout << endl;
				}
				
				//pushing timestamp data to our int arrays
				sent_seconds_array[packetCount] = seconds;
				sent_useconds_array[packetCount] = useconds;
				recv_seconds_array[packetCount] = recv_time.tv_sec;
				recv_useconds_array[packetCount] = recv_time.tv_usec;
								
				if(end_of_stream == 1)
					recv_progress = 0;
				
		} 
		//~ else 
		//~ {
			//~ printf("Not for me\n");
			//~ 
		//~ }
	
		
	}

  shutdown(sockfd,0);
 
  //let's calculate the delay
	get_delay_data();
  
  //free data arrays
  delete [] sent_seconds_array;
  delete [] sent_useconds_array;
  delete [] recv_seconds_array;
  delete [] recv_useconds_array;
  
	return ret;
}
