R = [1.0,  0.0, 0.0;   0.0, -0.866025404,  0.5; 0.0, -0.5,         -0.866025404]

rad2deg(rotm2eul(R, 'XYZ'))

R*eul2rotm(deg2rad([180.0000         0         0]), 'XYZ')


eul2rotm(deg2rad([180.0000         0         0]), 'XYZ')*R