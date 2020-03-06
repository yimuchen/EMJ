#include "UserUtils/EDMUtils/interface/EDHistogramAnalyzer.hpp"

#include "DataFormats/HepMCCandidate/interface/GenParticle.h"
#include "DataFormats/Math/interface/deltaR.h"
#include "DataFormats/PatCandidates/interface/PackedCandidate.h"

#include "EMJ/QualCheck/interface/EMJObjectSelect.hpp"

// For tracking information
#include "FWCore/Framework/interface/ESHandle.h"
#include "TrackingTools/IPTools/interface/IPTools.h"
#include "TrackingTools/Records/interface/TransientTrackRecord.h"
#include "TrackingTools/TransientTrack/interface/TransientTrackBuilder.h"


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
  edm::EDGetToken tok_pv;
};

//
// constructors and destructor
//
TrackQualityHist::TrackQualityHist( const edm::ParameterSet& iConfig ) :
  usr::EDHistogramAnalyzer( iConfig ),
  tok_pfcand(  GetToken<std::vector<pat::PackedCandidate> >( "PFCandidate" ) ),
  tok_pv( GetToken<std::vector<reco::Vertex> >( "PrimaryVertex" ) )
{
}

void TrackQualityHist::beginJob()
{
  BookHist1D( "TkNum",    500, 0,    500 );
  BookHist1D( "PT",       400, 0,     10 );
  BookHist1D( "Eta",      600, -3.0, 3.0 );
  BookHist1D( "Phi",      640, -3.2, 3.2 );
  BookHist1D( "Chi2",      20, 0,     20 );
  BookHist1D( "IP2D",     400, 0,    1.0 );
  BookHist1D( "IP2D_sig", 400, 0,      5 );
  BookHist1D( "IPZ",      400, 0,     10 );
  BookHist1D( "NHits",     40, 0,     40 );
  BookHist1D( "NPix",       5, 0,      5 );
}

void TrackQualityHist::analyze( const edm::Event&      event,
                                const edm::EventSetup& setup )
{
  edm::Handle<std::vector<pat::PackedCandidate> > candHandle;
  edm::Handle<std::vector<reco::Vertex> > pvHandle;
  edm::ESHandle<TransientTrackBuilder> ttBuilder;

  event.getByToken( tok_pv,       pvHandle );
  event.getByToken( tok_pfcand, candHandle );
  setup.get<TransientTrackRecord>().get( "TransientTrackBuilder", ttBuilder );

  // Getting the primary vertex
  const int pv_idx = GetPrimaryVertex( *pvHandle );
  if( pv_idx < 0 ){ return; }
  const auto pv = pvHandle->at( pv_idx );

  unsigned numtrack = 0;

  for( const auto& cand : *candHandle ){
    if( !cand.hasTrackDetails() ){ continue; }
    const auto track = cand.pseudoTrack();
    if( track.pt() < 1.0 ){ continue; }

    numtrack++;

    const auto ttrack = ttBuilder->build( track );

    // Filling In Track information
    Hist( "PT" ).Fill( track.pt() );
    Hist( "Eta" ).Fill( track.eta() );
    Hist( "Phi" ).Fill( track.phi() );
    Hist( "Chi2" ).Fill( track.normalizedChi2() );
    Hist( "NHits" ).Fill( track.numberOfValidHits() );
    Hist( "NPix" ).Fill( track.hitPattern().numberOfValidPixelHits() );

    // Impact parameter information
    auto ip3d_p = IPTools::absoluteImpactParameter3D( ttrack, pv );
    auto ip2d_p = IPTools::absoluteTransverseImpactParameter( ttrack, pv );

    const double ip3d = ip3d_p.second.value();
    const double ip2d = ip2d_p.second.value();
    // const double ip2d_sig = ip2d_p.second.significance();
    const double ipz = std::sqrt( ip3d*ip3d - ip2d*ip2d );

    Hist( "IP2D" ).Fill( ip2d );
    Hist( "IPZ" ).Fill( ipz );
    Hist( "IP2D_sig" ).Fill( ip2d_p.second.significance() );
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
    "PrimaryVertex", edm::InputTag( "offlineSlimmedPrimaryVertices" ) );

  descriptions.add( "TrackQualityHist", desc );
}


DEFINE_FWK_MODULE( TrackQualityHist );
