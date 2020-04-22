#!/bin/bash

case $(uname) in
Linux) ECHO="echo -e >>> EMJ >>>" ;;
*) ECHO="echo >>> EMJ >>>" ;;
esac

# Global svariable for usage
CMSSW_RELEASE="CMSSW_10_2_21"
PYTHIA_BRANCH="emg/230"
PYTHIA_SOURCE="kpedro88"

function main() {
  parserArgument $@
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
  $ECHO "-f [fork]     \tPythia fork to use (default = ${PYTHIA_SOURCE})"
  $ECHO "-b [branch]   \tPythia branch to use (default = ${PYTHIA_BRANCH})"
}

function parserArgument() {
  while getopts "c:b:s:h" opt; do
    case "$opt" in
    c)
      CMSSW_RELEASE=$OPTARG
      ;;
    b)
      PYTHIA_BRANCH=$OPTARG
      ;;
    s)
      PYTHIA_SOURCE=$OPTARG
      ;;
    h)
      usage 0
      ;;
    esac
  done
}

function installCMSSW() {
  case "$CMSSW_RELEASE" in
  CMSSW_10_2_*)
    export SCRAM_ARCH="slc6_amd64_gcc700"
    ;;
  *)
    $ECHO "UNSUPPORTED CMSSW RELEASE! Exiting..."
    exit 1
    ;;
  esac

  source /cvmfs/cms.cern.ch/cmsset_default.sh
  scramv1 project CMSSW $CMSSW_RELEASE
  cd $CMSSW_RELEASE
  eval $(scramv1 runtime -sh)
  if [ -z $CMSSW_BASE ]; then
    $ECHO "Failed to setup CMSSW Evnrionment"
    exit 1
  else
    $ECHO "git cms-init $CMSSW_RELEASE..."
    cd ${CMSSW_BASE}/src
    git cms-init >/dev/null 2>&1
    ## Must initialize the cmssw directory before anything else gets installed
  fi
}

function installPythia() {
  ## Getting the same libraries as the CMSSW tool
  cd ${CMSSW_BASE}
  HEPMC_BASE=$(scram tool info hepmc | grep "HEPMC_BASE" | sed 's/HEPMC_BASE=//')
  BOOST_BASE=$(scram tool info boost | grep "BOOST_BASE" | sed 's/BOOST_BASE=//')
  LHAPDF_BASE=$(scram tool info lhapdf | grep "LHAPDF_BASE" | sed 's/LHAPDF_BASE=//')

  ## Getting and compiling the modified pythia stuff
  $ECHO "Getting pythia8"
  git clone "https://github.com/${PYTHIA_SOURCE}/pythia8.git" -b ${PYTHIA_BRANCH} > /dev/null 2>&1

  $ECHO "Compiling custom pythia8"
  cd ${CMSSW_BASE}/pythia8
  ./configure --enable-shared \
    --with-boost=${BOOST_BASE} \
    --with-hepmc2=${HEPMC_BASE} \
    --with-lhapdf6=${LHAPDF_BASE} \
    --with-lhapdf6-plugin=LHAPDF6.h

  make -j $(($(nproc) / 2)) > /dev/null 2>&1
  make install >/dev/null 2>&1
  cd ${CMSSW_BASE}

  ## Generating configuration file for CMSSW to use new pythia package
  cat <<'EOF_TOOLFILE' >${CMSSW_BASE}/config/toolbox/${SCRAM_ARCH}/tools/selected/pythia8.xml
<tool name="pythia8" version="230">
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
  scram setup pythia8 > /dev/null 2>&1

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

function installOthers(){
  EMJ_BRANCH="master"
  case "$CMSSW_RELEASE" in
  CMSSW_10_2_*)
    EMJ_BRANCH="master"
    ;;
  esac

  $ECHO "Getting all Condor scripts..."
  git clone https://github.com/kpedro88/CondorProduction Condor/Production > /dev/null 2>&1

  $ECHO "Getting EMJ core code..."
  git clone https://gitlab.cern.ch/cms-emj/emj-production EMJ/Production > /dev/null 2>&1
}


## Calling the main function
main $@
