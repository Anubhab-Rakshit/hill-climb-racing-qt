"""
Asynchronous Advantage Actor-Critic (A3C).
Features parallel asynchronous worker threads pushing gradients
to a shared global model with n-step bootstrapping and entropy regularization.
"""

import numpy as np
import copy
from neural_core import GaussianPolicy, ValueFunction

class A3CAgent:
    def __init__(self, obs_dim=20, act_dim=1, gamma=0.99, n_workers=4,
                 lr_pi=3e-4, lr_v=1e-3, entropy_coef=0.01):
        self.gamma = gamma
        self.n_workers = n_workers
        self.entropy_coef = entropy_coef

        # Global parameters
        self.global_policy = GaussianPolicy(obs_dim, act_dim, hidden_dims=[64, 64], lr=lr_pi)
        self.global_value = ValueFunction(obs_dim, hidden_dims=[64, 64], lr=lr_v)

    def select_action(self, obs, deterministic=False):
        return self.global_policy.sample_action(obs, deterministic=deterministic)

    def train_async_step(self, worker_trajectories):
        """
        Simulates asynchronous multi-worker updates where each worker collects
        trajectories under its local policy copy and applies async gradient steps.
        """
        for traj in worker_trajectories:
            obs = np.array(traj['obs'])
            acts = np.array(traj['actions'])
            rews = np.array(traj['rewards'])
            dones = np.array(traj['dones'])

            T = len(rews)
            # n-step returns
            values = self.global_value.forward(obs).squeeze(-1)
            returns = np.zeros(T, dtype=np.float32)
            running_r = 0.0 if dones[-1] else values[-1]
            for t in reversed(range(T)):
                running_r = rews[t] + self.gamma * running_r * (1.0 - dones[t])
                returns[t] = running_r

            advantages = returns - values

            # 1. Update Global Value Function
            self.global_value.zero_grad()
            pred_v = self.global_value.forward(obs).squeeze(-1)
            v_grad = (pred_v - returns).reshape(-1, 1) / T
            self.global_value.mlp.backward(v_grad)
            self.global_value.optimizer.step()

            # 2. Update Global Policy
            self.global_policy.zero_grad()
            mu, std = self.global_policy.forward(obs)
            var = std ** 2
            
            # Policy gradient + entropy regularization
            # Entropy H(pi) = 0.5 * log(2*pi*e*sigma^2)
            d_logp_mu = (acts - mu) / var
            d_mu = - (advantages[:, None] * d_logp_mu) / T
            self.global_policy.mlp.backward(d_mu)

            d_logp_std = ((acts - mu)**2) / var - 1.0
            # Entropy gradient w.r.t log_std is +1.0
            d_std = - np.mean(advantages[:, None] * d_logp_std, axis=0) - self.entropy_coef * 1.0
            self.global_policy.grad_log_std += d_std

            self.global_policy.optimizer.step()
