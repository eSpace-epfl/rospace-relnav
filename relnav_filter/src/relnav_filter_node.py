#!/usr/bin/env python
"""
ROS node for relative navigation filtering

    Author: Michael Pantic

"""

import rospy
import numpy as np
import scipy
import tf
import message_filters
from geometry_msgs.msg import PointStamped

from UKFRelativeOrbitalFilter import UKFRelativeOrbitalFilter
from rospace_msgs.msg import *
from rospace_lib import *

filter = None

if __name__ == '__main__':
    rospy.init_node("cso_gnc_target_estimator", anonymous=True)

    # read configuration values
    rf_cfg = rospy.get_param(rospy.get_namespace() + "/relnav_filter", 0)

    enable_bias = bool(rf_cfg["enable_bias"])
    enable_emp = bool(rf_cfg["enable_emp"])

    init_cfg = rf_cfg["filter_init"]
    P_roe = np.array(init_cfg["P"]).astype(np.float).reshape((6, 6), order='C')

    if enable_bias:
        P_bias = np.diag(np.array([np.float(x) for x in str(init_cfg["P_bias"]).split(" ")]))
    else:
        P_bias = np.array((0, 0))

    if enable_emp:
        P_emp = np.diag(np.array([np.float(x) for x in str(init_cfg["P_emp"]).split(" ")]))
    else:
        P_emp = np.array((0, 0))

    # build covariance matrix P based on enabled options
    if enable_emp and enable_bias:
        P_init = scipy.linalg.block_diag(P_roe, P_bias, P_emp)
    elif enable_emp and not enable_bias:
        P_init = scipy.linalg.block_diag(P_roe, P_emp)
    elif not enable_emp and enable_bias:
        P_init = scipy.linalg.block_diag(P_roe, P_bias)
    else:
        P_init = P_roe

    # set up R and Q
    R_init = np.diag(np.array([np.float(x) for x in str(init_cfg["R"]).split(" ")]))
    Q_init = np.diag(np.array([np.float(x) for x in str(init_cfg["Q"]).split(" ")]))

    # defines the used mean-to-osculating tranformation
    mode = rf_cfg["mode"]

    # Load initial x_roe state
    x_dict = rospy.get_param("/scenario/init_coords/" + rospy.get_namespace() + "/init_roe")

    roe_init = QNSRelOrbElements()
    roe_init.dA = float(x_dict["dA"])
    roe_init.dL = float(x_dict["dL"])
    roe_init.dIx = float(x_dict["dIx"])
    roe_init.dIy = float(x_dict["dIy"])
    roe_init.dEx = float(x_dict["dEx"])
    roe_init.dEy = float(x_dict["dEy"])

    x_init = np.zeros(P_init.shape[0])
    x_init[0:6] = roe_init.as_vector().reshape(6)

    filter = UKFRelativeOrbitalFilter(x=x_init,
                                      P=P_init,
                                      R=R_init,
                                      Q=Q_init,
                                      enable_bias=enable_bias,
                                      enable_emp=enable_emp,
                                      mode=mode,
                                      output_debug=True)

    # set up combined target/chaser/aon subscription such that they are received synchronized
    # Note: Target state subscription is ONLY used for evaluation and debug!!
    target_oe_sub = message_filters.Subscriber('target_oe', SatelitePose)
    chaser_oe_sub = message_filters.Subscriber('chaser_oe', SatelitePose)
    aon_sub = message_filters.Subscriber('aon', AzimutElevationRangeStamped)

    ts = message_filters.TimeSynchronizer([target_oe_sub, chaser_oe_sub, aon_sub], 10)
    ts.registerCallback(filter.callback_aon)

    # set publisher and rate limiter
    pub = rospy.Publisher('target_est', OrbitalElementsStamped, queue_size=10)
    r = rospy.Rate(10)

    while not rospy.is_shutdown():

        [valid, t_ukf, osc_O_c_est] = filter.get_state()

        if not valid:
            continue

        msg = OrbitalElementsStamped()
        msg.header.stamp = rospy.Time.from_seconds(t_ukf)
        msg.orbit.semimajoraxis = osc_O_c_est.a
        msg.orbit.eccentricity = osc_O_c_est.e
        msg.orbit.inclination = np.rad2deg(osc_O_c_est.i)
        msg.orbit.arg_perigee = np.rad2deg(osc_O_c_est.w)
        msg.orbit.raan = np.rad2deg(osc_O_c_est.O)
        msg.orbit.true_anomaly = np.rad2deg(osc_O_c_est.v)

        pub.publish(msg)

        r.sleep()
