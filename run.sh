counter=6

while true; do
    python run_tests.py ran "$counter"
    sleep 10
    python run_tests.py gau "$counter"
    sleep 10
    python run_tests.py large "$counter"
    sleep 10
    ((counter++))
    if [ "$counter" -gt 17 ]; then
        counter=4
    fi
done
