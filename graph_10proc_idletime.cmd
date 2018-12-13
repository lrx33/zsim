set title '10 unit Processing on each node'
set grid
set key left top
set xlabel 'Total Program Time (Cycles)'
set ylabel 'Total Memory Idle Time (Cycles)'

plot 'results/10proc/vanilla_graphetch_1000000.cfg.10proc.out.result.reduced' with lines title '1MilNodesVanilla', \
'results/10proc/graphetch_graphetch_1000000.cfg_10proc.out.result.reduced' with lines title '1MilNodesGraphetch', \
'results/10proc/vanilla_graphetch_600000.cfg.10proc.out.result.reduced' with lines title '600kNodesVanilla', \
'results/10proc/graphetch_graphetch_600000.cfg_10proc.out.result.reduced' with lines title '600kNodesGraphetch',\
'results/10proc/vanilla_graphetch_400000.cfg.10proc.out.result.reduced' with lines title '400kNodesVanilla', \
'results/10proc/graphetch_graphetch_400000.cfg_10proc.out.result.reduced' with lines title '400kNodesGraphetch'
