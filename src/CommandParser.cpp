#include "CommandParser.hpp"

// Tmotor parser

void TMotorCommandParser::parseSendCommand(TMotor &motor, struct can_frame *frame, int canId, int dlc, float p_des, float v_des, float kp, float kd, float t_ff)
{
    // 모터 타입에 따른 제한값 설정
    setMotorLimits(motor);

    // 기존 변수를 계산
    p_des = fminf(fmaxf(GLOBAL_P_MIN, p_des), GLOBAL_P_MAX);
    v_des = fminf(fmaxf(GLOBAL_V_MIN, v_des), GLOBAL_V_MAX);
    kp = fminf(fmaxf(GLOBAL_KP_MIN, kp), GLOBAL_KP_MAX);
    kd = fminf(fmaxf(GLOBAL_KD_MIN, kd), GLOBAL_KD_MAX);
    t_ff = fminf(fmaxf(GLOBAL_T_MIN, t_ff), GLOBAL_T_MAX);

    motor.desPos = p_des;
    motor.desVel = v_des;
    motor.desTor = t_ff;

    // 계산된 변수를 이용하여 unsigned int로 변환
    int p_int = float_to_uint(p_des, GLOBAL_P_MIN, GLOBAL_P_MAX, 16);
    int v_int = float_to_uint(v_des, GLOBAL_V_MIN, GLOBAL_V_MAX, 12);
    int kp_int = float_to_uint(kp, GLOBAL_KP_MIN, GLOBAL_KP_MAX, 12);
    int kd_int = float_to_uint(kd, GLOBAL_KD_MIN, GLOBAL_KD_MAX, 12);
    int t_int = float_to_uint(t_ff, GLOBAL_T_MIN, GLOBAL_T_MAX, 12);

    // Set CAN frame id and data length code
    frame->can_id = canId & CAN_SFF_MASK; // Replace YOUR_CAN_ID with the appropriate id
    frame->can_dlc = dlc;                 // Data Length Code is set to maximum allowed length

    /// pack ints into the can buffer ///
    frame->data[0] = p_int >> 8;                           // Position 8 higher
    frame->data[1] = p_int & 0xFF;                         // Position 8 lower
    frame->data[2] = v_int >> 4;                           // Speed 8 higher
    frame->data[3] = ((v_int & 0xF) << 4) | (kp_int >> 8); // Speed 4 bit lower KP 4bit higher
    frame->data[4] = kp_int & 0xFF;                        // KP 8 bit lower
    frame->data[5] = kd_int >> 4;                          // Kd 8 bit higher
    frame->data[6] = ((kd_int & 0xF) << 4) | (t_int >> 8); // KP 4 bit lower torque 4 bit higher
    frame->data[7] = t_int & 0xff;                         // torque 4 bit lower
}

void TMotorCommandParser::setMotorLimits(TMotor &motor)
{
    if (motor.motorType == "AK10_9")
    {
        GLOBAL_V_MIN = -50;
        GLOBAL_V_MAX = 50;
        GLOBAL_T_MIN = -65;
        GLOBAL_T_MAX = 65;
    }
    else if (motor.motorType == "AK70_10")
    {
        GLOBAL_V_MIN = -50;
        GLOBAL_V_MAX = 50;
        GLOBAL_T_MIN = -25;
        GLOBAL_T_MAX = 25;
    }
    else if (motor.motorType == "AK60_6")
    {
        GLOBAL_V_MIN = -45;
        GLOBAL_V_MAX = 45;
        GLOBAL_T_MIN = -15;
        GLOBAL_T_MAX = 15;
    }
    else if (motor.motorType == "AK80_6")
    {
        GLOBAL_V_MIN = -76;
        GLOBAL_V_MAX = 76;
        GLOBAL_T_MIN = -12;
        GLOBAL_T_MAX = 12;
    }
    else if (motor.motorType == "AK80_9")
    {
        GLOBAL_V_MIN = -50;
        GLOBAL_V_MAX = 50;
        GLOBAL_T_MIN = -18;
        GLOBAL_T_MAX = 18;
    }
    else if (motor.motorType == "AK80_80" || motor.motorType == "AK80_64")
    {
        GLOBAL_V_MIN = -8;
        GLOBAL_V_MAX = 8;
        GLOBAL_T_MIN = -144;
        GLOBAL_T_MAX = 144;
    }
    else if (motor.motorType == "AK80_8")
    {
        GLOBAL_V_MIN = -37.5;
        GLOBAL_V_MAX = 37.5;
        GLOBAL_T_MIN = -32;
        GLOBAL_T_MAX = 32;
    }
    else
    {
        std::cout << "Error: Invalid motor motorType entered!" << std::endl;
    }
}

std::tuple<int, float, float, float> TMotorCommandParser::parseRecieveCommand(TMotor &motor, struct can_frame *frame)
{
    int id;
    float position, speed, torque;
    setMotorLimits(motor);
    /// unpack ints from can buffer ///
    id = frame->data[0];
    int p_int = (frame->data[1] << 8) | frame->data[2];
    int v_int = (frame->data[3] << 4) | (frame->data[4] >> 4);
    int i_int = ((frame->data[4] & 0xF) << 8) | frame->data[5];

    /// convert ints to floats ///
    position = uint_to_float(p_int, GLOBAL_P_MIN, GLOBAL_P_MAX, 16);
    speed = uint_to_float(v_int, GLOBAL_V_MIN, GLOBAL_V_MAX, 12);
    torque = uint_to_float(i_int, GLOBAL_T_MIN, GLOBAL_T_MAX, 12);

    motor.outPos = position;
    motor.outVel = speed;
    motor.outTor = torque;

    return std::make_tuple(id, position, speed, torque);
}
int TMotorCommandParser::float_to_uint(float x, float x_min, float x_max, unsigned int bits)
{
    float span = x_max - x_min;
    if (x < x_min)
        x = x_min;
    else if (x > x_max)
        x = x_max;
    return (int)((x - x_min) * ((float)((1 << bits) / span)));
};

float TMotorCommandParser::uint_to_float(int x_int, float x_min, float x_max, int bits)
{
    float span = x_max - x_min;
    float offset = x_min;
    return ((float)x_int) * span / ((float)((1 << bits) - 1)) + offset;
}

//////////////////////////////////////////////////////
// Maxon Parser definition
/////////////////////////////////////////////////////

void MaxonCommandParser::parseSendCommand(MaxonMotor &motor, struct can_frame *frame, float p_des_radians)
{
    // 라디안 값을 인코더 값으로 변환
    float p_des_degrees = p_des_radians * (180.0f / M_PI);                        // 라디안을 도로 변환
    int p_des_enc = static_cast<int>(p_des_degrees * (35.0f * 4096.0f) / 360.0f); // 도를 인코더 값으로 변환

    unsigned char posByte0 = p_des_enc & 0xFF;         // 하위 8비트
    unsigned char posByte1 = (p_des_enc >> 8) & 0xFF;  // 다음 8비트
    unsigned char posByte2 = (p_des_enc >> 16) & 0xFF; // 다음 8비트
    unsigned char posByte3 = (p_des_enc >> 24) & 0xFF; // 최상위 8비트

    // Set CAN frame id and data length code
    frame->can_id = motor.txPdoIds[1];
    frame->can_dlc = 4;

    /// pack ints into the can buffer ///
    frame->data[0] = posByte0;
    frame->data[1] = posByte1;
    frame->data[2] = posByte2;
    frame->data[3] = posByte3;
    frame->data[4] = 0x00;
    frame->data[5] = 0x00;
    frame->data[6] = 0x00;
    frame->data[7] = 0x00;
}

std::tuple<int, float, float> MaxonCommandParser::parseRecieveCommand(MaxonMotor &motor,struct can_frame *frame)
{
    int id = frame->can_id;

    int32_t currentPosition = 0;
    currentPosition |= static_cast<uint8_t>(frame->data[2]);
    currentPosition |= static_cast<uint8_t>(frame->data[3]) << 8;
    currentPosition |= static_cast<uint8_t>(frame->data[4]) << 16;
    currentPosition |= static_cast<uint8_t>(frame->data[5]) << 24;

    int16_t torqueActualValue = 0;
    torqueActualValue |= static_cast<uint8_t>(frame->data[6]);
    torqueActualValue |= static_cast<uint8_t>(frame->data[7]) << 8;

    // Motor rated torque 값을 N·m 단위로 변환 (mNm -> N·m)
    const float motorRatedTorqueNm = 0.127f; // 127 mNm = 0.127 N·m

    // 실제 토크 값을 N·m 단위로 계산
    // Torque actual value는 천분의 일 단위이므로, 실제 토크 값은 (torqueActualValue / 1000) * motorRatedTorqueNm
    float currentTorqueNm = (static_cast<float>(torqueActualValue) / 1000.0f) * motorRatedTorqueNm;

    float currentPositionDegrees = (static_cast<float>(currentPosition) / (35.0f * 4096.0f)) * 360.0f;
    float currentPositionRadians = currentPositionDegrees * (M_PI / 180.0f);


    motor.outPos = currentPositionRadians;
    motor.outTor = currentTorqueNm;

    return std::make_tuple(id, currentPositionRadians, currentTorqueNm);
}


void MaxonCommandParser::makeSync(struct can_frame *frame)
{
    frame->can_id = 0x80; // Replace YOUR_CAN_ID with the appropriate id
    frame->can_dlc = 0;   // Data Length Code is set to maximum allowed length

    /// pack ints into the can buffer ///
    frame->data[0] = 0x00;
}
