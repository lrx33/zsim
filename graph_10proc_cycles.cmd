set title 'Program Execution Time with different graphs (10 unit Processing)'
set grid
set key left top
set ylabel 'Execution Time (Cycles)'
set xlabel 'Number of graph nodes'

plot 'results/10proc/cycles.result.graphetch' with linespoints title 'Graphetch', \
'results/10proc/cycles.result.vanilla' with linespoints title 'Vanilla'
