#!/bin/bash
source ../common
source ../configs/3200GB
#source configs/2850GB
source ../configs/400GB_precond
GROUP_NO=1

function skip_group()
{
  if [ ! -z ${GROUP_TO_RUN+x} ]; then
    if [ $GROUP_TO_RUN -ne $GROUP_NO ]; then
      return 0
    fi
    return 1
  fi
  return 1
}

function run_group()
{
  GROUP_NO=(${1//./ }[0])
  if skip_group; then
    return 0
  fi

  if [ ! -d $1 ]; then
    echo "test group '$1' not found"
    exit 1
  fi
  cd $1
  GROUP_NO=$GROUP_NO ./run_ns8 $GROUP_ARG
  if [ $? -ne 0 ]; then
    exit $?
  fi
  cd ..
}

function help()
{
  echo "block-level testing tool v0.7"
  echo "usage: $0 [options | [test group | [test case]]]"
  echo "option:   help    shows this screen"
  echo "          list    lists available test cases"
  echo "          clean   removes journal and test logs"
}
cd ..

if [ $# -gt 0 ]; then
  if [ "$1" == "help" ]; then
    help
    exit
  elif [ "$1" == "list" ]; then
    GROUP_ARG=list
  elif [ "$1" == "clean" ]; then
    for index in {1..8}
    do
      NBIO_NSID=$index ./run clean 
    done
    exit
  else
    if ! [[ $1 =~ ^[0-9]+$ ]]; then
      echo "not a number $1"
      help
      exit
    fi
    GROUP_TO_RUN=$1
    if [ $# -gt 1 ]; then
      GROUP_ARG=$2
    fi
  fi
fi
start_time=$(date +%s)


run_group "1.single_seq"
run_group "2.single"
run_group "3.multiple"
run_group "4.mixed"
run_group "5.mt_mixed"
#run_group "5.wr_dependency"
#run_group "6.format"
#run_group "7.hotspot_read"
#run_group "8.close_open"
#run_group "9.seq_access"

end_time=$(date +%s)
exe_time=`echo "$end_time - $start_time" | bc`
htime=`echo "$exe_time/3600" | bc`
mtime=`echo "($exe_time/60) - ($htime * 60)" | bc`
stime=`echo "$exe_time - (($exe_time/60)*60)" | bc`
echo -e "${COLOR}All workloads have done! [${htime}H ${mtime}M ${stime}S] ${NC}"
