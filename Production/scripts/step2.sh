#!/bin/bash

# Wrapper for running runEMJ.py scripts for condor

export JOBNAME=""
export PART=""
export OUTDIR=""
export REDIR=""
export OPTIND=1
while [[ $OPTIND -lt $# ]]; do
  # getopts in silent mode, don't exit on errors
  getopts ":j:p:o:x:" opt || status=$?
  case "$opt" in
  j)
    export JOBNAME=$OPTARG
    ;;
  p)
    export PART=$OPTARG
    ;;
  o)
    export OUTDIR=$OPTARG
    ;;
  x)
    export REDIR=$OPTARG
    ;;
  # keep going if getopts had an error
  \? | :)
    OPTIND=$((OPTIND + 1))
    ;;
  esac
done

echo "parameter set:"
echo "OUTDIR:     $OUTDIR"
echo "JOBNAME:    $JOBNAME"
echo "PART:       $PART"
echo "REDIR:      $REDIR"
echo ""

# link files from CMSSW dir
ln -sf ${CMSSWVER}/src/EMJ/Production/test/runEMJ.py

# run CMSSW
ARGS=$(cat args_${JOBNAME}.txt)
ARGS="$ARGS part=$PART"
if [[ -n "$REDIR" ]]; then
  ARGS="$ARGS redir=${REDIR}"
fi
echo "cmsRun ruEMJ.py ${ARGS} 2>&1"
cmsRun runEMJ.py ${ARGS} 2>&1

CMSEXIT=$?

# cleanup
rm runSVJ.py
if ls *.pkl >&/dev/null; then
  rm *.pkl
fi

if [[ $CMSEXIT -ne 0 ]]; then
  rm *.root
  echo "exit code $CMSEXIT, skipping xrdcp"
  exit $CMSEXIT
fi

## Disabling xrdcp for local compilation

# # check for gfal case
# CMDSTR="xrdcp"
# GFLAG=""
# if [[ "$OUTDIR" == "gsiftp://"* ]]; then
#   CMDSTR="gfal-copy"
#   GFLAG="-g"
# fi
#
# # copy output to eos
# echo "xrdcp output for condor"
# for FILE in *.root; do
#   echo "${CMDSTR} -f ${FILE} ${OUTDIR}/${FILE}"
#   stageOut ${GFLAG} -x "-f" -i ${FILE} -o ${OUTDIR}/${FILE} 2>&1
#   XRDEXIT=$?
#   if [[ $XRDEXIT -ne 0 ]]; then
#     rm *.root
#     echo "exit code $XRDEXIT, failure in ${CMDSTR}"
#     exit $XRDEXIT
#   fi
#   rm ${FILE}
# done