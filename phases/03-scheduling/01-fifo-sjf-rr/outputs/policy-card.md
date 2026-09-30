# Policy card

- FIFO: arrival order. Wins: simplicity. Pathology: convoy (tiny waits behind huge).
- SJF: shortest-available first. Wins: min avg waiting *with known bursts*. Pathology: starvation, needs omniscience.
- RR(q): slices. Wins: response/interactivity. Pathology: switch storm if q tiny; ~FIFO if q huge.
- Calibration (P1 0/8, P2 1/4, P3 2/2): FIFO turn 10.33/wait 5.67; SJF 9.67/5.00; RRq2 9.00/4.33.
