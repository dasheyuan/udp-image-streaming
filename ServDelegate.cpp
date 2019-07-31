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


void configureWrapper(op::Wrapper &opWrapper) {
    try {
        // Configuring OpenPose

        // logging_level
        op::check(0 <= FLAGS_logging_level && FLAGS_logging_level <= 255, "Wrong logging_level value.",
                  __LINE__, __FUNCTION__, __FILE__);
        op::ConfigureLog::setPriorityThreshold((op::Priority) FLAGS_logging_level);
        op::Profiler::setDefaultX(FLAGS_profile_speed);

        // Applying user defined configuration - GFlags to program variables

        // cameraSize
        const auto cameraSize = op::flagsToPoint(FLAGS_camera_resolution, "-1x-1");
        // outputSize
        const auto outputSize = op::flagsToPoint(FLAGS_output_resolution, "-1x-1");
        // netInputSize
        const auto netInputSize = op::flagsToPoint(FLAGS_net_resolution, "-1x368");
        // poseMode
        const auto poseMode = op::flagsToPoseMode(FLAGS_body);
        // poseModel
        const auto poseModel = op::flagsToPoseModel(FLAGS_model_pose);
        //number_people_max
        const auto number_people_max = 1;

        // JSON saving
        if (!FLAGS_write_keypoint.empty())
            op::log("Flag `write_keypoint` is deprecated and will eventually be removed."
                    " Please, use `write_json` instead.", op::Priority::Max);
        // keypointScaleMode
        const auto keypointScaleMode = op::flagsToScaleMode(FLAGS_keypoint_scale);
        // heatmaps to add
        const auto heatMapTypes = op::flagsToHeatMaps(FLAGS_heatmaps_add_parts, FLAGS_heatmaps_add_bkg,
                                                      FLAGS_heatmaps_add_PAFs);
        const auto heatMapScaleMode = op::flagsToHeatMapScaleMode(FLAGS_heatmaps_scale);
        // >1 camera view?
        const auto multipleView = (FLAGS_3d || FLAGS_3d_views > 1 || FLAGS_flir_camera);
        // Face and hand detectors
        const auto faceDetector = op::flagsToDetector(FLAGS_face_detector);
        const auto handDetector = op::flagsToDetector(FLAGS_hand_detector);
        // Enabling Google Logging
        const bool enableGoogleLogging = true;

        // Initializing the user custom classes
        // GUI (Display)
        //auto wUserOutput = std::make_shared<WUserOutput>();
        // Add custom processing
        const auto workerOutputOnNewThread = true;
        //opWrapper.setWorker(op::WorkerType::Output, wUserOutput, workerOutputOnNewThread);

        // Pose configuration (use WrapperStructPose{} for default and recommended configuration)
        const op::WrapperStructPose wrapperStructPose{
                poseMode, netInputSize, outputSize, keypointScaleMode, FLAGS_num_gpu, FLAGS_num_gpu_start,
                FLAGS_scale_number, (float) FLAGS_scale_gap, op::flagsToRenderMode(FLAGS_render_pose, multipleView),
                poseModel, !FLAGS_disable_blending, (float) FLAGS_alpha_pose, (float) FLAGS_alpha_heatmap,
                FLAGS_part_to_show, FLAGS_model_folder, heatMapTypes, heatMapScaleMode, FLAGS_part_candidates,
                (float) FLAGS_render_threshold, number_people_max, FLAGS_maximize_positives, FLAGS_fps_max,
                FLAGS_prototxt_path, FLAGS_caffemodel_path, (float) FLAGS_upsampling_ratio, enableGoogleLogging};
        opWrapper.configure(wrapperStructPose);



        // No GUI. Equivalent to: opWrapper.configure(op::WrapperStructGui{});
        // Set to single-thread (for sequential processing and/or debugging and/or reducing latency)
        //if (FLAGS_disable_multi_thread)
        //opWrapper.disableMultiThreading();
    }
    catch (const std::exception &e) {
        op::error(e.what(), __LINE__, __FUNCTION__, __FILE__);
    }
}


ServDelegate::ServDelegate() {
    sock_.setLocalPort(PORT);
    configureWrapper(opWrapper);
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
    //VideoCapture cap(0);
    cap.set(CV_CAP_PROP_FRAME_HEIGHT, 360);
    cap.set(CV_CAP_PROP_FRAME_WIDTH, 640);

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
            if(imageToProcess.empty()) continue;
            frame = imageToProcess(Rect(imageToProcess.cols/4,0,imageToProcess.rows,imageToProcess.rows));
            auto datumProcessed = opWrapper.emplaceAndPop(frame);
            if (datumProcessed != nullptr) {
                //TODO
                printKeypoints(datumProcessed);
                send = datumProcessed->at(0)->cvOutputData.clone();
                if (!FLAGS_no_display)
                    display(datumProcessed);

            } else
                op::log("Image could not be processed.", op::Priority::High);


            if (count == frames_count - 1) {
                cap.set(CV_CAP_PROP_POS_FRAMES, 1);
                count = 0;
            }
            count++;
            //if (frame.size().width == 0)continue;//simple integrity check; skip erroneous data...
            //resize(frame, send, Size(FRAME_WIDTH, FRAME_HEIGHT), 0, 0, INTER_LINEAR);
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
                keypoints.push_back(poseKeypoints[{0, 1, 0}]);//0
                keypoints.push_back(poseKeypoints[{0, 1, 1}]);//Neck 1
                keypoints.push_back(poseKeypoints[{0, 2, 0}]);//2
                keypoints.push_back(poseKeypoints[{0, 2, 1}]);//RShoulder //3
                keypoints.push_back(poseKeypoints[{0, 3, 0}]);//4
                keypoints.push_back(poseKeypoints[{0, 3, 1}]);//RElbow 5
                keypoints.push_back(poseKeypoints[{0, 4, 0}]);// 6
                keypoints.push_back(poseKeypoints[{0, 4, 1}]);//RWrist 7
                keypoints.push_back(poseKeypoints[{0, 7, 0}]);// 8
                keypoints.push_back(poseKeypoints[{0, 7, 1}]);//LWrist 9
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
