import FWCore.ParameterSet.Config as cms

process = cms.Process("GenHist")

process.load("Configuration.StandardSequences.Services_cff")

# Auto generated from descriptions
process.load("EMJ.GenAnalysis.GenHistogram_cfi")

process.maxEvents = cms.untracked.PSet(
    input = cms.untracked.int32(-1)
)

process.source = cms.Source("PoolSource",
    fileNames = cms.untracked.vstring('file:test.root'),
)

process.TFileService = cms.Service("TFileService",
    fileName = cms.string("genhistogram.root")
)

process.p1 = cms.Path(process.GenHistogram)