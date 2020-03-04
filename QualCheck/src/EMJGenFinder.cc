#include "EMJ/QualCheck/interface/EMJGenFinder.hpp"

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

const reco::Candidate* FindSMQuark( const std::vector<reco::GenParticle>& vec )
{
  const reco::Candidate* smq = 0;

  for( const auto& gen : vec ){
    smq = 0;
    if( gen.pdgId() != 4900001 ){continue;}
    if( gen.numberOfDaughters() <= 1 ){continue;}

    for( unsigned i = 0; i < gen.numberOfDaughters(); ++i ){
      if( abs( gen.daughter( i )->pdgId() ) <= 6 ){
        smq = gen.daughter( i );
        break;
      }
    }

    if( !smq ){continue;}
    if( smq->numberOfDaughters() == 0 ){
      return smq;
    } else {
      while( smq->daughter( 0 )->pdgId() == smq->pdgId() ){
        smq = smq->daughter( 0 );
        return smq;
      }
    }
  }

  return smq;
}
