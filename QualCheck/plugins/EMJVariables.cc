#include "UserUtils/EDMUtils/interface/EDHistogramAnalyzer.hpp"

#include "DataFormats/HepMCCandidate/interface/GenParticle.h"
#include "DataFormats/Math/interface/deltaR.h"
#include "DataFormats/PatCandidates/interface/Jet.h"
#include "DataFormats/PatCandidates/interface/PackedCandidate.h"

// For tracking information
#include "FWCore/Framework/interface/ESHandle.h"
#include "TrackingTools/IPTools/interface/IPTools.h"
#include "TrackingTools/Records/interface/TransientTrackRecord.h"
#include "TrackingTools/TransientTrack/interface/TransientTrackBuilder.h"


#include "EMJ/QualCheck/interface/EMJGenFinder.hpp"


class EMJVariablesHist : public usr::EDHistogramAnalyzer
{
public:
  explicit EMJVariablesHist( const edm::ParameterSet& );
  ~EMJVariablesHist(){}

  static void fillDescriptions( edm::ConfigurationDescriptions& descriptions );

private:
  void beginJob() override;
  void doBeginRun_( const edm::Run&, const edm::EventSetup& ) override {}
  void analyze( const edm::Event&, const edm::EventSetup& ) override;
  void doEndRun_( const edm::Run&, const edm::EventSetup& ) override {}
  void endJob() override                                             {}

  // ----------member data ---------------------------
  // tokens
  edm::EDGetToken tok_ak4jet;
  edm::EDGetToken tok_pfcand;
  edm::EDGetToken tok_lost;
  edm::EDGetToken tok_pv;
  edm::EDGetToken tok_gen;

  bool RunJetVar( const pat::Jet&                          jet,
                  const reco::Vertex&                      pv,
                  const std::vector<reco::Track>&          tracks,
                  const std::vector<reco::TransientTrack>& ttracks,
                  const std::string&                       prefix  );

};

//
// constructors and destructor
//
EMJVariablesHist::EMJVariablesHist( const edm::ParameterSet& iConfig ) :
  usr::EDHistogramAnalyzer( iConfig ),
  tok_ak4jet( GetToken<std::vector<pat::Jet> >( "AK4Jet" ) ),
  tok_pfcand(  GetToken<std::vector<pat::PackedCandidate> >( "PFCandidate" ) ),
  tok_lost( GetToken<std::vector<pat::PackedCandidate> >( "LostTracks" ) ),
  tok_pv( GetToken<std::vector<reco::Vertex> >( "PrimaryVertex" ) ),
  tok_gen( GetToken<std::vector<reco::GenParticle> >( "GenPart" ) )
{
}

void EMJVariablesHist::beginJob()
{
  BookHist1D( "PVTrackF",  100,  0,   1 );

  BookHist1D( "DQ_DN",     100,  0, 100 );
  BookHist1D( "DQ_LogIP2D", 24, -5,   1 );
  BookHist1D( "DQ_NumTk",   50,  0,  50 );
  BookHist1D( "DQ_Alpha3D", 50,  0,   1 );

  BookHist1D( "SM_DN",     100,  0, 100 );
  BookHist1D( "SM_LogIP2D", 24, -5,   1 );
  BookHist1D( "SM_NumTk",  100,  0, 100 );
  BookHist1D( "SM_Alpha3D", 50,  0,   1 );
}

// Custom analysis helper functions
static reco::Vertex GetPrimaryVertex( const std::vector<reco::Vertex>& pvlist );

static std::vector<reco::Track> MakeTrackedCandidates(
  const reco::Vertex&                      pv,
  const std::vector<pat::PackedCandidate>& source1,
  const std::vector<pat::PackedCandidate>& source2,
  const TransientTrackBuilder&             ttBuilder );

static std::vector<reco::TransientTrack> MakeTTList(
  const std::vector<reco::Track>& track,
  const TransientTrackBuilder&    ttBuilder  );

static double PVTrackFraction( const reco::Vertex&                      vertex,
                               const std::vector<reco::TransientTrack>& tracks );

static bool IsGoodJet( const pat::Jet&, const reco::Candidate* darkq );

void EMJVariablesHist::analyze( const edm::Event&      event,
                                const edm::EventSetup& setup )
{
  edm::Handle<std::vector<pat::PackedCandidate> > candHandle;
  edm::Handle<std::vector<pat::PackedCandidate> > trackHandle;
  edm::Handle<std::vector<reco::GenParticle> > genHandle;
  edm::Handle<std::vector<pat::Jet> > jetHandle;
  edm::Handle<std::vector<reco::Vertex> > pvHandle;

  event.getByToken( tok_pfcand, candHandle );
  event.getByToken( tok_lost,  trackHandle );
  event.getByToken( tok_gen,     genHandle );
  event.getByToken( tok_ak4jet,  jetHandle );
  event.getByToken( tok_pv,       pvHandle );

  edm::ESHandle<TransientTrackBuilder> ttBuilder;
  setup.get<TransientTrackRecord>().get( "TransientTrackBuilder", ttBuilder );

  const reco::Vertex pv = GetPrimaryVertex( *pvHandle );
  const auto darkq      = FindDarkQuark( *genHandle );
  const auto smq        = FindSMQuark( *genHandle );

  const std::vector<reco::Track> tracklist
    = MakeTrackedCandidates( pv, *candHandle, *trackHandle, *ttBuilder );
  const std::vector<reco::TransientTrack> transTrackList
    = MakeTTList( tracklist, *ttBuilder );

  bool found_darkq = false;
  bool found_smq   = false;

  Hist( "PVTrackF" ).Fill( PVTrackFraction( pv, transTrackList ) );


  for( auto& jet : *jetHandle ){
    std::string prefix = "";
    if( !found_darkq && IsGoodJet( jet, darkq ) ){
      prefix      = std::string( "DQ_" );
      found_darkq = true;
    } else if( !found_smq && IsGoodJet( jet, smq ) ){
      prefix    = std::string( "SM_" );
      found_smq = true;
    } else {
      continue;
    }

    // Getting Tracks associated with jets ;
    if( !RunJetVar( jet, pv, tracklist, transTrackList, prefix ) ){
      if( prefix == "DQ_" ){ found_darkq = false; }
      if( prefix == "SM_" ){ found_smq = false; }
      continue;
    }

    // Early exit for single jet in event
    if( found_darkq && found_smq ){ break; }
  }
}

bool EMJVariablesHist::RunJetVar(
  const pat::Jet&                          jet,
  const reco::Vertex&                      pv,
  const std::vector<reco::Track>&          tracks,
  const std::vector<reco::TransientTrack>& ttracks,
  const std::string&                       prefix  )
{
  assert( tracks.size() == ttracks.size() );

  unsigned ntracks   = 0;
  double ip2d_mean   = 0;
  double alpha3d     = 0;
  double trackpt_sum = 0;

  for( unsigned i = 0; i < tracks.size(); ++i ){
    const auto& track  = tracks.at( i );
    const auto& ttrack = ttracks.at( i );

    // Additional track selection
    if( deltaR( track, jet ) > 0.4 ){ continue; }
    // If track takes up too much of the jet.
    if( track.pt() > jet.pt() * 0.6 ){ return false; }

    ++ntracks;

    // Getting impact parameter information
    auto ip3d_p = IPTools::absoluteImpactParameter3D( ttrack, pv );
    auto ip2d_p = IPTools::absoluteTransverseImpactParameter( ttrack, pv );

    const double ip3d     = ip3d_p.second.value();
    const double ip2d     = ip2d_p.second.value();
    const double ip2d_sig = ip2d_p.second.significance();

    // const double ip2d_sig = ip2d_p.second.significance();
    const double ipz = std::sqrt( ip3d*ip3d - ip2d*ip2d );

    const double DN = std::sqrt( ipz*ipz / 0.0001 + ip2d_sig*ip2d_sig );
    ip2d_mean += ip2d;
    if( DN > 4 ){
      alpha3d += track.pt();
    }
    trackpt_sum += track.pt();

    Hist( prefix + "DN" ).Fill( DN );
  }

  ip2d_mean /= ntracks;
  alpha3d   /= trackpt_sum;

  Hist( prefix + "LogIP2D" ).Fill( std::log( ip2d_mean )/std::log( 10 ) );
  Hist( prefix + "NumTk" ).Fill( ntracks );
  Hist( prefix + "Alpha3D" ).Fill( alpha3d );

  return true;
}



reco::Vertex GetPrimaryVertex( const std::vector<reco::Vertex>& pvlist )
{
  for( const auto& vertex : pvlist ){
    if( vertex.isFake() ){ continue; }
    if( vertex.z() > 15 ){ continue; }
    return vertex;
  }

  return pvlist.at( 0 );
}

std::vector<reco::Track> MakeTrackedCandidates(
  const reco::Vertex&                      pv,
  const std::vector<pat::PackedCandidate>& source1,
  const std::vector<pat::PackedCandidate>& source2,
  const TransientTrackBuilder&             ttBuilder )
{
  std::vector<reco::Track> ans;

  auto GoodTrack
    = [&pv, &ttBuilder]( const reco::Track& track )->bool {
        if( track.pt() < 1.0 ){ return false; }
        if( !track.quality( reco::TrackBase::highPurity ) ){ return false; }
        // Calculating impact parameter
        auto ttrack = ttBuilder.build( track );
        auto ip3d_p = IPTools::absoluteImpactParameter3D( ttrack, pv );
        auto ip2d_p = IPTools::absoluteTransverseImpactParameter( ttrack, pv );

        const double ip3d = ip3d_p.second.value();
        const double ip2d = ip2d_p.second.value();
        // const double ip2d_sig = ip2d_p.second.significance();
        const double ipz = std::sqrt( ip3d*ip3d - ip2d*ip2d );

        if( ipz > 2.5 ){ return false; }

        return true;
      };

  for( const auto& cand : source1 ){
    if( !cand.hasTrackDetails() ){ continue; }
    const auto track = cand.pseudoTrack();
    if( !GoodTrack( track ) ){ continue; }
    ans.push_back( track );
  }

  for( const auto& cand : source1 ){
    if( !cand.hasTrackDetails() ){ continue; }
    const auto track = cand.pseudoTrack();
    if( !GoodTrack( track ) ){ continue; }
    ans.push_back( track );
  }

  return ans;
}

std::vector<reco::TransientTrack> MakeTTList(
  const std::vector<reco::Track>& source,
  const TransientTrackBuilder&    ttBuilder )
{
  std::vector<reco::TransientTrack> ans;

  for( const auto& track : source ){
    ans.push_back( ttBuilder.build( track ) );
  }

  return ans;
}

double PVTrackFraction( const reco::Vertex&                      pv,
                        const std::vector<reco::TransientTrack>& tracks )
{
  unsigned passed = 0;


  for( const auto track : tracks ){
    const auto ip3d_p = IPTools::absoluteImpactParameter3D( track, pv );
    const auto ip2d_p = IPTools::absoluteTransverseImpactParameter( track, pv );
    const double ip3d = ip3d_p.second.value();
    const double ip2d = ip2d_p.second.value();
    const double ipz  = std::sqrt( ip3d*ip3d - ip2d*ip2d );

    if( ipz < 0.01 ){ ++passed; }
  }

  return double(passed)/double(tracks.size() );

}

bool IsGoodJet( const pat::Jet& jet, const reco::Candidate* darkq )
{
  // Basic quality cuts
  if( fabs( jet.eta() ) > 2.0 ){ return false; }
  if( jet.chargedEmEnergyFraction() > 0.9 ){ return false; }
  if( jet.neutralEmEnergyFraction() > 0.9 ){ return false; }

  // Match with dark quark
  if( darkq && deltaR( jet, *darkq ) > 0.4 ){ return false; }

  return true;
}


void EMJVariablesHist::fillDescriptions( edm::ConfigurationDescriptions&
                                         descriptions )
{
  edm::ParameterSetDescription desc;
  desc.add<edm::InputTag>(
    "AK4Jet", edm::InputTag( "slimmedJets" ) );
  desc.add<edm::InputTag>(
    "PFCandidate", edm::InputTag( "packedPFCandidates" ) );
  desc.add<edm::InputTag>(
    "LostTracks", edm::InputTag( "lostTracks" ) );
  desc.add<edm::InputTag>(
    "PrimaryVertex", edm::InputTag( "offlineSlimmedPrimaryVertices" ) );
  desc.add<edm::InputTag>(
    "GenPart", edm::InputTag( "prunedGenParticles" ) );

  descriptions.add( "EMJVariablesHist", desc );
}


DEFINE_FWK_MODULE( EMJVariablesHist );
