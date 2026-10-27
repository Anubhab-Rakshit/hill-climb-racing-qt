"""
Deep Deterministic Policy Gradient (DDPG).
Continuous off-policy actor-critic algorithm with target networks and Polyak averaging.
"""

import numpy as np
import copy
from neural_core import DeterministicPolicy, QFunction, ReplayBuffer

class DDPGAgent:
    def __init__(self, obs_dim=20, act_dim=1, gamma=0.99, tau=0.005,
                 lr_actor=1e-3, lr_critic=1e-3, buffer_size=50000):
        self.gamma = gamma
        self.tau = tau
        self.act_dim = act_dim

        self.actor = DeterministicPolicy(obs_dim, act_dim, hidden_dims=[64, 64], lr=lr_actor)
        self.critic = QFunction(obs_dim, act_dim, hidden_dims=[64, 64], lr=lr_critic)

        # Target networks
        self.target_actor = copy.deepcopy(self.actor)
        self.target_critic = copy.deepcopy(self.critic)

        self.replay_buffer = ReplayBuffer(max_size=buffer_size, obs_dim=obs_dim, act_dim=act_dim)

    def select_action(self, obs, noise_scale=0.1, deterministic=False):
        action = self.actor.forward(obs)
        if not deterministic and noise_scale > 0.0:
            noise = np.random.randn(self.act_dim).astype(np.float32) * noise_scale
            action = action + noise
        return np.clip(action, -1.0, 1.0)

    def update(self, batch_size=64):
        if self.replay_buffer.size < batch_size:
            return

        obs, acts, rews, next_obs, dones = self.replay_buffer.sample(batch_size)

        # 1. Critic Update
        # Compute target Q
        target_acts = self.target_actor.forward(next_obs)
        target_q = self.target_critic.forward(next_obs, target_acts)
        y = rews + self.gamma * (1.0 - dones) * target_q

        self.critic.zero_grad()
        current_q = self.critic.forward(obs, acts)
        q_loss_grad = (current_q - y) / batch_size
        self.critic.mlp.backward(q_loss_grad)
        self.critic.optimizer.step()

        # 2. Actor Update
        self.actor.zero_grad()
        pred_acts = self.actor.forward(obs)
        
        # d(-Q)/d_act: gradient of critic output w.r.t action input
        self.critic.zero_grad()
        self.critic.forward(obs, pred_acts)
        # Backprop through critic up to input layer
        in_delta = self.critic.mlp.backward(-np.ones_like(current_q) / batch_size)
        act_grad = in_delta[:, -self.act_dim:]
        
        # Chain rule: backprop through actor
        self.actor.mlp.backward(act_grad)
        self.actor.optimizer.step()

        # 3. Polyak Target Update
        self._polyak_update(self.actor.mlp, self.target_actor.mlp)
        self._polyak_update(self.critic.mlp, self.target_critic.mlp)

    def _polyak_update(self, src_mlp, tgt_mlp):
        src_params = src_mlp.get_params()
        tgt_params = tgt_mlp.get_params()
        for s, t in zip(src_params, tgt_params):
            t[...] = self.tau * s + (1.0 - self.tau) * t
