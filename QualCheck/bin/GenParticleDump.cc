#include "UserUtils/Common/interface/ArgumentExtender.hpp"
#include "UserUtils/Common/interface/STLUtils/OStreamUtils.hpp"

#include "DataFormats/FWLite/interface/ChainEvent.h"
#include "DataFormats/FWLite/interface/Handle.h"
#include "DataFormats/HepMCCandidate/interface/GenParticle.h"
#include "DataFormats/Math/interface/deltaR.h"
#include "DataFormats/PatCandidates/interface/PackedCandidate.h"

#include "TFile.h"
#include "TH1D.h"

int main( int argc, char* argv[] )
{
  usr::po::options_description desc( "Generating Relevent plot objects for Track"
                                     "quality control" );
  desc.add_options()
    ( "input", usr::po::multivalue<std::string>(),
    "Input MiniAOD Files" )
  ;

  usr::ArgumentExtender args;
  args.AddOptions( desc );
  args.ParseOptions( argc, argv );

  const std::vector<std::string> input = args.ArgList<std::string>( "input" );

  fwlite::ChainEvent evt( input );
  fwlite::Handle<std::vector<reco::GenParticle> > genHandle;

  unsigned evt_count = 0;

  for( evt.toBegin(); !evt.atEnd(); ++evt, ++evt_count ){
    usr::fout( "Event [%8d/%8d]\n", evt_count+1, evt.size() );
    genHandle.getByLabel( evt, "prunedGenParticles" );

    unsigned gen_index = 0;

    for( const auto& gen : genHandle.ref() ){
      if( fabs( gen.pdgId() ) > 4900000 ){
        usr::fout( "%5d | %10d %5d | %6.2lf %6.2lf %6.2lf | %10d %10d\n"
                 , gen_index
                 , gen.pdgId(), gen.status()
                 , gen.pt(), gen.eta(), gen.phi()
                 , gen.numberOfDaughters() > 0 ? gen.daughter( 0 )->pdgId() : 0
                 , gen.numberOfDaughters() > 1 ? gen.daughter( 1 )->pdgId() : 0
          );
      }
      gen_index++;
    }

  }

  return 0;
}
