"""
Proximal Policy Optimization (PPO).
Features Generalized Advantage Estimation (GAE) and clipped surrogate objective.
"""

import numpy as np
from neural_core import GaussianPolicy, ValueFunction

class PPOAgent:
    def __init__(self, obs_dim=20, act_dim=1, gamma=0.99, lam=0.95, clip_ratio=0.2,
                 lr_pi=3e-4, lr_v=1e-3, ppo_epochs=6):
        self.gamma = gamma
        self.lam = lam
        self.clip_ratio = clip_ratio
        self.ppo_epochs = ppo_epochs
        
        self.policy = GaussianPolicy(obs_dim, act_dim, hidden_dims=[64, 64], lr=lr_pi)
        self.value_fn = ValueFunction(obs_dim, hidden_dims=[64, 64], lr=lr_v)

    def select_action(self, obs, deterministic=False):
        return self.policy.sample_action(obs, deterministic=deterministic)

    def compute_gae(self, rewards, values, dones):
        T = len(rewards)
        advantages = np.zeros(T, dtype=np.float32)
        last_gae = 0.0
        for t in reversed(range(T)):
            next_val = values[t+1] if t < T - 1 else 0.0
            done = dones[t]
            delta = rewards[t] + self.gamma * next_val * (1.0 - done) - values[t]
            last_gae = delta + self.gamma * self.lam * (1.0 - done) * last_gae
            advantages[t] = last_gae
        returns = advantages + values
        return advantages, returns

    def train_on_batch(self, trajectories):
        all_obs = []
        all_acts = []
        all_returns = []
        all_advs = []
        all_old_logps = []

        for traj in trajectories:
            obs = np.array(traj['obs'])
            acts = np.array(traj['actions'])
            rews = np.array(traj['rewards'])
            dones = np.array(traj['dones'])

            vals = self.value_fn.forward(obs).squeeze(-1)
            advs, rets = self.compute_gae(rews, vals, dones)
            old_logps = self.policy.log_prob(obs, acts)

            all_obs.append(obs)
            all_acts.append(acts)
            all_returns.append(rets)
            all_advs.append(advs)
            all_old_logps.append(old_logps)

        obs_mat = np.concatenate(all_obs, axis=0)
        acts_mat = np.concatenate(all_acts, axis=0)
        rets_mat = np.concatenate(all_returns, axis=0)
        advs_mat = np.concatenate(all_advs, axis=0)
        old_logps_mat = np.concatenate(all_old_logps, axis=0)

        # Normalize advantages
        advs_mat = (advs_mat - np.mean(advs_mat)) / (np.std(advs_mat) + 1e-8)
        N = len(obs_mat)

        # Multi-epoch PPO updates
        for epoch in range(self.ppo_epochs):
            # Shuffle minibatch
            perm = np.random.permutation(N)
            batch_size = 64
            for s_idx in range(0, N, batch_size):
                b_idx = perm[s_idx:s_idx+batch_size]
                b_obs = obs_mat[b_idx]
                b_acts = acts_mat[b_idx]
                b_rets = rets_mat[b_idx]
                b_advs = advs_mat[b_idx]
                b_old_logp = old_logps_mat[b_idx]

                # 1. Value Network Update
                self.value_fn.zero_grad()
                pred_v = self.value_fn.forward(b_obs).squeeze(-1)
                v_grad = (pred_v - b_rets).reshape(-1, 1) / len(b_idx)
                self.value_fn.mlp.backward(v_grad)
                self.value_fn.optimizer.step()

                # 2. PPO Clipped Policy Update
                self.policy.zero_grad()
                mu, std = self.policy.forward(b_obs)
                var = std ** 2
                cur_logp = -0.5 * (((b_acts - mu)**2)/var + 2.0*self.policy.log_std + np.log(2.0*np.pi))
                cur_logp = np.sum(cur_logp, axis=-1)

                ratios = np.exp(cur_logp - b_old_logp)
                surr1 = ratios * b_advs
                surr2 = np.clip(ratios, 1.0 - self.clip_ratio, 1.0 + self.clip_ratio) * b_advs

                # Gradient mask: only backprop where surr1 <= surr2
                active = (surr1 <= surr2).astype(np.float32)
                # d(-surr)/dmu = - active * (ratios * adv) * d(cur_logp)/dmu
                d_logp_mu = (b_acts - mu) / var
                d_mu = - (active[:, None] * (ratios * b_advs)[:, None] * d_logp_mu) / len(b_idx)
                self.policy.mlp.backward(d_mu)

                # d_log_std
                d_logp_std = ((b_acts - mu)**2)/var - 1.0
                d_std = - np.mean(active[:, None] * (ratios * b_advs)[:, None] * d_logp_std, axis=0)
                self.policy.grad_log_std += d_std

                self.policy.optimizer.step()
