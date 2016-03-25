#!/bin/bash

red="\033[1;31m"
green="\033[1;32m"
blue="\033[1;34m"
yellow="\033[1;33m"
cyan="\033[1;36m"
white="\033[1;37m"
none="\033[1;0m"
bold="\033[1;1m"

#0 means that output of recv_eth will be filtered by a grep
#1 otherwise
debug=0

function print_help
{
  echo -e "${yellow}Usage: ./start.sh <client_iface> <server_iface> <num_packets> <num_exec>${none}"
  echo -e "${bold}Example: ./start.sh eth2 eth3 100 25${none}"
  exit -1
}

#in order to ensure that once a sudo is set, and there is no need for
#passwords again during the executions
sudo echo ""

c=$1
s=$2
num_pacets=$3
num_exec=$4

if [ $# -lt 4 ]
then
  echo -e "${red}Not enough arguments!${none}"
  print_help
elif [ $# -gt 4 ]
then
  echo -e "${red}Too many arguments!${none}"
  print_help
fi

echo -e "${green}Bringing up interfaces with promiscuous mode${none}"
sudo ifconfig $c up promisc
sudo ifconfig $s up promisc

if [ $debug == 0 ]
then 
  server="for i in {1..$num_exec}; do sudo ./recv_eth ${s} ${num_packets}|grep Avg;done"
else
  server="for i in {1..$num_exec}; do sudo ./recv_eth ${s} ${num_packets};done"
fi

client="for i in {1..$num_exec}; do sudo ./send_eth ${c} ${num_packets};done"

echo "#!/bin/bash" > server.sh
echo $server >> server.sh
echo "echo 'DONE'" >> server.sh

echo "#!/bin/bash" > client.sh
echo $client >> client.sh
echo "echo 'DONE'" >> client.sh

chmod +x server.sh
chmod +x client.sh

echo -e "${bold}Server mode started in a new xterm${none}"
xterm -geometry 100x1000 -hold -title 'Server' -e "./server.sh" &
sleep 1s
echo -e "${bold}Client mode started in a new xterm${none}"
xterm -geometry 100x1000 -hold -title 'Client' -e "./client.sh"





