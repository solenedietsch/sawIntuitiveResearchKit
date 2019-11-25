/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-    */
/* ex: set filetype=cpp softtabstop=4 shiftwidth=4 tabstop=4 cindent expandtab: */

/*
  Author(s):  Anton Deguet
  Created on: 2019-11-11

  (C) Copyright 2019 Johns Hopkins University (JHU), All Rights Reserved.

  --- begin cisst license - do not edit ---

  This software is provided "as is" under an open source license, with
  no warranty.  The complete license can be found in license.txt and
  http://www.cisst.org/cisst/license.txt.

  --- end cisst license ---
*/

#include <sawIntuitiveResearchKit/robManipulatorECM.h>
#include <math.h>

robManipulatorECM::robManipulatorECM(const std::vector<robKinematics *> linkParms,
                                     const vctFrame4x4<double> &Rtw0)
    : robManipulator(linkParms, Rtw0)
{
}

robManipulatorECM::robManipulatorECM(const std::string &robotfilename,
                                     const vctFrame4x4<double> &Rtw0)
    : robManipulator(robotfilename, Rtw0)
{
}

robManipulatorECM::robManipulatorECM(const vctFrame4x4<double> &Rtw0)
    : robManipulator(Rtw0)
{
}

robManipulator::Errno
robManipulatorECM::InverseKinematics(vctDynamicVector<double> & q,
                                     const vctFrame4x4<double> & Rts,
                                     double CMN_UNUSED(tolerance),
                                     size_t CMN_UNUSED(Niterations),
                                     double CMN_UNUSED(LAMBDA))
{
    if (q.size() != links.size()) {
        CMN_LOG_RUN_ERROR << CMN_LOG_DETAILS
                          << ": robManipulatorECM::InverseKinematics: expected " << links.size() << " joints values. "
                          << " Got " << q.size()
                          << std::endl;
        return robManipulator::EFAILURE;
    }

    if (links.size() == 0) {
        CMN_LOG_RUN_ERROR << CMN_LOG_DETAILS
                          << ": robManipulatorECM::InverseKinematics: the manipulator has no links."
                          << std::endl;
        return robManipulator::EFAILURE;
    }

    // take Rtw0 into account
    vctFrm4x4 Rt04;
    Rtw0.ApplyInverseTo(Rts, Rt04);

    const double x = Rt04.Translation().X();
    const double x2 = x * x;
    const double y = Rt04.Translation().Y();
    const double y2 = y * y;
    const double z = Rt04.Translation().Z();
    const double z2 = z * z;

    // hard coded check for dVRK classic, wouldn't work on S/Si system
    if (z > cmnTypeTraits<double>::Tolerance()) {
        std::cerr << "z is positive, out of reach: " << Rts << std::endl;
        return robManipulator::EFAILURE;
    }

    // first joint controls x position
    if (std::abs(x) < cmnTypeTraits<double>::Tolerance()) {
        q[0] = 0.0;
    } else {
        const double depthX = std::sqrt(x2 + z2);
        if (depthX < cmnTypeTraits<double>::Tolerance()) {
            q[0] = 0.0;
        } else {
            q[0] = asinl(x / depthX);
        }
    }

    // similar computation for second joint
    const double depth = std::sqrt(x2 + y2 + z2);
    if (std::abs(y) < cmnTypeTraits<double>::Tolerance()) {
        q[1] = 0.0;
    } else {
        const double depth = std::sqrt(x2 + y2 + z2);
        if (depth < cmnTypeTraits<double>::Tolerance()) {
            q[1] = 0.0;
        } else {
            q[1] = -asinl(y / depth);
        }
    }

    // third is translation, i.e. depth
    q[2] = depth - 0.0007; // 0.0007 is from DH link_3.offset - link_4.D

    // check that depth is reachable
    if (q[2] > links[2].GetKinematics()->PositionMax()) {
        std::cerr << "goal is too far, out of reach: " << Rts << std::endl;
        return robManipulator::EFAILURE;
    }

    // for orientation, we assume the goal is reachable
    vctFrm4x4 Rt03 = ForwardKinematics(q, 3);
    vctFrm4x4 Rt34; // rotation for last link
    Rt03.ApplyInverseTo(Rt04, Rt34);
    // find angle to align x axis
    q[3] = -acosl(vctDotProduct(Rt34.Rotation().Column(0).Ref<3>(), vct3(1.0, 0.0, 0.0)));

    return robManipulator::ESUCCESS;
}