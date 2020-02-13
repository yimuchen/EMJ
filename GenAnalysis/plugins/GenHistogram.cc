// framework headers
#include "CommonTools/UtilAlgos/interface/TFileService.h"
#include "FWCore/Framework/interface/ESHandle.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/Frameworkfwd.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/Framework/interface/one/EDAnalyzer.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/PluginManager/interface/ModuleDef.h"
#include "FWCore/ServiceRegistry/interface/Service.h"
#include "FWCore/Utilities/interface/StreamID.h"

// analysis headers
#include "DataFormats/Candidate/interface/Candidate.h"
#include "DataFormats/Candidate/interface/CandidateFwd.h"
#include "DataFormats/HepMCCandidate/interface/GenParticle.h"
#include "DataFormats/HepMCCandidate/interface/GenParticleFwd.h"
#include "DataFormats/JetReco/interface/GenJet.h"
#include "DataFormats/Math/interface/deltaPhi.h"
#include "DataFormats/Math/interface/deltaR.h"
#include "DataFormats/METReco/interface/GenMET.h"


// ROOT headers
#include "TH1D.h"

// STL headers
#include <iostream>
#include <map>
#include <vector>

//
// class declaration
//

class GenHistogram :
  public virtual edm::one::EDAnalyzer<edm::one::SharedResources>
{
public:
  explicit GenHistogram( const edm::ParameterSet& );
  ~GenHistogram(){}

  static void fillDescriptions( edm::ConfigurationDescriptions& descriptions );

private:
  void beginJob() override;
  void doBeginRun_( const edm::Run&, const edm::EventSetup& ) override {}
  void analyze( const edm::Event&, const edm::EventSetup& ) override;
  void doEndRun_( const edm::Run&, const edm::EventSetup& ) override {}
  void endJob() override                                             {}

  // ----------member data ---------------------------
  edm::Service<TFileService> fs;
  std::map<std::string, TH1D*> _histmap;
  // tokens
  edm::EDGetTokenT<std::vector<reco::GenMET> > tok_met;
  edm::EDGetTokenT<std::vector<reco::GenJet> > tok_jetak4;
  edm::EDGetTokenT<std::vector<reco::GenJet> > tok_jetak8;
  edm::EDGetTokenT<std::vector<reco::GenParticle> > tok_part;
};

//
// constructors and destructor
//
GenHistogram::GenHistogram( const edm::ParameterSet& iConfig ) :
  tok_met( consumes<std::vector<reco::GenMET> >(
    iConfig.getParameter<edm::InputTag>( "METTag" ) ) ),
  tok_jetak4( consumes<std::vector<reco::GenJet> >(
    iConfig.getParameter<edm::InputTag>( "JetAK4Tag" ) ) ),
  tok_jetak8( consumes<std::vector<reco::GenJet> >(
    iConfig.getParameter<edm::InputTag>( "JetAK8Tag" ) ) ),
  tok_part( consumes<std::vector<reco::GenParticle> >(
    iConfig.getParameter<edm::InputTag>( "ParticleTag" ) ) )
{
  usesResource( "TFileService" );
}

void GenHistogram::beginJob()
{
  _histmap["NAK8Jet"]   = fs->make<TH1D>( "NAK8Jet", "", 15, 0, 15 );
  _histmap["AK8JetPt"]  = fs->make<TH1D>( "AK8JetPt", "", 100, 0, 300 );
  _histmap["AK8JetEta"] = fs->make<TH1D>( "AK8JetEta", "", 30, -3, 3 );
}

void GenHistogram::analyze( const edm::Event& event, const edm::EventSetup& )
{
  edm::Handle<std::vector<reco::GenJet> > h_jetak8;
  event.getByToken( tok_jetak8, h_jetak8 );

  _histmap["NAK8Jet"]->Fill( h_jetak8->size() );

  for( const auto jet : ( *h_jetak8 ) ){
    _histmap["AK8JetPt"]->Fill( jet.pt() );
    _histmap["AK8JetEta"]->Fill( jet.eta() );
  }
}


void GenHistogram::fillDescriptions( edm::ConfigurationDescriptions&
                                     descriptions )
{
  edm::ParameterSetDescription desc;
  desc.add<edm::InputTag>( "METTag",      edm::InputTag( "genMetTrue" ) );
  desc.add<edm::InputTag>( "JetAK4Tag",   edm::InputTag( "ak4GenJetsNoNu" ) );
  desc.add<edm::InputTag>( "JetAK8Tag",   edm::InputTag( "ak8GenJetsNoNu" ) );
  desc.add<edm::InputTag>( "ParticleTag", edm::InputTag( "genParticles" ) );

  descriptions.add( "GenHistogram", desc );
}


DEFINE_FWK_MODULE( GenHistogram );
