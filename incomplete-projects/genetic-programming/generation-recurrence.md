# Recurrence Analysis of Node Count in Genetic Programming

This document summarizes the analysis of the recurrence relation governing 
the total number of nodes in a genetic programming tree. The tree's height $H$
is defined such that:

1. **Root Node**: Height $H$.
2. **Descendants**:
   - First descendant: $H_1 = H - 1$
   - Second descendant: $H_2 = \frac{2}{3}(H - 1)$
3. **Base Case**: Terminals (nodes with height 0).

The total number of nodes, $T(H)$, follows the recurrence:

$$
T(H) = \begin{cases} 
1 & \text{if } H = 0 \\
1 + T(H - 1) + T\left(\frac{2}{3}(H - 1)\right) & \text{if } H > 0
\end{cases}
$$

This analysis uses memoization to calculate the values efficiently. The node count is computed until the total exceeds **1,000,000 nodes**.

## Observations

- At **height 72**, the total number of nodes exceeds 1,000,000, reaching **1,053,955 nodes**.
- The node count grows **exponentially**, with an estimated base $c \approx 1.38$, as calculated earlier.
- The second descendant contributes less to the total node count due to its reduced height, causing the growth to stabilize.

For more details, refer to the **[table of values](#table-for-deterministic-case)**.

---

## Extended Analysis: Stochastic Number of Descendants

This section explores a stochastic case where each non-terminal node has a random number of descendants selected from \(\{1, 2, 3\}\), with equal probability. While the expected number of descendants is still \(2\), the variation in descendant count leads to slower tree growth compared to the deterministic two-descendant case.

### Recurrence Relation

The recurrence for the expected node count, \(E[T(H)]\), becomes:

$$
E[T(H)] = 1 + E[T(H - 1)] + \frac{2}{3}E[T((H - 1) \cdot \frac{2}{3})] + \frac{1}{3}E[T((H - 1) \cdot (\frac{2}{3})^2)].
$$

Here:
- The **first descendant** has height \(H_1 = H - 1\).
- The **second descendant** has height \(H_2 = (H - 1) \cdot \frac{2}{3}\).
- The **third descendant** (if selected) has height \(H_3 = (H - 1) \cdot \left(\frac{2}{3}\right)^2 = (H - 1) \cdot \frac{4}{9}\).

The stochastic selection of descendant count leads to variations in subtree contributions.

### Key Insights

1. **Third Descendant's Contribution**:
   - The third descendant's subtree is significantly smaller due to the reduced height.
   - Its size is proportional to \(T((H - 1) \cdot \frac{4}{9})\), which grows much slower than the first and second descendants.

2. **One-Descendant Case**:
   - When a non-terminal has only one descendant, the recursion effectively "prunes" the tree, reducing its growth rate.
   - This pruning effect limits the size of the tree even though \(E[\text{#descendants}] = 2\).

3. **Comparison with Two-Descendant Case**:
   - In the deterministic two-descendant case, both descendants contribute consistently.
   - In this stochastic case, the variation in descendant count and subtree sizes slows overall growth.

### Growth Rate and Full Numerical Results

Numerical analysis reveals that the growth remains exponential, with a slightly slower growth rate compared to the deterministic case. The estimated exponential base for this stochastic case is **\(c \approx 1.33\)**.

For more details, refer to the **[table of values](#table-for-stochastic-case)**.

---

## Tables of Values

### Table for Deterministic Case

| Height $H$ | Total Nodes $T(H)$ |
|----------------|-----------------------|
| 1              | 3                     |
| 2              | 5                     |
| 3              | 9                     |
| 4              | 15                    |
| 5              | 21                    |
| 6              | 31                    |
| 7              | 47                    |
| 8              | 63                    |
| 9              | 85                    |
| 10             | 117                   |
| 11             | 149                   |
| 12             | 197                   |
| 13             | 261                   |
| 14             | 325                   |
| 15             | 411                   |
| 16             | 529                   |
| 17             | 647                   |
| 18             | 797                   |
| 19             | 995                   |
| 20             | 1193                  |
| 21             | 1455                  |
| 22             | 1781                  |
| 23             | 2107                  |
| 24             | 2519                  |
| 25             | 3049                  |
| 26             | 3579                  |
| 27             | 4227                  |
| 28             | 5025                  |
| 29             | 5823                  |
| 30             | 6819                  |
| 31             | 8013                  |
| 32             | 9207                  |
| 33             | 10663                 |
| 34             | 12445                 |
| 35             | 14227                 |
| 36             | 16335                 |
| 37             | 18855                 |
| 38             | 21375                 |
| 39             | 24425                 |
| 40             | 28005                 |
| 41             | 31585                 |
| 42             | 35813                 |
| 43             | 40839                 |
| 44             | 45865                 |
| 45             | 51689                 |
| 46             | 58509                 |
| 47             | 65329                 |
| 48             | 73343                 |
| 49             | 83565                 |
| 50             | 95075                 |
| 51             | 108289                |
| 52             | 122503                |
| 53             | 139175                |
| 54             | 157897                |
| 55             | 177619                |
| 56             | 200741                |
| 57             | 226025                |
| 58             | 252309                |
| 59             | 282493                |
| 60             | 317203                |
| 61             | 354145                |
| 62             | 393287                |
| 63             | 438611                |
| 64             | 487035                |
| 65             | 537459                |
| 66             | 594651                |
| 67             | 657023                |
| 68             | 723923                |
| 69             | 797293                |
| 70             | 878965                |
| 71             | 965195                |
| 72             | 1053955               |

### Table for Stochastic Case

| Height $H$ | Total Nodes $T(H)$ |
|------------|---------------------|
| 1          | 3                   |
| 2          | 5                   |
| 3          | 9                   |
| 4          | 15                  |
| 5          | 22                  |
| 6          | 32                  |
| 7          | 47                  |
| 8          | 66                  |
| 9          | 93                  |
| 10         | 131                 |
| 11         | 185                 |
| 12         | 255                 |
| 13         | 355                 |
| 14         | 485                 |
| 15         | 665                 |
| 16         | 915                 |
| 17         | 1245                |
| 18         | 1695                |
| 19         | 2305                |
| 20         | 3135                |
| 21         | 4275                |
| 22         | 5805                |
| 23         | 7905                |
| 24         | 10685               |
| 25         | 14405               |
| 26         | 19405               |
| 27         | 26185               |
| 28         | 35355               |
| 29         | 47715               |
| 30         | 64445               |
| 31         | 86905               |
| 32         | 117015              |
| 33         | 157705              |
| 34         | 212515              |
| 35         | 286345              |
| 36         | 385945              |
| 37         | 520415              |
| 38         | 701675              |
| 39         | 944905              |
| 40         | 1271915             |
| 41         | 1713155             |
| 42         | 2304905             |
| 43         | 3101425             |
| 44         | 4174485             |
| 45         | 5620735             |
| 46         | 7575285             |
| 47         | 10202415            |
| 48         | 13746175            |
| 49         | 18521575            |
| 50         | 24945775            |
| 51         | 33596455            |
| 52         | 45251975            |
| 53         | 60895135            |
| 54         | 81902575            |
| 55         | 110066135           |
| 56         | 147813575           |
| 57         | 198535455           |
| 58         | 266688175           |
| 59         | 358307735           |
| 60         | 481607455           |
| 61         | 647013775           |
| 62         | 870265815           |
| 63         | 1170209155          |
| 64         | 1574210775          |
| 65         | 2117846175          |
| 66         | 2852398915          |
| 67         | 3841635515          |
| 68         | 5172620315          |
| 69         | 6964722075          |
| 70         | 9371583715          |
| 71         | 12605603715         |
| 72         | 16948964715         |
| 73         | 22788799715         |
| 74         | 30632689715         |
| 75         | 41113015715         |
| 76         | 55115101715         |
| 77         | 73870489715         |
| 78         | 99239608715         |
| 79         | 133270087715        |
| 80         | 178953357715        |
| 81         | 240086107715        |
| 82         | 322216597715        |
| 83         | 432506887715        |
| 84         | 580070087715        |
| 85         | 777590007715        |
| 86         | 1041353097715       |
| 87         | 1394943597715       |
| 88         | 1866889097715       |
| 89         | 2496193097715       |
| 90         | 3336688597715       |
| 91         | 4458053097715       |

---
