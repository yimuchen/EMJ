import FWCore.ParameterSet.Config as cms
from FWCore.ParameterSet.VarParsing import VarParsing

options = VarParsing('analysis')
options.outputFile = 'TrackQuality_output.root'
options.parseArguments()


process = cms.Process("TrackQualityHist")

process.load("Configuration.StandardSequences.Services_cff")

# Auto generated from descriptions
process.load("EMJ.QualCheck.TrackQualityHist_cfi")

process.maxEvents = cms.untracked.PSet(
    input = cms.untracked.int32(options.maxEvents)
)

process.source = cms.Source("PoolSource",
    fileNames = cms.untracked.vstring(options.inputFiles),
)

process.TFileService = cms.Service("TFileService",
    fileName = cms.string(options.outputFile)
)

# Other statements
process.load('Configuration.StandardSequences.GeometryRecoDB_cff')
process.load('Configuration.StandardSequences.MagneticField_cff')
process.load('Configuration.StandardSequences.FrontierConditions_GlobalTag_cff')
from Configuration.AlCa.GlobalTag import GlobalTag
process.GlobalTag = GlobalTag(process.GlobalTag, '106X_mc2017_realistic_v6', '')

process.load("TrackingTools/TransientTrack/TransientTrackBuilder_cfi")

# Matched stuff
process.PFTrackHist = process.TrackQualityHist.clone()
process.PFTrackHist.PFCandidate = cms.InputTag("packedPFCandidates")

# Lost track stuff
process.LostTrackHist = process.TrackQualityHist.clone()
process.LostTrackHist.PFCandidate = cms.InputTag("lostTracks")

# Making the program less verbose
process.load('FWCore.MessageService.MessageLogger_cfi')
process.MessageLogger.cerr.FwkReport.reportEvery = 100

process.p1 = cms.Path(process.PFTrackHist + process.LostTrackHist)

