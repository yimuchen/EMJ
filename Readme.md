# Emerging Jets Analysis Code

Code for the emerging jets analysis for CMSSW.

## Installation

Currently, the code is set up to generate emerging jet samples for various
Monte-Carlo run for CMS Run-II data. Since the production uses a modified version
of pythia8 (see the modifications
[here](https://github.com/kpedro88/pythia8/tree/emg/230)), there are various
restrictions on the CMSSW version and the SLC version used for production. Take
notice on the installation instructions.

```bash
## Setup a working directory
wget https://raw.githubusercontent.com/yimuchen/EMJ/master/installEMJ.sh
chmod +x installEMJ.sh

./installEMJ.sh -c CMSSW_10_2_21 # For 2018 Production
                                 # More version coming soon
```
