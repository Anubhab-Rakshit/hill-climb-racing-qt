"""
High-Fidelity 2D Physics Simulation Environment for Hill Climb Racing RL Benchmarks.
Faithfully mirrors the C++ software rasterization game mechanics:
- Suspension spring-damper dynamics (rear and front wheels)
- Multi-harmonic terrain profiles across biomes (countryside, desert, super_countryside, neon)
- Fuel depletion, coin and canister pickup mechanics
- Stunt tracking (backflips, frontflips, air time, wheelies)
- Chassis collision / neck injury conditions
"""

import math
import numpy as np

class HillClimbEnv:
    def __init__(self, biome="countryside", max_steps=2500):
        self.biome = biome
        self.max_steps = max_steps
        self.dt = 1.0 / 60.0  # 60 Hz physics step
        self.gravity = -9.81  # m/s^2

        # Vehicle physical properties (tuned to Offroader C++ parameters)
        self.chassis_mass = 420.0  # kg
        self.chassis_inertia = 250.0  # kg*m^2
        self.wheel_radius = 0.268  # m
        self.wheel_base = 1.184  # distance between wheels (2 * 0.592m)
        self.cg_height = 0.28  # center of gravity height (m)
        
        # Suspension parameters (tuned to C++ Offroader baseSpringRate / DamperRate)
        self.susp_rest_length = 0.18  # m
        self.susp_k = 38000.0  # N/m stiffness
        self.susp_c = 4200.0  # N*s/m damping

        # Drivetrain
        self.max_engine_torque = 3800.0  # N*m (matches C++ basePeakTorque)
        self.brake_torque = 1400.0  # N*m (matches C++ baseBrakingTorque)
        self.max_wheel_speed = 95.0  # rad/s (~140 km/h)
        self.fuel_capacity = 100.0  # arbitrary fuel units
        self.fuel_burn_rate = 0.045  # fuel per second base
        self.fuel_throttle_rate = 0.12  # fuel per second under full throttle

        # State space dimensions: 20 features
        # [v_x, v_y, cos_th, sin_th, omega, rear_w_spd, front_w_spd,
        #  rear_contact, front_contact, rear_susp_comp, front_susp_comp,
        #  fuel_ratio, 8 terrain height lookahead probes]
        self.obs_dim = 20
        self.action_dim = 1  # Continuous throttle [-1.0 (brake) to +1.0 (gas)]

        self.reset()

    def get_terrain_height_and_slope(self, x):
        """Analytical multi-harmonic terrain profile matching C++ Terrain."""
        # Flat starting runway for smooth launch and acceleration
        if x < 20.0:
            return 0.0, 0.0

        lx = x - 20.0
        # Cosine smoothstep entry ramp
        ramp = (0.5 - 0.5 * math.cos(lx * math.pi / 20.0)) if lx < 20.0 else 1.0
        d_ramp = (0.5 * math.pi / 20.0 * math.sin(lx * math.pi / 20.0)) if lx < 20.0 else 0.0

        if self.biome == "desert":
            # Long sandy rolling dunes
            raw_h = (math.sin(lx * 0.015) * 8.0 + 
                     math.sin(lx * 0.045 + 1.2) * 4.5 + 
                     math.sin(lx * 0.09) * 1.5)
            raw_dh = (0.015 * 8.0 * math.cos(lx * 0.015) + 
                      0.045 * 4.5 * math.cos(lx * 0.045 + 1.2) + 
                      0.09 * 1.5 * math.cos(lx * 0.09))
        elif self.biome == "super_countryside":
            # Extreme roller-coaster hills and valleys
            raw_h = (math.sin(lx * 0.02) * 14.0 + 
                     math.sin(lx * 0.06 + 0.8) * 7.0 + 
                     math.sin(lx * 0.14) * 3.0)
            raw_dh = (0.02 * 14.0 * math.cos(lx * 0.02) + 
                      0.06 * 7.0 * math.cos(lx * 0.06 + 0.8) + 
                      0.14 * 3.0 * math.cos(lx * 0.14))
        elif self.biome == "neon":
            # Smooth cyber highway with periodic ramps
            raw_h = (math.sin(lx * 0.012) * 6.0 + 
                     math.sin(lx * 0.035) * 3.0 + 
                     math.sin(lx * 0.08) * 1.0)
            raw_dh = (0.012 * 6.0 * math.cos(lx * 0.012) + 
                      0.035 * 3.0 * math.cos(lx * 0.035) + 
                      0.08 * 1.0 * math.cos(lx * 0.08))
        else:  # Countryside default
            raw_h = (math.sin(lx * 0.025) * 9.0 + 
                     math.sin(lx * 0.05 + 0.5) * 4.5 + 
                     math.sin(lx * 0.12) * 1.8)
            raw_dh = (0.025 * 9.0 * math.cos(lx * 0.025) + 
                      0.05 * 4.5 * math.cos(lx * 0.05 + 0.5) + 
                      0.12 * 1.8 * math.cos(lx * 0.12))

        h = raw_h * ramp
        dh = raw_dh * ramp + raw_h * d_ramp
        return h, dh

    def reset(self, seed=None, biome=None):
        if seed is not None:
            np.random.seed(seed)
        if biome is not None:
            self.biome = biome

        self.step_count = 0
        self.time = 0.0
        
        # Vehicle kinematics (Spawn at 5.0m on flat starting runway)
        self.pos_x = 5.0
        init_h, _ = self.get_terrain_height_and_slope(5.0)
        self.pos_y = init_h + self.susp_rest_length + self.wheel_radius + 0.05
        self.vel_x = 0.0
        self.vel_y = 0.0
        self.angle = 0.0  # radians
        self.angular_vel = 0.0  # rad/s

        self.rear_wheel_w = 0.0
        self.front_wheel_w = 0.0
        self.rear_contact = False
        self.front_contact = False
        self.rear_susp_comp = 0.0
        self.front_susp_comp = 0.0

        # Resources & Stunts
        self.fuel = self.fuel_capacity
        self.coins_collected = 0
        self.flips = 0
        self.air_time = 0.0
        self.total_air_time = 0.0
        self.cumulative_rotation = 0.0
        self.max_distance = 0.0
        self.is_crashed = False
        self.out_of_fuel = False

        # Periodic Pickups placed every 80m (fuel) and 25m (coins)
        self.next_fuel_x = 85.0
        self.next_coin_x = 25.0

        return self._get_obs(), {}

    def _get_obs(self):
        obs = np.zeros(self.obs_dim, dtype=np.float32)
        obs[0] = np.clip(self.vel_x / 25.0, -2.0, 2.0)
        obs[1] = np.clip(self.vel_y / 25.0, -2.0, 2.0)
        obs[2] = math.cos(self.angle)
        obs[3] = math.sin(self.angle)
        obs[4] = np.clip(self.angular_vel / 10.0, -2.0, 2.0)
        obs[5] = np.clip(self.rear_wheel_w / self.max_wheel_speed, -1.5, 1.5)
        obs[6] = np.clip(self.front_wheel_w / self.max_wheel_speed, -1.5, 1.5)
        obs[7] = 1.0 if self.rear_contact else 0.0
        obs[8] = 1.0 if self.front_contact else 0.0
        obs[9] = np.clip(self.rear_susp_comp / self.susp_rest_length, -1.0, 1.5)
        obs[10] = np.clip(self.front_susp_comp / self.susp_rest_length, -1.0, 1.5)
        obs[11] = self.fuel / self.fuel_capacity

        # 8 Lookahead probes ahead of vehicle: +2m, +4m, +7m, +10m, +15m, +20m, +25m, +30m
        probes = [2.0, 4.0, 7.0, 10.0, 15.0, 20.0, 25.0, 30.0]
        for i, dist in enumerate(probes):
            probe_x = self.pos_x + dist
            th, ts = self.get_terrain_height_and_slope(probe_x)
            # Relative height probe normalized to vehicle body
            rel_h = (th - self.pos_y) / 10.0
            obs[12 + i] = np.clip(rel_h, -3.0, 3.0)

        return obs

    def step(self, action):
        """
        Executes one 60Hz physics step.
        action: float in [-1.0, 1.0]
        """
        self.step_count += 1
        self.time += self.dt

        # Clip action
        act = float(np.clip(action, -1.0, 1.0))
        throttle = max(0.0, act)
        brake = max(0.0, -act)

        # Deplete fuel
        if self.fuel > 0.0:
            fuel_cost = (self.fuel_burn_rate + throttle * self.fuel_throttle_rate) * self.dt
            self.fuel = max(0.0, self.fuel - fuel_cost)
        else:
            throttle = 0.0  # Engine dies on empty fuel
            self.out_of_fuel = True

        # Wheel positions relative to chassis center
        half_wb = self.wheel_base * 0.5
        cos_a = math.cos(self.angle)
        sin_a = math.sin(self.angle)

        # Rear wheel mount position
        rw_mount_x = self.pos_x - half_wb * cos_a
        rw_mount_y = self.pos_y - half_wb * sin_a - self.cg_height

        # Front wheel mount position
        fw_mount_x = self.pos_x + half_wb * cos_a
        fw_mount_y = self.pos_y + half_wb * sin_a - self.cg_height

        # Terrain contact under wheels
        rw_ground_y, rw_slope = self.get_terrain_height_and_slope(rw_mount_x)
        fw_ground_y, fw_slope = self.get_terrain_height_and_slope(fw_mount_x)

        # Suspension compression
        rw_normal_len = (rw_mount_y - (rw_ground_y + self.wheel_radius))
        fw_normal_len = (fw_mount_y - (fw_ground_y + self.wheel_radius))

        rw_comp = max(0.0, self.susp_rest_length - rw_normal_len)
        fw_comp = max(0.0, self.susp_rest_length - fw_normal_len)
        self.rear_susp_comp = rw_comp
        self.front_susp_comp = fw_comp

        self.rear_contact = (rw_comp > 0.001)
        self.front_contact = (fw_comp > 0.001)

        # Calculate forces
        total_fx = 0.0
        total_fy = self.chassis_mass * self.gravity
        total_torque = 0.0

        # Rear suspension & traction
        if self.rear_contact:
            # Spring + damper force
            f_spring = self.susp_k * rw_comp - self.susp_c * self.vel_y
            f_norm = max(0.0, f_spring)
            
            # Ground normal vector
            ground_ang = math.atan(rw_slope)
            norm_x = -math.sin(ground_ang)
            norm_y = math.cos(ground_ang)
            tang_x = math.cos(ground_ang)
            tang_y = math.sin(ground_ang)

            # Drive torque from engine applied to rear wheel
            drive_force = (throttle * self.max_engine_torque / self.wheel_radius)
            brake_force = (brake * self.brake_torque / self.wheel_radius)
            traction_force = drive_force - brake_force

            # Friction limit (Coulomb friction mu ~ 0.95 in countryside, 0.70 in desert)
            mu = 0.72 if self.biome == "desert" else 0.96
            max_traction = f_norm * mu
            traction_force = np.clip(traction_force, -max_traction, max_traction)

            total_fx += f_norm * norm_x + traction_force * tang_x
            total_fy += f_norm * norm_y + traction_force * tang_y
            
            # Moment about CG
            arm_x = rw_mount_x - self.pos_x
            arm_y = rw_mount_y - self.pos_y
            total_torque += arm_x * (f_norm * norm_y) - arm_y * (f_norm * norm_x)

            self.rear_wheel_w = (self.vel_x * tang_x + self.vel_y * tang_y) / self.wheel_radius

        # Front suspension & braking
        if self.front_contact:
            f_spring = self.susp_k * fw_comp - self.susp_c * self.vel_y
            f_norm = max(0.0, f_spring)
            
            ground_ang = math.atan(fw_slope)
            norm_x = -math.sin(ground_ang)
            norm_y = math.cos(ground_ang)
            tang_x = math.cos(ground_ang)
            tang_y = math.sin(ground_ang)

            brake_force = (brake * self.brake_torque / self.wheel_radius)
            mu = 0.72 if self.biome == "desert" else 0.96
            max_traction = f_norm * mu
            traction_force = np.clip(-brake_force, -max_traction, max_traction)

            total_fx += f_norm * norm_x + traction_force * tang_x
            total_fy += f_norm * norm_y + traction_force * tang_y

            arm_x = fw_mount_x - self.pos_x
            arm_y = fw_mount_y - self.pos_y
            total_torque += arm_x * (f_norm * norm_y) - arm_y * (f_norm * norm_x)

            self.front_wheel_w = (self.vel_x * tang_x + self.vel_y * tang_y) / self.wheel_radius

        # Airborne tilt control (gyro torque in air: gas tilts back, brake tilts forward)
        if not self.rear_contact and not self.front_contact:
            air_tilt_torque = 180.0 * (throttle - brake)
            total_torque += air_tilt_torque
            self.air_time += self.dt
            self.total_air_time += self.dt
        else:
            self.air_time = 0.0

        # Air resistance (drag)
        speed = math.sqrt(self.vel_x**2 + self.vel_y**2)
        drag = 0.5 * 1.225 * 0.75 * 1.8 * (speed**2)
        if speed > 0.001:
            total_fx -= drag * (self.vel_x / speed)
            total_fy -= drag * (self.vel_y / speed)

        # Integrate vehicle acceleration
        acc_x = total_fx / self.chassis_mass
        acc_y = total_fy / self.chassis_mass
        ang_acc = total_torque / self.chassis_inertia

        self.vel_x += acc_x * self.dt
        self.vel_y += acc_y * self.dt
        self.angular_vel += ang_acc * self.dt

        # Damping on angular velocity
        self.angular_vel *= 0.998

        prev_x = self.pos_x
        self.pos_x += self.vel_x * self.dt
        self.pos_y += self.vel_y * self.dt
        self.angle += self.angular_vel * self.dt
        self.cumulative_rotation += self.angular_vel * self.dt

        # Check for full flips (360 deg)
        if abs(self.cumulative_rotation) >= 2.0 * math.pi:
            self.flips += int(abs(self.cumulative_rotation) / (2.0 * math.pi))
            self.cumulative_rotation = math.fmod(self.cumulative_rotation, 2.0 * math.pi)

        # Update max distance
        delta_dist = max(0.0, self.pos_x - prev_x)
        self.max_distance = max(self.max_distance, self.pos_x)

        # Item Pickups
        reward = 0.0
        # 1. Forward progression reward
        reward += delta_dist * 2.5
        # 2. Velocity reward (incentivizes pace)
        reward += max(0.0, self.vel_x) * 0.05
        # 3. Fuel efficiency penalty (very slight)
        reward -= (0.01 * throttle)

        # Fuel Canister Pickup
        if self.pos_x >= self.next_fuel_x:
            self.fuel = min(self.fuel_capacity, self.fuel + 40.0)
            self.next_fuel_x += 90.0
            reward += 15.0  # Big bonus for getting fuel!

        # Coin Pickup
        if self.pos_x >= self.next_coin_x:
            self.coins_collected += 1
            self.next_coin_x += 25.0
            reward += 5.0

        # Flip bonus
        if self.flips > 0:
            reward += self.flips * 20.0
            self.flips = 0

        # Check Crash condition: driver head / chassis touches ground upside down
        # Chassis center touches ground when pos_y <= terrain_h + 0.15
        chassis_ground_y, _ = self.get_terrain_height_and_slope(self.pos_x)
        cos_ang = math.cos(self.angle)
        
        # Inverted chassis / driver head crash check
        # When car is inverted (>105 deg pitch) and chassis body penetrates ground level
        if cos_ang < -0.25 and (self.pos_y - chassis_ground_y) < (self.wheel_radius + 0.12):
            self.is_crashed = True
            reward -= 50.0

        # Backward off start
        if self.pos_x < -15.0:
            self.is_crashed = True
            reward -= 20.0

        terminated = self.is_crashed or (self.out_of_fuel and abs(self.vel_x) < 0.2)
        truncated = (self.step_count >= self.max_steps)

        info = {
            "distance": self.pos_x,
            "max_distance": self.max_distance,
            "fuel": self.fuel,
            "speed_kmh": self.vel_x * 3.6,
            "coins": self.coins_collected,
            "is_crashed": self.is_crashed,
            "out_of_fuel": self.out_of_fuel,
            "air_time": self.total_air_time
        }

        return self._get_obs(), reward, terminated, truncated, info
