%{
    Compute the antenna gain pattern of the reflector antenna with circular aperture
    specified in the Sec. 6.4.1, 3GPP 38.811 v.15.4.0 and generate reference gain values
    for the CircularApertureAntennaModelTest.
%}
clc; clearvars;

%{
    Consider two testing vectors, one with elevation fixed to 90 degrees and
    varying azimuth, the other with azimuth fixed to 180 degrees and varying elevation.
    The boresight direction, is (az, el) = (180 degrees, 90 degrees)
%}
az_test_fixed_el = 90:10:180;
eL_test_fixed_az = 0:9:90;
%  Uncomment line below and comment line below for the fixed elevation test
[theta_vec, az_vec, el_vec] = theta_vec_from_sph_coord_vecs(az_test_fixed_el, 90);
%  Uncomment line above and comment line below for the fixed azimuth test
%[theta_vec, az_vec, el_vec] = theta_vec_from_sph_coord_vecs(180, eL_test_fixed_az);

%{
 The theta angle which represents the input of the pattern formula is
 computed as theta = arccos(<p1, p2>), where p_i = sin(theta_i) *
 cos(phi_i), sin(theta_i) * sin(phi_i), cos(theta_i). The default
 boresight is (phi1 (az), theta1(el)) = (180deg, 90deg)
%}

%{
    3GPP specifies the antenna gain only for |theta| < 90 degrees.
    The gain outside of this region is approximated as min_gain_dB
%}
min_gain_dB = -50;

max_gain_dB = 0;
c=physconst('LightSpeed');
f=28e9; % operating frequency
k=2*pi*f/c; % wave number
a=10*c/f; % radius antenna aperture
arg_vec=k*a*sind(theta_vec);
pattern=besselj(1, arg_vec)./arg_vec;
pattern= 4*(abs(pattern).^2);

% Manually set gain for theta = 0
where_arg_zero=find(~theta_vec);
pattern(where_arg_zero)=1;
gain_dB=10*log10(pattern) + max_gain_dB;

% Manually set gain to minimum gain for |theta| >= 90 degrees
where_arg_outside_pattern_domain = find(abs(theta_vec) >= 90);
gain_dB(where_arg_outside_pattern_domain)=min_gain_dB;

%{
    Plot and output the C++ code which defines the reference vector of data points.
    Test points are assumed to be encoded as C++ structs
%}
scatter(theta_vec, gain_dB);
out_str = ns3_output_from_vecs(az_vec, el_vec, gain_dB, max_gain_dB, min_gain_dB, a, f);


function theta = theta_from_sph_coord(phi1, theta1)
%    theta_from_sph_coord Computes theta (the input of the radiation pattern formula)
%    from the azimuth and elevation angles with resepct to boresight.
%    phi1 the azimuth with respect to boresight
%    theta1 the elevation angle with respect to boresight

    p1 = [sind(theta1)*cosd(phi1), sind(theta1)*sind(phi1), cosd(theta1)];
    % p2, i.e., the boresight direction, is phi1 (az), theta1(el)) =
    % (180deg, 90deg)
    theta2 = 90; phi2 = 180;
    p2 = [sind(theta2)*cosd(phi2), sind(theta2)*sind(phi2), cosd(theta2)];
    theta = acosd(dot(p1,p2));
end

function [theta_vec, az_vec, el_vec] = theta_vec_from_sph_coord_vecs(az_vec, el_vec)
%    theta_vec_from_sph_coord_vecs Computes the vectors of thetas
%    (the inputs of the radiation pattern formula)
%    from vectors of azimuth and elevation angles with resepct to boresight.
%    phi1 the vector of azimuths with respect to boresight
%    theta1 the vector of elevation angles with respect to boresight
    if size(az_vec, 2) > 1
        el_vec = repelem(el_vec, size(az_vec, 2));
    elseif size(el_vec, 2) > 1
        az_vec = repelem(az_vec, size(el_vec, 2));
    end
    theta_vec = arrayfun(@theta_from_sph_coord, az_vec, el_vec);
end

function out_str = ns3_output_from_vecs(az_vec, el_vec, gain_dB_vec, max_gain, min_gain, a, f)
%    ns3_output_from_vecs Outputs the reference gain values
%    as a vector of vectors.
%    The entries of the inner vectors, representing a single test point,
%    represent: (gain at boresight (dB), gain outside the 3GPP pattern region (dB),
%    aperture (meters), carrier frequency (Hz), test azimuth (degrees),
%    test elevation angle (degrees), reference antenna gain (dB))
    out_str = '';
    for i=1:size(el_vec, 2)
        out_str = [out_str, '{', ...
                            num2str(max_gain), ',', ...
                            num2str(min_gain), ',', ...
                            num2str(a), ',', ...
                            num2str(f), ',', ...
                            num2str(az_vec(i) - 180), ',', ...
                            num2str(el_vec(i)), ',', ...
                            num2str(gain_dB_vec(i)), '},'];
    end
end
