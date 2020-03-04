#include "UserUtils/EDMUtils/interface/EDHistogramAnalyzer.hpp"

#include "DataFormats/HepMCCandidate/interface/GenParticle.h"
#include "DataFormats/Math/interface/deltaR.h"
#include "DataFormats/PatCandidates/interface/PackedCandidate.h"

#include "EMJ/QualCheck/interface/EMJGenFinder.hpp"


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
  BookHist1D( "TkNum",    60,   0,     60 );
  BookHist1D( "PT",      400,   0,     10 );
  BookHist1D( "Eta",     500,   -2.5, 2.5 );
  BookHist1D( "Phi",     640,   -3.2, 3.2 );
  BookHist1D( "Chi2",     20,   0,     20 );
  BookHist1D( "D0",      400,   0,      3 );
  BookHist1D( "DZ",      400, -10,     10 );
  BookHist1D( "NHits",    40,   0,     40 );
  BookHist1D( "NMHits",   20,   0,     20 );
  BookHist1D( "InnerHit", 64,   0,     64 );
  BookHist1D( "LastBits",  4,   0,      4 );
}

// Custom analysis helper functions
static bool IsGoodCandidate( const pat::PackedCandidate&
                           , const reco::Candidate* );

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

    const auto track = cand.pseudoTrack();
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

    // The first hit pattern
    const auto innerhit =
      track.hitPattern().getHitPattern( reco::HitPattern::TRACK_HITS, 0 );

    Hist( "InnerHit" ).Fill( ( ( innerhit % 1024 ) >> 3 ) - 16 );
    Hist( "LastBits" ).Fill( innerhit & 3 );
  }

  Hist( "TkNum" ).Fill( numtrack );
}

bool IsGoodCandidate( const pat::PackedCandidate& cand,
                      const reco::Candidate*      darkq )
{
  if( !cand.hasTrackDetails() ){ return false; }
  if( deltaR( *darkq, cand.pseudoTrack() ) > 0.4 ){ return false; }

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
