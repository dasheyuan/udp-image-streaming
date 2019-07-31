//
// Created by chenyuan on 7/31/19.
//

#ifndef WSHUB_H
#define WSHUB_H

#include <uWS/uWS.h>
class WsHub {
public:
    WsHub();
    ~WsHub();

    uWS::Hub h_;

    void send();

    void run();
};


#endif //WSHUB_H
