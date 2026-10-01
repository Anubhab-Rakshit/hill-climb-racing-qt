# Document 02: Core Physics Engine Specification
**Mathematical Modeling, Vehicle Dynamics, and Numerical Integration**

---

## 1. System Overview
The physics engine powers the entire player experience. Hill Climb Racing relies on a dynamic balance between **suspension oscillation**, **chassis inertia**, **tire grip**, and **in-air rotational agility**. 

To deliver high-end simulation fidelity and guarantee absolute stability, we specify a custom **2D Multi-Body Spring-Damper Simulation** using **Symplectic Euler Integration** operating at a fixed rate of $120\text{ Hz}$ with sub-stepping.

---

## 2. Mathematical Vehicle Model

```
                    ^ y
                    |
                    |         Chassis Body (Mass M, Inertia I)
                    +---> x       +-----------------------+
                                  |       COM (xc, yc)    |   (Driver Head)
                                  |           (x)         |       [O]
                                  +---+---------------+---+
                                     /                 \
                                    / (k_r, c_r)        \ (k_f, c_f)
                                   / (Rear Strut)        \ (Front Strut)
                                  O                       O
                             Rear Wheel               Front Wheel
                             (mw, Iw, R)              (mw, Iw, R)
                         =============================================
                                     Terrain y = f(x)
```

### 2.1 State Variables
The vehicle is composed of three interconnected bodies:
1. **Chassis Rigid Body**:
   - Mass: $M_c$ ($\text{kg}$)
   - Moment of Inertia about COM: $I_c = \frac{1}{12} M_c (w^2 + h^2)$ ($\text{kg}\cdot\text{m}^2$)
   - Center of Mass Position: $\vec{P}_c = \begin{bmatrix} x_c \\ y_c \end{bmatrix}$
   - Linear Velocity: $\vec{V}_c = \begin{bmatrix} \dot{x}_c \\ \dot{y}_c \end{bmatrix}$
   - Chassis Pitch Angle: $\theta_c$ ($\text{rad}$)
   - Angular Velocity: $\omega_c = \dot{\theta}_c$ ($\text{rad/s}$)

2. **Rear & Front Wheels ($i \in \{r, f\}$)**:
   - Mass: $m_w$ ($\text{kg}$)
   - Moment of Inertia: $I_w = \frac{1}{2} m_w R_w^2$ ($\text{kg}\cdot\text{m}^2$)
   - Radius: $R_w$ ($\text{m}$)
   - Center Position: $\vec{P}_{w,i} = \begin{bmatrix} x_{w,i} \\ y_{w,i} \end{bmatrix}$
   - Linear Velocity: $\vec{V}_{w,i} = \begin{bmatrix} \dot{x}_{w,i} \\ \dot{y}_{w,i} \end{bmatrix}$
   - Wheel Spin Angle: $\phi_{w,i}$ ($\text{rad}$)
   - Angular Velocity: $\omega_{w,i} = \dot{\phi}_{w,i}$ ($\text{rad/s}$)

---

## 3. Forces & Dynamical Equations

### 3.1 Gravity
Each body experiences downward gravitational acceleration $\vec{g} = \begin{bmatrix} 0 \\ -9.81 \end{bmatrix}\text{ m/s}^2$:
$$\vec{F}_{g,c} = M_c \vec{g}, \quad \vec{F}_{g,w,i} = m_w \vec{g}$$

### 3.2 Suspension Dynamics (Spring-Damper Mechanism)
Let $\vec{A}_i$ be the chassis mount point in world coordinates:
$$\vec{A}_i = \vec{P}_c + \mathbf{R}(\theta_c) \vec{r}_{local,i}$$
where $\mathbf{R}(\theta_c) = \begin{bmatrix} \cos\theta_c & -\sin\theta_c \\ \sin\theta_c & \cos\theta_c \end{bmatrix}$.

The suspension vector from wheel to mount point is:
$$\vec{S}_i = \vec{A}_i - \vec{P}_{w,i}$$
The current length is $L_i = \|\vec{S}_i\|$, and unit direction is $\hat{u}_i = \frac{\vec{S}_i}{L_i}$.

Let $L_0$ be the uncompressed rest length. The compression displacement is:
$$\Delta L_i = L_0 - L_i$$

The relative velocity along the suspension axis is:
$$v_{rel,i} = (\vec{V}_c + \omega_c \times (\vec{A}_i - \vec{P}_c) - \vec{V}_{w,i}) \cdot \hat{u}_i$$

The suspension force magnitude $F_{susp,i}$ combines Hooke's Law and viscous damping:
$$F_{susp,i} = k_{susp} \Delta L_i - c_{damp} v_{rel,i}$$

**Bump Stop Constraint**: If compression exceeds maximum travel $L_{min}$, a stiff penalty spring engages to prevent clipping:
$$F_{susp,i} = \max(0, F_{susp,i}) + k_{bump} \max(0, L_{min} - L_i)$$

The force applied to the wheel is:
$$\vec{F}_{w\_susp,i} = -F_{susp,i} \hat{u}_i$$
The equal and opposite reaction on the chassis mount point produces both linear force and torque:
$$\vec{F}_{c\_susp,i} = +F_{susp,i} \hat{u}_i$$
$$\tau_{c\_susp,i} = (\vec{A}_i - \vec{P}_c) \times \vec{F}_{c\_susp,i}$$

---

### 3.3 Drive Torque, Braking, and Airborne Control

#### On-Ground Traction Mode:
When the throttle pedal is depressed ($u_{gas} \in [0, 1]$):
$$\tau_{drive, r} = u_{gas} \cdot T_{max}$$
For 4WD vehicles:
$$\tau_{drive, f} = \lambda_{4wd} \cdot u_{gas} \cdot T_{max}, \quad \tau_{drive, r} = (1 - \lambda_{4wd}) \cdot u_{gas} \cdot T_{max}$$

When the brake pedal is depressed ($u_{brake} \in [0, 1]$):
$$\tau_{brake, i} = -u_{brake} \cdot T_{brake} \cdot \operatorname{sign}(\omega_{w,i})$$

#### In-Air Gyroscopic/Attitude Control Mode (Signature Mechanic):
When both wheels lose ground contact ($contact_r = \text{false} \land contact_f = \text{false}$):
- Depressing **GAS** rotates the vehicle counter-clockwise (leans back):
  $$\tau_{air} = +K_{air} \cdot u_{gas}$$
- Depressing **BRAKE** rotates the vehicle clockwise (leans forward):
  $$\tau_{air} = -K_{air} \cdot u_{brake}$$
This allows skilled players to land parallel to slopes, perform backflips, and prevent nose-dives.

---

### 3.4 Terrain Collision & Friction Model

#### Heightmap Definition:
The terrain elevation is given by a continuous or piecewise-cubic function $y = H(x)$. At any point $x$, the terrain slope and unit normal are:
$$m = \frac{dH}{dx}(x), \quad \hat{n}(x) = \frac{1}{\sqrt{1 + m^2}} \begin{bmatrix} -m \\ 1 \end{bmatrix}, \quad \hat{t}(x) = \frac{1}{\sqrt{1 + m^2}} \begin{bmatrix} 1 \\ m \end{bmatrix}$$

#### Wheel Penetration:
For wheel center $(x_w, y_w)$, find ground elevation $y_g = H(x_w)$.
The signed distance to surface along the normal is approximately:
$$d = (y_w - y_g) \cdot n_y$$
If $d < R_w$, a collision occurs with penetration depth:
$$\delta = R_w - d$$

#### Normal Reaction Force:
Using penalty spring-damper formulation at the contact patch:
$$F_N = \max\left(0, k_{contact} \cdot \delta - c_{contact} (\vec{V}_w \cdot \hat{n})\right)$$

#### Tangential Friction & Slip Dynamics:
The relative slip speed at the contact patch is:
$$v_{slip} = (\vec{V}_w \cdot \hat{t}) - \omega_w R_w$$
The Coulomb friction limit is $F_{f\_max} = \mu \cdot F_N$.
Using a smoothed sigmoid slip curve to eliminate numerical chatter near zero speed:
$$F_t = -\mu \cdot F_N \cdot \tanh\left(\frac{v_{slip}}{v_{threshold}}\right)$$

The force $\vec{F}_t = F_t \hat{t}$ accelerates the wheel center linearly and exerts counter-torque on wheel rotation:
$$\tau_{friction} = -F_t \cdot R_w$$

---

## 4. Numerical Integration Scheme

We employ the **Symplectic (Semi-Implicit) Euler Method**. Unlike standard Explicit Euler (which adds energy and blows up stiff springs), Symplectic Euler conserves phase space volume and ensures unconditional stability for our spring constants.

```cpp
void PhysicsWorld::step(float dt) {
    // 1. Accumulate Forces & Torques
    computeSuspensionForces();
    computeTerrainCollisions();
    computeAirborneTorques();
    
    // 2. Integrate Velocities (First!)
    chassis.velocity += (chassis.netForce / chassis.mass) * dt;
    chassis.angularVelocity += (chassis.netTorque / chassis.inertia) * dt;
    
    for (Wheel& w : wheels) {
        w.velocity += (w.netForce / w.mass) * dt;
        w.angularVelocity += (w.netTorque / w.inertia) * dt;
        
        // Angular damping
        w.angularVelocity *= (1.0f - w.rollingResistance * dt);
    }
    
    // 3. Integrate Positions (Using UPDATED velocities!)
    chassis.position += chassis.velocity * dt;
    chassis.angle += chassis.angularVelocity * dt;
    
    for (Wheel& w : wheels) {
        w.position += w.velocity * dt;
        w.angle += w.angularVelocity * dt;
    }
}
```

---

## 5. Death Condition: Driver Neck-Snap
A circular bounding sensor of radius $R_{head}$ is attached to the driver's head:
$$\vec{P}_{head} = \vec{P}_c + \mathbf{R}(\theta_c) \vec{r}_{head\_offset}$$
If the head penetrates the ground:
$$P_{head,y} - H(P_{head,x}) \le R_{head}$$
and impact impulse $\|\vec{V}_c \cdot \hat{n}\| > v_{fatal\_threshold}$, the state machine triggers **DRIVER DOWN (Neck Snapped)**, terminating the run with a dramatic ragdoll / head-bob impulse.
