function [dataRaw,dataCal,err] = ParseBinaryData(filename,T,offsetStart,...
    offsetEnd,plotOn)
% PARSEBINARYDATA - Parses a binary data file.
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
% dataRaw - Data structure containing parsed raw data
%   .t - Time vector (s)
%   .GX - Gyro X (deg/s)
%   .GY - Gyro Y (deg/s)
%   .GZ - Gyro Z (deg/s)
%   .AX - Accelerometer X (milli-g)
%   .AY - Accelerometer Y (milli-g)
%   .AZ - Accelerometer Z (milli-g)
%
% dataCal - Data structure containing parsed calibrated data
%   .t - Time vector (s)
%   .GX - Gyro X (deg/s)
%   .GY - Gyro Y (deg/s)
%   .GZ - Gyro Z (deg/s)
%   .AX - Accelerometer X (milli-g)
%   .AY - Accelerometer Y (milli-g)
%   .AZ - Accelerometer Z (milli-g)

display('Parsing...');

% Read in data file
fid = fopen(filename);
fileData = fread(fid,'uint8')';
fclose(fid);

% Find all the zeros which signify the packet separators
packetBounds = find(fileData==0);
packetCount = size(packetBounds,2);

tic;
rawCount = 1;  % Counter for raw data packets
calCount = 1;  % Counter for calibrated data packets
tickCount = 0; % Tick count in the data
err = 0;

dataRawParsed = zeros(packetCount,225);
dataCalParsed = zeros(packetCount,15);

% Loop through each packet
tic;
for i = 1 : packetCount-1
    % Get the encoded data within this set of packet bounds
    encData = fileData(packetBounds(i)+1:packetBounds(i+1));

    % RAW DATA PACKET 1
    if(size(encData,2) == 244)
        % Decode the data
        [dataRow,errCobs] = cobs_decode(encData);
        err = err + errCobs;
        % Copy tick count
        dataRawParsed(rawCount,1) = typecast(uint8(dataRow(1:4)), 'uint32');
        tickCount = dataRawParsed(rawCount,1);
        % Copy raw data for the first 17 sensors
        dataRawParsed(rawCount,2:120) = typecast(uint8(dataRow(5:end)), 'int16');
        
    % RAW DATA PACKET 2
    elseif(size(encData,2) == 212)
        % Decode the data
        [dataRow,errCobs] = cobs_decode(encData);
        err = err + errCobs;
        % Copy raw data for the last 15 sensors
        dataRawParsed(rawCount,121:225) = typecast(uint8(dataRow), 'int16');
        rawCount = rawCount + 1;
        
    % CALIBRATED DATA PACKET
    elseif(length(encData) == 58)
        % Decode the data
        [dataRow,errCobs] = cobs_decode(encData);
        err = err + errCobs;
        % Copy tick count from raw data packets since this packet came
        % right after those and is associated with that tick count
        dataCalParsed(calCount,1) = tickCount;
        % Copy calibrated data
        dataCalParsed(calCount,2:15) = typecast(uint8(dataRow), 'single');
        calCount = calCount + 1;
       
    else
        err = err + 1;
    end
end
toc;

% Remove the junk rows at the beginning and end of the dataset
dataRawParsed = dataRawParsed(1+offsetStart:rawCount-offsetEnd,:);
dataCalParsed = dataCalParsed(1+offsetStart:calCount-offsetEnd,:);

% Organize data into a structure
dataRaw = struct('t',T*dataRawParsed(:,1),...
    'AX',dataRawParsed(:,2:7:219), 'AY',dataRawParsed(:,3:7:220),...
    'AZ',dataRawParsed(:,4:7:221), 'GX',dataRawParsed(:,5:7:222),...
    'GY',dataRawParsed(:,6:7:223), 'GZ',dataRawParsed(:,7:7:224),...
    'Temp',dataRawParsed(:,8:7:225));

dataCal = struct('t',T*dataCalParsed(:,1),'AX',dataCalParsed(:,2),...
    'AY',dataCalParsed(:,3), 'AZ',dataCalParsed(:,4),...
    'GX',dataCalParsed(:,5), 'GY',dataCalParsed(:,6),...
    'GZ',dataCalParsed(:,7), 'Temp',dataCalParsed(:,8));

% Plot raw data
if(plotOn == 1)
    figure;
    subplot(3,2,1);
    plot(dataRaw.t, dataRaw.GX);
    title('Gyro - X'); xlabel('Time (s)'); ylabel('deg/s');
    subplot(3,2,3);
    plot(dataRaw.t, dataRaw.GY);
    title('Gyro - Y'); xlabel('Time (s)'); ylabel('deg/s');
    subplot(3,2,5);
    plot(dataRaw.t, dataRaw.GZ);
    title('Gyro - Z'); xlabel('Time (s)'); ylabel('deg/s');
    subplot(3,2,2);
    plot(dataRaw.t, dataRaw.AX);
    title('Specific Force - X'); xlabel('Time (s)'); ylabel('milli-g');
    subplot(3,2,4);
    plot(dataRaw.t, dataRaw.AY);
    title('Specific Force - Y'); xlabel('Time (s)'); ylabel('milli-g');
    subplot(3,2,6);
    plot(dataRaw.t, dataRaw.AZ);
    title('Specific Force - Z'); xlabel('Time (s)'); ylabel('milli-g');
end

% Plot calibrated data
if(plotOn == 1)
    figure;
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