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


    if (argc != 2) { // Test for correct number of parameters
        cerr << "Usage: " << argv[0] << " <Server Port>" << endl;
        exit(1);
    }

    unsigned short servPort = atoi(argv[1]); // First arg:  local port
    string foreignAddress;
    unsigned short foreignPort;
    try {
        UDPSocket sock(servPort);

        pool.enqueue([&sock, &foreignAddress, &foreignPort] {
            while (1) {
                char buffer[BUF_LEN]; // Buffer for echo string
                int recvMsgSize; // Size of received message
                string sourceAddress; // Address of datagram source
                unsigned short sourcePort; // Port of datagram source
                // Block until receive message from a client
                do {
                    recvMsgSize = sock.recvFrom(buffer, BUF_LEN, foreignAddress, foreignPort);
                } while (recvMsgSize > sizeof(int));
            }
        });

        pool.enqueue([&sock, &foreignAddress, &foreignPort] {
            int jpegqual = ENCODE_QUALITY; // Compression Parameter

            Mat frame, send;
            vector<uchar> encoded;
            VideoCapture cap("test.mp4"); // Grab the camera

            //cap.set(CV_CAP_PROP_FRAME_HEIGHT, 360);
            //cap.set(CV_CAP_PROP_FRAME_WIDTH, 640);

            int fps = cap.get(CV_CAP_PROP_FPS);
            int frames_count = cap.get(CV_CAP_PROP_FRAME_COUNT);
            int count = 0;


            //namedWindow("send", CV_WINDOW_AUTOSIZE);
            if (!cap.isOpened()) {
                cerr << "OpenCV Failed to open camera";
                exit(1);
            }

            std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();
            std::chrono::high_resolution_clock::time_point end;

            while (1) {
                if (foreignAddress.empty()) {
                    cout << "ForeignAddress empty..." << endl;
                    std::this_thread::sleep_for(std::chrono::seconds(3));
                    continue;
                }
                cap >> frame;
                if (count == frames_count-1) {
                    cap.set(CV_CAP_PROP_POS_FRAMES, 1);
                    count = 0;
                }
                count++;
                if (frame.size().width == 0)continue;//simple integrity check; skip erroneous data...
                resize(frame, send, Size(FRAME_WIDTH, FRAME_HEIGHT), 0, 0, INTER_LINEAR);
                vector<int> compression_params;
                compression_params.push_back(CV_IMWRITE_JPEG_QUALITY);
                compression_params.push_back(jpegqual);

                imencode(".jpg", send, encoded, compression_params);


                //imshow("send", send);
                int total_pack = 1 + (encoded.size() - 1) / PACK_SIZE;
                Mat rawData = Mat(1, PACK_SIZE * total_pack, CV_8UC1, encoded.data());
//            std::cout<<rawData<<std::endl;
//            std::ofstream ofs ("test1.jpg", std::ofstream::out);
//            ofs.write((char*)rawData.data,PACK_SIZE * total_pack*sizeof(uchar));
//            ofs.close();
//            break;
                int ibuf[1];
                ibuf[0] = total_pack;
                sock.sendTo(ibuf, sizeof(int), foreignAddress, foreignPort);

                for (int i = 0; i < total_pack; i++)
                    sock.sendTo(&encoded[i * PACK_SIZE], PACK_SIZE, foreignAddress, foreignPort);

                //waitKey(1000/fps);

                end = std::chrono::high_resolution_clock::now();
                auto cost = (int) std::chrono::duration_cast<std::chrono::milliseconds>(
                        end - start).count();
                cout << "effective FPS:" << 1.0 / (cost / 1000.0) << " \tkB/s:"
                     << (PACK_SIZE * total_pack / (cost / 1000.0) / 1024)
                     << endl;
                start = end;
            }
        });
        for (;;) {

        }
    } catch (SocketException &e) {
        cerr << e.what() << endl;
        exit(1);
    }

    return 0;
}