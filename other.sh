rm data/stops_30000.sqlite
rm data/nyc_30000.sqlite
bin/sample2 data/stops.sqlite stops cent 30000
bin/sample2 data/nyc.sqlite nyc cent 30000
cp data/stops.lizard data/stops_30000.lizard
cp data/nyc.lizard data/nyc_30000.lizard
