"""
Sensor model for a generic sensor that calculates visibility
based on:
   Field of view (either horizontal/vertical or custom based on angles)
   Range (either simple l2 range or custom function based on angles and distance)

    Author: Michael Pantic
"""
import numpy as np
from tf import transformations


class GenericFOVSensor(object):

        def __init__(self):
            self.fov_x = 0
            self.fov_y = 0
            self.max_range = 0
            # transformatin from body frame to sensor frame
            self.T_CS_CB = np.identity(4)

        def set_frame_transform(self, T):
            """ Sets the transformation between body and sensor frame"""
            self.T_CS_CB = T

        def set_frame_by_string(self, str_q, str_p):
            """Parses a trasnformation matrix that is stored as a string (from configuration)"""
            T = GenericFOVSensor.get_transform_by_string(str_q, str_p)
            self.set_frame_transform(T)

        @staticmethod
        def get_transform_by_string(str_q, str_p):
            """ takes a quaternion (q) from frame A to frame B and a position (p) in frame A
                to calculate a transformation A => B
                same semantics as tf strings"""
            q = np.array([float(x) for x in str_q.split(" ")])
            p = np.array([float(x) for x in str_p.split(" ")])

            # calculate transformation
            # pre-multiply rotation , as P is in frame A
            transform = np.dot(transformations.quaternion_matrix([q[3], q[0], q[1], q[2]]).T,
                               transformations.translation_matrix(-p))

            return transform

        def to_sensor_frame(self, r_CB):
            """Converts vector to sensor frame"""

            # test if 3 or 4 vector (homogenous)
            if r_CB.size == 3:
                # make homogenous
                r_CB = np.append(r_CB, 1)

            # apply transform
            r_CS = np.dot(self.T_CS_CB, r_CB)

            # return non-homgenous 3-vector
            return r_CS[0:3]

        def set_fov_range(self, fov_x, fov_y, max_range):
            self.fov_x = fov_x
            self.fov_y = fov_y
            self.max_range = max_range

        # To be implemented in subclass
        def get_measurement(self, coord):
            return 0

        def get_measurement_noise(self, coord):
            return 0

        def is_visible(self, coord):
            """ Tests if the given coordinate is in field of view (range and angle)"""

            try:
                # returns error if measurement is behind sensor plane (=out of view anyway)
                angles = self.get_angles(coord)

            except:
                print "OUT OF VIEW"
                return False

            dist = np.linalg.norm(coord)
            return self.is_in_range(angles, dist) and self.is_in_fov(angles)

        def get_angles(self, coord):
            """Calculates angle of the observed Coordinates
                w.r.t. sensor frame in order to determine if measurement is
                in field of view. Not to be confused with the angles-only measurement model"""
            angles = np.zeros(2)

            # angle between z-axis and coord in the x-z plane
            angles[0] = np.arctan2(coord[0], coord[2])


            # angle between z-axis and coord in the y-z plane
            angles[1] = np.arctan2(coord[1], coord[2])

            # handle wrap around
            if angles[0] > np.pi / 2.0:
                angles[0] = angles[0] - np.pi

            if angles[1] > np.pi / 2.0:
                angles[1] = angles[1] - np.pi

            return angles

        def is_in_range(self, angles, dist):
            return dist <= self.max_range

        def is_in_fov(self, angles):
            return abs(angles[0]) <= self.fov_x/2.0 and abs(angles[1]) <= self.fov_y/2.0
