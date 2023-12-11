#include "../include/PathManager.hpp" // 적절한 경로로 변경하세요.

PathManager::PathManager(queue<can_frame> &sendBufferRef, map<string, shared_ptr<TMotor>> &tmotorsRef)
    : sendBuffer(sendBufferRef), tmotors(tmotorsRef)
{
}

void PathManager::motorInitialize(map<string, shared_ptr<TMotor>> &tmotorsRef)
{
    this->tmotors = tmotorsRef;
    // 참조 확인
    cout << "tmotors size in PathManager constructor: " << tmotors.size() << endl;
}

string PathManager::trimWhitespace(const std::string &str)
{
    size_t first = str.find_first_not_of(" \t");
    if (std::string::npos == first)
    {
        return str;
    }
    size_t last = str.find_last_not_of(" \t");
    return str.substr(first, (last - first + 1));
}

vector<double> PathManager::connect(vector<double> &Q1, vector<double> &Q2, int k, int n)
{
    vector<double> Qi;
    std::vector<double> A, B;

    // Compute A and B
    for (long unsigned int i = 0; i < Q1.size(); ++i)
    {
        A.push_back(0.5 * (Q1[i] - Q2[i]));
        B.push_back(0.5 * (Q1[i] + Q2[i]));
    }

    // Compute Qi using the provided formula
    for (long unsigned int i = 0; i < Q1.size(); ++i)
    {
        double val = A[i] * cos(M_PI * k / n) + B[i];
        Qi.push_back(val);
    }

    return Qi;
}

// 행렬의 determinant 계산 함수
double determinant(double mat[3][3])
{
    return mat[0][0] * (mat[1][1] * mat[2][2] - mat[2][1] * mat[1][2]) -
           mat[0][1] * (mat[1][0] * mat[2][2] - mat[2][0] * mat[1][2]) +
           mat[0][2] * (mat[1][0] * mat[2][1] - mat[2][0] * mat[1][1]);
}

// 역행렬 계산 함수
void inverseMatrix(double mat[3][3], double inv[3][3])
{
    double det = determinant(mat);

    if (det == 0)
    {
        std::cerr << "역행렬이 존재하지 않습니다." << std::endl;
        return;
    }

    double invDet = 1.0 / det;

    inv[0][0] = (mat[1][1] * mat[2][2] - mat[2][1] * mat[1][2]) * invDet;
    inv[0][1] = (mat[0][2] * mat[2][1] - mat[0][1] * mat[2][2]) * invDet;
    inv[0][2] = (mat[0][1] * mat[1][2] - mat[0][2] * mat[1][1]) * invDet;

    inv[1][0] = (mat[1][2] * mat[2][0] - mat[1][0] * mat[2][2]) * invDet;
    inv[1][1] = (mat[0][0] * mat[2][2] - mat[0][2] * mat[2][0]) * invDet;
    inv[1][2] = (mat[1][0] * mat[0][2] - mat[0][0] * mat[1][2]) * invDet;

    inv[2][0] = (mat[1][0] * mat[2][1] - mat[2][0] * mat[1][1]) * invDet;
    inv[2][1] = (mat[2][0] * mat[0][1] - mat[0][0] * mat[2][1]) * invDet;
    inv[2][2] = (mat[0][0] * mat[1][1] - mat[1][0] * mat[0][1]) * invDet;
}

void PathManager::iconnect(vector<double> &P0, vector<double> &P1, vector<double> &P2, vector<double> &V0, double t1, double t2, double t)
{
    vector<double> V1;
    vector<double> p_out;
    vector<double> v_out;
    for (size_t i = 0; i < P0.size(); ++i)
    {
        if ((P1[i] - P0[i]) / (P2[i] - P1[i]) > 0)
            V1.push_back((P2[i] - P0[i]) / t2);
        else
            V1.push_back(0);

        double f = P0[i];
        double d = 0;
        double e = V0[i];

        double M[3][3] = {
            {20.0 * pow(t1, 2), 12.0 * t1, 6.0},
            {5.0 * pow(t1, 4), 4.0 * pow(t1, 3), 3.0 * pow(t1, 2)},
            {pow(t1, 5), pow(t1, 4), pow(t1, 3)}};
        double ANS[3] = {0, V1[i] - V0[i], P1[i] - P0[i] - V0[i] * t1};

        double invM[3][3];
        inverseMatrix(M, invM);
        // Multiply the inverse of T with ANS
        double tem[3];
        for (size_t j = 0; j < 3; ++j)
        {
            tem[j] = 0;
            for (size_t k = 0; k < 3; ++k)
            {
                tem[j] += invM[j][k] * ANS[k];
            }
        }

        double a = tem[0];
        double b = tem[1];
        double c = tem[2];

        p_out.push_back(a * pow(t, 5) + b * pow(t, 4) + c * pow(t, 3) + d * pow(t, 2) + e * t + f);
        v_out.push_back(5 * a * pow(t, 4) + 4 * b * pow(t, 3) + 3 * c * pow(t, 2) + 3 * d * t + e);
    }

    p.push_back(p_out);
    v.push_back(v_out);
}

vector<double> PathManager::IKfun(vector<double> &P1, vector<double> &P2, vector<double> &R, double s, double z0)
{
    vector<double> Qf;

    double X1 = P1[0], Y1 = P1[1], z1 = P1[2];
    double X2 = P2[0], Y2 = P2[1], z2 = P2[2];
    double r1 = R[0], r2 = R[1], r3 = R[2], r4 = R[3];

    int j = 0;
    vector<double> the3(100);
    for (int i = 0; i < 100; i++)
    {
        the3[i] = -M_PI / 2 + (M_PI * i) / 99;
    }

    double zeta = z0 - z2;

    double det_the4;
    double the34;
    double the4;
    double r;
    double det_the1;
    double the1;
    double det_the0;
    double the0;
    double L;
    double det_the2;
    double the2;
    double T;
    double det_the5;
    double sol;
    double the5;
    double alpha, beta, gamma;
    double det_the6;
    double rol;
    double the6;
    double Z;

    vector<double> Q0;
    vector<double> Q1;
    vector<double> Q2;
    vector<double> Q3;
    vector<double> Q4;
    vector<double> Q5;
    vector<double> Q6;

    for (int i = 0; i < 99; i++)
    {
        det_the4 = (z0 - z1 - r1 * cos(the3[i])) / r2;

        if (det_the4 < 1 && det_the4 > -1)
        {
            the34 = acos((z0 - z1 - r1 * cos(the3[i])) / r2);
            the4 = the34 - the3[i];

            if (the4 > 0)
            {
                r = r1 * sin(the3[i]) + r2 * sin(the34);

                det_the1 = (X1 * X1 + Y1 * Y1 - r * r - s * s / 4) / (s * r);
                if (det_the1 < 1 && det_the1 > -1)
                {
                    the1 = acos(det_the1);

                    alpha = asin(X1 / sqrt(X1 * X1 + Y1 * Y1));
                    det_the0 = (s / 4 + (X1 * X1 + Y1 * Y1 - r * r) / s) / sqrt(X1 * X1 + Y1 * Y1);
                    if (det_the0 < 1 && det_the0 > -1)
                    {
                        the0 = asin(det_the0) - alpha;

                        L = sqrt(pow(X2 - 0.5 * s * cos(the0 + M_PI), 2) +
                                 pow(Y2 - 0.5 * s * sin(the0 + M_PI), 2));
                        det_the2 = (X2 + 0.5 * s * cos(the0)) / L;

                        if (det_the2 < 1 && det_the2 > -1)
                        {
                            the2 = acos(det_the2) - the0;

                            T = (zeta * zeta + L * L + r3 * r3 - r4 * r4) / (r3 * 2);
                            det_the5 = L * L + zeta * zeta - T * T;

                            if (det_the5 > 0)
                            {
                                sol = T * L - abs(zeta) * sqrt(L * L + zeta * zeta - T * T);
                                sol /= (L * L + zeta * zeta);
                                the5 = asin(sol);

                                alpha = L - r3 * sin(the5);
                                beta = r4 * sin(the5);
                                gamma = r4 * cos(the5);

                                det_the6 = gamma * gamma + beta * beta - alpha * alpha;

                                if (det_the6 > 0)
                                {
                                    rol = alpha * beta - abs(gamma) * sqrt(det_the6);
                                    rol /= (beta * beta + gamma * gamma);
                                    the6 = acos(rol);
                                    Z = z0 - r1 * cos(the5) - r2 * cos(the5 + the6);

                                    if (Z < z2 + 0.001 && Z > z2 - 0.001)
                                    {
                                        Q0.push_back(the0);
                                        Q1.push_back(the1);
                                        Q2.push_back(the2);
                                        Q3.push_back(the3[i]);
                                        Q4.push_back(the4);
                                        Q5.push_back(the5);
                                        Q6.push_back(the6);

                                        j++;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    vector<vector<double>> Q;
    Q.push_back(Q0);
    Q.push_back(Q1);
    Q.push_back(Q2);
    Q.push_back(Q3);
    Q.push_back(Q4);
    Q.push_back(Q5);
    Q.push_back(Q6);

    // Find the median index
    int num_columns = Q[0].size();
    int index_theta0_min = 0, index_theta0_max = 0;

    // Find index of minimum and maximum values in the first row of A
    for (int i = 1; i < num_columns; i++)
    {
        if (Q[0][i] > Q[0][index_theta0_max])
            index_theta0_max = i;
        if (Q[0][i] < Q[0][index_theta0_min])
            index_theta0_min = i;
    }

    // Calculate the median index of the min and max
    int index_theta0_med = round((index_theta0_min + index_theta0_max) / 2);

    Qf.resize(7);

    for (int i = 0; i < 7; i++)
    {
        // 모터 방향에 따라 부호 결정
        if(i == 5 || i == 6){
            Qf[i] = -Q[i][index_theta0_med];
        }
        else{
            Qf[i] = Q[i][index_theta0_med];
        }
    }

    return Qf;
}

void PathManager::GetMusicSheet()
{
    ifstream inputFile("../include/rT.txt");

    if (!inputFile.is_open())
    {
        cerr << "Failed to open the file."
             << "\n";
    }

    // Read data into a 2D vector
    vector<vector<double>> inst_xyz(6, vector<double>(8, 0));

    for (int i = 0; i < 6; ++i)
    {
        for (int j = 0; j < 8; ++j)
        {
            inputFile >> inst_xyz[i][j];
            if(i == 0 || i == 1 || i == 3 || i == 4){
                inst_xyz[i][j] = inst_xyz[i][j] * 1.25;
            }
        }
    }

    // Extract the desired elements
    vector<double> right_B = {0, 0, 0};
    vector<double> right_S;
    vector<double> right_FT;
    vector<double> right_MT;
    vector<double> right_HT;
    vector<double> right_HH;
    vector<double> right_R;
    vector<double> right_RC;
    vector<double> right_LC;

    for (int i = 0; i < 3; ++i)
    {
        right_S.push_back(inst_xyz[i][0]);
        right_FT.push_back(inst_xyz[i][1]);
        right_MT.push_back(inst_xyz[i][2]);
        right_HT.push_back(inst_xyz[i][3]);
        right_HH.push_back(inst_xyz[i][4]);
        right_R.push_back(inst_xyz[i][5]);
        right_RC.push_back(inst_xyz[i][6]);
        right_LC.push_back(inst_xyz[i][7]);
    }

    vector<double> left_B = {0, 0, 0};
    vector<double> left_S;
    vector<double> left_FT;
    vector<double> left_MT;
    vector<double> left_HT;
    vector<double> left_HH;
    vector<double> left_R;
    vector<double> left_RC;
    vector<double> left_LC;

    for (int i = 3; i < 6; ++i)
    {
        left_S.push_back(inst_xyz[i][0]);
        left_FT.push_back(inst_xyz[i][1]);
        left_MT.push_back(inst_xyz[i][2]);
        left_HT.push_back(inst_xyz[i][3]);
        left_HH.push_back(inst_xyz[i][4]);
        left_R.push_back(inst_xyz[i][5]);
        left_RC.push_back(inst_xyz[i][6]);
        left_LC.push_back(inst_xyz[i][7]);
    }

    // Combine the elements into right_inst and left_inst
    right_inst = {right_B, right_RC, right_R, right_S, right_HH, right_HH, right_FT, right_MT, right_LC, right_HT};
    left_inst = {left_B, left_RC, left_R, left_S, left_HH, left_HH, left_FT, left_MT, left_LC, left_HT};

    /////////// 드럼로봇 악기정보 텍스트 -> 딕셔너리 변환
    map<string, int> instrument_mapping = {
        {"0", 10}, {"1", 3}, {"2", 6}, {"3", 7}, {"4", 9}, {"5", 4}, {"6", 5}, {"7", 4}, {"8", 8}, {"11", 3}, {"51", 3}, {"61", 3}, {"71", 3}, {"81", 3}, {"91", 3}};

    string score_path = "../include/codeConfession.txt";

    ifstream file(score_path);
    if (!file.is_open())
    {
        cerr << "Error opening file." << endl;
    }

    string line;
    int lineIndex = 0;
    while (getline(file, line))
    {
        istringstream iss(line);
        string item;
        vector<string> columns;
        while (getline(iss, item, '\t'))
        {
            item = trimWhitespace(item);
            columns.push_back(item);
        }

        vector<int> inst_arr_R(10, 0), inst_arr_L(10, 0);
        time_arr.push_back(stod(columns[1]) * 100 / bpm);

        if (columns[2] != "0")
        {
            inst_arr_R[instrument_mapping[columns[2]]] = 1;
        }
        if (columns[3] != "0")
        {
            inst_arr_L[instrument_mapping[columns[3]]] = 1;
        }

        RF.push_back(stoi(columns[6]) == 1 ? 1 : 0);
        LF.push_back(stoi(columns[7]) == 2 ? 1 : 0);

        RA.push_back(inst_arr_R);
        LA.push_back(inst_arr_L);

        lineIndex++;
    }

    file.close();

    end = RF.size();
}

void PathManager::GetReadyArr()
{
    cout << "Get Ready...\n";
    struct can_frame frame;

    vector<double> Qi;
    vector<vector<double>> q_ready;

    // tmotors의 상태를 확인
    cout << "tmotors size: " << tmotors.size() << "\n";
    int cnt = 0;
    for (auto &entry : tmotors)
    {
        cnt++;
        cout << "cnt : " <<cnt <<endl;
        std::shared_ptr<TMotor> &motor = entry.second;
        c_MotorAngle[motor_mapping[entry.first]] = motor->currentPos;
        // 각 모터의 현재 위치 출력
        cout << "Motor " << entry.first << " current position: " << motor->currentPos << "\n";
    }
    

    int n = 800;
    for (int k = 0; k < n; k++)
    {
        Qi = connect(c_MotorAngle, standby, k, n);
        q_ready.push_back(Qi);

        for (auto &entry : tmotors)
        {
            std::shared_ptr<TMotor> &motor = entry.second;
            float p_des = Qi[motor_mapping[entry.first]];
            TParser.parseSendCommand(*motor, &frame, motor->nodeId, 8, p_des, 0, 200.0, 3.0, 0.0);
            sendBuffer.push(frame);
            // Frame이 추가됨을 확인
            cout << "Frame added for motor: " << entry.first << ", sendBuffer size: " << sendBuffer.size() << "\n";
        }
    }

    c_MotorAngle = Qi;
    // 최종적인 sendBuffer의 크기 출력
    cout << "Final sendBuffer size: " << sendBuffer.size() << "\n";
}

void PathManager::PathLoopTask()
{
    struct can_frame frame;

    // 처음 시작할 때 Q2, Q4 모두 계산
    if (line == 0)
    {
        c_R = 0;
        c_L = 0;

        for (int j = 0; j < n_inst; ++j)
        {
            if (RA[line][j] != 0)
            {
                P1 = right_inst[j];
                c_R = 1;
            }
            if (LA[line][j] != 0)
            {
                P2 = left_inst[j];
                c_L = 1;
            }
        }

        if (c_R == 0 && c_L == 0)
        { // 왼손 & 오른손 안침
            Q1 = c_MotorAngle;
            if (p_R == 1)
            {
                Q1[4] = Q1[4] + M_PI / 36;
            }
            if (p_L == 1)
            {
                Q1[6] = Q1[6] - M_PI / 36;
            }
            Q2 = Q1;
        }
        else
        {
            Q1 = IKfun(P1, P2, R, s, z0);
            Q2 = Q1;
            if (c_R != 0 && c_L != 0)
            { // 왼손 & 오른손 침
                Q1[4] = Q1[4] + M_PI / 18;
                Q1[6] = Q1[6] - M_PI / 18;
            }
            else if (c_L != 0)
            { // 왼손만 침
                Q1[4] = Q1[4] + M_PI / 36;
                Q2[4] = Q2[4] + M_PI / 36;
                Q1[6] = Q1[6] - M_PI / 18;
            }
            else if (c_R != 0)
            { // 오른손만 침
                Q1[4] = Q1[4] + M_PI / 18;
                Q2[6] = Q2[6] - M_PI / 36;
                Q2[6] = Q2[6] - M_PI / 36;
            }
            // 허리는 Q1 ~ Q2 동안 계속 이동
            Q1[0] = (Q1[0] + c_MotorAngle[0]) / 2.0;
        }

        p_R = c_R;
        p_L = c_L;

        line++;

        p.push_back(c_MotorAngle);
        v.push_back({0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0});
    }

    c_R = 0;
    c_L = 0;

    for (int j = 0; j < n_inst; ++j)
    {
        if (RA[line][j] != 0)
        {
            P1 = right_inst[j];
            c_R = 1;
        }
        if (LA[line][j] != 0)
        {
            P2 = left_inst[j];
            c_L = 1;
        }
    }

    if (c_R == 0 && c_L == 0)
    { // 왼손 & 오른손 안침
        Q3 = Q2;
        if (p_R == 1)
        {
            Q3[4] = Q3[4] + M_PI / 36;
        }
        if (p_L == 1)
        {
            Q3[6] = Q3[6] - M_PI / 36;
        }
        Q4 = Q3;
    }
    else
    {
        Q3 = IKfun(P1, P2, R, s, z0);
        Q4 = Q3;
        if (c_R != 0 && c_L != 0)
        { // 왼손 & 오른손 침
            Q3[4] = Q3[4] + M_PI / 18;
            Q3[6] = Q3[6] - M_PI / 18;
        }
        else if (c_L != 0)
        { // 왼손만 침
            Q3[4] = Q3[4] + M_PI / 36;
            Q4[4] = Q4[4] + M_PI / 36;
            Q3[6] = Q3[6] - M_PI / 18;
        }
        else if (c_R != 0)
        { // 오른손만 침
            Q3[4] = Q3[4] + M_PI / 18;
            Q4[6] = Q4[6] - M_PI / 36;
            Q4[6] = Q4[6] - M_PI / 36;
        }
        // 허리는 Q3 ~ Q4 동안 계속 이동
        Q3[0] = (Q3[0] + Q2[0]) / 2.0;
    }

    p_R = c_R;
    p_L = c_L;

    double t1 = time_arr[line - 1];
    double t2 = time_arr[line];
    double t = 0.005;
    int n = round((t1 / 2) / t);
    vector<double> Pi;
    vector<double> Vi;
    vector<double> V0 = v.back();
    for (int i = 0; i < n; i++)
    {
        iconnect(c_MotorAngle, Q1, Q2, V0, t1 / 2, t1, t * i);
        Pi = p.back();
        Vi = v.back();

        for (auto &entry : tmotors)
        {
            std::shared_ptr<TMotor> &motor = entry.second;
            float p_des = Pi[motor_mapping[entry.first]];
            float v_des = Vi[motor_mapping[entry.first]];
            
            if(p_des < motor->rMin){
                cout << entry.first << " is out of range.  ( " << p_des << " => " << motor->rMin << " )\n";
                p_des = motor->rMin;
                v_des = 0.0f;
                getchar();
            }
            else if(p_des > motor->rMax){
                cout << entry.first << " is out of range.  ( " << p_des << " => " << motor->rMax << " )\n";
                p_des = motor->rMax;
                v_des = 0.0f;
                getchar();
            }
            
            TParser.parseSendCommand(*motor, &frame, motor->nodeId, 8, p_des, v_des, 200.0, 3.0, 0.0);
            sendBuffer.push(frame);
        }
    }
    V0 = v.back();
    for (int i = 0; i < n; i++)
    {
        iconnect(Q1, Q2, Q3, V0, t1 / 2, (t1 + t2) / 2, t * i);
        Pi = p.back();
        Vi = v.back();

        for (auto &entry : tmotors)
        {
            std::shared_ptr<TMotor> &motor = entry.second;
            float p_des = Pi[motor_mapping[entry.first]];
            float v_des = Vi[motor_mapping[entry.first]];
            
            if(p_des < motor->rMin){
                cout << entry.first << " is out of range.  ( " << p_des << " => " << motor->rMin << " )\n";
                p_des = motor->rMin;
                v_des = 0.0f;
                getchar();
            }
            else if(p_des > motor->rMax){
                cout << entry.first << " is out of range.  ( " << p_des << " => " << motor->rMax << " )\n";
                p_des = motor->rMax;
                v_des = 0.0f;
                getchar();
            }
            
            TParser.parseSendCommand(*motor, &frame, motor->nodeId, 8, p_des, v_des, 200.0, 3.0, 0.0);
            sendBuffer.push(frame);
        }
    }
    c_MotorAngle = p.back();
    Q1 = Q3;
    Q2 = Q4;
}

void PathManager::GetBackArr()
{
    struct can_frame frame;

    vector<double> Q0(7, 0);
    vector<vector<double>> q_finish;

    //// 끝나는자세 배열 생성
    vector<double> Qi;
    int n = 800;
    for (int k = 0; k < n; ++k)
    {
        Qi = connect(c_MotorAngle, Q0, k, n);
        q_finish.push_back(Qi);

        for (auto &entry : tmotors)
        {
            std::shared_ptr<TMotor> &motor = entry.second;
            float p_des = Qi[motor_mapping[entry.first]];
            TParser.parseSendCommand(*motor, &frame, motor->nodeId, 8, p_des, 0, 200.0, 3.0, 0.0);
            sendBuffer.push(frame);
        }
        // cout << "\n";
    }
}
