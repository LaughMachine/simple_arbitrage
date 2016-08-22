# Simple Arbitrage
This calculates simple arbitrage given different potential future scenarios. 

## Requirements:
- Gurobi license
- gurobipy package 
- Python 2.7

## Linear Program
We detect arbitrage by formulating a linear program and solving it utilizing gurobi. If V_S is the portfolio value in the future scenario S, then there is arbitrage if V_S > (1 + R_S)V_0, where V_0 is the current value of the portfolio. The linear program is as follows:

#### Objective:
maximize D_min
#### Constraints:
V_i = V_0*R_i + D_i

D_i >= D_min
#### Bounds:
-1 <= x_j <= 1, where x_j is the weight of the jth security in the portfolio V

## Description:
The code reads an input file similar to the one provided in this repository and generates the file containing the linear program that will be solved by Gurobi. The program checks if the objective is indicates arbitrage and returns the solution. 
