//
// Created by chenyuan on 7/30/19.
//
#include <set>

#include "Machine.h"
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>


//WebSocket
typedef websocketpp::server<websocketpp::config::asio> server;

using websocketpp::connection_hdl;
using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;

class broadcast_server {
public:
    broadcast_server() {
        m_server.init_asio();

        m_server.set_open_handler(bind(&broadcast_server::on_open, this, ::_1));
        m_server.set_close_handler(bind(&broadcast_server::on_close, this, ::_1));
        m_server.set_message_handler(bind(&broadcast_server::on_message, this, ::_1, ::_2));
    }

    void on_open(connection_hdl hdl) {
        m_connections.insert(hdl);
    }

    void on_close(connection_hdl hdl) {
        m_connections.erase(hdl);
    }

    void on_message(connection_hdl hdl, server::message_ptr msg) {
        for (auto it : m_connections) {
            m_server.send(it, msg);
        }
    }

    void run(uint16_t port) {
        m_server.listen(port);
        m_server.start_accept();
        m_server.run();
    }

    void send(std::string msg) {
        for (auto it : m_connections) {
            m_server.send(it, msg, websocketpp::frame::opcode::text);
        }
    }

private:
    typedef std::set<connection_hdl, std::owner_less<connection_hdl>> con_list;

    server m_server;
    con_list m_connections;
};

broadcast_server ws_server;

Machine::Machine() : current_(new DownState()) {
    //std::cout << "Start DownState" << std::endl;
    std::thread t1(&broadcast_server::run, &ws_server, 9009);
    t1.detach();
//    std::thread t2(&WsHub::run, &wsHub);
//    t2.detach();
}

Machine::~Machine() {
    delete current_;
}

void Machine::run() {
    current_->handle(this);
}


void UpState::handle(Machine *m) {

    std::vector<float> points = m->getKeypoints();
    //cout << "fabs: " << fabsf(points.at(7) - points.at(9)) << endl;
    if (points.at(5) > 0 && points.at(7) > 0 && points.at(5) < points.at(7) &&
        fabsf(points.at(7) - points.at(9)) < 20.0) {
        m->setCurrent(new DownState());
        delete this;
    }
}

void DownState::handle(Machine *m) {
    std::vector<float> points = m->getKeypoints();
    if (points.at(5) > 0 && points.at(7) > 0 && points.at(5) > points.at(7) &&
        fabsf(points.at(7) - points.at(9)) < 20.0) {
        m->setCurrent(new UpState());
        ws_server.send("{\"cmd\":\"yes\"}");
        cout << ++m->count_ << endl;
        delete this;
    }
}

