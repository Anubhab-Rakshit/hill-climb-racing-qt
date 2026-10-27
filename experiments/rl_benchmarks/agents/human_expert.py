"""
Human Expert Heuristic Controller.
Mimics experienced human player inputs:
- Smooth progressive throttle modulation on uphill gradients
- Proportional pitch gyro stabilization in mid-air for smooth parallel landing
- Downhill brake dampening to prevent rollover crashes
- Crest lookahead anticipation
"""

import math
import numpy as np

class HumanExpertAgent:
    def __init__(self):
        self.name = "Human Veteran (Telemetry)"

    def select_action(self, obs, deterministic=True):
        # State vector extraction:
        # obs[0]: vx (normalized), obs[1]: vy (normalized)
        # obs[2]: cos(theta), obs[3]: sin(theta)
        # obs[4]: angular velocity omega
        # obs[7]: rear contact, obs[8]: front contact
        # obs[11]: fuel ratio
        # obs[12..19]: 8 relative height lookahead probes

        vx = obs[0] * 25.0
        sin_th = obs[3]
        cos_th = obs[2]
        angle = math.atan2(sin_th, cos_th)
        omega = obs[4] * 10.0
        rear_contact = (obs[7] > 0.5)
        front_contact = (obs[8] > 0.5)
        is_airborne = not rear_contact and not front_contact
        
        # Crest lookahead (+4m to +10m relative height drop)
        h_near = obs[12]
        h_mid = obs[14]
        is_cresting = (h_mid < h_near - 0.25)

        if is_airborne:
            # Airborne attitude stabilization (PD controller)
            # Target angle is 0 (level) or slightly tilted forward for uphill slope
            target_angle = 0.05
            angle_err = angle - target_angle
            # Gas tilts backward (+torque), Brake tilts forward (-torque)
            u = - 1.8 * angle_err - 0.35 * omega
            return np.array([np.clip(u, -1.0, 1.0)], dtype=np.float32)

        # On ground:
        # If front wheel lifted high in a wheelie and pitching back dangerously, tap brake
        if (not front_contact and angle > 0.55) or omega > 3.5:
            return np.array([-0.65], dtype=np.float32)

        # If vehicle is diving nose first on steep downhill, apply gas to lift front
        if angle < -0.40 and front_contact and not rear_contact:
            return np.array([0.85], dtype=np.float32)

        # Near crest with high speed: feather throttle to prevent violent launches
        if is_cresting and vx > 15.0:
            return np.array([0.35], dtype=np.float32)

        # Standard forward climb: maintain strong healthy momentum
        if vx < 20.0:
            return np.array([1.0], dtype=np.float32)
        else:
            return np.array([0.80], dtype=np.float32)
