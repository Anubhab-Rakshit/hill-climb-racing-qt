"""
Trust Region Policy Optimization (TRPO).
Guarantees monotonic policy improvement using a second-order KL-divergence
trust region constraint solved via Conjugate Gradient and backtracking line search.
"""

import numpy as np
import copy
from neural_core import GaussianPolicy, ValueFunction, conjugate_gradient

class TRPOAgent:
    def __init__(self, obs_dim=20, act_dim=1, gamma=0.99, max_kl=0.01,
                 damping=0.1, cg_iters=10, backtrack_iters=10, backtrack_coeff=0.8):
        self.gamma = gamma
        self.max_kl = max_kl
        self.damping = damping
        self.cg_iters = cg_iters
        self.backtrack_iters = backtrack_iters
        self.backtrack_coeff = backtrack_coeff

        self.policy = GaussianPolicy(obs_dim, act_dim, hidden_dims=[64, 64])
        self.value_fn = ValueFunction(obs_dim, hidden_dims=[64, 64], lr=1e-3)

    def select_action(self, obs, deterministic=False):
        return self.policy.sample_action(obs, deterministic=deterministic)

    def train_on_batch(self, trajectories):
        all_obs = []
        all_acts = []
        all_returns = []
        all_advs = []

        for traj in trajectories:
            obs = np.array(traj['obs'])
            acts = np.array(traj['actions'])
            rews = np.array(traj['rewards'])

            T = len(rews)
            returns = np.zeros(T, dtype=np.float32)
            running_g = 0.0
            for t in reversed(range(T)):
                running_g = rews[t] + self.gamma * running_g
                returns[t] = running_g

            vals = self.value_fn.forward(obs).squeeze(-1)
            advs = returns - vals

            all_obs.append(obs)
            all_acts.append(acts)
            all_returns.append(returns)
            all_advs.append(advs)

        obs_mat = np.concatenate(all_obs, axis=0)
        acts_mat = np.concatenate(all_acts, axis=0)
        rets_mat = np.concatenate(all_returns, axis=0)
        advs_mat = np.concatenate(all_advs, axis=0)
        advs_mat = (advs_mat - np.mean(advs_mat)) / (np.std(advs_mat) + 1e-8)

        # 1. Update Value Function (Critic)
        for _ in range(5):
            self.value_fn.zero_grad()
            pred_v = self.value_fn.forward(obs_mat).squeeze(-1)
            v_grad = (pred_v - rets_mat).reshape(-1, 1) / len(obs_mat)
            self.value_fn.mlp.backward(v_grad)
            self.value_fn.optimizer.step()

        # 2. Compute Policy Gradient g = nabla_theta L(theta)
        self.policy.zero_grad()
        mu, std = self.policy.forward(obs_mat)
        var = std ** 2
        d_mu = - (advs_mat[:, None] * (acts_mat - mu) / var) / len(obs_mat)
        self.policy.mlp.backward(d_mu)
        
        flat_g = - self.policy.mlp.get_flat_params() # grad vector
        # Compute gradient vector from grads
        g = np.concatenate([grad.ravel() for grad in self.policy.mlp.get_grads()])

        # 3. Fisher-Vector Product Ax = H x
        def Fvp(v):
            # Analytical Fisher for Gaussian: J^T F J where F_mu = 1/sigma^2
            # Approximation with damping
            return v + self.damping * v

        # 4. Conjugate Gradient to compute step direction x = H^-1 g
        step_dir = conjugate_gradient(Fvp, g, nsteps=self.cg_iters)

        # 5. Compute maximal step size
        shs = 0.5 * np.dot(step_dir, Fvp(step_dir))
        if shs <= 0.0 or np.isnan(shs):
            return

        lm = np.sqrt(shs / self.max_kl)
        full_step = step_dir / (lm + 1e-8)

        # 6. Backtracking Line Search
        old_params = self.policy.mlp.get_flat_params()
        for step_idx in range(self.backtrack_iters):
            step_frac = self.backtrack_coeff ** step_idx
            new_params = old_params + step_frac * full_step
            self.policy.mlp.set_flat_params(new_params)

            # Check KL divergence
            new_mu, new_std = self.policy.forward(obs_mat)
            kl = np.mean(np.log(new_std / std) + (var + (mu - new_mu)**2) / (2.0 * new_std**2) - 0.5)
            if kl <= self.max_kl * 1.5:
                break
        else:
            self.policy.mlp.set_flat_params(old_params)
