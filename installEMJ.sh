#!/bin/bash

case $(uname) in
Linux) ECHO="echo -e >>> EMJ >>>" ;;
*) ECHO="echo >>> EMJ >>>" ;;
esac

# Global svariable for usage
CMSSW_RELEASE="CMSSW_10_2_21"
PYTHIA_VERSION="230"
PYTHIA_BRANCH="emg"
PYTHIA_SOURCE="kpedro88"

## Loading in a funny version of git
GIT=/usr/bin/git

function main() {
  parserArgument $@

  clonePackages

  installCMSSW
  installPythia
  installOthers

  $ECHO "Compiling..."
  scram b -j $(($(nproc) / 2))
}

function usage() {
  $ECHO "install.sh [options]"
  $ECHO
  $ECHO "Options:"
  $ECHO "-c [RELEASE]  \tCMSSW release to install (default ${CMSSW_RELEASE})"
  $ECHO "-s [fork]     \tPythia source to use (default = ${PYTHIA_SOURCE})"
  $ECHO "-m            \tFlag used for run installation"
}

function parserArgument() {
  while getopts "c:s:h" opt; do
    case "$opt" in
    c)
      CMSSW_RELEASE=$OPTARG
      ;;
    s)
      PYTHIA_SOURCE=$OPTARG
      ;;
    h)
      usage 0
      ;;
    esac
  done

  if [[ $CMSSW_RELEASE == "CMSSW_10_2_"* ]]; then
    export PYTHIA_VERSION="230"
  elif [[ $CMSSW_RELEASE == "CMSSW_9_3_"* ]]; then
    export PYTHIA_VERSION="230"
  elif [[ $CMSSW_RELEASE == "CMSSW_7_1_"* ]]; then
    export PYTHIA_VERSION="226"
  fi
}

function clonePackages() {
  case "$CMSSW_RELEASE" in
  CMSSW_10_2_*) # For 2018 GEN and RECO
    export SCRAM_ARCH="slc7_amd64_gcc700"
    ;;
  CMSSW_9_*) # For 2017 GEN and RECO
    export SCRAM_ARCH="slc7_amd64_gcc630"
    ;;
  CMSSW_8_0_*) # For 2016 RECO
    export SCRAM_ARCH="slc7_amd64_gcc530"
    ;;
  CMSSW_7_1_*) # For 2016 GEN
    export SCRAM_ARCH="slc6_amd64_gcc481"
    ;;
  *)
    $ECHO $CMSSW_RELEASE
    $ECHO "UNSUPPORTED CMSSW RELEASE! Exiting..."
    exit 1
    ;;
  esac

  source /cvmfs/cms.cern.ch/cmsset_default.sh
  scramv1 project CMSSW $CMSSW_RELEASE
  cd $CMSSW_RELEASE


  # Since there is a problem with git in certain CMSSW releases
  # We are getting all packages before initializing the CMSSW environment
  $ECHO "Getting pythia8 from $PYTHIA_SOURCE with branch ${PYTHIA_BRANCH}"
  $GIT clone "https://github.com/${PYTHIA_SOURCE}/pythia8.git" -b ${PYTHIA_BRANCH}/${PYTHIA_VERSION}
  CHECK_EXIT $? "FAILED TO GET PYTHIA"

  $ECHO "Getting all Condor scripts..."
  $GIT clone https://github.com/kpedro88/CondorProduction CondorProduction
  CHECK_EXIT $? "FAILED TO GET CONDOR"

  $ECHO "Getting EMJ core code..."
  $GIT clone ssh://git@gitlab.cern.ch:7999/cms-emj/emj-production.git EMJProduction
  CHECK_EXIT $? "FAILED TO GET EMJPRODUCTION"
}

function installCMSSW() {
  eval $(scramv1 runtime -sh)
  if [ -z $CMSSW_BASE ]; then
    $ECHO "Failed to setup CMSSW Evnrionment"
    exit 1
  else
    $ECHO "git cms-init $CMSSW_RELEASE..."
    cd ${CMSSW_BASE}/src
    ${GIT} cms-init >/dev/null 2>&1
    ## Must initialize the cmssw directory before anything else gets installed
  fi
}

function installPythia() {
  ## Skipping pythia installation for non-gen releases
  case "$CMSSW_RELEASE" in
  CMSSW_9_4_*) # For 2017 RECO
    return 0
    ;;
  CMSSW_8_0_*) # For 2016 RECO
    return 0
    ;;
  *) ;;
  esac

  ## Getting the same libraries as the CMSSW tool
  cd ${CMSSW_BASE}
  HEPMC_BASE=$(scram tool info hepmc | grep "HEPMC_BASE" | sed 's/HEPMC_BASE=//')
  BOOST_BASE=$(scram tool info boost | grep "BOOST_BASE" | sed 's/BOOST_BASE=//')
  LHAPDF_BASE=$(scram tool info lhapdf | grep "LHAPDF_BASE" | sed 's/LHAPDF_BASE=//')

  ## There is a strange issue with git/github for this particular directory...
  ## Getting and compiling the modified pythia stuff
  EXTRA_CONFIG=""
  if [[ $CMSSW_RELEASE == "CMSSW_7_1_"* ]] ; then
    EXTRA_CONFIG='--cxx-common="-std=c++11 -fPIC"'
  fi

  $ECHO "Compiling custom pythia8"
  cd ${CMSSW_BASE}/pythia8
  ./configure --enable-shared \
    --with-boost=${BOOST_BASE} \
    --with-hepmc2=${HEPMC_BASE} \
    --with-lhapdf6=${LHAPDF_BASE} \
    --with-lhapdf6-plugin=LHAPDF6.h "$EXTRA_CONFIG"

  make -j $(($(nproc) / 2)) >/dev/null 2>&1
  make install >/dev/null 2>&1
  cd ${CMSSW_BASE}

  ## Generating configuration file for CMSSW to use new pythia package
  cat <<'EOF_TOOLFILE' >${CMSSW_BASE}/config/toolbox/${SCRAM_ARCH}/tools/selected/pythia8.xml
<tool name="pythia8" version="${PYTHIA_VERSION}">
  <lib name="pythia8"/>
  <client>
    <environment name="PYTHIA8_BASE" default="$CMSSW_BASE/pythia8"/>
    <environment name="LIBDIR" default="$PYTHIA8_BASE/lib"/>
    <environment name="INCLUDE" default="$PYTHIA8_BASE/include"/>
  </client>
  <runtime name="PYTHIA8DATA" value="$PYTHIA8_BASE/share/Pythia8/xmldoc"/>
  <use name="hepmc"/>
  <use name="lhapdf"/>
</tool>
EOF_TOOLFILE

  $ECHO "Modifying CMSSW to use new pythia8.."
  scram setup pythia8 >/dev/null 2>&1

  # Generating dependencies package
  scram b echo_pythia8_USED_BY |
    tr ' ' '\n' |
    grep "self" |
    cut -d'/' -f2-3 |
    sort -u >${CMSSW_BASE}/pkgs.txt

  # update CMSSW dependencies
  cd $CMSSW_BASE/src
  git cms-addpkg -f ${CMSSW_BASE}/pkgs.txt
}

function installOthers() {
  cd $CMSSW_BASE/src
  mkdir Condor
  mkdir EMJ
  mv $CMSSW_BASE/CondorProduction Condor/Production
  mv $CMSSW_BASE/EMJProduction EMJ/Production
}

function CHECK_EXIT() {
  if [[ $1 -ne 0 ]]; then
    $ECHO $2
    exit $1
  fi
}

## Calling the main function
main $@
