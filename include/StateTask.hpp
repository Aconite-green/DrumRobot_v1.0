#pragma once

#include <stdio.h>
#include <map>
#include <memory>
#include <string>
#include <functional>
#include <queue>
#include <algorithm>
#include <thread>
#include <cerrno>  // errno
#include <cstring> // strerror
#include <sstream>
#include <iomanip>
#include <filesystem>
#include <iostream>
#include <vector>
#include <limits>
#include <ctime>
#include <fstream>
#include <atomic>
#include <cmath>
#include <chrono>
#include <set>

#include "SystemState.hpp"
#include "SenSor.hpp"
#include "../include/CanSocketUtils.hpp"
#include "../include/CommandParser.hpp"
#include "../include/ErrorHandle.hpp"
#include "../include/Motor.hpp"
#include "../include/TaskUtility.hpp"
#include "../include/Global.hpp"

#include <QObject>

using namespace std;

class StateTask : public QObject
{
    Q_OBJECT

signals:
    void stateChanged(Main newState);

public:
    // 생성자 선언
    StateTask(SystemState &systemStateRef,
              CanSocketUtils &canUtilsRef,
              std::map<std::string, std::shared_ptr<TMotor>> &tmotorsRef,
              std::map<std::string, std::shared_ptr<MaxonMotor>> &maxonMotorsRef);

    // operator() 함수 선언
    void operator()();

private:
    SystemState &systemState; // 상태 참조
    CanSocketUtils &canUtils;
    std::map<std::string, std::shared_ptr<TMotor>> &tmotors; // 모터 배열
    std::map<std::string, std::shared_ptr<MaxonMotor>> &maxonMotors;

    TMotorCommandParser TParser;
    MaxonCommandParser MParser;
    Sensor sensor;

    // State Utility
    void displayAvailableCommands() const;
    bool processInput(const std::string &input);
    void idealStateRoutine();
    void checkUserInput();

    // System Initiallize
    void initializeTMotors();
    void initializeCanUtils();
    void ActivateControlTask();
    vector<string> extractIfnamesFromMotors(const map<string, shared_ptr<TMotor>> &motors);
    void DeactivateControlTask();
    bool CheckCurrentPosition(std::shared_ptr<TMotor> motor);
    bool CheckAllMotorsCurrentPosition();

    // Home
    void homeModeLoop();
    void SetHome(std::shared_ptr<TMotor> &motor, const std::string &motorName);
    void HomeMotor(std::shared_ptr<TMotor> &motor, const std::string &motorName);
    float MoveMotorToSensorLocation(std::shared_ptr<TMotor> &motor, const std::string &motorName, int sensorBit);
    void RotateMotor(std::shared_ptr<TMotor> &motor, const std::string &motorName, double direction, double degree, float midpoint);
    void SendCommandToMotor(std::shared_ptr<TMotor> &motor, struct can_frame &frame, const std::string &motorName);
    bool PromptUserForHoming(const std::string &motorName);
    void displayHomingStatus();
    void UpdateHomingStatus();

    // Tune
    void FixMotorPosition();
    void Tuning(float kp, float kd, float sine_t, const std::string selectedMotor, int cycles, float peakAngle, int pathType);
    void TuningLoopTask();
    void InitializeTuningParameters(const std::string selectedMotor, float &kp, float &kd, float &peakAngle, int &pathType);

    // Perform
    void runModeLoop();
};
