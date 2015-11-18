function dataCal = ParseCSVDataCal(filename,T,offsetStart,offsetEnd,plotOn)
% PARSECSVDATA - Parses a CSV data file (calibrated data format)
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
%   .DTheta - Latest delta theta (rad)
%   .DV - Latest delta velocity (m/s)
%   .Q - Attitude quaternion
%   .Temp - Averaged temperature across all sensors
%   .AccumV - Accumulated velocity (m/s)


tic;
display('Parsing...');

% Read in data file
fid = fopen(filename);
fileData = textscan(fid,'%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f')';
fclose(fid);

fileData = cell2mat(fileData');

% Remove rows from beginning and end of dataset
fileData = fileData(1+offsetStart:end-offsetEnd, :);

dataCal = struct(...
    't',fileData(:,1)*T,...               % Time
    'DTheta',fileData(:,2:4),...          % Delta theta
    'DV',fileData(:,5:7),...              % Delta velocity
    'Q',fileData(:,8:11),...              % Attitude quaternion
    'Temp',fileData(:,12),...             % Temperature
    'AccumV',fileData(:,13:15));          % Accumulated velocity
toc;

if(plotOn == 1)
    % TODO - Plotting of calibrated data. Put into separate file
end

end