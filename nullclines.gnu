set terminal 'wxt'
set xlabel 'Número de Soldados - E(t)'
set ylabel 'Número de Inimigos - I(t)'
set zeroaxis
set xrange [-50:550]
set yrange [-50:550]

set title 'Nullclines' font 'Arial,15'

plot 'nullclines.dat' using 1:2:3:4 with vectors filled head lw 3 title 'I-Nullcline', \
     'nullclines.dat' using 2:1:4:3 with vectors filled head lw 3 title 'E-Nullcline'

set output 'report/figs/nullclines.png'
set terminal pngcairo enhanced font 'Arial,10' fontscale 1.0
replot


