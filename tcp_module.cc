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

#define MSS 536


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
                cerr << "\n";
                cerr << "Packet received\n";
                Packet p;
                MinetReceive(mux, p);
                p.ExtractHeaderFromPayload<TCPHeader>(TCP_HEADER_BASE_LENGTH);
                TCPHeader th = p.FindHeader(Headers::TCPHeader);
                IPHeader ih = p.FindHeader(Headers::IPHeader);
                IPAddress srcAddr, dstAddr;
                ih.GetSourceIP(srcAddr);
		        ih.GetDestIP(dstAddr);
                unsigned char flags;
                unsigned short srcPort, destPort;
                th.GetFlags(flags);
                th.GetSourcePort(destPort);
                th.GetDestPort(srcPort);
                if (IS_SYN(flags)) {
                    // TODO After network facing complete, implement check for server socket

                    Packet resp;

                    IPHeader ipResp;
		            ipResp.SetTotalLength(TCP_HEADER_BASE_LENGTH + IP_HEADER_BASE_LENGTH);
                    ipResp.SetSourceIP(dstAddr); // TODO Implement better source of our IP address
                    ipResp.SetDestIP(srcAddr);
                    ipResp.SetProtocol(IP_PROTO_TCP);
                    resp.PushFrontHeader(ipResp);

                    cerr << "No error after IPHeader\n";

                    TCPHeader tcpResp;
                    unsigned int dSeq;
                    th.GetSeqNum(dSeq);
                    dSeq += 1;
                    tcpResp.SetAckNum(dSeq, resp);
                    unsigned int sSeq = 0;
                    tcpResp.SetSeqNum(sSeq, resp);
                    SET_ACK(flags);
                    cerr << "Flags: " << flags << "\n";
                    tcpResp.SetFlags(flags, resp);
                    tcpResp.SetSourcePort(srcPort, resp);
                    tcpResp.SetDestPort(destPort, resp);
                    tcpResp.SetWinSize(MSS, resp);
                    tcpResp.SetHeaderLen(TCP_HEADER_BASE_LENGTH, resp);
                    resp.PushBackHeader(tcpResp);

                    cerr << "No error after TCPHeader\n";



                    tcpResp.ComputeChecksum(resp);
                    int e = MinetSend(mux, resp);
		            cerr << "Response code: " << e << "\n";

                    // TODO Setup for accept()
                    cerr << "Connection established with " << srcAddr << " on " << dstAddr << "\n";

                    ipResp.GetSourceIP(srcAddr);
                    ipResp.GetDestIP(dstAddr);
                    cerr << srcAddr << " -> " << dstAddr << "\n";
                    cerr << resp << "\n";
                } else
                    cerr << "Other packet received.";
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
