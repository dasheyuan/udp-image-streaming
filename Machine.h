//
// Created by chenyuan on 19-1-25.
//

#ifndef MACHINE_H
#define MACHINE_H

#include <vector>

class Machine {
private:
    class MachineState *current_;

    std::vector<float> keypoints_;



public:
    Machine();

    ~Machine();

    void setCurrent(MachineState *s) {
        current_ = s;
    }

    void run();

    void setKeypoints(std::vector<float> &keypoints) {
        keypoints_ = keypoints;
    }

    const std::vector<float> &getKeypoints() const {
        return keypoints_;
    }
    int count_ = 0;
};

class MachineState {
public:
    virtual void handle(Machine *m) = 0; //纯虚函数
};

class UpState : public MachineState {
private:

public:
    UpState() {}

    ~UpState() = default;

    void handle(Machine *m) override;


};

class DownState : public MachineState {
private:
public:
    DownState() {}

    ~DownState() = default;

    void handle(Machine *m) override;
};

#endif
