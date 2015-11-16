Ts = 0.01;      % Sampling rate = 1/Ts
Tu = 5*Ts;      % Update rate = 1/Tu
ts = 0:Ts:1;    % Time vector, spaced by sampling rate
tu = 0:Tu:1;    % Time vector, spaced by update rate
Ns = length(ts);    % Number of raw data samples
Nu = length(tu);    % Number of calibrated data samples

% Raw data varaibles (16 bit signed integers)
tick_raw = ts/Ts;       % Tick count
AX = zeros(32,Ns);      % X acceleration
AY = zeros(32,Ns);      % Y acceleration
AZ = zeros(32,Ns);      % Z acceleration
GX = zeros(32,Ns);      % X angular rate
GY = zeros(32,Ns);      % Y angular rate
GZ = zeros(32,Ns);      % Z angular rate
Temp = zeros(32,Ns);    % Temperature

% Calibrated data variables (32-bit floating point)
tick_cal = tu/Ts;           % Tick count (same base as tick_raw)
DeltaTheta = zeros(3,Nu);   % Non-destruct delta theta
DeltaV = zeros(3,Nu);       % Non-destruct delta velocity
AccumVel = zeros(3,Nu);     % Accumulated velocity vector
Q = zeros(4,Nu);            % Attitude quaternion
AvgTemp = zeros(1,Nu);      % Average temperature

% Organize data into a structure
imu_data_raw = struct('tick',tick_raw,...
    'AX',AX,...
    'AY',AY,...
    'AZ',AZ,...
    'GX',GX,...
    'GY',GY,...
    'GZ',GZ,...
    'Temp',Temp);

imu_data_cal = struct('tick',tick_cal,...
    'DeltaTheta',DeltaTheta,...
    'DeltaV',DeltaV,...
    'AccumVel',AccumVel,...
    'Q',Q,...
    'AvgTemp',AvgTemp);