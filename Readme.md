# Emerging Jets Analysis Code

Code for the emerging jets analysis for CMSSW.

## Installation instructions

Currently, the code is aimed for the ultra-legacy Monte-Carlo run for CMS RunII
data using some branch of `CMSSW_10_6_4`. Notice that this must use a SLC7
machines, so be careful where you are running (`siab-1` in UMD, `lxplus7.cern.ch`
at CERN... etc.)

```bash
export SCRAM_ARCH='slc7_amd64_gcc700'
cmsrel CMSSW_10_6_4
cd CMSSW_10_6_4/src

# For batch cmsRun scripts
git clone https://github.com/kpedro88/CondorProduction Condor/Production
# For EMJ specific code
git clone https://github.com/yimuchen/EMJ
# For augmented plotting library in CMSSW
git clone https://github.com/yimuchen/UserUtils


scram b -j 8
```
