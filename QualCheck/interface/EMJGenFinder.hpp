#ifndef EMJ_GENFINDER_HPP
#define EMJ_GENFINDER_HPP

// #include "DataFormats/Candidate/interface/Candidate.h"
#include "DataFormats/HepMCCandidate/interface/GenParticle.h"

const reco::Candidate* FindDarkQuark( const std::vector<reco::GenParticle>& );
const reco::Candidate* FindSMQuark( const std::vector<reco::GenParticle>& );


#endif
