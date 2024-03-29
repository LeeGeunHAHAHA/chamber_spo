#!/bin/bash
FQA_TRUNK_DIR=~/fqa/trunk
PATH_FQA_TEST_SCRIPT=~/fqa/trunk/fqa_test_script
SPOR_SCRIPT_DIR=~/fqa/trunk/spor/2.spor_w_write

GREEN='\033[1;32m'
RED='\033[1;31m'
NC='\033[0m'

cd $FQA_TRUNK_DIR
source source.bash
./setup.sh

SESSION_NAME="IF_SPOR"

# if session already exist, then just attach to it
tmux has-session -t ${SESSION_NAME} &> /dev/null
if [ $? == 0 ]; then
    tmux attach -t ${SESSION_NAME}
    exit 0
fi


# create new session
tmux -2 new-session -s ${SESSION_NAME} -n SPOR -d

# split window to 4 panes
tmux split-window
tmux split-window
tmux split-window
tmux select-layout tiled

# run uart setup script
tmux select-pane -t 0
tmux send-keys "${SPOR_SCRIPT_DIR}/run_if 1 2>&1 | tee spor_if_1.log"
tmux select-pane -t 1
tmux send-keys "${SPOR_SCRIPT_DIR}/run_if 1 2>&1 | tee spor_if_1.log"
tmux select-pane -t 2
tmux send-keys "${SPOR_SCRIPT_DIR}/run_if 1 2>&1 | tee spor_if_1.log"
tmux select-pane -t 3
tmux send-keys "${SPOR_SCRIPT_DIR}/run_if 1 2>&1 | tee spor_if_1.log"

# go back to first window
tmux select-window -t 1
tmux select-pane -t 1

# attach to new session
tmux attach -t ${SESSION_NAME}
