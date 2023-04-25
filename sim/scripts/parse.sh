dir=tmp/t && (( $# > 1 )) && dir=$2

rm $dir/*.dat
rm $dir/*.pdf
rm $dir/*.plot

./parse_output $1 -ascii | grep SINK | awk '{print $5;}' | sort | uniq > $dir/sinks.dat
./parse_output $1 -ascii | grep SINK | awk -f scripts/parse_bw.awk

echo "set term pdfcairo font 'Helvetica,18'" > $dir/plot_bw.plot
echo "set output 'bw.pdf'" >> $dir/plot_bw.plot
echo "set xlabel 'Time (s)'" >> $dir/plot_bw.plot
echo "set ylabel 'Throughput (Mbps)'" >> $dir/plot_bw.plot
echo "plot \c" >> $dir/plot_bw.plot

while read -r line;
do
    echo \"$dir/$line.dat\" "using (\$1):(\$13*8/1000000) w l notitle,\\" >> $dir/plot_bw.plot
done < $dir/sinks.dat

gnuplot $dir/plot_bw.plot
open bw.pdf

echo "set term pdfcairo font 'Helvetica,18'" > $dir/plot_cwnd.plot
echo "set output 'cwnd.pdf'" >> $dir/plot_cwnd.plot
echo "set xlabel 'Time (s)'" >> $dir/plot_cwnd.plot
echo "set ylabel 'CWND (MSS)'" >> $dir/plot_cwnd.plot
echo "plot \c" >> $dir/plot_cwnd.plot

while read -r line;
do
    echo \"$dir/$line.dat\" "using (\$1):(\$11/1500) w l notitle,\\" >> $dir/plot_cwnd.plot
done < $dir/sinks.dat

gnuplot $dir/plot_cwnd.plot
open cwnd.pdf

./parse_output $1 -ascii | grep QUEUE_APPROX | grep RANGE | tr "=" " " > $dir/q.dat
echo "set term pdfcairo font 'Helvetica,18'" > $dir/plot_q.plot
echo "set output 'q.pdf'" >> $dir/plot_q.plot
echo "set xlabel 'Time (s)'" >> $dir/plot_q.plot
echo "set ylabel 'Queuesize (MSS)'" >> $dir/plot_q.plot
echo "plot \c" >> $dir/plot_q.plot
echo \"$dir/q.dat\" "using (\$1):(\$13/1500) w l notitle,\\" >> $dir/plot_q.plot

gnuplot $dir/plot_q.plot
open q.pdf

paste $dir/0.dat $dir/1.dat > $dir/chiujain.dat

echo "set term pdfcairo font 'Helvetica,18'" > $dir/plot_chiujain.plot
echo "set output 'chiujain.pdf'" >> $dir/plot_chiujain.plot
echo "set size square" >> $dir/plot_chiujain.plot
echo "set grid" >> $dir/plot_chiujain.plot
echo "set xrange [0:500]" >> $dir/plot_chiujain.plot
echo "set yrange [0:500]" >> $dir/plot_chiujain.plot
echo "set grid" >> $dir/plot_chiujain.plot
echo "set xlabel 'CWND 1'" >> $dir/plot_chiujain.plot
echo "set ylabel 'CWND 2'" >> $dir/plot_chiujain.plot

echo "plot \"tmp/t/chiujain.dat\" using (\$11/1500):(\$24/1500) w dots notitle" >> $dir/plot_chiujain.plot

gnuplot $dir/plot_chiujain.plot
