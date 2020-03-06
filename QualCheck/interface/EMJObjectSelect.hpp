#ifndef EMJ_OBJECT_SELECT_HPP
#define EMJ_OBJECT_SELECT_HPP

#include "DataFormats/HepMCCandidate/interface/GenParticle.h"
#include "DataFormats/TrackReco/interface/Track.h"
#include "DataFormats/VertexReco/interface/Vertex.h"

#include <vector>

const reco::Candidate* FindDarkQuark( const std::vector<reco::GenParticle>& );
const reco::Candidate* FindSMQuark( const std::vector<reco::GenParticle>& );

int GetPrimaryVertex( const std::vector<reco::Vertex>& pvlist );


#endif
