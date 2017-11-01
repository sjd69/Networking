// You will build this in project part B - this is merely a
// stub that does nothing but integrate into the stack

// For project parts A and B, an appropriate binary will be
// copied over as part of the build process



#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>


#include <iostream>

#include "Minet.h"

using namespace std;

struct TCPState {
    // need to write this
    std::ostream & Print(std::ostream &os) const {
	os << "TCPState()" ;
	return os;
    }
};


int main(int argc, char * argv[]) {
    MinetHandle mux;
    MinetHandle sock;

    ConnectionList<TCPState> clist;

    MinetInit(MINET_TCP_MODULE);

    mux = MinetIsModuleInConfig(MINET_IP_MUX) ?
	MinetConnect(MINET_IP_MUX) :
	MINET_NOHANDLE;

    sock = MinetIsModuleInConfig(MINET_SOCK_MODULE) ?
	MinetAccept(MINET_SOCK_MODULE) :
	MINET_NOHANDLE;

    if ( (mux == MINET_NOHANDLE) &&
	 (MinetIsModuleInConfig(MINET_IP_MUX)) ) {

	MinetSendToMonitor(MinetMonitoringEvent("Can't connect to ip_mux"));

	return -1;
    }

    if ( (sock == MINET_NOHANDLE) &&
	 (MinetIsModuleInConfig(MINET_SOCK_MODULE)) ) {

	MinetSendToMonitor(MinetMonitoringEvent("Can't accept from sock_module"));

	return -1;
    }

    cerr << "tcp_module STUB VERSION handling tcp traffic.......\n";

    MinetSendToMonitor(MinetMonitoringEvent("tcp_module STUB VERSION handling tcp traffic........"));

    MinetEvent event;
    double timeout = 1;

    while (MinetGetNextEvent(event, timeout) == 0) {

	    if ((event.eventtype == MinetEvent::Dataflow) &&
	       (event.direction == MinetEvent::IN)) {

	        if (event.handle == mux) {
		        // ip packet has arrived!
                cerr << "Packet received\n";
                Packet p;
                MinetReceive(mux, p);
                p.ExtractHeaderFromPayload<TCPHeader>(TCP_HEADER_BASE_LENGTH);
                TCPHeader th = p.FindHeader(Headers::TCPHeader);
                IPHeader ih = p.FindHeader(Headers::IPHeader);
                IPAddress addr;
                ih.GetSourceIP(addr);
                unsigned char flags;
                unsigned short srcPort, destPort;
                th.GetFlags(flags);
                th.GetSourcePort(destPort);
                th.GetDestPort(srcPort);
                if (IS_SYN(flags)) {
                    // TODO After network facing complete, implement check for server socket

                    TCPHeader tcpResp;
                    SETACK(flags);
                    tcpResp.SetFlags(flags);
                    int dSeq;
                    tcpResp.GetSeqNum(dSeq);
                    dSeq += 1;
                    tcpResp.SetAckNum();
                    sSeq = rand();
                    tcpResp.setSeqNum();
                    tcpResp.SetSourcePort(srcPort);
                    tcpResp.SetDestPort(destPort);

                    IPHeader ipResp;
                    ipResp.SetProtocol(IP_PROTO_TCP);
                    ipResp.SetDestIP(addr);
                    ih.GetDestIP(addr); // TODO Implement better source of our IP address
                    ipResp.SetSourceIP(addr);
                    ipResp.SetLength(TCP_HEADER_BASE_LENGTH + IP_HEADER_BASE_LENGTH);
                    ih.GetSourceIP(addr);

                    Packet resp;
                    resp.pushFrontHeader(ipResp);
                    resp.pushBackHeader(tcpResp);

                    MinetSend(mux, p);

                    // TODO Setup for accept()
                    ih.GetSourceIP(addr);
                    cerr << "Connection established with " << addr << "\n";
                }
	        }

	        if (event.handle == sock) {
		        // socket request or tcpResponse has arrived
	        }
	    }

	    if (event.eventtype == MinetEvent::Timeout) {
	        // timeout ! probably need to resend some packets
                //cerr << "Timer\n";
	    }

    }

    MinetDeinit();

    return 0;
}
