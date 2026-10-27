"""
Twin Delayed Deep Deterministic Policy Gradient (TD3).
State-of-the-art continuous control addressing overestimation bias via:
- Clipped Double Q-Learning (twin critics)
- Target Policy Smoothing Regularization
- Delayed Policy Updates
"""

import numpy as np
import copy
from neural_core import DeterministicPolicy, QFunction, ReplayBuffer

class TD3Agent:
    def __init__(self, obs_dim=20, act_dim=1, gamma=0.99, tau=0.005,
                 policy_noise=0.2, noise_clip=0.5, policy_delay=2,
                 lr_actor=1e-3, lr_critic=1e-3, buffer_size=50000):
        self.gamma = gamma
        self.tau = tau
        self.policy_noise = policy_noise
        self.noise_clip = noise_clip
        self.policy_delay = policy_delay
        self.act_dim = act_dim
        self.total_it = 0

        # Actor
        self.actor = DeterministicPolicy(obs_dim, act_dim, hidden_dims=[64, 64], lr=lr_actor)
        self.target_actor = copy.deepcopy(self.actor)

        # Twin Critics
        self.critic1 = QFunction(obs_dim, act_dim, hidden_dims=[64, 64], lr=lr_critic)
        self.critic2 = QFunction(obs_dim, act_dim, hidden_dims=[64, 64], lr=lr_critic)
        self.target_critic1 = copy.deepcopy(self.critic1)
        self.target_critic2 = copy.deepcopy(self.critic2)

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

        self.total_it += 1
        obs, acts, rews, next_obs, dones = self.replay_buffer.sample(batch_size)

        # 1. Target Policy Smoothing
        noise = np.random.randn(*acts.shape).astype(np.float32) * self.policy_noise
        noise = np.clip(noise, -self.noise_clip, self.noise_clip)
        target_acts = np.clip(self.target_actor.forward(next_obs) + noise, -1.0, 1.0)

        # Clipped Double-Q target
        target_q1 = self.target_critic1.forward(next_obs, target_acts)
        target_q2 = self.target_critic2.forward(next_obs, target_acts)
        target_q = np.minimum(target_q1, target_q2)
        y = rews + self.gamma * (1.0 - dones) * target_q

        # 2. Update Twin Critics
        self.critic1.zero_grad()
        q1 = self.critic1.forward(obs, acts)
        q1_loss_grad = (q1 - y) / batch_size
        self.critic1.mlp.backward(q1_loss_grad)
        self.critic1.optimizer.step()

        self.critic2.zero_grad()
        q2 = self.critic2.forward(obs, acts)
        q2_loss_grad = (q2 - y) / batch_size
        self.critic2.mlp.backward(q2_loss_grad)
        self.critic2.optimizer.step()

        # 3. Delayed Policy Update
        if self.total_it % self.policy_delay == 0:
            self.actor.zero_grad()
            pred_acts = self.actor.forward(obs)

            # d(-Q1)/d_act
            self.critic1.zero_grad()
            self.critic1.forward(obs, pred_acts)
            in_delta = self.critic1.mlp.backward(-np.ones_like(q1) / batch_size)
            act_grad = in_delta[:, -self.act_dim:]

            self.actor.mlp.backward(act_grad)
            self.actor.optimizer.step()

            # Soft updates
            self._polyak_update(self.actor.mlp, self.target_actor.mlp)
            self._polyak_update(self.critic1.mlp, self.target_critic1.mlp)
            self._polyak_update(self.critic2.mlp, self.target_critic2.mlp)

    def _polyak_update(self, src_mlp, tgt_mlp):
        src_params = src_mlp.get_params()
        tgt_params = tgt_mlp.get_params()
        for s, t in zip(src_params, tgt_params):
            t[...] = self.tau * s + (1.0 - self.tau) * t
