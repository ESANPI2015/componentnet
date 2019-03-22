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
        static const UniqueId executedOnUid;
        static const UniqueId reachableViaUid;

        Mapper(const ResourceCost::Model& rcm,
               const Software::Network& sw = Software::Network(),
               const ::Hardware::Computational::Network& hw = ::Hardware::Computational::Network()
              );
        
        static Hyperedges partitionFuncLeft (const ResourceCost::Model& rcm);
        static Hyperedges partitionFuncRight (const ResourceCost::Model& rcm);
        static float matchFunc (const ResourceCost::Model& rcm, const UniqueId& consumerUid, const UniqueId& providerUid);
        static void mapFunc (CommonConceptGraph& ccg, const UniqueId& consumerUid, const UniqueId& providerUid); 

        /* Uses the implemented functions to map software implementations to hardware processors */
        float map();
};

}

}

#endif
