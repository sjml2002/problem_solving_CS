# MMKP (Multidimensional Multiple-choice Knapsack Problem)

## Problem Formulation

N개의 클래스(그룹)와 M개의 자원 차원이 주어졌을 때, MMKP는
**클래스마다 정확히 하나의 아이템**을 선택해 총 가치를 최대화하면서
M개의 자원 용량 제약을 만족시키는 문제입니다.

### Sets and Parameters
- $N$: 클래스 수
- $M$: 자원 차원 수
- $I_i$: 클래스 i의 아이템 수
- $V_ij$: 클래스 i의 아이템 j의 가치
- $W_ijm$: 클래스 i의 아이템 j가 자원 m에서 소모하는 가중치
- $Q_m$: 자원 m의 용량

### Decision Variable
$x_ij ∈ {0, 1}$ — 클래스 i에서 아이템 j를 선택하면 1, 아니면 0

### Objective Function
$Z = max(Σᵢ Σⱼ V_ij · x_ij)$

### Constraints
- 자원 제약(M개): $Σᵢ Σⱼ W_ijm · x_ij ≤ Q_m, ∀m$
- 다중선택 제약(N개): $Σⱼ x_ij = 1, ∀i$
- 이진 제약: $x_ij ∈ {0, 1}$

### Summary
MMKP는 NP-hard이며, "그룹당 1개 선택"이라는 다중선택 제약과
"여러 자원 동시 제약"이라는 다차원 배낭 제약이 결합된 문제입니다.


---
## References
- Luca Di Bello. _mmkp_. GitHub. 2023.\
[https://github.com/lucadibello/mmkp#24-input-data-format]



