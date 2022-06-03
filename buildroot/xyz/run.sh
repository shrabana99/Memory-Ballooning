#!/bin/sh

if [ $# -ne 1 ]; then
    echo "Script needs 1 argument : path to executable to be run"
    exit
fi

USER_PROCESS_BINARY=$1
USER_PROCESS_OUTILE="out_main.txt"
TARGET_FREE_MEMORY=850

cleanup()
{
    echo "Killing user program"
    kill -9 $pid_program
}

allowAbort=true;
myInterruptHandler()
{
    if $allowAbort; then
        echo "SIGINT received, cleaning up..."
        cleanup
        exit 1;
    fi;
}

trap myInterruptHandler 2;

wrapInterruptable()
{
    # disable the abortability of the script
    allowAbort=false;
    # run the passed arguments 1:1
    "$@";
    # save the returned value
    local ret=$?;
    # make the script abortable again
    allowAbort=true;
    # and return
    return "$ret";
}

timestamp=$(date +"%m%d-%H:%M:%S")
echo "------------------------------" >> $USER_PROCESS_OUTILE
echo "run started at $timestamp" >> $USER_PROCESS_OUTILE
$USER_PROCESS_BINARY >> $USER_PROCESS_OUTILE 2>&1 &
pid_program=$!
echo "pid of user process is : $pid_program"



loop_ctr=1
while [ 1 -eq 1 ]; do
    echo ""
    echo "--------------- Iteration $loop_ctr -----------------------"
    MEM_FREE_MB=$(free -m | grep Mem | tr -s ' ' | cut -d' ' -f4)
    echo "Current free memory is $MEM_FREE_MB MB"
    echo "Press 'y' to continue iteration, 'n' to stop"
    read input_key

    if [ "$input_key" = "n" ]; then
        echo "Terminating execution!"
        break
    else
        MEM_FREE_MB=$(free -m | grep Mem | tr -s ' ' | cut -d' ' -f4)
        MEM_TO_ALLOCATE=$(echo "$MEM_FREE_MB-$TARGET_FREE_MEMORY" | bc)
        MEM_TO_ALLOCATE=$(( $MEM_TO_ALLOCATE < 0 ? 0 : $MEM_TO_ALLOCATE ))
        echo "Running memory eater with argument $MEM_TO_ALLOCATE MB"        

        ./mem_eater $MEM_TO_ALLOCATE > /dev/null &
        pid_mem_eater=$!
        wrapInterruptable ./watch_data.sh $pid_program
        kill -9 $pid_mem_eater
    fi

    loop_ctr=$(echo "$loop_ctr+1" | bc)
done

echo "Killing user program"
kill -9 $pid_program