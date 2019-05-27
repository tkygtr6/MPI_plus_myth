#printenv
#echo $(hostname) $PMI_RANK

echo $PMI_RANK: $(( $(($PMI_RANK % 18)) * 2)),$(($(($PMI_RANK % 18))*2+1))

export MYTH_NUM_WORKERS=2
taskset -c $(( $(($PMI_RANK % 18)) * 2)),$(($(($PMI_RANK % 18))*2+1))  ./miniFE.x nx=$1
