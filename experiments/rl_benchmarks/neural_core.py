"""
Pure NumPy Deep Neural Network & Optimization Framework.
Provides vectorised layers, optimizers, Gaussian policies, Q-functions,
replay buffers, and conjugate gradient routines for reinforcement learning.
Zero heavyweight external framework dependencies required.
"""

import numpy as np

class Linear:
    def __init__(self, in_features, out_features, init_scale=1.0):
        # He / Glorot initialization
        std = np.sqrt(2.0 / (in_features + out_features)) * init_scale
        self.W = np.random.randn(in_features, out_features).astype(np.float32) * std
        self.b = np.zeros(out_features, dtype=np.float32)
        
        self.grad_W = np.zeros_like(self.W)
        self.grad_b = np.zeros_like(self.b)
        self.x = None

    def forward(self, x):
        self.x = x
        return np.dot(x, self.W) + self.b

    def backward(self, delta):
        # delta shape: (batch_size, out_features) or (out_features,)
        if delta.ndim == 1:
            delta = delta.reshape(1, -1)
            x = self.x.reshape(1, -1)
        else:
            x = self.x
        self.grad_W += np.dot(x.T, delta)
        self.grad_b += np.sum(delta, axis=0)
        return np.dot(delta, self.W.T)

    def zero_grad(self):
        self.grad_W.fill(0.0)
        self.grad_b.fill(0.0)

class MLP:
    def __init__(self, in_dim, hidden_dims, out_dim, activation='tanh', out_activation=None):
        self.layers = []
        dims = [in_dim] + hidden_dims + [out_dim]
        for i in range(len(dims) - 1):
            is_last = (i == len(dims) - 2)
            scale = 0.1 if is_last else 1.0
            self.layers.append(Linear(dims[i], dims[i+1], init_scale=scale))
        
        self.activation = activation
        self.out_activation = out_activation
        self.acts = []

    def forward(self, x):
        h = x
        self.acts = [h]
        for i, layer in enumerate(self.layers):
            is_last = (i == len(self.layers) - 1)
            z = layer.forward(h)
            if not is_last:
                if self.activation == 'tanh':
                    h = np.tanh(z)
                elif self.activation == 'relu':
                    h = np.maximum(0.0, z)
                else:
                    h = z
            else:
                if self.out_activation == 'tanh':
                    h = np.tanh(z)
                elif self.out_activation == 'sigmoid':
                    h = 1.0 / (1.0 + np.exp(-np.clip(z, -15.0, 15.0)))
                else:
                    h = z
            self.acts.append(h)
        return h

    def backward(self, delta):
        cur_delta = delta
        for i in reversed(range(len(self.layers))):
            is_last = (i == len(self.layers) - 1)
            if is_last:
                if self.out_activation == 'tanh':
                    cur_delta = cur_delta * (1.0 - self.acts[i+1]**2)
            else:
                if self.activation == 'tanh':
                    cur_delta = cur_delta * (1.0 - self.acts[i+1]**2)
                elif self.activation == 'relu':
                    cur_delta = cur_delta * (self.acts[i+1] > 0.0).astype(np.float32)

            cur_delta = self.layers[i].backward(cur_delta)
        return cur_delta

    def zero_grad(self):
        for l in self.layers:
            l.zero_grad()

    def get_params(self):
        params = []
        for l in self.layers:
            params.append(l.W)
            params.append(l.b)
        return params

    def get_grads(self):
        grads = []
        for l in self.layers:
            grads.append(l.grad_W)
            grads.append(l.grad_b)
        return grads

    def get_flat_params(self):
        return np.concatenate([p.ravel() for p in self.get_params()])

    def set_flat_params(self, flat_params):
        idx = 0
        for l in self.layers:
            w_size = l.W.size
            l.W = flat_params[idx:idx+w_size].reshape(l.W.shape).copy()
            idx += w_size
            b_size = l.b.size
            l.b = flat_params[idx:idx+b_size].reshape(l.b.shape).copy()
            idx += b_size

class Adam:
    def __init__(self, params, grads, lr=1e-3, beta1=0.9, beta2=0.999, eps=1e-8, weight_decay=1e-4):
        self.params = params
        self.grads = grads
        self.lr = lr
        self.beta1 = beta1
        self.beta2 = beta2
        self.eps = eps
        self.weight_decay = weight_decay
        self.m = [np.zeros_like(p) for p in params]
        self.v = [np.zeros_like(p) for p in params]
        self.t = 0

    def step(self):
        self.t += 1
        for i in range(len(self.params)):
            g = self.grads[i] + self.weight_decay * self.params[i]
            self.m[i] = self.beta1 * self.m[i] + (1.0 - self.beta1) * g
            self.v[i] = self.beta2 * self.v[i] + (1.0 - self.beta2) * (g ** 2)
            
            m_hat = self.m[i] / (1.0 - self.beta1 ** self.t)
            v_hat = self.v[i] / (1.0 - self.beta2 ** self.t)
            
            self.params[i] -= self.lr * m_hat / (np.sqrt(v_hat) + self.eps)

class GaussianPolicy:
    """Stochastic policy for continuous action space with diagonal Gaussian."""
    def __init__(self, obs_dim, act_dim, hidden_dims=[64, 64], lr=3e-4):
        self.mlp = MLP(obs_dim, hidden_dims, act_dim, activation='tanh', out_activation='tanh')
        # Bias towards positive throttle initially for exploration of forward driving
        self.mlp.layers[-1].b.fill(0.65)
        self.log_std = np.zeros(act_dim, dtype=np.float32) - 0.5
        self.grad_log_std = np.zeros_like(self.log_std)

        self.params = self.mlp.get_params() + [self.log_std]
        self.grads = self.mlp.get_grads() + [self.grad_log_std]
        self.optimizer = Adam(self.params, self.grads, lr=lr)

    def forward(self, obs):
        mu = self.mlp.forward(obs)
        std = np.exp(self.log_std)
        return mu, std

    def sample_action(self, obs, deterministic=False):
        mu, std = self.forward(obs)
        if deterministic:
            return np.clip(mu, -1.0, 1.0)
        noise = np.random.randn(*mu.shape).astype(np.float32)
        action = mu + std * noise
        return np.clip(action, -1.0, 1.0)

    def log_prob(self, obs, action):
        mu, std = self.forward(obs)
        var = std ** 2
        logp = -0.5 * (((action - mu) ** 2) / var + 2.0 * self.log_std + np.log(2.0 * np.pi))
        return np.sum(logp, axis=-1)

    def zero_grad(self):
        self.mlp.zero_grad()
        self.grad_log_std.fill(0.0)

class DeterministicPolicy:
    """Deterministic actor network for continuous control (DDPG / TD3)."""
    def __init__(self, obs_dim, act_dim, hidden_dims=[64, 64], lr=1e-3):
        self.mlp = MLP(obs_dim, hidden_dims, act_dim, activation='relu', out_activation='tanh')
        self.mlp.layers[-1].b.fill(0.65)
        self.optimizer = Adam(self.mlp.get_params(), self.mlp.get_grads(), lr=lr)

    def forward(self, obs):
        return self.mlp.forward(obs)

    def zero_grad(self):
        self.mlp.zero_grad()

class QFunction:
    """Action-Value Q(s, a) critic network."""
    def __init__(self, obs_dim, act_dim, hidden_dims=[64, 64], lr=1e-3):
        self.mlp = MLP(obs_dim + act_dim, hidden_dims, 1, activation='relu', out_activation=None)
        self.optimizer = Adam(self.mlp.get_params(), self.mlp.get_grads(), lr=lr)

    def forward(self, obs, act):
        # Concatenate state and action along last axis
        if obs.ndim == 1:
            x = np.concatenate([obs, act.reshape(-1)])
        else:
            x = np.concatenate([obs, act], axis=-1)
        return self.mlp.forward(x)

    def zero_grad(self):
        self.mlp.zero_grad()

class ValueFunction:
    """State-Value V(s) baseline critic."""
    def __init__(self, obs_dim, hidden_dims=[64, 64], lr=1e-3):
        self.mlp = MLP(obs_dim, hidden_dims, 1, activation='tanh', out_activation=None)
        self.optimizer = Adam(self.mlp.get_params(), self.mlp.get_grads(), lr=lr)

    def forward(self, obs):
        return self.mlp.forward(obs)

    def zero_grad(self):
        self.mlp.zero_grad()

class ReplayBuffer:
    """Circular experience replay buffer for off-policy algorithms."""
    def __init__(self, max_size=50000, obs_dim=20, act_dim=1):
        self.max_size = max_size
        self.ptr = 0
        self.size = 0
        self.obs = np.zeros((max_size, obs_dim), dtype=np.float32)
        self.actions = np.zeros((max_size, act_dim), dtype=np.float32)
        self.rewards = np.zeros((max_size, 1), dtype=np.float32)
        self.next_obs = np.zeros((max_size, obs_dim), dtype=np.float32)
        self.dones = np.zeros((max_size, 1), dtype=np.float32)

    def add(self, obs, action, reward, next_obs, done):
        self.obs[self.ptr] = obs
        self.actions[self.ptr] = action
        self.rewards[self.ptr] = reward
        self.next_obs[self.ptr] = next_obs
        self.dones[self.ptr] = 1.0 if done else 0.0

        self.ptr = (self.ptr + 1) % self.max_size
        self.size = min(self.size + 1, self.max_size)

    def sample(self, batch_size=64):
        idxs = np.random.randint(0, self.size, size=batch_size)
        return (self.obs[idxs], self.actions[idxs], self.rewards[idxs],
                self.next_obs[idxs], self.dones[idxs])

def conjugate_gradient(Ax_fn, b, nsteps=10, residual_tol=1e-10):
    """Solves Ax = b using the Conjugate Gradient algorithm."""
    x = np.zeros_like(b)
    r = b.copy()
    p = r.copy()
    rdotr = np.dot(r, r)

    for _ in range(nsteps):
        Ap = Ax_fn(p)
        alpha = rdotr / (np.dot(p, Ap) + 1e-8)
        x += alpha * p
        r -= alpha * Ap
        new_rdotr = np.dot(r, r)
        if new_rdotr < residual_tol:
            break
        beta = new_rdotr / (rdotr + 1e-8)
        p = r + beta * p
        rdotr = new_rdotr
    return x
