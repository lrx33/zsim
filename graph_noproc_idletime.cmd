set title 'Zero Processing on nodes'
set grid
set key left top
set xlabel 'Total Program Time'
set ylabel 'Total Memory Idle Time'

plot 'results/noproc/vanilla_graphetch_1000000.cfg.out.result.reduced' with lines title '1MilNodesVanilla', \
'results/noproc/graphetch_graphetch_1000000.cfg.out.result.reduced' with lines title '1MilNodesGraphetch', \
'results/noproc/vanilla_graphetch_600000.cfg.out.result.reduced' with lines title '600kNodesVanilla', \
'results/noproc/graphetch_graphetch_600000.cfg.out.result.reduced' with lines title '600kNodesGraphetch',\
'results/noproc/vanilla_graphetch_400000.cfg.out.result.reduced' with lines title '400kNodesVanilla', \
'results/noproc/graphetch_graphetch_400000.cfg.out.result.reduced' with lines title '400kNodesGraphetch'


# 'results/100proc/vanilla_graphetch_600000.cfg.100proc.out.result.reduced' with lines title '100procVanilla600000', \
# 'results/100proc/vanilla_graphetch_400000.cfg.100proc.out.result.reduced' with lines title '100procVanilla400000', \
# 'results/100proc/vanilla_graphetch_200000.cfg.100proc.out.result.reduced' with lines title '100procVanilla200000'
# 
# plot 'results/100proc/vanilla_graphetch_80000.cfg.100proc.out.result.reduced' with lines title '100procVanilla80000'
# 'results/100proc/vanilla_graphetch_20000.cfg.100proc.out.result.reduced' with lines title '100procVanilla20000', \
# 'results/100proc/vanilla_graphetch_10000.cfg.100proc.out.result.reduced' with lines title '100procVanilla10000', \
# 'results/100proc/vanilla_graphetch_5000.cfg.100proc.out.result.reduced' with lines title '100procVanilla5000'
# 
# plot 'results/100proc/vanilla_graphetch_1000.cfg.100proc.out.result.reduced' with lines title '100procVanilla1000', \
# 'results/100proc/graphetch_graphetch_600000.cfg_100proc.out.result.reduced' with lines title '100procGraphetch600000', \
# 'results/100proc/graphetch_graphetch_400000.cfg_100proc.out.result.reduced' with lines title '100procGraphetch400000', \
# 'results/100proc/graphetch_graphetch_200000.cfg_100proc.out.result.reduced' with lines title '100procGraphetch200000', \
# 'results/100proc/graphetch_graphetch_80000.cfg_100proc.out.result.reduced' with lines title '100procGraphetch80000'
# 
# plot 'results/100proc/graphetch_graphetch_20000.cfg_100proc.out.result.reduced' with lines title '100procGraphetch20000', \
# 'results/100proc/graphetch_graphetch_10000.cfg_100proc.out.result.reduced' with lines title '100procGraphetch10000', \
# 'results/100proc/graphetch_graphetch_5000.cfg_100proc.out.result.reduced' with lines title '100procGraphetch5000', \
# 'results/100proc/graphetch_graphetch_1000.cfg_100proc.out.result.reduced' with lines title '100procGraphetch1000', \
