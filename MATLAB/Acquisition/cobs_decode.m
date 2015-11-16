function [dst,err] = cobs_decode(src)
%COBS_DECODE - Decodes an array of bytes using COBS bit stuffing

src = src(1:end-1);      % Remove trailing zero
N = length(src);         % Length of encoded data
src_idx = 1;
dst_idx = 1;

dst = zeros(1,N);      % Decoded data is one byte smaller
err = 0;
try

    while(src_idx < N)
        code = src(src_idx);
        if(code > numel(src))
            err = 1;
            break;
        end

        src_idx = src_idx + 1;
        for i = 2 : code
            dst(dst_idx) = src(src_idx);
            dst_idx = dst_idx + 1;
            src_idx = src_idx + 1;
        end
        if(code < 255)
            dst(dst_idx) = 0;
            dst_idx = dst_idx + 1;
        end
    end
catch
    err = 1;
end

% Remove trailing zero
dst = dst(1:end-1);
end