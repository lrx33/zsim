set title 'Memory Idle Time (500 unit Processing per node)'
set grid
set key left top
set xlabel 'Total Program Time (Cycles)
set ylabel 'Total Memory Idle Time (Cycles)'

plot 'results/500proc/vanilla_graphetch_1000000.cfg.500proc.out.result.reduced' with lines title '1MilNodesVanilla', \
'results/500proc/graphetch_graphetch_1000000.cfg_500proc.out.result.reduced' with lines title '1MilNodesGraphetch', \
'results/500proc/vanilla_graphetch_600000.cfg.500proc.out.result.reduced' with lines title '600kNodesVanilla', \
'results/500proc/graphetch_graphetch_600000.cfg_500proc.out.result.reduced' with lines title '600kNodesGraphetch',\
'results/500proc/vanilla_graphetch_400000.cfg.500proc.out.result.reduced' with lines title '400kNodesVanilla', \
'results/500proc/graphetch_graphetch_400000.cfg_500proc.out.result.reduced' with lines title '400kNodesGraphetch'
