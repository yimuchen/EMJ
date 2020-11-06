# Emerging Jets Analysis Code

Code for the emerging jets analysis for CMSSW. Main code has been moved to the
[CERN gitlab](https://gitlab.cern.ch/)
[repository](https://gitlab.cern.ch/yichen/EMJ). This page is only kept for easy
access to the installation scripts.

## Pythia calculations

The file [emjHelper.py](emjHelper.py) contains the class used for pythia card
generation for the Emerging Jets analysis. The file support stdout outputs of the
generated pythias cards and various information. For interoperability with CMSSW,
this scripts is designed to work with python2.

### Dumping Pythia settings

For dummping the pythia settings passed onto CMSSW for a single parameters point.
Run the command:

```bash
python2 emjHelper.py dumpcard --mMed=1000 --mDark=100 --kappa=1 --type=down --mode=aligned
```

The following options are supported:

```bash
  --mMed MMED           Dark mediator mass [GeV]
  --kappa KAPPA         Kappa0 squared (factor to scale decay lifetime)
  --mDark MDARK         Dark meson mass [GeV]
  --type {down,up}      Coupling to SM up/down type SM quarks
  --mode {aligned,unflavored}
                        Mixing scenarios to simulate
```

### Getting the decay-time verses mark meson mass results.

Run the command

```bash
python emjHelper.py dumptime --mMed=1000 --kappa=1
```

This prints a five columns of text. The first column is the mediator mass (in
GeV) the following 4 columns are the decay lifetime (in mm) for the "neutral"
dark pion (`pi0->qq`), "charged" dark pion (`pi->ds`), “neutral” dark kaon
(`K0->db`), and the "charged" dark kaon (`K->sb`). Standard plotting scripts for
the output of this file can be found in `EMJ/QualCheck`.
