rm data/synth0.sqlite
rm data/synth1.sqlite
rm data/synth2.sqlite
rm data/synth0_30000.sqlite
rm data/synth1_30000.sqlite
rm data/synth2_30000.sqlite
bin/getpoints2 67
bin/getpoints data/synth0.sqlite synth0 cent data/synth0.dat
bin/getpoints data/synth1.sqlite synth1 cent data/synth1.dat
bin/getpoints data/synth2.sqlite synth2 cent data/synth2.dat
python cluster.py data/synth0.dat
python cluster.py data/synth1.dat
python cluster.py data/synth2.dat
bin/sample2 data/synth0.sqlite synth0 cent 30000
bin/sample2 data/synth1.sqlite synth1 cent 30000
bin/sample2 data/synth2.sqlite synth2 cent 30000
cp data/synth0.lizard data/synth0_30000.lizard
cp data/synth1.lizard data/synth1_30000.lizard
cp data/synth2.lizard data/synth2_30000.lizard

