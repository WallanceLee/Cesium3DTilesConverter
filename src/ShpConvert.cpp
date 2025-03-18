#include "ShpConvert.h"

#include <cmath>

#include "GDALWrapper.h"
#include "QuadTree.h"
#include "ShpFeature.h"
#include "ShpLayer.h"
#include "TilesConvertException.h"

namespace scially
{
    void ShpLayer::generateQuadTree(QuadTree* tree, int minLOD, int maxLOD)
    {
        tree->setEnvelope(this->_envelope);
        tree->setNo(0, 0, minLOD - 1);
        tree->split(minLOD, maxLOD);
        this->initLOD(tree, minLOD, maxLOD);
        for (auto& feature : this->_features)
        {
            tree->add(feature);
        }
    }

    void ShpLayer::initLOD(QuadTree* tree, int minLOD, int maxLOD)
    {
        this->calculateLodAndThreshold(minLOD, maxLOD);
        this->initLOD0(tree, minLOD, maxLOD, minLOD);
        this->initThresholds(tree);
    }

    void ShpLayer::initThresholds(QuadTree* tree)
    {
        std::queue<QuadTree*> nodes;
        nodes.push(tree);
        while (!nodes.empty())
        {
            QuadTree* current = nodes.front();
            nodes.pop();
            double threshold = this->thresholds.find(current->getLevel()) != this->thresholds.end()
                                   ? this->thresholds[current->getLevel()]
                                   : std::numeric_limits<double>::infinity();
            current->setMinHeight(threshold);
            QVector<QuadTree*> children = current->children();
            if (!children.isEmpty())
            {
                for (QuadTree* child : children)
                {
                    nodes.push(child);
                }
            }
        }
    }

    void ShpLayer::calculateLodAndThreshold(int minLOD, int maxLOD)
    {
        this->thresholds = std::map<int, double>();
        int featureCount = this->_features.size();
        int lodCount = maxLOD - minLOD + 1;
        int offset = 0;
        for (int i = 0; i < lodCount - 1; i++)
        {
            int lodSliceSize = featureCount >> (lodCount - i);
            int lod = minLOD + i;
            double threshold = this->_features[offset + lodSliceSize].height();
            this->thresholds[lod] = threshold;
            offset += lodSliceSize;
        }
        this->thresholds[maxLOD] = 0;
    }


    void ShpLayer::initLOD0(QuadTree* tree, int minLOD, int maxLOD, int currentLOD)
    {
        tree->split(minLOD, maxLOD);
    }

    void ShpConvert::convertTiles(const QString& output, int minLOD, int maxLOD)
    {
        GDALDatasetWrapper ds = GDALDatasetWrapper::open(fileName.toStdString().data(), 1);
        OGRLayerWrapper layer = ds.GetLayerByName(layerName.toStdString().data());
        int heightIndex = layer.GetLayerDefn()->GetFieldIndex(heightField.toStdString().data());
        if (heightIndex == -1)
            throw TilesConvertException(heightField + "not found in layer");

        // 初始化开始
        layer.ResetReading();
        OGREnvelope layerEnvelope = layer.GetExtent();
        ShpLayer shpLayer = ShpLayer(heightIndex, layerEnvelope);

        OGRFeatureWrapper feature = layer.GetNextFeature();
        while (feature.isValid())
        {
            OGRGeometry* geometry = feature.GetGeometryRef();
            if (geometry == nullptr)
                continue;

            OGREnvelope envelope;
            geometry->getEnvelope(&envelope);
            double height = feature.GetFieldAsDouble(heightIndex);
            ShpFeature shp_feature = ShpFeature(feature.GetFID(), geometry, envelope, height);
            shpLayer.addFeature(shp_feature);
            feature = layer.GetNextFeature();
        }
        shpLayer.sortFeaturesByHeight();

        QuadTree tree;
        shpLayer.generateQuadTree(&tree, minLOD, maxLOD);

        BaseTile tile;
        tree.generateTileset(&tile, output);

    }
}
