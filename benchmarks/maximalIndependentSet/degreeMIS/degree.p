o = 'pdfs/'
filename = 'rand_1000000_200_degrees.txt'
set terminal pdf
set output o.filename.".pdf"
set termoption dashed
set logscale xy
#set yrange [0.01:1000]
set title filename
set xlabel "Rank"
set ylabel "Degree"
plot filename u 1:2 t "degree" with dots
