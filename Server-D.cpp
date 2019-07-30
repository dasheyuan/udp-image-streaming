/*
 *   C++ UDP socket server for live image upstreaming
 *   Modified from http://cs.ecs.baylor.edu/~donahoo/practical/CSockets/practical/UDPEchoServer.cpp
 *   Copyright (C) 2015
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include "ThreadPool.h"
#include "PracticalSocket.h" // For UDPSocket and SocketException
#include "ServDelegate.h"
#include <iostream>          // For cout and cerr
#include <cstdlib>           // For atoi()
#include <chrono>
#include <fstream>      // std::ofstream
#include <mutex>

#define BUF_LEN 65540 // Larger than maximum UDP packet size

#include "opencv2/opencv.hpp"

using namespace cv;
using namespace std;

#include "config.h"


int main(int argc, char *argv[]) {
    ThreadPool pool(4);
    std::vector<std::future<void> > results;

    ServDelegate servDelegate;

    std::thread t1(&ServDelegate::cmdHandle, &servDelegate);
    std::thread t2(&ServDelegate::sendHandle, &servDelegate);


    t1.join();
    t2.join();
    return 0;
}