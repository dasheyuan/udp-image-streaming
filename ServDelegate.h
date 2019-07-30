//
// Created by chenyuan on 7/29/19.
//

#ifndef SERVDELEGATE_H
#define SERVDELEGATE_H

#include <vector>
#include <atomic>
#include "PracticalSocket.h"
#include "config.h"
#include "Machine.h"

// OpenPose dependencies
#include <openpose/headers.hpp>

#define PORT 10101

using namespace std;

class ServDelegate {
public:
    ServDelegate();
    ~ServDelegate();

    void sendTo(vector< unsigned char> &data);
    void setForeignAddress(const string &foreignAddress);
    void setForeignPort(unsigned short foreignPort);

    UDPSocket sock_;
    op::Wrapper opWrapper{op::ThreadManagerMode::Asynchronous};
    Machine fsm_;

    void cmdHandle();
    void sendHandle();

private:
    string foreignAddress_="127.0.0.1";
    unsigned short foreignPort_=10100;

    void printKeypoints(const std::shared_ptr<std::vector<std::shared_ptr<op::Datum>>>& datumsPtr);
};


#endif //SERVDELEGATE_H
