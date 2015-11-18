function dataCal = ParseCSVDataRaw(filename,T,offsetStart,offsetEnd,plotOn)
% PARSECSVDATA - Parses a CSV data file (raw output format)
%
% Arguments
% ---------------------------------
% filename - Name of the file to parse
% T - Sampling time of IMU (250 Hz by default)
% offsetStart - The number of beginning data points to toss out.
% offsetEnd - The number of end data points to toss out
% plotOn - If plotOn = 1, plots data
%
% Currently, the first few recorded data points are either all zero or are
% from the previous data acquisition loop. The offset variables provide an
% easy way to remove these junk points from the dataset.
%
% Returns
% ----------------------------------
% dataCal - Data structure containing parsed data
%   .t - Time vector (s)
%   .GX - Gyro X (deg/s)
%   .GY - Gyro Y (deg/s)
%   .GZ - Gyro Z (deg/s)
%   .AX - Accelerometer X (milli-g)
%   .AY - Accelerometer Y (milli-g)
%   .AZ - Accelerometer Z (milli-g)

tic;
display('Parsing...');

% Read in data file
fid = fopen(filename);
fileData = textscan(fid,'%f,%f,%f,%f,%f,%f,%f')';
fclose(fid);

fileData = cell2mat(fileData');

% Remove rows from beginning and end of dataset
fileData = fileData(1+offsetStart:end-offsetEnd, :);

dataCal = struct(...
    't',fileData(:,1)*T,...         % Time
    'GX',fileData(:,2),...          % Gyro X
    'GY',fileData(:,3),...          % Gyro Y
    'GZ',fileData(:,4),...          % Gyro Z
    'AX',fileData(:,5)*1000,...    % Accelerometer X
    'AY',fileData(:,6)*1000,...    % Accelerometer Y
    'AZ',fileData(:,7)*1000);      % Accelerometer Z
toc;

if(plotOn == 1)
    subplot(3,2,1);
    plot(dataCal.t, dataCal.GX);
    title('Gyro - X'); xlabel('Time (s)'); ylabel('deg/s');
    subplot(3,2,3);
    plot(dataCal.t, dataCal.GY);
    title('Gyro - Y'); xlabel('Time (s)'); ylabel('deg/s');
    subplot(3,2,5);
    plot(dataCal.t, dataCal.GZ);
    title('Gyro - Z'); xlabel('Time (s)'); ylabel('deg/s');
    subplot(3,2,2);
    plot(dataCal.t, dataCal.AX);
    title('Specific Force - X'); xlabel('Time (s)'); ylabel('milli-g');
    subplot(3,2,4);
    plot(dataCal.t, dataCal.AY);
    title('Specific Force - Y'); xlabel('Time (s)'); ylabel('milli-g');
    subplot(3,2,6);
    plot(dataCal.t, dataCal.AZ);
    title('Specific Force - Z'); xlabel('Time (s)'); ylabel('milli-g');
end

end