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
#include <sys/time.h>
#include <fcntl.h>
#include <errno.h>


#include <iostream>

#include "Minet.h"
#include "tcpstate.h"

using namespace std;

/*
enum MODE {
    ZERO = 0,
    CONNECTING_LOCAL_INIT, // Entered via connect()
    ACCEPTING, // Entered via accept()
    CONNECTING_REMOTE_INIT, // Entered after SYNACK, but before ACK
    NORMAL, // CONNECTED already used in sock_mod_structs.h
    CLOSED_LOCALLY, // Entered after local close()
    CLOSED_REMOTELY, // Entered after FIN
    CLOSED_TOTALLY, // Enter after close(), then FIN
    CLOSED_WAITING // Entered after FIN, then close (waiting on ACK to our FIN)
};

struct TCPState {
    MODE mode;
    unsigned int localCurrentACK, localNextACK;
    unsigned int remoteSeqNum;

    Buffer &SendBuffer, &ReceiveBuffer;

    double EstimatedRTT, DevRTT;
    struct timeval sendTime;

    bool sockRequestValid;
    SockRequestResponse request;


    // need to write this
    std::ostream &Print(std::ostream &os) const {
        os << "TCPState()";
        return os;
    }
};
*/

enum HEADER_TYPES {
    SYN,
    SYNACK,
    ACK,
    PSHACK,
    FIN,
    FINACK,
    RESET
};

#define MSS 536

void generateTCPHeader(TCPHeader &, Packet &, unsigned short, unsigned short, unsigned int, unsigned int, unsigned char,
                       unsigned short);

void generateIPHeader(IPHeader &, IPAddress, IPAddress, unsigned short);

int main(int argc, char *argv[]) {
    MinetHandle mux;
    MinetHandle sock;

    ConnectionList <TCPState> clist;

    MinetInit(MINET_TCP_MODULE);

    mux = MinetIsModuleInConfig(MINET_IP_MUX) ?
          MinetConnect(MINET_IP_MUX) :
          MINET_NOHANDLE;

    sock = MinetIsModuleInConfig(MINET_SOCK_MODULE) ?
           MinetAccept(MINET_SOCK_MODULE) :
           MINET_NOHANDLE;

    if ((mux == MINET_NOHANDLE) &&
        (MinetIsModuleInConfig(MINET_IP_MUX))) {

        MinetSendToMonitor(MinetMonitoringEvent("Can't connect to ip_mux"));

        return -1;
    }

    if ((sock == MINET_NOHANDLE) &&
        (MinetIsModuleInConfig(MINET_SOCK_MODULE))) {

        MinetSendToMonitor(MinetMonitoringEvent("Can't accept from sock_module"));

        return -1;
    }

    cerr << "tcp_module STUB VERSION handling tcp traffic.......\n";

    MinetSendToMonitor(MinetMonitoringEvent("tcp_module STUB VERSION handling tcp traffic........"));

    ConnectionList<TCPState> connections;

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
                Connection c (localAddr, remoteAddr, localPort, remotePort, IP_PROTO_TCP);
                if (IS_SYN(flags)) {
                    if (IS_ACK(flags)) { // Active open
                        // TODO Implement client side active open handling
                    } else { // Passive open

                        ConnectionList<TCPState>::iterator it;
                        int connectionListSize = connections.size();
                        int i = 0;
                        for (it = connections.begin(); i < connectionListSize; it++, i++) {
                            ConnectionToStateMapping<TCPState> &m = *it;
                            if (m.state.GetState() == LISTEN) {
                            Packet resp;

                                // IP HEADER
                                IPHeader ipResp;
                                generateIPHeader(ipResp, localAddr, remoteAddr, 0); // TODO Implement better source of our IP address
                                resp.PushFrontHeader(ipResp);

                                // TCP HEADER
                                TCPHeader tcpResp;
                                // Acknowledgement number
                                unsigned int dSeq;
                                th.GetSeqNum(dSeq);
                                dSeq += 1;
                                // Flags
                                SET_ACK(flags);
                                // Window size
                                unsigned short windowSize = MSS;
                                // Generate TCP header
                                generateTCPHeader(tcpResp, resp, localPort, remotePort, 0, dSeq, flags, windowSize);
                                // Push to stack
                                resp.PushBackHeader(tcpResp);

                                tcpResp.ComputeChecksum(resp);
                                MinetSend(mux, resp);

                                // TODO Setup for accept()
                                cerr << "Connection established with " << remoteAddr << " on " << localAddr << "\n";
                                cerr << "Local starting number: " << 0 << "\n";
                                cerr << "Remote starting number: " << dSeq << "\n";

                                // Establishing state
                                m.state.SetState(SYN_RCVD);
                                m.state.SetLastSent(0);
                                m.state.SetLastAcked(-1);
                                m.state.SetLastRecvd(dSeq);

                                m.connection = c;
                            }
                            // TODO Handle lost SYNACK
                        }
                    }

                } else if (IS_FIN(flags)) {
                    // TODO Implement closing logic
                } else { // TODO Will need major update when connection state is stored
                    if (IS_ACK(flags)) {
                        // TODO Handle later when connection state is stored
                    }

                    unsigned short dataLength;
                    unsigned char headerLength;
                    ih.GetTotalLength(dataLength);
                    cerr << "Total packet length: " << dataLength << "\n";
                    ih.GetHeaderLength(headerLength);
                    dataLength -= (headerLength * 4);
                    cerr << "TCP segment length: " << dataLength << "\n";
                    th.GetHeaderLen(headerLength);
                    dataLength -= (headerLength * 4);
                    cerr << "Data length: " << dataLength << "\n";

                    if (dataLength > 0) { // TODO Handle incoing payload.
                        Packet resp;

                        IPHeader ipResp;
                        generateIPHeader(ipResp, localAddr, remoteAddr, 0); // TODO Implement better source of our IP address
                        resp.PushFrontHeader(ipResp);

                        TCPHeader tcpResp;
                        // Acknowledgement
                        unsigned int dSeq;
                        th.GetSeqNum(dSeq);
                        dSeq += dataLength;
                        // Flags
                        unsigned char flags = 0;
                        SET_ACK(flags);
                        generateTCPHeader(tcpResp, resp, localPort, remotePort, 1, dSeq, flags, MSS);
                        resp.PushBackHeader(tcpResp);

                        MinetSend(mux, resp);

                        cerr << "Packet starting at byte " << dSeq << " received.\n";
                    }
                }
            }

            if (event.handle == sock) {
                // socket request or response has arrived
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

void generateIPHeader(IPHeader &h, IPAddress srcAddr, IPAddress dstAddr, unsigned short dataLength) {
    unsigned short length = IP_HEADER_BASE_LENGTH + TCP_HEADER_BASE_LENGTH + dataLength;
    h.SetTotalLength(length);
    h.SetSourceIP(srcAddr);
    h.SetDestIP(dstAddr);
    h.SetProtocol(IP_PROTO_TCP);
}

void generateTCPHeader(TCPHeader &h, Packet &p, unsigned short srcPort, unsigned short dstPort, unsigned int seq,
                       unsigned int ack,
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

//void make_packet(Packet &packet, ConnectionToStateMapping<TCPState> &connectionToStateMapping,
//                 HEADER_TYPE tcpHeaderType, int sizeOfData, bool isTimeout) {
//    cerr << "Creating a Packet" << endl;
//    unsigned char flags = 0;
//    int packetSize = sizeOfData + TCP_HEADER_BASE_LENGTH + IP_HEADER_BASE_LENGTH;
//    IPHeader ipHeader;
//    TCPHeader tcpHeader;
//
//    // Create the IP Header
//    ipHeader.SetSourceIP(connectionToStateMapping.connection.src);
//    ipHeader.SetDestIP(connectionToStateMapping.connection.dest);
//    ipHeader.SetTotalLength(packetSize);
//    ipHeader.SetProtocol(IP_PROTO_TCP);
//    packet.PushFrontHeader(ipHeader);
//    cerr << "\nIPHeader: \n" << ipHeader << endl;
//
//    // Create the TCP Header
//    tcpHeader.SetSourcePort(connectionToStateMapping.connection.srcport, packet);
//    tcpHeader.SetDestPort(connectionToStateMapping.connection.destport, packet);
//    tcpHeader.SetHeaderLen(TCP_HEADER_BASE_LENGTH, packet);
//
//    tcpHeader.SetAckNum(connectionToStateMapping.state.GetLastRecvd(), packet);
//    tcpHeader.SetWinSize(connectionToStateMapping.state.GetRwnd(), packet);
//    tcpHeader.SetUrgentPtr(0, packet);
//
//    // Determine what kind of packet it is
//    switch (tcpHeaderType) {
//        case SYN:
//            SET_SYN(flags);
//            cerr << "HEADER_TYPE = SYN" << endl;
//            break;
//
//        case ACK:
//            SET_ACK(flags);
//            cerr << "HEADER_TYPE = ACK" << endl;
//            break;
//
//        case SYNACK:
//            SET_ACK(flags);
//            SET_SYN(flags);
//            cerr << "HEADER_TYPE = SYNACK" << endl;
//            break;
//
//        case PSHACK:
//            SET_PSH(flags);
//            SET_ACK(flags);
//            cerr << "HEADER_TYPE = PSHACK" << endl;
//            break;
//
//        case FIN:
//            SET_FIN(flags);
//            cerr << "HEADER_TYPE = FIN" << endl;
//            break;
//
//        case FINACK:
//            SET_FIN(flags);
//            SET_ACK(flags);
//            cerr << "HEADER_TYPE = FINACK" << endl;
//            break;
//
//        case HEADERTYPE_RST:
//            SET_RST(flags);
//            cerr << "HEADER_TYPE = RESET" << endl;
//            break;
//
//        default:
//            break;
//    }
//
//    // Set the flag type
//    tcpHeader.SetFlags(flags, packet);
//
//    // Print out the finished TCP header
//    cerr << "\nTCPHeader: \n" << tcpHeader << endl;
//
//    if (isTimeout) {
//        tcpHeader.SetSeqNum(connectionToStateMapping.state.GetLastSent() + 1, packet);
//    }
//    else {
//        tcpHeader.SetSeqNum(connectionToStateMapping.state.GetLastSent(), packet);
//    }
//
//    // Recompute the packet's checksum
//    tcpHeader.RecomputeChecksum(packet);
//
//    // Add header to packet
//    packet.PushBackHeader(tcpHeader);
//
//    cerr << "END OF PACKET CREATION\n" << endl;
//}

void socket_handler(const MinetHandle &mux, const MinetHandle &sock, ConnectionList<TCPState> &connectionList) {
    SockRequestResponse socketRequest;
    SockRequestResponse socketReply;

    MinetReceive(sock, socketRequest);

    ConnectionList<TCPState>::iterator cs = connectionList.FindMatching(socketRequest.connection);

    switch(socketResponse.type) {
        case CONNECT:
            cerr << "\nSOCKET HANDLER - CONNECT\n" << endl;
            Packet packet;
            TCPState clientState = TCPState(0, SYN_SENT, 3);

            //What to set timeout as?
            ConnectionToStateMapping<TCPState> newConnectionToStateMapping
                    = ConnectionToStateMapping(socketRequest.connection, Time() + 3, clientState, true);

            //MAKE PACKET
            MinetSend(mux, packet);

            socketReply.type = STATUS;
            socketReply.error = EOK;
            socketReply.connection = socketRequest.connection;
            MinetSend(sock, socketReply);
            cerr << "\nSOCKET HANDLER - CONNECTION CREATED\n" << endl;

            break;
        case ACCEPT:
            cerr << "\nSOCKET HANDLER - ACCEPT\n" << endl;

            //Sequence number starts at 0, correct?
            TCPState serverState = TCPState(0, LISTEN, 3);

            //What to set timeout as? 0?
            ConnectionToStateMapping<TCPState> newConnectionToStateMapping
                    = ConnectionToStateMapping(socketRequest.connection, Time(), serverState, false);

            connectionList.push_front(newConnectionToStateMapping);

            socketReply.type = STATUS;
            socketReply.error = EOK;
            socketReply.connection = socketRequest.connection;
            MinetSend(sock, socketReply);

            cerr << "\nSOCKET HANDLER - CONNECTION ACCEPTED\n" << endl;

            break;
        case WRITE:
            break;
        case FORWARD:
            break;
        case CLOSE:
            break;
        case STATUS:
            break;
        default:
            break;

    }
}
