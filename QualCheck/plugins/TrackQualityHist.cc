#include "UserUtils/EDMUtils/interface/EDHistogramAnalyzer.hpp"

#include "DataFormats/HepMCCandidate/interface/GenParticle.h"
#include "DataFormats/Math/interface/deltaR.h"
#include "DataFormats/PatCandidates/interface/PackedCandidate.h"

class TrackQualityHist : public usr::EDHistogramAnalyzer
{
public:
  explicit TrackQualityHist( const edm::ParameterSet& );
  ~TrackQualityHist(){}

  static void fillDescriptions( edm::ConfigurationDescriptions& descriptions );

private:
  void beginJob() override;
  void doBeginRun_( const edm::Run&, const edm::EventSetup& ) override {}
  void analyze( const edm::Event&, const edm::EventSetup& ) override;
  void doEndRun_( const edm::Run&, const edm::EventSetup& ) override {}
  void endJob() override                                             {}

  // ----------member data ---------------------------
  // tokens
  edm::EDGetToken tok_pfcand;
  edm::EDGetToken tok_genpart;
};

//
// constructors and destructor
//
TrackQualityHist::TrackQualityHist( const edm::ParameterSet& iConfig ) :
  usr::EDHistogramAnalyzer( iConfig ),
  tok_pfcand(  GetToken<std::vector<pat::PackedCandidate> >( "PFCandidate" ) ),
  tok_genpart( GetToken<std::vector<reco::GenParticle> >( "GenPart" ) )
{
}

void TrackQualityHist::beginJob()
{
  BookHist1D( "TkNum",  60, 0,   60    );
  BookHist1D( "PT",    400, 0,   10    );
  BookHist1D( "Eta",   500, -2.5, +2.5 );
  BookHist1D( "Phi",   640, -3.2, +3.2 );
  BookHist1D( "Chi2",   20, 0,   20    );
  BookHist1D( "D0",    400, 0,    3    );
  BookHist1D( "DZ",    400, 0,   10    );
  BookHist1D( "NHits",  40, 0,   40    );
  BookHist1D( "NMHits", 20, 0,   20    );
}

// Custom analysis helper functions
const reco::Candidate* FindDarkQuark( const std::vector<reco::GenParticle>& );
bool                   IsGoodCandidate( const pat::PackedCandidate&, const reco::Candidate* );

void TrackQualityHist::analyze( const edm::Event& event, const edm::EventSetup& )
{
  edm::Handle<std::vector<pat::PackedCandidate> > candHandle;
  edm::Handle<std::vector<reco::GenParticle> > genHandle;

  event.getByToken( tok_pfcand, candHandle );
  event.getByToken( tok_genpart, genHandle );

  const auto darkq = FindDarkQuark( *genHandle );
  if( darkq == 0 ){ return; }// Early Exist for non-dark quark events

  unsigned numtrack = 0;
  for( const auto& cand : *candHandle ){
    if( !IsGoodCandidate( cand, darkq ) ){ continue; }

    const auto& track = *cand.bestTrack();
    numtrack++;

    // Filling In Track information
    Hist( "PT" ).Fill( track.pt() );
    Hist( "Eta" ).Fill( track.eta() );
    Hist( "Phi" ).Fill( track.phi() );
    Hist( "Chi2" ).Fill( track.normalizedChi2() );
    Hist( "D0" ).Fill( track.d0() );
    Hist( "DZ" ).Fill( track.dz() );
    Hist( "NHits" ).Fill( track.numberOfValidHits() );
    Hist( "NMHits" ).Fill( track.numberOfLostHits() );
  }

  Hist( "Num" ).Fill( numtrack );
}

const reco::Candidate* FindDarkQuark( const std::vector<reco::GenParticle>& vec )
{
  const reco::Candidate* darkq = 0;
  const reco::Candidate* smq   = 0;

  for( const auto& gen : vec ){
    darkq = 0;
    smq   = 0;
    if( gen.pdgId() != 4900001 ){continue;}
    if( gen.numberOfDaughters() <= 1  ){continue;}

    for( unsigned i = 0; i < gen.numberOfDaughters(); ++i ){
      if( abs( gen.daughter( i )->pdgId() ) <= 6 ){
        smq = gen.daughter( i );
        break;
      }
    }

    if( !smq ){continue;}

    for( unsigned i = 0; i < gen.numberOfDaughters(); ++i ){
      if( abs( gen.daughter( i )->pdgId() ) > 4900000 ){
        darkq = gen.daughter( i );

        while( darkq->daughter( 0 )->pdgId() == darkq->pdgId() ){
          darkq = darkq->daughter( 0 );
        }

        break;
      }
    }

    if( darkq && smq ){ break; }
  }

  return darkq;
}

bool IsGoodCandidate( const pat::PackedCandidate& cand,
                      const reco::Candidate*      darkq )
{
  if( !cand.bestTrack() ){ return false;  }
  const auto& track = *cand.bestTrack();
  if( deltaR( *darkq, track ) > 0.4 ){ return false; }

  return true;
}

void TrackQualityHist::fillDescriptions( edm::ConfigurationDescriptions&
                                         descriptions )
{
  edm::ParameterSetDescription desc;
  desc.add<edm::InputTag>(
    "PFCandidate", edm::InputTag( "packedPFCandidates" ) );
  desc.add<edm::InputTag>(
    "GenPart", edm::InputTag( "prunedGenParticles" ) );

  descriptions.add( "TrackQualityHist", desc );
}


DEFINE_FWK_MODULE( TrackQualityHist );
