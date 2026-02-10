clc
clear all
close all

% --- Custom Modulation Function ---
function out = CustomQAM16Mod(data, mode)
    persistent customConst;
    if isempty(customConst)
        theta8 = (0:7) * (pi/4);
        % Inner ring (8 pts) and Outer ring (8 pts)
        inner16 = 0.5 * exp(1i * theta8); 
        outer16 = 1.0 * exp(1i * (theta8 + pi/8));
        customConst = [inner16, outer16]; 
    end
    
    if strcmp(mode, 'modulate')
        out = customConst(data + 1).'; 
    elseif strcmp(mode, 'demodulate')
        out = zeros(size(data));
        for j = 1:length(data)
            [~, minIdx] = min(abs(data(j) - customConst));
            out(j) = minIdx - 1;
        end
    end
end

% --- Define color styles ---
% 8-PSK (Blue), 16-PSK (Red), 16-StarQAM (Black)
colors = {'b-o', 'r-s', 'k-d'};
labels = {'8-PSK', '16-PSK', '16-QAM'};

% Modulation order M
M_vec = [8, 16, 16];

for i = 1:3
    M = M_vec(i);
    k = log2(M); % Bits per symbol
    EbNoVec = (5:15)'; % Eb/No values (dB)
    numSymPerFrame = 100; % Number of QAM symbols per frame
    berEst = zeros(size(EbNoVec));
    isCustomQAM = (i == 3);
    
    for n = 1:length(EbNoVec)
        % Convert Eb/No to SNR
        snrdB = EbNoVec(n) + 10*log10(k);
        % Reset the error and bit counters
        numErrs = 0;
        numBits = 0;
        
        while numErrs < 200 && numBits < 1e7
            % Generate binary data and convert to symbols
            dataIn = randi([0 1],numSymPerFrame,k);
            dataSym = bi2de(dataIn);
            
            %----------------------------------------------------------------    
            % Your modulator here:
            if (~isCustomQAM)
                modSig = pskmod(dataSym, M); % PSK Modulation Example
            else
                modSig = CustomQAM16Mod(dataSym, 'modulate');
            end
            
            % Pass through AWGN channel:
            % 'measured' for fair comparison
            rxSig = awgn(modSig, snrdB, 'measured'); % Additive White Gaussian Noise
            
            % Your demodulator here:
            if (~isCustomQAM)
                rxSym = pskdemod(rxSig, M); % PSK Demodulation Example
            else
                rxSym = CustomQAM16Mod(rxSig, 'demodulate');
            end
            %----------------------------------------------------------------
            
            % Convert received symbols to bits
            dataOut = de2bi(rxSym,k);
            
            % Calculate the number of bit errors
            nErrors = biterr(dataIn,dataOut);
            
            % Increment the error and bit counters
            numErrs = numErrs + nErrors;
            numBits = numBits + numSymPerFrame*k;
        end
        
        % Estimate the BER
        berEst(n) = numErrs/numBits;
        
        fprintf("snrdB: %.6f berEst: %.6f " + ...
            " \n \t numErrs: %d numBits: %d\n\n", snrdB, berEst(n), numErrs, numBits);
    end
    
    semilogy(EbNoVec, berEst, colors{i}, 'LineWidth', 2, 'DisplayName', labels{i});
    hold on
end

grid on;
xlabel('E_b/N_0 (dB)');
ylabel('Bit Error Rate (BER)');
title('BER Performance Comparison');
legend('show'); % Show legend to distinguish the lines