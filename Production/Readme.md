# Instructions for production

One goal of this package is the private production of signal MC files for faster
turn around. Instructions will give you an guide line on how to run jobs, both
interactively for debugging session and through condor for large sample
submissions.

## Interactive generation of events

The `test/runEMJ.py` file wraps standard generation config files for CMSSW used
for production (typically generated using cmsDriver.py commands), and adds
runtime process adjustments to the run process (adjusting pythia settings, adding
additional input/output file locations in preparation for condor jobs...etc). For
the generation of a signal sample (GEN Level only), run the following command:

```bash
cd EMJ/Production/test
cmsRun runEMJ.py                                   \
        config=EMJ.Production.2017UL.emj_step0_GEN \
        outpre=step0                               \
        part=1 maxEvents=10                        \
        mX=1000.0 mDPi=10.0 tauDPi=30.0
```

The generated ROOT file will have the various generation parameters inluded in
the file name. Since the scripts assume that you are performing standard
processes for the same set of files, future steps would assume that input files
follow the same naming scheme. For example, for to run the SIM step for the
output of the command above, run `runEMJ.py` as follows:

```bash
cmsRun runEMJ.py                                   \
        config=EMJ.Production.2017UL.emj_step1_SIM \
        outpre=step1                               \
        inpre=step0                                \
        part=1 maxEvents=10                        \
        mX=1000.0 mDPi=10.0 tauDPi=30.0
```

The file name of the input file is then automatically determined (assuming a
local production), and run with the SIM step configuration. In case that the
input file is generate somewhere else and doesn't follow the same naming scheme
as define in this package, you can still overwrite the input file using the
option `inputFiles=file:other_file1.root,file:other_file2.root` and such.

## Running in batch

Condor files are automatically generated using the
[CondorProduction](https://github.com/kpedro88/CondorProduction), and augmented
via the [jobSubmitterEMJ](../python/jobSubmitter.py) class, essentially adding
additional wrapping to the input options.

To construct a directory for managing the condor process, run the commands

```bash
cd EMJ/Production/test/
../scripts/makeProductionDir.sh myProduction
cd myProduction
```

Inside you should see the required scripts already linked into the directory.
then run the command:

```bash
python submitJobs.py -p                                           \
       -o [YOUR_OUTPUT_DIRECTORY]                                 \
       -d $CMSSW_BASE/src/EMJ/Production/data/gen_signal_test.py  \
       -E 500 -N 20                                               \
       --outpre step0_GEN                                         \
       --config EMJ.Production.2017UL.emj_step0_GEN -s
```

The `-E <E>` and `-N <N>` options effectively generates jobs that has the options
`maxEvents=<N>` and `part=1,2,3,4,...,N` as the regular runEMJ.py configuration
files. Files are automatically move the output directory.

TO continue the production, one can user the same command with additional input
and output options:

```bash
python submitJobs.py -p                                           \
       -o [YOUR_OUTPUT_DIRECTORY]                                 \
       -d $CMSSW_BASE/src/EMJ/Production/data/gen_signal_test.py  \
       -E 500 -N 20                                               \
       --outpre step1_SIM                                         \
       --config EMJ.Production.2017UL.emj_step1_GEN -s            \
      --indir=[PREVIOUS_OUTPUT_DIR]                               \
      --redir=[REMOTE_SITE_URL]
      --inpre=step0_GEN
```

Notice that in the case the you are moving the output files to some remote
server, (ex, to `root://cmseos.fnal.gov/`) you only need to specify this once in
the output stage via the `-o` option, like:

```bash
-o root://cmseos.fnal.gov//store/user/myusername/EMJProduction/
```

To use the output from the production command, the input would need to be
specified via the `--indir` and `--redir` options separately, so the
corresponding for the next step with the output above would be something like:

```bash
--redir=root://cmseos.fnal.gov/
--indir=/store/user/myusername/EMJProduction/
```

By default, `--redir` would look for files on the directly accessible via the
local machines (effectively `--redir=file:`).

## Specific Production Run Commands

### UMD 2017UL

```bash
# GEN ONLY
python submitJobs.py -p                                           \
       -o /data/users/yichen/EmergingJets/LocalProduction/output  \
       -d $CMSSW_BASE/src/EMJ/Production/data/gen_signal_test.py  \
       -E 500 -N 20                                               \
       --outpre step0_GEN                                         \
       --config EMJ.Production.2017UL.emj_step0_GEN -s

# SIM ONLY
python submitJobs.py -p                                              \
       -o /data/users/yichen/EmergingJets/LocalProduction/output     \
       -d $CMSSW_BASE/src/EMJ/Production/data/gen_signal_test.py     \
       -E 500 -N 20                                                  \
       --outpre step1_SIM                                            \
       --config EMJ.Production.2017UL.emj_step1_SIM -s               \
      --indir=/data/users/yichen/EmergingJets/LocalProduction/output \
      --redir=""   --inpre=step0_GEN

# DIGI ONLY
python submitJobs.py -p                                              \
       -o /data/users/yichen/EmergingJets/LocalProduction/output     \
       -d $CMSSW_BASE/src/EMJ/Production/data/gen_signal_test.py     \
       -E 500 -N 20                                                  \
       --outpre step2_DIGI                                           \
       --config EMJ.Production.2017UL.emj_step2_DIGI -s              \
      --indir=/data/users/yichen/EmergingJets/LocalProduction/output \
      --redir=""   --inpre=step1_SIM

# HLT
python submitJobs.py -p                                              \
       -o /data/users/yichen/EmergingJets/LocalProduction/output     \
       -d $CMSSW_BASE/src/EMJ/Production/data/gen_signal_test.py     \
       -E 500 -N 20                                                  \
       --outpre step3_HLT                                            \
       --config EMJ.Production.2017UL.emj_step3_HLT -s               \
      --indir=/data/users/yichen/EmergingJets/LocalProduction/output \
      --redir=""   --inpre=step2_DIGI

# Reco
python submitJobs.py -p                                              \
       -o /data/users/yichen/EmergingJets/LocalProduction/output     \
       -d $CMSSW_BASE/src/EMJ/Production/data/gen_signal_test.py     \
       -E 500 -N 20                                                  \
       --outpre step4_AOD                                            \
       --config EMJ.Production.2017UL.emj_step4_AOD -s               \
      --indir=/data/users/yichen/EmergingJets/LocalProduction/output \
      --redir=""   --inpre=step3_HLT

# MINIAOD
python submitJobs.py -p                                              \
       -o /data/users/yichen/EmergingJets/LocalProduction/output     \
       -d $CMSSW_BASE/src/EMJ/Production/data/gen_signal_test.py     \
       -E 500 -N 20                                                  \
       --outpre step5_MINIAOD                                        \
       --config EMJ.Production.2017UL.emj_step5_MINIAOD -s           \
      --indir=/data/users/yichen/EmergingJets/LocalProduction/output \
      --redir=""   --inpre=step4_AOD

```
