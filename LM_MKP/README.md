# 문제 설명
- MKP는 Multidimensional knapsack Problem의 약자입니다.
    - 이름에서 알 수 있다시피, knapsack problem(KP)의 일반적인 문제입니다.  
    기본 KP에서는 1개의 가방에 아이템을 넣는 경우의 수이지만,
    - MKP는 $m$개의 가방에 $n$개의 아이템을 넣습니다. 각 아이템은 가방에 따라 서로 다른 무게(가중치)를 가지고 있습니다. 이를 $a_{ij}$ 로 표현합니다.

---

# 수식 설명
- $m$개의 배낭이 있고 각 배낭의 용량은 $b_i$ 입니다. (즉 배낭의 index는 $i$입니다.)
    - 배낭들의 용량 벡터는 $B$ 로 표현합니다. ($B = (b_1, b_2, b_3, ..., b_n)$)
- $n$개의 아이템이 있고 배낭 $b_i$ 에 대한 아이템의 가중치는 $a_{ij}$ 입니다. (즉, 아이템의 index는 $j$입니다.)
    - 아이템들의 가중치는 2차원 벡터(행렬)로 표현할 수 있으며 이 행렬을 $A$ 로 표시합니다.
    - 각 아이템의 가치는 $c_j$ 입니다.
- $x$는 0 or 1의 값만을 가지는 길이 $n$의 벡터입니다.
    - $x$는 아이템을 할당할지 말지를 결정하는 보조 벡터입니다.

- 수식은 다음과 같습니다.
$$
    max(\sum^n_{j=1}c_jx_j) \,\,\, subject \,\, to \,\, \sum^n_{j=1}a_{ij}x_j ≤ b_i \newline
    (\,∀i(1≤i≤m)\,\,, \,\,x_j∈\{0, 1\}\,)
$$

---
# Lagrangian Multiplier Preliminaries
- 위 수식에서 -subject to 에 해당하는- 제약사항을 Lagrangian Multiplier를 사용해서 완화시킬 것입니다.
- Lagrangian Multiplier란, 목적함수 $f(x)$와 제약사항 $g(x)$ 가 있을 때, 적당한 $λ$를 찾아서 최종 값을 얻어냅니다. 수식으로 나타내자면 다음과 같이 됩니다.
    - $λ$는 $λ≥0$ 인 실수 벡터입니다.
$$ res = max(f(x) - λg(x)) $$
    
---
# 우리가 할 것
- 우리는 discrete domain에서 보고 있기 때문에, 위처럼 수식화할 수 없습니다.  
따라서 $max(\sum_{i} f(x_i) - λ_ig(x_i))$ 로 진행합니다.
- 이제 $λ$를 구하는 것이 핵심인데, 이를 Genetic Algorithm(GA)으로 적당한 $λ$를 구합니다.
    - 아이템이 n개 있을 때, 2^n을 전부보는 것이 아닌 n개를 봄으로서, 진행합니다.
    - 따라서 GA의 목적함수는 O(n)의 시간이 걸립니다.
        - 목적함수 구현방법에 따라 다르겠지만, 이 프로젝트에서는 n개를 보는 목적함수를 설계합니다.
- $λ$를 구한 뒤, 원문제(MKP)를 풀어서 최적해와 비교를 하는 것으로 실험이 종료됩니다.

---
## References
- Yourim Yoon, Yong-Hyuk Kim, Byung-Ro Moon,
A theoretical and empirical investigation on the Lagrangian capacities of the 0-1 multidimensional knapsack problem,
European Journal of Operational Research,
Volume 218, Issue 2,
2012,
Pages 366-376,
ISSN 0377-2217,
https://doi.org/10.1016/j.ejor.2011.11.011.
(https://www.sciencedirect.com/science/article/pii/S0377221711009982)

- Raidl, Günther R. "Weight-codings in a genetic algorithm for the multi-constraint knapsack problem." Proceedings of the 1999 Congress on Evolutionary Computation-CEC99 (Cat. No. 99TH8406). Vol. 1. IEEE, 1999.

- Test data : https://people.brunel.ac.uk/~mastjjb/jeb/orlib/mknapinfo.htmls