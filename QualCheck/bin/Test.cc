#include "UserUtils/Common/interface/ArgumentExtender.hpp"
#include "UserUtils/Common/interface/STLUtils/OStreamUtils.hpp"

#include "DataFormats/FWLite/interface/Event.h"
#include "DataFormats/FWLite/interface/Handle.h"
#include "DataFormats/Math/interface/deltaR.h"
#include "DataFormats/PatCandidates/interface/Jet.h"
#include "DataFormats/PatCandidates/interface/PackedCandidate.h"

#include "TFile.h"

int main( int argc, char* argv[] )
{
  TFile* f = TFile::Open( argv[1] );
  fwlite::Event evt( f );

  fwlite::Handle<std::vector<pat::PackedCandidate> > lostTrackHandle;
  fwlite::Handle<std::vector<pat::PackedCandidate> > pfCandHandle;
  unsigned evt_count = 0;

  for( evt.toBegin(); !evt.atEnd(); ++evt, ++evt_count ){
    usr::fout( "Event [%8d/%8d]\n", evt_count+1, evt.size() );
    lostTrackHandle.getByLabel( evt, "lostTracks" );
    pfCandHandle.getByLabel( evt, "packedPFCandidates" );

    for( const auto& lostTrack : lostTrackHandle.ref() ){

      if( !lostTrack.hasTrackDetails() ){ continue; }
      const auto& track = lostTrack.pseudoTrack();
      std::cout << track.referencePoint().x() << " "
                << track.referencePoint().y() << " "
                << track.referencePoint().z() << std::endl;
    }

  }

  return 0;
}
