set title 'Program Execution Time with different graphs (Zero Processing)'
set grid
set key left top
set ylabel 'Execution Time (Cycles)'
set xlabel 'Number of graph nodes'

plot 'results/noproc/cycles.result.graphetch' with linespoints title 'Graphetch', \
'results/noproc/cycles.result.vanilla' with linespoints title 'Vanilla'
