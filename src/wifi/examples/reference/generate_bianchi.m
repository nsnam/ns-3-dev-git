% Copyright 2020 University of Washington
%
% SPDX-License-Identifier: BSD-3-Clause
%
% Co-authored by Leonardo Lanante, Hao Yin, and Sebastien Deronne

clear;

%11a DIFS
fid = fopen('bianchi_11a_difs.txt', 'wt');
nA = [5:5:50];

%6Mbps
data_rate = 6e6;
ack_rate = 6e6;
[bianchi_result] = Bianchi11a(data_rate, ack_rate, 1);
fprintf(fid, "// 6 Mbps - TC with DIFS\n");
for k=1:10
    fprintf(fid, "        {%d, %.4f},\n", nA(k), bianchi_result(k));
end
fprintf(fid, "\n");

%9Mbps
data_rate = 9e6;
ack_rate = 6e6;
[bianchi_result] = Bianchi11a(data_rate, ack_rate, 1);
fprintf(fid, "// 9 Mbps - TC with DIFS\n");
for k=1:10
    fprintf(fid, "        {%d, %.4f},\n", nA(k), bianchi_result(k));
end
fprintf(fid, "\n");

%12Mbps
data_rate = 12e6;
ack_rate = 12e6;
[bianchi_result] = Bianchi11a(data_rate, ack_rate, 1);
fprintf(fid, "// 12 Mbps - TC with DIFS\n");
for k=1:10
    fprintf(fid, "        {%d, %.4f},\n", nA(k), bianchi_result(k));
end
fprintf(fid, "\n");

%18Mbps
data_rate = 18e6;
ack_rate = 12e6;
[bianchi_result] = Bianchi11a(data_rate, ack_rate, 1);
fprintf(fid, "// 18 Mbps - TC with DIFS\n");
for k=1:10
    fprintf(fid, "        {%d, %.4f},\n", nA(k), bianchi_result(k));
end
fprintf(fid, "\n");

%24Mbps
data_rate = 24e6;
ack_rate = 24e6;
[bianchi_result] = Bianchi11a(data_rate, ack_rate, 1);
fprintf(fid, "// 24 Mbps - TC with DIFS\n");
for k=1:10
    fprintf(fid, "        {%d, %.4f},\n", nA(k), bianchi_result(k));
end
fprintf(fid, "\n");

%36Mbps
data_rate = 36e6;
ack_rate = 24e6;
[bianchi_result] = Bianchi11a(data_rate, ack_rate, 1);
fprintf(fid, "// 36 Mbps - TC with DIFS\n");
for k=1:10
    fprintf(fid, "        {%d, %.4f},\n", nA(k), bianchi_result(k));
end
fprintf(fid, "\n");

%48Mbps
data_rate = 48e6;
ack_rate = 24e6;
[bianchi_result] = Bianchi11a(data_rate, ack_rate, 1);
fprintf(fid, "// 48 Mbps - TC with DIFS\n");
for k=1:10
    fprintf(fid, "        {%d, %.4f},\n", nA(k), bianchi_result(k));
end
fprintf(fid, "\n");

%54Mbps
data_rate = 54e6;
ack_rate = 24e6;
[bianchi_result] = Bianchi11a(data_rate, ack_rate, 1);
fprintf(fid, "// 54 Mbps - TC with DIFS\n");
for k=1:10
    fprintf(fid, "        {%d, %.4f},\n", nA(k), bianchi_result(k));
end
fprintf(fid, "\n");
fclose(fid);

%11a EIFS
fid = fopen('bianchi_11a_eifs.txt', 'wt');

%6Mbps
data_rate = 6e6;
ack_rate = 6e6;
[bianchi_result] = Bianchi11a(data_rate, ack_rate, 0);
fprintf(fid, "// 6 Mbps - TC with EIFS\n");
for k=1:10
    fprintf(fid, "        {%d, %.4f},\n", nA(k), bianchi_result(k));
end
fprintf(fid, "\n");

%9Mbps
data_rate = 9e6;
ack_rate = 6e6;
[bianchi_result] = Bianchi11a(data_rate, ack_rate, 0);
fprintf(fid, "// 9 Mbps - TC with EIFS\n");
for k=1:10
    fprintf(fid, "        {%d, %.4f},\n", nA(k), bianchi_result(k));
end
fprintf(fid, "\n");

%12Mbps
data_rate = 12e6;
ack_rate = 12e6;
[bianchi_result] = Bianchi11a(data_rate, ack_rate, 0);
fprintf(fid, "// 12 Mbps - TC with EIFS\n");
for k=1:10
    fprintf(fid, "        {%d, %.4f},\n", nA(k), bianchi_result(k));
end
fprintf(fid, "\n");

%18Mbps
data_rate = 18e6;
ack_rate = 12e6;
[bianchi_result] = Bianchi11a(data_rate, ack_rate, 0);
fprintf(fid, "// 18 Mbps - TC with EIFS\n");
for k=1:10
    fprintf(fid, "        {%d, %.4f},\n", nA(k), bianchi_result(k));
end
fprintf(fid, "\n");

%24Mbps
data_rate = 24e6;
ack_rate = 24e6;
[bianchi_result] = Bianchi11a(data_rate, ack_rate, 0);
fprintf(fid, "// 24 Mbps - TC with EIFS\n");
for k=1:10
fprintf(fid, "        {%d, %.4f},\n", nA(k), bianchi_result(k));
end
fprintf(fid, "\n");

%36Mbps
data_rate = 36e6;
ack_rate = 24e6;
[bianchi_result] = Bianchi11a(data_rate, ack_rate, 0);
fprintf(fid, "// 36 Mbps - TC with EIFS\n");
for k=1:10
    fprintf(fid, "        {%d, %.4f},\n", nA(k), bianchi_result(k));
end
fprintf(fid, "\n");

%48Mbps
data_rate = 48e6;
ack_rate = 24e6;
[bianchi_result] = Bianchi11a(data_rate, ack_rate, 0);
fprintf(fid, "// 48 Mbps - TC with EIFS\n");
for k=1:10
    fprintf(fid, "        {%d, %.4f},\n", nA(k), bianchi_result(k));
end
fprintf(fid, "\n");

%54Mbps
data_rate = 54e6;
ack_rate = 24e6;
[bianchi_result] = Bianchi11a(data_rate, ack_rate, 0);
fprintf(fid, "// 54 Mbps - TC with EIFS\n");
for k=1:10
    fprintf(fid, "        {%d, %.4f},\n", nA(k), bianchi_result(k));
end
fprintf(fid, "\n");
fclose(fid);


%11g DIFS
fid = fopen('bianchi_11g_difs.txt', 'wt');

%6Mbps
data_rate = 6e6;
ack_rate = 6e6;
[bianchi_result] = Bianchi11g(data_rate, ack_rate, 1);
fprintf(fid, "// 6 Mbps - TC with DIFS\n");
for k=1:10
    fprintf(fid, "        {%d, %.4f},\n", nA(k), bianchi_result(k));
end
fprintf(fid, "\n");

%9Mbps
data_rate = 9e6;
ack_rate = 6e6;
[bianchi_result] = Bianchi11g(data_rate, ack_rate, 1);
fprintf(fid, "// 9 Mbps - TC with DIFS\n");
for k=1:10
    fprintf(fid, "        {%d, %.4f},\n", nA(k), bianchi_result(k));
end
fprintf(fid, "\n");

%12Mbps
data_rate = 12e6;
ack_rate = 12e6;
[bianchi_result] = Bianchi11g(data_rate, ack_rate, 1);
fprintf(fid, "// 12 Mbps - TC with DIFS\n");
for k=1:10
    fprintf(fid, "        {%d, %.4f},\n", nA(k), bianchi_result(k));
end
fprintf(fid, "\n");

%18Mbps
data_rate = 18e6;
ack_rate = 12e6;
[bianchi_result] = Bianchi11g(data_rate, ack_rate, 1);
fprintf(fid, "// 18 Mbps - TC with DIFS\n");
for k=1:10
    fprintf(fid, "        {%d, %.4f},\n", nA(k), bianchi_result(k));
end
fprintf(fid, "\n");

%24Mbps
data_rate = 24e6;
ack_rate = 24e6;
[bianchi_result] = Bianchi11g(data_rate, ack_rate, 1);
fprintf(fid, "// 24 Mbps - TC with DIFS\n");
for k=1:10
    fprintf(fid, "        {%d, %.4f},\n", nA(k), bianchi_result(k));
end
fprintf(fid, "\n");

%36Mbps
data_rate = 36e6;
ack_rate = 24e6;
[bianchi_result] = Bianchi11g(data_rate, ack_rate, 1);
fprintf(fid, "// 36 Mbps - TC with DIFS\n");
for k=1:10
    fprintf(fid, "        {%d, %.4f},\n", nA(k), bianchi_result(k));
end
fprintf(fid, "\n");

%48Mbps
data_rate = 48e6;
ack_rate = 24e6;
[bianchi_result] = Bianchi11g(data_rate, ack_rate, 1);
fprintf(fid, "// 48 Mbps - TC with DIFS\n");
for k=1:10
    fprintf(fid, "        {%d, %.4f},\n", nA(k), bianchi_result(k));
end
fprintf(fid, "\n");

%54Mbps
data_rate = 54e6;
ack_rate = 24e6;
[bianchi_result] = Bianchi11g(data_rate, ack_rate, 1);
fprintf(fid, "// 54 Mbps - TC with DIFS\n");
for k=1:10
    fprintf(fid, "        {%d, %.4f},\n", nA(k), bianchi_result(k));
end
fprintf(fid, "\n");
fclose(fid);

%11g EIFS
fid = fopen('bianchi_11g_eifs.txt', 'wt');

%6Mbps
data_rate = 6e6;
ack_rate = 6e6;
[bianchi_result] = Bianchi11g(data_rate, ack_rate, 0);
fprintf(fid, "// 6 Mbps - TC with EIFS\n");
for k=1:10
    fprintf(fid, "        {%d, %.4f},\n", nA(k), bianchi_result(k));
end
fprintf(fid, "\n");

%9Mbps
data_rate = 9e6;
ack_rate = 6e6;
[bianchi_result] = Bianchi11g(data_rate, ack_rate, 0);
fprintf(fid, "// 9 Mbps - TC with EIFS\n");
for k=1:10
    fprintf(fid, "        {%d, %.4f},\n", nA(k), bianchi_result(k));
end
fprintf(fid, "\n");

%12Mbps
data_rate = 12e6;
ack_rate = 12e6;
[bianchi_result] = Bianchi11g(data_rate, ack_rate, 0);
fprintf(fid, "// 12 Mbps - TC with EIFS\n");
for k=1:10
    fprintf(fid, "        {%d, %.4f},\n", nA(k), bianchi_result(k));
end
fprintf(fid, "\n");

%18Mbps
data_rate = 18e6;
ack_rate = 12e6;
[bianchi_result] = Bianchi11g(data_rate, ack_rate, 0);
fprintf(fid, "// 18 Mbps - TC with EIFS\n");
for k=1:10
    fprintf(fid, "        {%d, %.4f},\n", nA(k), bianchi_result(k));
end
fprintf(fid, "\n");

%24Mbps
data_rate = 24e6;
ack_rate = 24e6;
[bianchi_result] = Bianchi11g(data_rate, ack_rate, 0);
fprintf(fid, "// 24 Mbps - TC with EIFS\n");
for k=1:10
    fprintf(fid, "        {%d, %.4f},\n", nA(k), bianchi_result(k));
end
fprintf(fid, "\n");

%36Mbps
data_rate = 36e6;
ack_rate = 24e6;
[bianchi_result] = Bianchi11g(data_rate, ack_rate, 0);
fprintf(fid, "// 36 Mbps - TC with EIFS\n");
for k=1:10
    fprintf(fid, "        {%d, %.4f},\n", nA(k), bianchi_result(k));
end
fprintf(fid, "\n");

%48Mbps
data_rate = 48e6;
ack_rate = 24e6;
[bianchi_result] = Bianchi11g(data_rate, ack_rate, 0);
fprintf(fid, "// 48 Mbps - TC with EIFS\n");
for k=1:10
    fprintf(fid, "        {%d, %.4f},\n", nA(k), bianchi_result(k));
end
fprintf(fid, "\n");

%54Mbps
data_rate = 54e6;
ack_rate = 24e6;
[bianchi_result] = Bianchi11g(data_rate, ack_rate, 0);
fprintf(fid, "// 54 Mbps - TC with EIFS\n");
for k=1:10
    fprintf(fid, "        {%d, %.4f},\n", nA(k), bianchi_result(k));
end
fprintf(fid, "\n");
fclose(fid);


%11b DIFS
fid = fopen('bianchi_11b_difs.txt', 'wt');

%1Mbps
txduration = 12480e-6;
ack = 304e-6;
[bianchi_result] = Bianchi11b(txduration, ack, 1);
fprintf(fid, "// 1 Mbps - TC with DIFS\n");
for k=1:10
    fprintf(fid, "        {%d, %.4f},\n", nA(k), bianchi_result(k));
end
fprintf(fid, "\n");

%2Mbps
txduration = 6336e-6;
ack = 248e-6;
[bianchi_result] = Bianchi11b(txduration, ack, 1);
fprintf(fid, "// 2 Mbps - TC with DIFS\n");
for k=1:10
    fprintf(fid, "        {%d, %.4f},\n", nA(k), bianchi_result(k));
end
fprintf(fid, "\n");

%5.5Mbps
txduration = 2427e-6;
ack = 248e-6;
[bianchi_result] = Bianchi11b (txduration, ack, 1);
fprintf(fid, "// 5.5 Mbps - TC with DIFS\n");
for k=1:10
    fprintf(fid, "        {%d, %.4f},\n", nA(k), bianchi_result(k));
end
fprintf(fid, "\n");

%11Mbps
txduration = 1310e-6;
ack = 248e-6;
[bianchi_result] = Bianchi11b (txduration, ack, 1);
fprintf(fid, "// 11 Mbps - TC with DIFS\n");
for k=1:10
    fprintf(fid, "        {%d, %.4f},\n", nA(k), bianchi_result(k));
end
fprintf(fid, "\n");

%11b EIFS
fid = fopen ('bianchi_11b_eifs.txt', 'wt');

%1Mbps
txduration = 12480e-6;
ack = 304e-6;
[bianchi_result] = Bianchi11b (txduration, ack, 0);
fprintf (fid, "// 1 Mbps - TC with EIFS\n");
for k=1:10
    fprintf (fid, "        {%d, %.4f},\n", nA(k), bianchi_result(k));
end
fprintf (fid, "\n");

%2Mbps
txduration = 6336e-6;
ack = 248e-6;
[bianchi_result] = Bianchi11b(txduration, ack, 0);
fprintf(fid, "// 2 Mbps - TC with EIFS\n");
for k=1:10
    fprintf(fid, "        {%d, %.4f},\n", nA(k), bianchi_result(k));
end
fprintf(fid, "\n");

%5.5Mbps
txduration = 2427e-6;
ack = 248e-6;
[bianchi_result] = Bianchi11b(txduration, ack, 0);
fprintf(fid, "// 5.5 Mbps - TC with EIFS\n");
for k=1:10
    fprintf(fid, "        {%d, %.4f},\n", nA(k), bianchi_result(k));
end
fprintf(fid, "\n");

%11Mbps
txduration = 1310e-6;
ack = 248e-6;
[bianchi_result] = Bianchi11b(txduration, ack, 0);
fprintf(fid, "// 11 Mbps - TC with EIFS\n");
for k=1:10
    fprintf(fid, "        {%d, %.4f},\n", nA(k), bianchi_result(k));
end
fprintf(fid, "\n");
