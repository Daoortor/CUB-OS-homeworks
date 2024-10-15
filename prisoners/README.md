# Times measured
```shell
[whalle@spitsbergen cmake-build-debug]$ ./prisoners -n 2000
random_global      0/2000 wins/games: 0.00% 4722.789/2000 ms = 2.361 ms
random_drawer      0/2000 wins/games: 0.00% 4872.116/2000 ms = 2.436 ms
smart_global      627/2000 wins/games: 31.35% 4633.810/2000 ms = 2.317 ms
smart_drawer      622/2000 wins/games: 31.10% 4827.000/2000 ms = 2.413 ms
```
# Observations
As expected, the random strategy didn't result in any wins as the probability of victory 
is $2^{-100}$. The smart strategy resulted in about $31$% of wins, which fits the mathematically
computed probability of victory $1-H_{100}+H_{50} \approx 0.31183$. \
As of the execution time, there appears to be no significant difference between the $4$
strategies. The "global lock" strategy saves time by performing less lock/unlock operations
(which are expensive), but results in less concurrency (only one thread can access drawers at a time).
These effects add up and result in roughly the same execution time.