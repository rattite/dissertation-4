while true; do
	python run_tests.py nyc_30000 nyc 8
	sleep 10
	python run_tests.py stops_30000 stops 8
	sleep 10
	python run_tests.py synth_0_30000 synth_0 8
	sleep 10
	python run_tests.py synth_1_30000 synth_1 8
	sleep 10
	python run_tests.py synth_2_30000 synth_2 8
done
