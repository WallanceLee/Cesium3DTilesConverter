//
// Created by Wallance on 2025/3/21.
//

#ifndef SHPLAYER_H
#define SHPLAYER_H
#include "OGRException.h"
#include "ShpFeature.h"

namespace scially
{
    class ShpLayer
    {
    public:
        ShpLayer(int heightIndex, OGREnvelope &envelope) : _heightIndex(heightIndex), _envelope(envelope)
        {
            _features = std::vector<ShpFeature>();
        }

        void addFeature(ShpFeature& feature)
        {
            this->_features.emplace_back(feature);
        }

        void sortFeaturesByHeight()
        {
            std::sort(this->_features.begin(), this->_features.end(), [](const ShpFeature& f1, const ShpFeature& f2)
            {
                return f1.height() > f2.height();
            });
        }

        void generateQuadTree(QuadTree *tree, int minLOD, int maxLOD);

        void initLOD(QuadTree *tree, int minLOD, int maxLOD);


    private:
        void calculateLodAndThreshold(int minLOD, int maxLOD);
        void initLOD0(QuadTree *tree, int minLOD, int maxLOD, int currentLOD);
        void initThresholds(QuadTree *tree);
        std::vector<ShpFeature> _features;
        int _heightIndex;
        OGREnvelope _envelope;
        std::map<int, double> thresholds;
    };
}

#endif //SHPLAYER_H
