#!/bin/env python2
import subprocess
import argparse
import os
import sys

parser = argparse.ArgumentParser('MakeDriverCmd')
parser.add_argument('--configfile',
                    type=str,
                    default='EMJ/Production/python/EmptyFragment_cfi.py',
                    help='Configuration for first gen step')
parser.add_argument('--python_prefix',
                    type=str,
                    default='emj',
                    help='Prefix for generated python file')
parser.add_argument('--nthreads',
                    type=int,
                    default=1,
                    help='Number of threads argument in generated config files')
parser.add_argument('--conditions',
                    type=str,
                    default='106X_mc2017_realistic_v6',
                    help='Global tag settings')
parser.add_argument('--beamspot',
                    type=str,
                    default='Realistic25ns13TeVEarly2017Collision',
                    help='Beam spot configuration.')
parser.add_argument('--geometry',
                    type=str,
                    default='DB:Extended',
                    help='Geometry configuration settting')
parser.add_argument('--era',
                    type=str,
                    default='Run2_2017',
                    help='Run Era setting')
parser.add_argument('--hltversion',
                    type=str,
                    default='@relval2017',
                    help='HLT version string')
parser.add_argument(
    '--pileup',
    type=str,
    default=
    'dbs:/Neutrino_E-10_gun/RunIISummer19ULPrePremix-UL17_106X_mc2017_realistic_v6-v1/PREMIX',
    help='Pile up configuration file')
parser.add_argument('--hlt_condition',
                    type=str,
                    default='94X_mc2017_realistic_v15',
                    help='Setting the HTL configuration')


def print_and_run(step_name, cmd):
  print('>>> {0} Command:'.format(step_name))
  print('  '.join(cmd))
  print('\n')
  subprocess.call(cmd)
  print('\n\n\n')
  print('>>>> {0} Command Finished\n\n\n'.format(step_name) )


def makeGEN(args):
  cmd = """cmsDriver.py {cfg_name}
      --mc --eventcontent=RAWSIM --datatier=GEN --step=GEN
      --python_filename={python_prefix}_step0_GEN.py
      --conditions={condition}
      --beamspot={beamspot}
      --geometry={geometry}
      --era={era}
      --nThreads={threads}
      --fileout=file:step0.root
      --no_exec
  """.format(cfg_name=args.configfile,
             python_prefix=args.python_prefix,
             condition=args.conditions,
             beamspot=args.beamspot,
             threads=args.nthreads,
             geometry=args.geometry,
             era=args.era).split()
  print_and_run('GEN', cmd)


def makeSIM(args):
  cmd = """cmsDriver.py step1
    --python_filename={python_prefix}_step1_SIM.py
    --mc --eventcontent=RAWSIM --runUnscheduled --datatier=SIM  --step=SIM
    --conditions={conditions}
    --beamspot={beamspot}
    --geometry={geometry}
    --era={era}
    --nThreads={threads}
    --fileout=file:step1.root
    --no_exec
  """.format(cfg_name=args.configfile,
             python_prefix=args.python_prefix,
             conditions=args.conditions,
             beamspot=args.beamspot,
             threads=args.nthreads,
             geometry=args.geometry,
             era=args.era).split()
  print_and_run('SIM', cmd)


def makeDIGI(args):
  cmd = """cmsDriver.py step2
  --fileout file:step2.root
    --python_filename={python_prefix}_step2_DIGI.py
  --pileup_input=\"{pileup}\"
  --mc --eventcontent=PREMIXRAW --runUnscheduled --datatier=GEN-SIM-DIGI
  --conditions={conditions}
  --step=DIGI,DATAMIX,L1,DIGI2RAW
  --procModifiers=premix_stage2
  --nThreads={threads}
  --geometry={geometry}
  --datamix=PreMix
  --era={era}
  --no_exec""".format(python_prefix=args.python_prefix,
                      threads=args.nthreads,
                      conditions=args.conditions,
                      era=args.era,
                      pileup=args.pileup,
                      geometry=args.geometry).split()
  print_and_run('DIGI', cmd)


def makeHLT(args):
  cmd = """ cmsDriver.py step3
  --python_filename={python_prefix}_step3_HLT.py
  --mc --eventcontent RAWSIM --datatier GEN-SIM-RAW
  --conditions={conditions}
  --customise_commands='process.source.bypassVersionCheck=cms.untracked.bool(True)'
  --step=HLT:{hltversion}
  --nThreads={threads}
  --geometry={geometry}
  --era={era}
  --fileout=step3.root
  --no_exec""".format(conditions=args.hlt_condition,
                      python_prefix=args.python_prefix,
                      hltversion=args.hltversion,
                      threads=args.nthreads,
                      geometry=args.geometry,
                      era=args.era).split()
  print_and_run( 'HLT', cmd )


def makeAOD(args):
  cmd = """cmsDriver.py step4
    --python_filename={python_prefix}_step4_AOD.py
    --mc --eventcontent AODSIM --runUnscheduled --datatier AODSIM
    --step RAW2DIGI,L1Reco,RECO,RECOSIM
    --conditions={conditions}
    --nThreads={threads}
    --geometry={geometry}
    --era={era}
    --fileout=step4.root
    --no_exec""".format(python_prefix=args.python_prefix,
                        conditions=args.conditions,
                        threads=args.nthreads,
                        geometry=args.geometry,
                        era=args.era).split()
  print_and_run('AOD', cmd)


def makeMINIAOD(args):
  cmd = """cmsDriver.py  step5
  --python_filename={python_prefix}_step5_MINIAOD.py
  --mc --eventcontent MINIAODSIM --runUnscheduled --datatier MINIAODSIM
  --step PAT
  --conditions={conditions}
   --geometry={geometry}
  --era={era}
  --nThreads={threads}
  --filein=step4.root
  --fileout=step5.root
  --no_exec""".format(python_prefix=args.python_prefix,
                      conditions=args.conditions,
                      geometry=args.geometry,
                      threads=args.nthreads,
                      era=args.era).split()
  print_and_run('MINIAOD', cmd)


if __name__ == '__main__':
  args = parser.parse_args()
  makeGEN(args)
  makeSIM(args)
  makeDIGI(args)
  makeHLT(args)
  makeAOD(args)
  makeMINIAOD(args)

  print("Generation Command: MakeDriverCmd.py " +
        "\n\t".join(["--{0}={1}".format(x,
                                        vars(args)[x]) for x in vars(args)]))
