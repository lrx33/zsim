set title 'Program Execution Time with different graphs (500 unit Processing)'
set grid
set key left top
set ylabel 'Execution Time'
set xlabel 'Number of graph nodes'

plot 'results/500proc/cycles.result.graphetch' with linespoints title 'Graphetch', \
'results/500proc/cycles.result.vanilla' with linespoints title 'Vanilla'
