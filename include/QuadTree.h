#pragma once

#include <ogrsf_frmts.h>

#include <functional>

#include <QVector>
#include <QString>

#include "ShpFeature.h"
#include "Cesium3DTiles/BaseTile.h"

namespace scially
{
    class QuadTree
    {
    public:
        QuadTree() = default;
        QuadTree(double minX, double maxX, double minY, double maxY, QuadTree* parent = nullptr);

        QuadTree(const OGREnvelope& e, QuadTree* parent = nullptr): envelope(e), parent(parent)
        {
        }

        QVector<QuadTree*> children() { return this->nodes; }

        bool add(ShpFeature& feature);

        void setMinHeight(double minHeight)
        {
            this->minHeight = minHeight;
        }

        void setNo(int r, int c, int l)
        {
            row = r;
            col = c;
            level = l;
        }

        void setEnvelope(const OGREnvelope& e)
        {
            envelope.MinX = e.MinX;
            envelope.MaxX = e.MaxX;
            envelope.MinY = e.MinY;
            envelope.MaxY = e.MaxY;
        }

        void generateTileset(BaseTile* tile, const QString& output);

        int geomsSize() const { return geoms.size(); }
        int getRow() const { return row; }
        int getCol() const { return col; }
        const int getLevel() { return level; }
        void split(int minLOD, int maxLOD);
        void writeTileset(const QString& output);

        virtual ~QuadTree()
        {
            for (auto iter = nodes.begin(); iter != nodes.end(); iter++)
            {
                if (*iter != nullptr)
                    delete *iter;
            }
        }

    private:
        void split0(QuadTree* parent);
        bool generateTileWithoutChild(QuadTree* tree, RootTile* tile, const QString& output);
        bool generateTileWithChild(QuadTree* tree, BaseTile* tile, const QString& output);
        OGREnvelope envelope;
        QuadTree* parent = nullptr;
        QVector<QuadTree*> nodes;
        int row = 0;
        int col = 0;
        int level = 0;
        double minHeight = 512;
        double centerX = 0;
        double centerY = 0;
        double length = 0;
        double width = 0;
        double height = 0;
        QVector<ShpFeature> geoms;
    };
}
