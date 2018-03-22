"""
Angles-only measurement model according to [1]

References:
    [1] Improved Maneuver-free approach to angles-only navigation for space rendezvous,
        J. Sullivan, A.W. Koenig, S. D'Amico, AAS 16-530

    Author: Michael Pantic
    License: TBD
"""
import numpy as np
from GenericFOVSensor import GenericFOVSensor

class PaperAnglesSensor(GenericFOVSensor):

    def __init__(self):
        super(PaperAnglesSensor, self).__init__()
        self.mu = [0, 0]
        self.sigma = [1, 1]

    def set_measurement_noise(self, mu, sigma):
        self.mu = mu
        self.sigma = sigma

    def get_measurement(self, r_CB):

        # get measurement in sensor frame for simulation
        r_CS = self.to_sensor_frame(r_CB)

        is_visible = self.is_visible(r_CS)

        # calculate measurement
        angles = np.zeros(2)
        angles[0] = np.arcsin(r_CS[1] / np.linalg.norm(r_CS))
        angles[1] = np.arctan2(r_CS[0], r_CS[2])

        # add noise
        angles[0] += np.asscalar(np.random.normal(self.mu[0], self.sigma[0], 1))
        angles[0] += np.asscalar(np.random.normal(self.mu[1], self.sigma[1], 1))

        return is_visible, angles

    def get_measurement_noise(self, coord):
        return np.array([0, 0])

