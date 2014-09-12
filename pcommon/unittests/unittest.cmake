
cdrun () (
  cd $(dirname $1)
  RUNTEST=./${1##*/}
  shift
  export PCOMN_TESTDIR=$(PCOMN_TESTDIR)
  echo flock -w 7200 $PCOMN_TESTDIR -c \"$RUNTEST $@\"
  )
