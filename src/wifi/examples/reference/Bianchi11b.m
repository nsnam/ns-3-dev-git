% Copyright 2020 University of Washington
%
% SPDX-License-Identifier: BSD-3-Clause
%
% Co-authored by Leonardo Lanante, Hao Yin, and Sebastien Deronne

function [bianchi_result] = Bianchi11b(txduration, ack, difs)
nA = [5:5:50];
CWmin = 31;
CWmax = 1023;
eP = 1500*8;
B = 1/(CWmin + 1);
EP = eP/(1 - B);
T_ACK = ack;  %txDuration value from WiFiPhy.cc
T_AMPDU = txduration; %txDuration value from WiFiPhy.cc
T_SIFS = 10e-6;
T_DIFS = 50e-6;
T_SLOT = 20e-6;
delta = 1e-7;
T_s = T_AMPDU + T_SIFS + T_ACK + T_DIFS;
if difs == 1 %DIFS
    T_C = T_AMPDU + T_DIFS;
else %EIFS
    T_s = T_AMPDU + T_SIFS + T_ACK + T_DIFS + delta;
    T_C = T_AMPDU + T_DIFS + T_SIFS + T_ACK + delta;
end
T_S = T_s/(1 - B) + T_SLOT;
S_bianchi = zeros(size(nA));
for j = 1:length(nA)
    n = nA(j)*1;
    W = CWmin + 1;
    m = log2((CWmax + 1)/(CWmin + 1));
    tau1 = linspace(0, 1, 1e4);
    p=1 - (1 - tau1).^(n - 1);
    ps = p*0;

    for i = 0:m-1
        ps = ps + (2*p).^i;
    end
    taup = 2./(1 + W + p.*W.*ps);
    [a,b] = min(abs(tau1 - taup));
    tau = taup(b);

    Ptr = 1 - (1 - tau)^n;
    Ps=n*tau*(1 - tau)^(n - 1)/Ptr;

    S_bianchi(j) = Ps*Ptr*EP/((1-Ptr)*T_SLOT+Ptr*Ps*T_S+Ptr*(1-Ps)*T_C)/1e6;
end

bianchi_result = S_bianchi;
