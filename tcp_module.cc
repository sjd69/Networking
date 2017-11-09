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
void socket_handler(const MinetHandle&, const MinetHandle&, ConnectionList<TCPState>&);
void timeout_handler(const MinetHandle&, const MinetHandle&, ConnectionList<TCPState>&);

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
                        cerr << "Received SYNACK\n";
                        ConnectionList<TCPState>::iterator it;
                        cerr << connections.size() << "\n";
                        for (it = connections.begin(); it != connections.end(); it++) {
                            ConnectionToStateMapping<TCPState> &m = *it;
                            cerr << "Enters loop\n";
                            if ((m.state.GetState() == SYN_SENT || m.state.GetState() == SYN_SENT1)
                                    && m.connection.Matches(c)) {
                                cerr << "Enters if\n";
                                Packet resp;

                                // IP HEADER
                                IPHeader ipResp;
                                generateIPHeader(ipResp, localAddr, remoteAddr, 0); // TODO Implement better source of our IP address
                                resp.PushFrontHeader(ipResp);

                                // TCP HEADER
                                TCPHeader tcpResp;
                                // Sequence number
                                unsigned int s_seq = m.state.last_sent;
                                unsigned int passedAck;
                                th.GetAckNum(passedAck);
                                cerr << s_seq << " vs. " << passedAck << "\n";
                                if (passedAck != s_seq)
                                    continue;

                                // Acknowledgement number
                                unsigned int dSeq;
                                th.GetSeqNum(dSeq);
                                dSeq += 1;
                                // Flags
                                CLR_SYN(flags);
                                // Window size
                                unsigned short windowSize = MSS;
                                // Generate TCP header
                                generateTCPHeader(tcpResp, resp, localPort, remotePort, m.state.last_sent, dSeq, flags, windowSize);
                                // Push to stack
                                resp.PushBackHeader(tcpResp);

                                tcpResp.ComputeChecksum(resp);
                                cerr << "Sending back ACK";
                                MinetSend(mux, resp);

                                if (m.state.GetState() == SYN_SENT) {
                                    // Informing the socket
                                    Buffer b;
                                    SockRequestResponse acceptResponse(WRITE, m.connection, b, 0, EOK);
                                    MinetSend(sock, acceptResponse);

                                    cerr << "Connection established with " << remoteAddr << "\n";
                                    m.state.last_acked = m.state.last_sent;
                                }

                                m.state.SetState(SYN_SENT1);
                                m.state.SetLastRecvd(dSeq);
                                m.bTmrActive = 0;
                                break;
                            }
                        }
                    } else { // Passive open
                        ConnectionList<TCPState>::iterator it;
                        int connectionListSize = connections.size();
                        int i = 0;
                        bool success = 0;
                        for (it = connections.begin(); i < connectionListSize; it++, i++) {
                            ConnectionToStateMapping<TCPState> &m = *it;
                            if ((m.state.GetState() == LISTEN && m.connection.MatchesSource(c)) ||
                                    (m.state.GetState() == SYN_RCVD && m.connection.Matches(c))) {
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

                                if (m.state.GetState() == LISTEN) {
                                    cerr << "Connection established with " << remoteAddr << "\n";
                                    cerr << "Local starting number: " << 0 << "\n";
                                    cerr << "Remote starting number: " << dSeq << "\n";
                                } else
                                    cerr << "Retransmitted SYNACK to " << remoteAddr << "\n";

                                // Establishing state
                                m.state.SetState(SYN_RCVD);
                                m.state.SetLastSent(1);
                                m.state.SetLastAcked(0);
                                m.state.SetLastRecvd(dSeq);
                                m.connection = c;

                                success = 1;
                                break;
                            }
                        }
                        if (!success)
                            cerr << "SYN packet caught, but no one was listening.\n";
                    }
                } else {
                    ConnectionList<TCPState>::iterator it = connections.FindMatching(c);
                    ConnectionToStateMapping<TCPState> &m = *it;

                    unsigned short dataLength;
                    unsigned char headerLength;
                    bool ackNeeded = 0, finNeeded = 0;

                    ih.GetTotalLength(dataLength);
                    cerr << "Total packet length: " << dataLength << "\n";
                    ih.GetHeaderLength(headerLength);
                    dataLength -= (headerLength * 4);
                    cerr << "TCP segment length: " << dataLength << "\n";
                    th.GetHeaderLen(headerLength);
                    dataLength -= (headerLength * 4);
                    cerr << "Data length: " << dataLength << "\n";

                    // ACKNOWLEDGEMENTS
                    if (IS_ACK(flags)) {

                        unsigned int ack;
                        th.GetAckNum(ack);

                        if (m.state.GetState() == SYN_SENT1) {
                            m.state.SetState(ESTABLISHED);
                        }


                        // Saving valid acknowledgements.
                        if (ack == m.state.last_sent) {
                            // Dropping data from buffer
                            m.state.SendBuffer.ExtractFront(m.state.last_sent - m.state.last_acked);

                            // Saving ACKed position
                            m.state.last_acked = m.state.last_sent;
                            cerr << "Acknowledgement confirmed";

                            if (m.state.GetState() == CLOSING) {
                                connections.erase(it);
                            }
                            if (m.state.GetState() == FIN_WAIT1)
                                m.state.SetState(FIN_WAIT2);

                            //if (m.state.GetState() == FIN_WAIT1)
                            //    m.state.SetState(CLOSE_WAIT);
                        }

                        cerr << m.state.last_sent << " " << m.state.last_acked << "\n";

                        // Completing handshake for passive open
                        if (m.state.GetState() == SYN_RCVD) {
                            m.state.SetState(ESTABLISHED);

                            // Informing the socket
                            Buffer b;
                            SockRequestResponse acceptResponse(WRITE, m.connection, b, 0, EOK);
                            MinetSend(sock, acceptResponse);

                            cerr << "Connection accepted with " << remoteAddr << "\n";
                        } else if (m.state.GetState() == CLOSE_WAIT && m.state.SendBuffer.GetSize() == 0) {
                            m.state.SetState(FIN_WAIT1);
                        }

                        // Completing handshake for passive open
                        if (m.state.GetState() == SYN_SENT1) {
                            m.state.SetState(ESTABLISHED);
                            finNeeded = 1;
                        }
                    }

                    // INCOMING DATA
                    if (dataLength > 0) {
                        ackNeeded = 1;

                        // Remote sequence number
                        unsigned int dSeq;
                        th.GetSeqNum(dSeq);

                        if (dSeq == m.state.last_recvd) { // If valid sequence number
                            cerr << "Data received starting at byte number" << dSeq << "\n";

                            // Updating remote sequence number
                            m.state.last_recvd = m.state.last_recvd + dataLength;

                            // Passing up to socket
                            Buffer b = p.GetPayload();
                            SockRequestResponse socketResponse (WRITE, m.connection, b, dataLength, EOK);
                            MinetSend(sock, socketResponse);
                        } else
                            cerr << "Data received with incorred sequence number\n";
                    }

                    // OUTGOING DATA
                    bool writeNeeded = m.state.last_sent == m.state.last_acked && m.state.SendBuffer.GetSize() > 0;

                    if (IS_FIN(flags)) {
                        if (m.state.GetState() == FIN_WAIT1) {
                            m.state.last_sent = m.state.last_acked + 1;
                            finNeeded = 1;
                        } else if (m.state.GetState() == FIN_WAIT2) {
                            m.state.last_recvd++;
                            ackNeeded = 1;
                            m.state.SetState(TIME_WAIT);
                            m.timeout = Time() + 5;
                        } else if (m.state.GetState() == ESTABLISHED) {
                            m.state.SetState(CLOSING);
                            finNeeded = 1;
                            ackNeeded = 1;
                            if (dataLength == 0)
                                m.state.last_recvd++;

                            Buffer b;
                            SockRequestResponse resp (CLOSE, c, b, 0, EOK);
                            MinetSend(sock, resp);
                        }
                    }

                    // WRITING TO REMOTE
                    if (ackNeeded || writeNeeded) {
                        Packet resp;
                        unsigned short payloadLength;

                        if (writeNeeded) {
                            char buf[MSS];
                            size_t size = m.state.SendBuffer.GetSize();
                            payloadLength = size;
                            if (size > MSS)
                                size = MSS;
                            m.state.SendBuffer.GetData(buf, size, 0);
                            const char* constBuf = buf;
                            resp = Packet(constBuf, size);
                        } else
                            payloadLength = 0;

                        IPHeader ipResp;
                        generateIPHeader(ipResp, localAddr, remoteAddr, payloadLength); // TODO Implement better source of our IP address
                        resp.PushFrontHeader(ipResp);

                        TCPHeader tcpResp;
                        // Flags
                        unsigned char flags = 0;
                        if (ackNeeded)
                            SET_ACK(flags);
                        if (writeNeeded)
                            SET_PSH(flags);
                        if (finNeeded)
                            SET_FIN(flags);
                        generateTCPHeader(tcpResp, resp, localPort, remotePort, m.state.last_sent, m.state.last_recvd, flags, MSS);
                        resp.PushBackHeader(tcpResp);

                        tcpResp.ComputeChecksum(resp);
                        MinetSend(mux, resp);

                        if (writeNeeded) {
                            m.timeout = Time() + 1;
                            m.bTmrActive = 1;
                        }
                    }
                }
            }

            if (event.handle == sock) {
                // socket request or response has arrived
                socket_handler(mux, sock, connections);
            }
        }

        if (event.eventtype == MinetEvent::Timeout) {
            timeout_handler(mux, sock, connections);
        }

        // Timeout setup
        ConnectionList<TCPState>::iterator it = connections.FindEarliest();
        if (it != connections.end()) {
            cerr << "Some time found!\n";
            Time& t = (*it).timeout;
            timeout = double(t) - double(Time());
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

void make_packet(Packet &packet, ConnectionToStateMapping<TCPState> &connectionToStateMapping,
                 HEADER_TYPES tcpHeaderType, int sizeOfData) {
    cerr << "Creating a Packet" << endl;
    unsigned char flags = 0;
    int packetSize = sizeOfData + TCP_HEADER_BASE_LENGTH + IP_HEADER_BASE_LENGTH;
    IPHeader ipHeader;
    TCPHeader tcpHeader;

    // Create the IP Header
    ipHeader.SetSourceIP(connectionToStateMapping.connection.src);
    ipHeader.SetDestIP(connectionToStateMapping.connection.dest);
    ipHeader.SetTotalLength(packetSize);
    ipHeader.SetProtocol(IP_PROTO_TCP);
    packet.PushFrontHeader(ipHeader);
    cerr << "\nIPHeader: \n" << ipHeader << endl;

    // Create the TCP Header
    tcpHeader.SetSourcePort(connectionToStateMapping.connection.srcport, packet);
    tcpHeader.SetDestPort(connectionToStateMapping.connection.destport, packet);
    tcpHeader.SetHeaderLen(5, packet);

    tcpHeader.SetAckNum(connectionToStateMapping.state.GetLastRecvd(), packet);
    tcpHeader.SetWinSize(connectionToStateMapping.state.GetRwnd(), packet);
    tcpHeader.SetUrgentPtr(0, packet);

    // Determine what kind of packet it is
    switch (tcpHeaderType) {
        case SYN:
            SET_SYN(flags);
            cerr << "HEADER_TYPE = SYN" << endl;
            break;

        case ACK:
            SET_ACK(flags);
            cerr << "HEADER_TYPE = ACK" << endl;
            break;

        case SYNACK:
            SET_ACK(flags);
            SET_SYN(flags);
            cerr << "HEADER_TYPE = SYNACK" << endl;
            break;

        case PSHACK:
            SET_PSH(flags);
            SET_ACK(flags);
            cerr << "HEADER_TYPE = PSHACK" << endl;
            break;

        case FIN:
            SET_FIN(flags);
            cerr << "HEADER_TYPE = FIN" << endl;
            break;

        case FINACK:
            SET_FIN(flags);
            SET_ACK(flags);
            cerr << "HEADER_TYPE = FINACK" << endl;
            break;

        case RESET:
            SET_RST(flags);
            cerr << "HEADER_TYPE = RESET" << endl;
            break;

        default:
            break;
    }

    // Set the flag type
    tcpHeader.SetFlags(flags, packet);

    // Print out the finished TCP header
    cerr << "\nTCPHeader: \n" << tcpHeader << endl;

	//Not entirely sure what to set the sequence number as here
    tcpHeader.SetSeqNum(connectionToStateMapping.state.last_acked, packet); // See comments for CONNECT about change - Brendan

    // Recompute the packet's checksum
    tcpHeader.RecomputeChecksum(packet);

    // Add header to packet
    packet.PushBackHeader(tcpHeader);

    cerr << "END OF PACKET CREATION\n" << endl;
}

void socket_handler(const MinetHandle &mux, const MinetHandle &sock, ConnectionList<TCPState> &connectionList) {
    SockRequestResponse socketRequest;
    SockRequestResponse socketReply;
    Packet packet;

    MinetReceive(sock, socketRequest);

    ConnectionList<TCPState>::iterator cs = connectionList.FindMatching(socketRequest.connection);

    switch(socketRequest.type) {
        case CONNECT: {
            cerr << "\nSOCKET HANDLER - CONNECT\n" << endl;

            TCPState clientState = TCPState(0, SYN_SENT, 6);
            clientState.last_sent = 1; // Using last_sent as next ack for simplicity of other parts -Brendan
            clientState.last_acked = 0; // Using last_acked as ack for last packet

            //What to set timeout as?
            ConnectionToStateMapping<TCPState> newConnectionToStateMapping
                    = ConnectionToStateMapping<TCPState>(socketRequest.connection, Time() + 3, clientState, true);

			make_packet(packet, newConnectionToStateMapping, SYN, 0);
            MinetSend(mux, packet);

            socketReply.type = STATUS;
            socketReply.error = EOK;
            socketReply.connection = socketRequest.connection;
            MinetSend(sock, socketReply);
            cerr << "\nSOCKET HANDLER - CONNECTION CREATED\n" << endl;

            connectionList.push_front(newConnectionToStateMapping);

            break;
        } case ACCEPT: {
            cerr << "\nSOCKET HANDLER - ACCEPT\n" << endl;

            //Sequence number starts at 0, correct?
            TCPState serverState = TCPState(0, LISTEN, 3);

            //What to set timeout as? 0?
            ConnectionToStateMapping<TCPState> newConnectionToStateMapping
                    = ConnectionToStateMapping<TCPState>(socketRequest.connection, Time(), serverState, false);

            connectionList.push_front(newConnectionToStateMapping);

            socketReply.type = STATUS;
            socketReply.error = EOK;
            socketReply.connection = socketRequest.connection;
            MinetSend(sock, socketReply);

            cerr << "\nSOCKET HANDLER - CONNECTION ACCEPTED\n" << endl;

            break;
        } case WRITE:
        {
            cerr << "\nSOCKET HANDLER - CONNECTION WRITE\n" << endl;

            unsigned int connState = (*cs).state.GetState();

            if (connState == ESTABLISHED || connState == SYN_SENT1) {
                unsigned int bytes = socketRequest.data.GetSize();
                TCPState *state = &cs->state;

                state->SendBuffer.AddBack(socketRequest.data);

                cerr << state->last_sent << " " <<state->last_acked << "\n";
                if (state->last_sent == state->last_acked) {

                    bytes = state->SendBuffer.GetSize();
                    if (bytes > MSS)
                        bytes = MSS;

                    char buf[MSS];
                    state->SendBuffer.GetData(buf, bytes, 0);
                    packet = Packet(buf, bytes);

                    IPHeader ip;
                    generateIPHeader(ip, (*cs).connection.src, (*cs).connection.dest, bytes);
                    packet.PushFrontHeader(ip);

                    TCPHeader tcp;
                    unsigned char flags;
                    SET_PSH(flags);
                    SET_ACK(flags);
                    generateTCPHeader(tcp, packet, (*cs).connection.srcport, (*cs).connection.destport,
                                        (*cs).state.last_acked, (*cs).state.last_recvd, flags, MSS);
                    packet.PushBackHeader(tcp);

                    MinetSend(mux, packet);

                    (*cs).state.last_sent = (*cs).state.last_acked + bytes;
                }

                socketReply.connection = socketRequest.connection;
                socketReply.type = STATUS;
                socketReply.bytes = bytes;
                socketReply.error = EOK;

                MinetSend(sock, socketReply);
            }
        }

            break;
        case FORWARD:
            break;
        case CLOSE:
        {
            cerr << "\nSOCKET HANDLER - CONNECTION CLOSE\n" << endl;
            unsigned  int connState = (*cs).state.GetState();

            if (connState == ESTABLISHED) {
                if ((*cs).state.last_sent == (*cs).state.last_acked) {
                    (*cs).state.SetState(FIN_WAIT1);

                    make_packet(packet, *cs, FIN, 0);
                    (*cs).state.last_sent++;
                    MinetSend(mux, packet);
                } else  {
                    (*cs).state.SetState(CLOSE_WAIT);
                }

                socketReply.type = STATUS;
                socketReply.connection = socketRequest.connection;
                socketReply.error = EOK;
                MinetSend(sock, socketReply);
            }
        }
            break;
        case STATUS:
            //DO we need anything for status?
            break;
        default:
            //DO we need anything for default?
            break;

    }
}

void timeout_handler(const MinetHandle& mux, const MinetHandle& sock, ConnectionList<TCPState>& connections) {
/*
    cerr << "\nTIMEOUT HANDLER\n" << endl;
    ConnectionList<TCPState>::iterator it = connections.FindEarliest();
    unsigned int state = (*it).state.GetState();

    ConnectionToStateMapping<TCPState> &connectionToStateMapping = *it;

    Packet packet;
    char* buf;

    switch (state) {
        case SYN_SENT:
        {
            cerr << "\nTIMEOUT SYN_SENT\n" << endl;
            make_packet(packet, *it, SYN, 0);
            MinetSend(mux, packet);
        }

            break;
        case SYN_RCVD:
        {
            cerr << "\nTIMEOUT SYN_RCVD\n" << endl;
            make_packet(packet, *it, SYNACK, 0);
            MinetSend(mux, packet);
        }

            break;
        case ESTABLISHED:
        {
            cerr << "\nTIMEOUT ESTABLISHED\n" << endl;
            int bufLen = connectionToStateMapping.state.SendBuffer.GetSize();
            int data = connectionToStateMapping.state.SendBuffer.GetData(buf, bufLen, 0);
            packet = Packet(buf, data);
            MinetSend(mux, packet);
        }

            break;
        case FIN_WAIT1:
        {
            cerr << "\nTIMEOUT FIN_WAIT1\n" << endl;
            make_packet(packet, *it, FIN, 0);
            MinetSend(mux, packet);
        }

            break;
        case LAST_ACK:
        {
            cerr << "\nTIMEOUT LACK_ACK\n" << endl;
            make_packet(packet, *it, FIN, 0);
            MinetSend(mux, packet);
        }

            break;
        case TIME_WAIT:
        {
            cerr << "\nTIMEOUT TIME_WAIT\n" << endl;
            (*it).state.SetState(CLOSED);
            connections.erase(it);
        }

            break;
    }
*/
    ConnectionList<TCPState>::iterator it = connections.FindEarliest();
    while (it != connections.end() && (*it).timeout < Time()) {
        ConnectionToStateMapping<TCPState> &m = *it;
        if (m.state.GetState() == SYN_SENT) {
            m.state.tmrTries--;
            if (m.state.tmrTries > 0) {
                Packet resp;

                // IP HEADER
                IPHeader ipResp;
                generateIPHeader(ipResp, m.connection.src, m.connection.dest, 0);
                resp.PushFrontHeader(ipResp);

                // TCP HEADER
                TCPHeader tcpResp;
                unsigned int s_seq = m.state.last_acked;
                unsigned int ackNum = 0;
                unsigned char flags = 0;
                SET_SYN(flags);
                generateTCPHeader(tcpResp, resp, m.connection.srcport, m.connection.destport, s_seq, ackNum, flags, MSS);
                resp.PushBackHeader(tcpResp);

                MinetSend(mux, resp);
                cerr << "\nSYN to " << m.connection.dest << "retransmitted\n";

                // Reset timer
                m.timeout = Time() + 3;

            } else {
                Buffer b;
                SockRequestResponse resp (CONNECT, m.connection, b, 0, ECONN_FAILED);
             MinetSend(sock, resp);
                connections.erase(it);
            }
        }

        it = connections.FindEarliest();
    }
}
