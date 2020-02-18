#!/bin/bash

usage() {
  echo "makeProductionDir.sh [Dirname]"
  echo "Creating a directory for managing job submission"
  exit 1
}

LINK="ln --symbolic"
TODIR=""

if [[ $# -eq 1 ]]; then
  TODIR=$1
else
  usage
fi

mkdir -p $TODIR
mkdir -p $TODIR/input
$LINK ${CMSSW_BASE}/src/Condor/Production/scripts/jobExecCondor.jdl $TODIR
$LINK ${CMSSW_BASE}/src/Condor/Production/scripts/jobExecCondor.sh  $TODIR
$LINK ${CMSSW_BASE}/src/Condor/Production/scripts/checkVomsTar.sh   $TODIR
$LINK ${CMSSW_BASE}/src/Condor/Production/scripts/step1.sh          $TODIR
$LINK ${CMSSW_BASE}/src/EMJ/Production/scripts/step2.sh             $TODIR
$LINK ${CMSSW_BASE}/src/EMJ/Production/scripts/submitJobs.py        $TODIR
