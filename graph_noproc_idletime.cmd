set title '0 unit processing on each node'
set grid
set key left top
set xlabel 'Total Program Time (Cycles)'
set ylabel 'Total Memory Idle Time (cycles)'

plot 'results/noproc/vanilla_graphetch_1000000.cfg.out.result.reduced' with lines title '1MilNodesVanilla', \
'results/noproc/graphetch_graphetch_1000000.cfg.out.result.reduced' with lines title '1MilNodesGraphetch', \
'results/noproc/vanilla_graphetch_600000.cfg.out.result.reduced' with lines title '600kNodesVanilla', \
'results/noproc/graphetch_graphetch_600000.cfg.out.result.reduced' with lines title '600kNodesGraphetch',\
'results/noproc/vanilla_graphetch_400000.cfg.out.result.reduced' with lines title '400kNodesVanilla', \
'results/noproc/graphetch_graphetch_400000.cfg.out.result.reduced' with lines title '400kNodesGraphetch'

