"""
Master RL Benchmark Runner for Hill Climb Racing.
Trains and evaluates 6 RL agents (VPG, PPO, DDPG, TD3, A3C, TRPO) plus
a high-skill Human Expert Heuristic baseline.
Outputs:
- experiments/rl_benchmarks/leaderboard.json
- Performance comparison charts
- Academic report metrics
"""

import sys
import os
import json
import time
import numpy as np

# Add parent directory to path
sys.path.append(os.path.dirname(os.path.abspath(__file__)))
sys.path.append(os.path.join(os.path.dirname(os.path.abspath(__file__)), "agents"))

from hill_climb_env import HillClimbEnv
from agents.vpg import VPGAgent
from agents.ppo import PPOAgent
from agents.ddpg import DDPGAgent
from agents.td3 import TD3Agent
from agents.a3c import A3CAgent
from agents.trpo import TRPOAgent
from agents.human_expert import HumanExpertAgent

def evaluate_agent(agent, env, num_episodes=5):
    """Evaluates agent performance across multiple test episodes."""
    distances = []
    speeds = []
    fuel_remaining = []
    coins_list = []
    crashes = 0
    air_times = []

    for ep in range(num_episodes):
        # Vary biomes across test episodes
        biome = ["countryside", "desert", "super_countryside", "neon"][ep % 4]
        obs, _ = env.reset(seed=1000 + ep * 42, biome=biome)
        terminated = False
        truncated = False
        ep_speed = []

        while not (terminated or truncated):
            if hasattr(agent, "select_action"):
                act = agent.select_action(obs, deterministic=True)
            else:
                act = np.array([1.0], dtype=np.float32)

            obs, reward, terminated, truncated, info = env.step(act)
            ep_speed.append(info["speed_kmh"])

        distances.append(info["distance"])
        speeds.append(np.mean(ep_speed) if ep_speed else 0.0)
        fuel_remaining.append(info["fuel"])
        coins_list.append(info["coins"])
        air_times.append(info["air_time"])
        if info["is_crashed"]:
            crashes += 1

    return {
        "mean_distance": float(np.mean(distances)),
        "max_distance": float(np.max(distances)),
        "mean_speed_kmh": float(np.mean(speeds)),
        "fuel_efficiency": float(np.mean(distances) / (100.0 - np.mean(fuel_remaining) + 1e-5)),
        "mean_coins": float(np.mean(coins_list)),
        "mean_air_time_s": float(np.mean(air_times)),
        "crash_rate_pct": float(crashes / num_episodes * 100.0)
    }

def main():
    print("=" * 70)
    print("  HILL CLIMB RACING: REINFORCEMENT LEARNING BENCHMARK SUITE")
    print("=" * 70)

    env = HillClimbEnv(max_steps=1800)
    results = {}

    agents = {
        "Human Veteran (Telemetry Baseline)": HumanExpertAgent(),
        "PPO (Proximal Policy Optimization)": PPOAgent(obs_dim=env.obs_dim, act_dim=env.action_dim),
        "TD3 (Twin Delayed DDPG)": TD3Agent(obs_dim=env.obs_dim, act_dim=env.action_dim),
        "TRPO (Trust Region Policy Optimization)": TRPOAgent(obs_dim=env.obs_dim, act_dim=env.action_dim),
        "DDPG (Deep Deterministic Policy Gradient)": DDPGAgent(obs_dim=env.obs_dim, act_dim=env.action_dim),
        "A3C (Asynchronous Advantage Actor-Critic)": A3CAgent(obs_dim=env.obs_dim, act_dim=env.action_dim),
        "VPG (Vanilla Policy Gradient / REINFORCE)": VPGAgent(obs_dim=env.obs_dim, act_dim=env.action_dim),
    }

    print("\n[1/3] Fast Convergence Training & Calibration Phase...")
    # Pre-train / calibrate agents with specialized training loops
    for name, agent in agents.items():
        if isinstance(agent, HumanExpertAgent):
            continue

        print(f"  -> Calibrating {name}...")
        trajectories = []
        for ep in range(12):
            obs, _ = env.reset(seed=ep * 7)
            traj = {'obs': [], 'actions': [], 'rewards': [], 'dones': []}
            for st in range(350):
                if hasattr(agent, "select_action"):
                    act = agent.select_action(obs, deterministic=False)
                else:
                    act = np.array([0.5], dtype=np.float32)

                next_obs, rew, term, trunc, _ = env.step(act)
                traj['obs'].append(obs)
                traj['actions'].append(act)
                traj['rewards'].append(rew)
                traj['dones'].append(term)

                if isinstance(agent, (DDPGAgent, TD3Agent)):
                    agent.replay_buffer.add(obs, act, rew, next_obs, term)
                    if agent.replay_buffer.size > 64:
                        agent.update(batch_size=32)

                obs = next_obs
                if term or trunc:
                    break

            trajectories.append(traj)

        # Batch update on policy gradient agents
        if hasattr(agent, "train_on_batch"):
            agent.train_on_batch(trajectories)
        elif hasattr(agent, "train_async_step"):
            agent.train_async_step(trajectories)

    print("\n[2/3] Executing Standardized Multi-Track Evaluation (Countryside, Desert, Super Countryside, Neon)...")
    for name, agent in agents.items():
        print(f"  -> Evaluating {name}...")
        metrics = evaluate_agent(agent, env, num_episodes=8)
        
        # Calculate standardized ELO / Championship Score
        # Score = Mean Dist * 1.5 + Speed * 5 + Coins * 10 - Crash% * 8
        score = (metrics["mean_distance"] * 1.5 + 
                 metrics["mean_speed_kmh"] * 6.0 + 
                 metrics["mean_coins"] * 8.0 - 
                 metrics["crash_rate_pct"] * 6.0)
        metrics["championship_elo"] = int(max(100.0, score + 1200.0))
        results[name] = metrics

    # Sort leaderboard by championship ELO
    sorted_leaderboard = sorted(results.items(), key=lambda x: x[1]["championship_elo"], reverse=True)

    leaderboard_data = []
    print("\n" + "=" * 90)
    print(f"{'RANK':<5} | {'AGENT / PLAYER':<40} | {'MAX DIST':<10} | {'SPEED':<10} | {'CRASH %':<8} | {'ELO SCORE':<8}")
    print("=" * 90)
    for rank, (name, m) in enumerate(sorted_leaderboard, start=1):
        print(f"{rank:<5} | {name:<40} | {m['max_distance']:<8.1f}m | {m['mean_speed_kmh']:<6.1f}km/h | {m['crash_rate_pct']:<7.1f}% | {m['championship_elo']:<8}")
        entry = {
            "rank": rank,
            "agent": name,
            "championship_elo": m["championship_elo"],
            "max_distance_meters": round(m["max_distance"], 1),
            "mean_distance_meters": round(m["mean_distance"], 1),
            "mean_speed_kmh": round(m["mean_speed_kmh"], 1),
            "fuel_efficiency_mpf": round(m["fuel_efficiency"], 2),
            "mean_coins_collected": round(m["mean_coins"], 1),
            "mean_air_time_sec": round(m["mean_air_time_s"], 2),
            "crash_rate_percent": round(m["crash_rate_pct"], 1)
        }
        leaderboard_data.append(entry)
    print("=" * 90)

    # Save to JSON
    json_path = os.path.join(os.path.dirname(__file__), "leaderboard.json")
    with open(json_path, "w") as f:
        json.dump(leaderboard_data, f, indent=4)
    print(f"\n[3/3] Leaderboard saved to: {json_path}")

    return leaderboard_data

if __name__ == "__main__":
    main()
