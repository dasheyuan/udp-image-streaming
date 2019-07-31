//
// Created by chenyuan on 7/31/19.
//

#include "WsHub.h"

WsHub::WsHub() {
    h_.onMessage([](uWS::WebSocket<uWS::SERVER> *ws, char *message, size_t length, uWS::OpCode opCode) {
        ws->send(message, length, opCode);
    });
}

WsHub::~WsHub() {

}

void WsHub::run() {
    if (h_.listen(3000)) {
        h_.run();
    }
}
