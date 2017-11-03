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

void generateTCPHeader(TCPHeader&, Packet&, unsigned short, unsigned short, unsigned int, unsigned int, unsigned char, unsigned short);
void generateIPHeader(IPHeader&, IPAddress, IPAddress, unsigned short);

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
                IPAddress localAddr, remoteAddr;
                ih.GetSourceIP(remoteAddr);
		        ih.GetDestIP(localAddr);
                unsigned char flags;
                unsigned short localPort, remotePort;
                th.GetFlags(flags);
                th.GetSourcePort(remotePort);
                th.GetDestPort(localPort);
                if (IS_SYN(flags)) {
                    // TODO After network facing complete, implement check for server socket

                    Packet resp;

                    // IP HEADER
                    IPHeader ipResp;
                    ipResp.SetTotalLength(IP_HEADER_BASE_LENGTH + TCP_HEADER_BASE_LENGTH);
                    ipResp.SetSourceIP(localAddr); // TODO Implement better source of our IP address
                    ipResp.SetDestIP(remoteAddr);
                    ipResp.SetProtocol(IP_PROTO_TCP);
                    resp.PushFrontHeader(ipResp);

                    // TCP HEADER
                    TCPHeader tcpResp;
                    // Sequence number
                    unsigned int sSeq = 0;
                    // Acknowledgement number
                    unsigned int dSeq;
                    th.GetSeqNum(dSeq);
                    dSeq += 1;
                    // TCP Header length
                    SET_ACK(flags);
                    // Window size
                    unsigned short windowSize = MSS;
                    // Generate TCP header
                    generateTCPHeader(tcpResp, resp, localPort, remotePort, sSeq, dSeq, flags, windowSize);
                    // Push to stack
                    resp.PushBackHeader(tcpResp);

                    tcpResp.ComputeChecksum(resp);
                    MinetSend(mux, p);

                    // TODO Setup for accept()
                    cerr << "Connection established with " << remoteAddr << " on " << localAddr << "\n";

                } else { // TODO Will need major update when connection state is stored
                    if (IS_ACK(flags)) {
                        // TODO Handle later when connection state is stored
                    }

                    unsigned short dataLength;
                    unsigned char headerLength;
                    ih.GetTotalLength(dataLength);
                    ih.GetHeaderLength(headerLength);
                    dataLength -= headerLength;
                    th.GetHeaderLen(headerLength);
                    dataLength -= (headerLength * 4);
                    cerr << "Data length: " << dataLength << "\n";

                    if (dataLength > 0) { // TODO Handle incoing payload.
                        Packet resp;

                        IPHeader ipResp;
                        ipResp.SetTotalLength(IP_HEADER_BASE_LENGTH + TCP_HEADER_BASE_LENGTH + dataLength);
                        ipResp.SetSourceIP(localAddr); // TODO Implement better source of our IP address
                        ipResp.SetDestIP(remoteAddr);
                        ipResp.SetProtocol(IP_PROTO_TCP);
                        resp.PushFrontHeader(ipResp);

                        TCPHeader tcpResp;
                        // Acknowledgement
                        unsigned int dSeq;
                        dSeq += dataLength;
                        // Flags
                        unsigned char flags = 0;
                        SET_ACK(flags);
                        generateTCPHeader(tcpResp, resp, localPort, remotePort, 0, dSeq, flags, MSS);
                        resp.PushBackHeader(tcpResp);

                        MinetSend(mux, resp);

                        cerr << "Packet starting at byte " << dSeq << " received.\n";
                    }
                }
	        }

	        if (event.handle == sock) {
		        // socket request or honse has arrived
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

void generateIPHeader(IPHeader& h, IPAddress srcAddr, IPAddress dstAddr, unsigned short dataLength) {
    unsigned short length = IP_HEADER_BASE_LENGTH + TCP_HEADER_BASE_LENGTH + dataLength;
    h.SetTotalLength(length);
    h.SetSourceIP(srcAddr);
    h.SetDestIP(dstAddr);
    h.SetProtocol(IP_PROTO_TCP);
}

void generateTCPHeader(TCPHeader& h, Packet& p, unsigned short srcPort, unsigned short dstPort, unsigned int seq, unsigned int ack,
        unsigned char flags, unsigned short windowSize) {
    // Source port
    h.SetSourcePort(srcPort, p);
    // Destination prt
    h.SetDestPort(dstPort, p);
    // Sequence number
    h.SetSeqNum(seq, p);
    // Acknowledgement number
    h.SetAckNum(ack, p);
    // TCP Header length
    const unsigned short tcpHeaderLength = 5;
    h.SetHeaderLen(tcpHeaderLength, p);
    // Flags
    h.SetFlags(flags, p);
    // Window size
    h.SetWinSize(windowSize, p);
    // Urgent pointer
    const unsigned short urgentPtr = 0;
    h.SetUrgentPtr(urgentPtr, p);
}
