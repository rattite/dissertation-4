rm data/synth.sqlite
rm data/synthi.sqlite
bin/getpoints2 67
bin/getpoints data/synth.sqlite synth cent data/synth.dat
bin/getpoints data/synthi.sqlite synthi cent data/synthi.dat
python cluster.py data/synth.dat
python cluster.py data/synthi.dat
