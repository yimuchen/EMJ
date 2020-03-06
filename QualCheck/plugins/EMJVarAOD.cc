#include "UserUtils/EDMUtils/interface/EDHistogramAnalyzer.hpp"

#include "DataFormats/HepMCCandidate/interface/GenParticle.h"
#include "DataFormats/JetReco/interface/PFJet.h"
#include "DataFormats/Math/interface/deltaR.h"
#include "DataFormats/TrackReco/interface/Track.h"
#include "DataFormats/VertexReco/interface/Vertex.h"

// For tracking information
#include "FWCore/Framework/interface/ESHandle.h"
#include "TrackingTools/IPTools/interface/IPTools.h"
#include "TrackingTools/Records/interface/TransientTrackRecord.h"
#include "TrackingTools/TransientTrack/interface/TransientTrackBuilder.h"


#include "EMJ/QualCheck/interface/EMJObjectSelect.hpp"


class EMJVarAOD : public usr::EDHistogramAnalyzer
{
public:
  explicit EMJVarAOD( const edm::ParameterSet& );
  ~EMJVarAOD(){}

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
  edm::EDGetToken tok_track;
  edm::EDGetToken tok_pv;
  edm::EDGetToken tok_gen;

  bool RunJetVar( const reco::PFJet&                       jet,
                  const reco::Vertex&                      pv,
                  const std::vector<reco::Track>&          tracks,
                  const std::vector<reco::TransientTrack>& ttracks,
                  const std::string&                       prefix  );

};

//
// constructors and destructor
//
EMJVarAOD::EMJVarAOD( const edm::ParameterSet& iConfig ) :
  usr::EDHistogramAnalyzer( iConfig ),
  tok_ak4jet( GetToken<std::vector<reco::PFJet> >( "AK4Jet" ) ),
  tok_track(  GetToken<std::vector<reco::Track> >( "Tracks" ) ),
  tok_pv( GetToken<std::vector<reco::Vertex> >( "PrimaryVertex" ) ),
  tok_gen( GetToken<std::vector<reco::GenParticle> >( "GenPart" ) )
{
}

void EMJVarAOD::beginJob()
{
  BookHist1D( "PVTrackF",  100,  0,   1 );

  BookHist1D( "DQ_DN",     100,  0, 100 );
  BookHist1D( "DQ_LogIP2D", 24, -5,   1 );
  BookHist1D( "DQ_NumTk",   50,  0,  50 );
  BookHist1D( "DQ_Alpha3D", 50,  0,   1 );

  BookHist1D( "SM_DN",     100,  0, 100 );
  BookHist1D( "SM_LogIP2D", 70, -5,   2 );
  BookHist1D( "SM_NumTk",  100,  0, 100 );
  BookHist1D( "SM_Alpha3D", 50,  0,   1 );
}

// Custom analysis helper functions

static std::vector<reco::Track> SelectTracks(
  const reco::Vertex&             pv,
  const std::vector<reco::Track>& tracks,
  const TransientTrackBuilder&    ttBuilder );

static std::vector<reco::TransientTrack> MakeTTList(
  const std::vector<reco::Track>& track,
  const TransientTrackBuilder&    ttBuilder  );

static double PVTrackFraction( const reco::Vertex&                      vertex,
                               const std::vector<reco::TransientTrack>& tracks );

static bool IsGoodJet( const reco::PFJet&, const reco::Candidate* darkq );

void EMJVarAOD::analyze( const edm::Event&      event,
                         const edm::EventSetup& setup )
{
  edm::Handle<std::vector<reco::Track> > trackHandle;
  edm::Handle<std::vector<reco::PFJet> > jetHandle;
  edm::Handle<std::vector<reco::Vertex> > pvHandle;
  edm::Handle<std::vector<reco::GenParticle> > genHandle;

  event.getByToken( tok_track, trackHandle );
  event.getByToken( tok_gen,     genHandle );
  event.getByToken( tok_ak4jet,  jetHandle );
  event.getByToken( tok_pv,       pvHandle );

  edm::ESHandle<TransientTrackBuilder> ttBuilder;
  setup.get<TransientTrackRecord>().get( "TransientTrackBuilder", ttBuilder );

  const int pv_idx = GetPrimaryVertex( *pvHandle );
  if( pv_idx < 0 ){ return; }
  const auto pv    = pvHandle->at( pv_idx );
  const auto darkq = FindDarkQuark( *genHandle );
  const auto smq   = FindSMQuark( *genHandle );

  const std::vector<reco::Track> tracklist
    = SelectTracks( pv, *trackHandle, *ttBuilder );
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

bool EMJVarAOD::RunJetVar(
  const reco::PFJet&                       jet,
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
    const double DN  = std::sqrt( ipz*ipz / 0.0001 + ip2d_sig*ip2d_sig );

    ip2d_mean += ip2d;
    if( DN < 10 ){
      alpha3d += track.pt();
    }
    trackpt_sum += track.pt();

    Hist( prefix + "DN" ).Fill( DN );
  }

  if( ntracks == 0 ){ return false; }

  ip2d_mean /= ntracks;
  alpha3d   /= trackpt_sum;

  Hist( prefix + "LogIP2D" ).Fill( std::log( ip2d_mean )/std::log( 10 ) );
  Hist( prefix + "NumTk" ).Fill( ntracks );
  Hist( prefix + "Alpha3D" ).Fill( alpha3d );

  return true;
}

std::vector<reco::Track> SelectTracks(
  const reco::Vertex&             pv,
  const std::vector<reco::Track>& source,
  const TransientTrackBuilder&    ttBuilder )
{
  std::vector<reco::Track> ans;

  for( const auto& track : source ){
    if( track.pt() < 1.0 ){ continue; }
    // if( !track.quality( reco::TrackBase::highPurity ) ){ continue;  }
    // Calculating impact parameter

    /*
    auto ttrack = ttBuilder.build( track );
    auto ip3d_p = IPTools::absoluteImpactParameter3D( ttrack, pv );
    auto ip2d_p = IPTools::absoluteTransverseImpactParameter( ttrack, pv );

    const double ip3d = ip3d_p.second.value();
    const double ip2d = ip2d_p.second.value();
    // const double ip2d_sig = ip2d_p.second.significance();
    const double ipz = std::sqrt( ip3d*ip3d - ip2d*ip2d );

    // if( ipz > 2.5 ){ continue; }
    */
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

  return double(passed)/double( tracks.size() );

}

bool IsGoodJet( const reco::PFJet& jet, const reco::Candidate* darkq )
{
  // Basic quality cuts
  if( fabs( jet.eta() ) > 2.0 ){ return false; }
  if( jet.chargedEmEnergyFraction() > 0.9 ){ return false; }
  if( jet.neutralEmEnergyFraction() > 0.9 ){ return false; }

  // Match with dark quark
  if( darkq && deltaR( jet, *darkq ) > 0.4 ){ return false; }

  return true;
}


void EMJVarAOD::fillDescriptions( edm::ConfigurationDescriptions&
                                  descriptions )
{
  edm::ParameterSetDescription desc;
  desc.add<edm::InputTag>(
    "AK4Jet", edm::InputTag( "ak4PFJetsCHS" ) );
  desc.add<edm::InputTag>(
    "Tracks", edm::InputTag( "generalTracks" ) );
  desc.add<edm::InputTag>(
    "PrimaryVertex", edm::InputTag( "offlinePrimaryVertices" ) );
  desc.add<edm::InputTag>(
    "GenPart", edm::InputTag( "genParticles" ) );

  descriptions.add( "EMJVarAOD", desc );
}


DEFINE_FWK_MODULE( EMJVarAOD );
