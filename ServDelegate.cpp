//
// Created by chenyuan on 7/29/19.
//

#include <thread>
#include <iostream>
#include "opencv2/opencv.hpp"
#include "ServDelegate.h"
#include <openpose/flags.hpp>

#define BUF_LEN 65540 // Larger than maximum UDP packet size

using namespace cv;
using namespace std;

std::atomic<bool> ready(true);


// Display
DEFINE_bool(no_display, false,
            "Enable to disable the visual display.");
DEFINE_bool(no_send, true,
            "Enable to send the jpg.");

// This worker will just read and return all the jpg files in a directory
void display(const std::shared_ptr<std::vector<std::shared_ptr<op::Datum>>> &datumsPtr) {
    try {
        // User's displaying/saving/other processing here
        // datum.cvOutputData: rendered frame with pose or heatmaps
        // datum.poseKeypoints: Array<float> with the estimated pose
        if (datumsPtr != nullptr && !datumsPtr->empty()) {
            // Display image
            cv::imshow(OPEN_POSE_NAME_AND_VERSION + " - Tutorial C++ API", datumsPtr->at(0)->cvOutputData);
            cv::waitKey(1);
        } else
            op::log("Nullptr or empty datumsPtr found.", op::Priority::High);
    }
    catch (const std::exception &e) {
        op::error(e.what(), __LINE__, __FUNCTION__, __FILE__);
    }
}

Machine::Machine() : current_(new DownState()) {
    std::cout << "DownState" << std::endl;
}

Machine::~Machine() {
    delete current_;
}

void Machine::run() {
    current_->handle(this);
}


void UpState::handle(Machine *m) {

    std::vector<float> points = m->getKeypoints();
    points.at(4);
    if (points.at(1) > 0 && points.at(3) > 0 && points.at(1) < points.at(3)) {
        m->setCurrent(new DownState());
        delete this;
    }
}

void DownState::handle(Machine *m) {
    std::vector<float> points = m->getKeypoints();
    if (points.at(1) > 0 && points.at(3) > 0 && points.at(1) > points.at(3)) {
        m->setCurrent(new UpState());
        //server1.send("{\"cmd\":\"yes\"}");
        cout << m->count_++ << endl;
        delete this;
    }
}

ServDelegate::ServDelegate() {
    sock_.setLocalPort(PORT);
    opWrapper.start();
}

void ServDelegate::sendTo(std::vector<unsigned char> &data) {
    try {
        int total_pack = 1 + (data.size() - 1) / PACK_SIZE;
        int ibuf[1];
        ibuf[0] = total_pack;
        sock_.sendTo(ibuf, sizeof(int), foreignAddress_, foreignPort_);

        for (int i = 0; i < total_pack; i++)
            sock_.sendTo(&data[i * PACK_SIZE], PACK_SIZE, foreignAddress_, foreignPort_);
    } catch (SocketException &e) {
        cerr << e.what() << endl;
        exit(1);
    }
}

void ServDelegate::setForeignAddress(const string &foreignAddress) {
    foreignAddress_ = foreignAddress;
}

void ServDelegate::setForeignPort(unsigned short foreignPort) {
    foreignPort_ = foreignPort;
}


ServDelegate::~ServDelegate() {
    opWrapper.stop();
}

void ServDelegate::cmdHandle() {
    while (1) {
        char buffer[BUF_LEN]; // Buffer for echo string
        int recvMsgSize; // Size of received message
        string sourceAddress; // Address of datagram source
        unsigned short sourcePort; // Port of datagram source
        // Block until receive message from a client
        do {
            recvMsgSize = sock_.recvFrom(buffer, BUF_LEN, sourceAddress, sourcePort);
        } while (recvMsgSize > sizeof(int));
        if (buffer[0] == 49) {
            ready = true;
        } else {
            ready = false;
        }
        setForeignAddress(sourceAddress);
        setForeignPort(sourcePort);
    }
}

void ServDelegate::sendHandle() {

    int jpegqual = ENCODE_QUALITY; // Compression Parameter

    Mat frame, send, imageToProcess;
    vector<uchar> encoded;
    VideoCapture cap("../test2.mp4"); // Grab the camera

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

    while (true) {
        while (ready) {
            cap >> imageToProcess;
            auto datumProcessed = opWrapper.emplaceAndPop(imageToProcess);
            if (datumProcessed != nullptr) {
                //TODO
                printKeypoints(datumProcessed);
                frame = datumProcessed->at(0)->cvOutputData.clone();
                if (!FLAGS_no_display)
                    display(datumProcessed);

            } else
                op::log("Image could not be processed.", op::Priority::High);


            if (count == frames_count - 1) {
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
            if (!FLAGS_no_send)
                sendTo(encoded);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
}

void ServDelegate::printKeypoints(const std::shared_ptr<std::vector<std::shared_ptr<op::Datum>>> &datumsPtr) {
    try {
        // Example: How to use the pose keypoints
        if (datumsPtr != nullptr && !datumsPtr->empty()) {
            //op::log("Body keypoints: " + datumsPtr->at(0)->poseKeypoints.toString(), op::Priority::High);
            const auto &poseKeypoints = datumsPtr->at(0)->poseKeypoints;

            std::vector<float> keypoints;
            for (auto person = 0; person < poseKeypoints.getSize(0); person++) {
//                keypoints.push_back(poseKeypoints[{0, 1, 0}]);
//                keypoints.push_back(poseKeypoints[{0, 1, 1}]);//Neck
//                keypoints.push_back(poseKeypoints[{0, 2, 0}]);
//                keypoints.push_back(poseKeypoints[{0, 2, 1}]);//RShoulder
                keypoints.push_back(poseKeypoints[{0, 3, 0}]);//0
                keypoints.push_back(poseKeypoints[{0, 3, 1}]);//RElbow 1
                keypoints.push_back(poseKeypoints[{0, 4, 0}]);// 2
                keypoints.push_back(poseKeypoints[{0, 4, 1}]);//RWrist 3
                keypoints.push_back(poseKeypoints[{0, 7, 0}]);// 4
                keypoints.push_back(poseKeypoints[{0, 7, 1}]);//LWrist 5
                fsm_.setKeypoints(keypoints);
                fsm_.run();
            }
        } else
            op::log("Nullptr or empty datumsPtr found.", op::Priority::High);
    }
    catch (const std::exception &e) {
        op::error(e.what(), __LINE__, __FUNCTION__, __FILE__);
    }
}
