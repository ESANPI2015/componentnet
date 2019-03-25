#ifndef _MAPPER_HPP
#define _MAPPER_HPP

#include "SoftwareNetwork.hpp"
#include "HardwareComputationalNetwork.hpp"
#include "ResourceCostModel.hpp"

namespace Software
{

namespace Hardware
{

/*
*/

class Mapper : public ResourceCost::Model
{
    public:
        static const UniqueId ExecutedOnUid;
        static const UniqueId ReachableViaUid;

        Mapper(const ResourceCost::Model& rcm,
               const Software::Network& sw = Software::Network(),
               const ::Hardware::Computational::Network& hw = ::Hardware::Computational::Network()
              );
        
        static Hyperedges implementations (const ResourceCost::Model& rcm);
        static Hyperedges processors (const ResourceCost::Model& rcm);
        static float matchImplementationAndProcessor (const ResourceCost::Model& rcm, const UniqueId& consumerUid, const UniqueId& providerUid);
        static void mapImplementationToProcessor (CommonConceptGraph& ccg, const UniqueId& consumerUid, const UniqueId& providerUid); 

        /* Uses the implemented functions to map software implementations to hardware processors */
        float mapAllImplementationsToProcessors();

        static Hyperedges swInterfaces (const ResourceCost::Model& rcm);
        static Hyperedges hwInterfaces (const ResourceCost::Model& rcm);
        static float matchSwToHwInterface (const ResourceCost::Model& rcm, const UniqueId& consumerUid, const UniqueId& providerUid);
        static void mapSwToHwInterface (CommonConceptGraph& ccg, const UniqueId& consumerUid, const UniqueId& providerUid); 
        
        /* Uses the implemented functions to map software to hardware interfaces */
        float mapAllSwAndHwInterfaces();

        /*
            I. map implementations to processors
            II. map sw to hw interfaces
        */
        float map();
};

}

}

#endif
