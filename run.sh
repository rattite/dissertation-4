counter=4

while true; do
    python run_tests.py large "$counter"
    sleep 10
    ((counter++))

    if [ "$counter" -gt 17 ]; then
        counter=4
    fi
done
