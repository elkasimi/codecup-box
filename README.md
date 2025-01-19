# CodeCup 2025 Entry: Box Game

This repository contains my entry for CodeCup 2025, featuring the game **Box**.

### Game Description:
You can find the rules and details of the game [here](https://www.codecup.nl/box/rules.php).

### Competition Performance:
My entry ranked **7th** out of 59 participants, making it to the top 10.

You can view the full competition rankings [here](https://www.codecup.nl/competition.php?comp=322).

---

### Algorithm Overview:
I implemented an **MCTS-UCT**-based algorithm for my player, augmented with several enhancements for improved performance.

#### 1. Game Tree:
- **Progressive Widening:**
  - Due to the high branching factor (~300 in the middle game), I employed progressive widening.
  - The number of expanded nodes for a given state is proportional to the square root of the state's visit count.

#### 2. Opponent Detection:
- I tracked the impact of opponent moves on the evaluation of all colors.
- Based on these evaluations, I computed weights that indicate the likelihood of a color belonging to the opponent.
- When the probability exceeds 0.66, I assign the opponent’s color accordingly, reducing the need to evaluate other colors during simulation payoffs.

#### 3. Dot Color Statistics:
- During each simulation, I maintained global statistics for setting a dot to a given color.
- Move estimation formula:
  
  **E(move) = Sum of value(dot_i, color_i) / 12**

#### 4. Progressive Bias:
- During selection, I used the dot color statistics as a bias.
- This bias progressively diminished as the number of visits increased.

#### 5. Variance-Dependent K:
- Formula used for node evaluation:
  
  **v = vi + K * sqrt(log(n) / (ni + 1)) + bias / (ni + 1)**

- **Dynamic K:**
  - Instead of a constant, the value of K was updated dynamically based on standard deviation, allocating more resources to nodes with higher variance.

#### 6. Warmup Simulations:
- To enhance progressive widening at the root state, I ran warmup simulations without tree selection.

#### 7. Consistency Checks:
- After reaching the maximum number of simulations, I verified that the most visited node matched the best-evaluated one.
- If discrepancies occurred, additional iterations were run until they aligned.

---

This approach allowed me to optimize performance in a complex game environment, achieving a strong placement in the competition.

