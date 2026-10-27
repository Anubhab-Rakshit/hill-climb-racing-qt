"""
Vanilla Policy Gradient (VPG / REINFORCE with baseline).
Standard Monte Carlo policy gradient algorithm with learned value function baseline.
"""

import numpy as np
from neural_core import GaussianPolicy, ValueFunction

class VPGAgent:
    def __init__(self, obs_dim=20, act_dim=1, gamma=0.99, lr_pi=3e-4, lr_v=1e-3):
        self.gamma = gamma
        self.policy = GaussianPolicy(obs_dim, act_dim, hidden_dims=[64, 64], lr=lr_pi)
        self.value_fn = ValueFunction(obs_dim, hidden_dims=[64, 64], lr=lr_v)

    def select_action(self, obs, deterministic=False):
        return self.policy.sample_action(obs, deterministic=deterministic)

    def train_on_batch(self, trajectories):
        """
        trajectories: list of dicts with 'obs', 'actions', 'rewards', 'dones'
        """
        all_obs = []
        all_acts = []
        all_returns = []
        all_advantages = []

        for traj in trajectories:
            obs = np.array(traj['obs'])
            acts = np.array(traj['actions'])
            rews = np.array(traj['rewards'])

            # Compute discounted returns G_t
            T = len(rews)
            returns = np.zeros(T, dtype=np.float32)
            running_g = 0.0
            for t in reversed(range(T)):
                running_g = rews[t] + self.gamma * running_g
                returns[t] = running_g

            # Compute baseline values and advantages
            values = self.value_fn.forward(obs).squeeze(-1)
            advs = returns - values

            all_obs.append(obs)
            all_acts.append(acts)
            all_returns.append(returns)
            all_advantages.append(advs)

        all_obs = np.concatenate(all_obs, axis=0)
        all_acts = np.concatenate(all_acts, axis=0)
        all_returns = np.concatenate(all_returns, axis=0)
        all_advantages = np.concatenate(all_advantages, axis=0)

        # Standardize advantages
        all_advantages = (all_advantages - np.mean(all_advantages)) / (np.std(all_advantages) + 1e-8)

        # 1. Update Value Function (MSE)
        self.value_fn.zero_grad()
        pred_v = self.value_fn.forward(all_obs).squeeze(-1)
        v_loss_grad = (pred_v - all_returns).reshape(-1, 1) / len(all_obs)
        self.value_fn.mlp.backward(v_loss_grad)
        self.value_fn.optimizer.step()

        # 2. Update Policy (REINFORCE with advantage)
        self.policy.zero_grad()
        mu, std = self.policy.forward(all_obs)
        var = std ** 2

        # Gradient of -log_prob * advantage w.r.t mu
        # d(-logp * A)/dmu = - A * (act - mu) / var
        d_mu = - (all_advantages[:, None] * (all_acts - mu) / var) / len(all_obs)
        self.policy.mlp.backward(d_mu)

        # Gradient w.r.t log_std
        d_log_std = - np.mean(all_advantages[:, None] * (((all_acts - mu)**2)/var - 1.0), axis=0)
        self.policy.grad_log_std += d_log_std

        self.policy.optimizer.step()
