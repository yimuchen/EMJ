import FWCore.ParameterSet.Config as cms
from FWCore.ParameterSet.VarParsing import VarParsing
import sys, os
from EMJ.Production.emjHelper import emjHelper

options = VarParsing("analysis")
options.register("signal", True, VarParsing.multiplicity.singleton,
                 VarParsing.varType.bool)
options.register("scan", "", VarParsing.multiplicity.singleton,
                 VarParsing.varType.string)
options.register("mX", 1000.0, VarParsing.multiplicity.singleton,
                 VarParsing.varType.float)
options.register("mDPi", 10.0, VarParsing.multiplicity.singleton,
                 VarParsing.varType.float)
options.register("tauDPi", 15, VarParsing.multiplicity.singleton,
                 VarParsing.varType.float)
options.register("part", 1, VarParsing.multiplicity.singleton,
                 VarParsing.varType.int)
options.register("outpre", "step1", VarParsing.multiplicity.list,
                 VarParsing.varType.string)
options.register("config", "EMJ.Production.2017UL.emj_step0_GEN",
                 VarParsing.multiplicity.singleton, VarParsing.varType.string)
options.register("dump", False, VarParsing.multiplicity.singleton,
                 VarParsing.varType.bool)
options.parseArguments()

# safety checks to handle multiple year
cmssw_version = os.getenv("CMSSW_VERSION")
cmssw_major = int(cmssw_version.split('_')[1])
if ".2016." in options.config and not (cmssw_major == 7 or cmssw_major == 8):
  raise ValueError("2016 config (" + options.config +
                   ") should not be used in non-2016 CMSSW version (" +
                   cmssw_version + ")")
elif ".2017." in options.config and not (cmssw_major == 9):
  raise ValueError("2017 config (" + options.config +
                   ") should not be used in non-2017 CMSSW version (" +
                   cmssw_version + ")")
elif ".2018." in options.config and not (cmssw_major == 10):
  raise ValueError("2018 config (" + options.config +
                   ") should not be used in non-2018 CMSSW version (" +
                   cmssw_version + ")")

# this is needed because options.outpre is not really a list
outpre = [x for x in options.outpre]
_helper = emjHelper()
_helper.setModel(mX=options.mX,
                 mDPi=options.mDPi,
                 tauDPi=options.tauDPi)

# output name definition
_outname = _helper.getOutName(events=options.maxEvents,
                              part=options.part,
                              signal=options.signal)
_outname += ".root"

_inname = ""

# import process
process = getattr(__import__(options.config, fromlist=["process"]), "process")

# input settings
process.maxEvents.input = cms.untracked.int32(options.maxEvents)
if len(_inname) > 0: process.source.fileNames = cms.untracked.vstring(_inname)
else:
  process.source.firstEvent = cms.untracked.uint32((options.part - 1) *
                                                   options.maxEvents + 1)
if len(options.scan) > 0:
  process.source.numberEventsInLuminosityBlock = cms.untracked.uint32(200)

# output settings
oprocess = process if (not hasattr(process, 'subProcesses') or len(
    process.subProcesses) == 0) else process.subProcesses[-1].process()
output_modules = sorted(oprocess.outputModules_())
for iout, output in enumerate(output_modules):
  if len(output) == 0: continue
  if not hasattr(oprocess, output):
    raise ValueError("Unavailable output module: " + output)
  getattr(oprocess,
          output).fileName = 'file:' + _outname.replace("outpre", outpre[iout])

# reset all random numbers to ensure statistically distinct but reproducible jobs
from IOMC.RandomEngine.RandomServiceHelper import RandomNumberServiceHelper
randHelper = RandomNumberServiceHelper(process.RandomNumberGeneratorService)
randHelper.resetSeeds(123)

if options.signal:
  if hasattr(process, 'generator'):
    process.generator.crossSection = cms.untracked.double(_helper.xsec)
    process.generator.PythiaParameters.processParameters = cms.vstring(
        _helper.getPythiaSettings())
    process.generator.maxEventsToPrint = cms.untracked.int32(10)

  # gen filter settings
  # pythia implementation of model has 4900111/211 -> -51 51 and 4900113/213 -> -53 53
  # this is a stand-in for direct production of a single stable dark meson in the hadronization
  # stable mesons should be produced in pairs (Z2 symmetry),
  # so require total number produced by pythia to be a multiple of 4
  # do *not* require this separately for 111/211 and 113/213 (pseudoscalar vs. vector)
  if hasattr(process, 'ProductionFilterSequence'):
    process.darkhadronZ2filter = cms.EDFilter("EMJMCFilter",
                                              moduleLabel=cms.InputTag(
                                                  'generator', 'unsmeared'),
                                              particleIDs=cms.vint32(51, 53),
                                              multipleOf=cms.uint32(4),
                                              absID=cms.bool(True),
                                              )
    process.ProductionFilterSequence += process.darkhadronZ2filter

  # also filter out events with Zprime -> SM quarks
  if hasattr(process, 'ProductionFilterSequence'):
    process.darkquarkFilter = cms.EDFilter("EMJMCFilter",
                                           moduleLabel=cms.InputTag(
                                               'generator', 'unsmeared'),
                                           particleIDs=cms.vint32(4900101),
                                           multipleOf=cms.uint32(2),
                                           absID=cms.bool(True),
                                           min=cms.uint32(2),
                                           status=cms.int32(23),
                                           )
    process.ProductionFilterSequence += process.darkquarkFilter

# genjet/met settings - treat DM stand-ins as invisible
_particles = [
    "genParticlesForJetsNoMuNoNu", "genParticlesForJetsNoNu",
    "genCandidatesForMET", "genParticlesForMETAllVisible"
]
for _prod in _particles:
  if hasattr(process, _prod):
    getattr(process, _prod).ignoreParticleIDs.extend([51, 52, 53])
if hasattr(process, 'recoGenJets') and hasattr(process, 'recoAllGenJetsNoNu'):
  process.recoGenJets += process.recoAllGenJetsNoNu
if hasattr(process, 'genJetParticles') and hasattr(process,
                                                   'genParticlesForJetsNoNu'):
  process.genJetParticles += process.genParticlesForJetsNoNu
  for output in output_modules:
    if len(output) == 0: continue
    output_attr = getattr(oprocess, output)
    if hasattr(output_attr, "outputCommands"):
      output_attr.outputCommands.extend([
          'keep *_genParticlesForJets_*_*', 'keep *_genParticlesForJetsNoNu_*_*',
      ])

# miniAOD settings
_pruned = ["prunedGenParticlesWithStatusOne", "prunedGenParticles"]
for _prod in _pruned:
  if hasattr(process, _prod):
    # keep HV & DM particles
    getattr(process, _prod).select.extend([
        "keep (4900001 <= abs(pdgId) <= 4900991 )",
        "keep (51 <= abs(pdgId) <= 53)",
    ])

if options.dump:
  print process.dumpPython()
  sys.exit(0)
