#include <QFile>
#include <QuadTree.h>

#include "GeometryMesh.h"
#include "Cesium3DTiles/BaseTile.h"

namespace scially
{
    QuadTree::QuadTree(double minX, double maxX, double minY, double maxY, QuadTree* parent) : parent(parent)
    {
        envelope.MinX = minX;
        envelope.MaxX = maxX;
        envelope.MinY = minY;
        envelope.MaxY = maxY;
    }

    bool QuadTree::add(ShpFeature& feature)
    {
        if (!envelope.Intersects(feature.envelope()))
        {
            return false;
        }
        if (feature.height() >= minHeight)
        {
            geoms.append(feature);
            return true;
        }

        if (envelope.Intersects(feature.envelope()))
        {
            for (int i = 0; i < 4; i++)
            {
                //when box is added to a node, stop the loop
                if (nodes[i]->add(feature))
                {
                    return true;
                }
            }
        }
        return false;
    }

    void QuadTree::split(int minLOD, int maxLOD)
    {
        std::queue<QuadTree*> nodes;
        nodes.push(this);
        while (nodes.size() > 0)
        {
            QuadTree* first = nodes.front();
            nodes.pop();
            if (first->level == maxLOD)
            {
                break;
            }
            split0(first);
            for (QuadTree* node : first->nodes)
            {
                nodes.push(node);
            }
        }
    }

    void QuadTree::split0(QuadTree* parent)
    {
        double cX = (parent->envelope.MinX + parent->envelope.MaxX) / 2.0;
        double cY = (parent->envelope.MinY + parent->envelope.MaxY) / 2.0;
        parent->nodes.resize(4);
        for (int i = 0; i < 4; i++)
        {
            OGREnvelope box;
            switch (i)
            {
            case 0:
                box.MinX = parent->envelope.MinX;
                box.MaxX = cX;
                box.MinY = parent->envelope.MinY;
                box.MaxY = cY;

                parent->nodes[i] = new QuadTree(box, parent);
                parent->nodes[i]->setNo(parent->row * 2, parent->col * 2, parent->level + 1);
                break;
            case 1:
                box.MinX = cX;
                box.MaxX = envelope.MaxX;
                box.MinY = envelope.MinY;
                box.MaxY = cY;

                parent->nodes[i] = new QuadTree(box, parent);
                parent->nodes[i]->setNo(parent->row * 2 + 1, parent->col * 2, parent->level + 1);
                break;
            case 2:
                box.MinX = cX;
                box.MaxX = envelope.MaxX;
                box.MinY = cY;
                box.MaxY = envelope.MaxY;

                parent->nodes[i] = new QuadTree(box, parent);
                parent->nodes[i]->setNo(parent->row * 2 + 1, parent->col * 2 + 1, parent->level + 1);
                break;
            case 3:
                box.MinX = envelope.MinX;
                box.MaxX = cX;
                box.MinY = cY;
                box.MaxY = envelope.MaxY;

                parent->nodes[i] = new QuadTree(box, parent);
                parent->nodes[i]->setNo(parent->row * 2, parent->col * 2 + 1, parent->level + 1);
                break;
            }
        }
    }

    void QuadTree::generateTileset(BaseTile *tile, const QString& output)
    {
        tile->asset.assets["version"] = "1.0";
        tile->asset.assets["gltfUpAxis"] = "Z";
        bool first = true;
        double geometricError = 0;
        BoundingVolumeRegion region;

        for (int i = 0; i < this->nodes.size(); i++)
        {
            BaseTile child;
            bool result = generateTileWithChild(this->nodes[i], &child, output);
            if (result)
            {
                RootTile rootTile;
                rootTile.geometricError = child.geometricError;
                rootTile.boundingVolume = child.root.boundingVolume;
                rootTile.content.emplace();
                rootTile.content->uri = QString("./Tile_%1_%2_%3.json").arg(this->nodes[i]->level).arg(this->nodes[i]->row).arg(this->nodes[i]->col);
                tile->root.children.append(rootTile);
                if (first)
                {
                    region = child.root.boundingVolume.region.value();
                }
                else
                {
                    region.merge(child.root.boundingVolume.region.value());
                }
            }

        }

        BoundingVolumeRegion rootBounding = region;
        tile->geometricError = tile->root.children[0].geometricError * 2;
        tile->root.geometricError = tile->geometricError;
        tile->root.boundingVolume = rootBounding;

        QFile tilesetFile(QString("%1/tileset.json").arg(output));
        tilesetFile.open(QIODevice::WriteOnly);
        tilesetFile.write(QJsonDocument(tile->write()).toJson());
    }

    bool QuadTree::generateTileWithoutChild(QuadTree* root, RootTile* tile, const QString &output)
    {
        OGREnvelope nodeBox;

        // Calc All Geometry Envelope
        for (int i = 0; i < root->geomsSize(); i++)
        {
            ShpFeature& feature = root->geoms[i];
            const OGREnvelope& envelope = feature.envelope();
            if (nodeBox.IsInit())
            {
                nodeBox.Merge(envelope);
            }
            else
            {
                nodeBox = envelope;
            }
        }

        // Build 3D Model per geometry
        double centerX = (nodeBox.MinX + nodeBox.MaxX) / 2;
        double centerY = (nodeBox.MinY + nodeBox.MaxY) / 2;
        double boxWidth = (nodeBox.MaxX - nodeBox.MinX);
        double boxHeight = (nodeBox.MaxY - nodeBox.MinY);
        double maxHeight = 0;

        GeometryMesh meshes;
        for (int i = 0; i < root->geomsSize(); i++)
        {
            ShpFeature& feature = root->geoms[i];
            OGRGeometry* geometry = feature.geometry();
            const OGREnvelope& envelope = feature.envelope();
            double height = feature.height();
            maxHeight = std::max(height, maxHeight);

            if (wkbFlatten(geometry->getGeometryType()) == wkbPolygon)
            {
                OGRPolygon* polygon = (OGRPolygon*)geometry;
                meshes.add(centerX, centerY, height, polygon);
            }
            else if (wkbFlatten(geometry->getGeometryType()) == wkbMultiPolygon)
            {
                OGRMultiPolygon* multipolygon = (OGRMultiPolygon*)geometry;
                for (int j = 0; j < multipolygon->getNumGeometries(); j++)
                {
                    OGRPolygon* polygon = (OGRPolygon*)multipolygon->getGeometryRef(j);
                    meshes.add(centerX, centerY, height, polygon);;
                }
            }
            else
            {
                qWarning() << "Only support Polygon(MultiPolygon)";
            }
        }
        QByteArray b3dmBuffer = meshes.toB3DM(true);
        QString b3dmFilePath = QString("%1/Tile_%2_%3_%4.b3dm").
                     arg(output).
                     arg(root->getLevel()).
                     arg(root->getRow()).
                     arg(root->getCol());
        QFile b3dmFile = QFile(b3dmFilePath);
        if (!b3dmFile.open(QIODevice::WriteOnly))
        {
            qWarning() << "Can't write file: " << b3dmFile.fileName();
        }

        int writeBytes = b3dmFile.write(b3dmBuffer);
        if (writeBytes <= 0)
        {
            qWarning() << "Can't write file: " << b3dmFile.fileName();
        }
        b3dmFile.flush();
        b3dmFile.close();

        tile->boundingVolume = BoundingVolumeRegion::fromCenterXY(

            centerX, centerY,
            nodeBox.MaxX - nodeBox.MinX, nodeBox.MaxY - nodeBox.MinY,
            0, maxHeight);
        tile->geometricError = 0;
        tile->transform = Transform::fromXYZ(centerX, centerY, 0);
        tile->content.emplace();
        tile->content->uri = QString("./Tile_%1_%2_%3.b3dm").
                     arg(root->getLevel()).
                     arg(root->getRow()).
                     arg(root->getCol());;
        return true;
    }

    bool QuadTree::generateTileWithChild(QuadTree* tree, BaseTile* tile, const QString &output)
    {
        tile->asset.assets["version"] = "1.0";
        tile->asset.assets["gltfUpAxis"] = "Z";
        RootTile content;
        bool resultGeoms = false;
        double geometricError = 0;
        if (tree->nodes.empty() && tree->geomsSize() == 0)
        {
            return false;
        }
        BoundingVolumeRegion region;
        bool initBoundingVolume = false;
        if (tree->geomsSize() > 0)
        {
            resultGeoms = generateTileWithoutChild(tree, &content, output);
            if (resultGeoms)
            {
                tile->root.children.append(content);
                geometricError = std::max(content.geometricError, geometricError);
                region = content.boundingVolume.region.value();
                initBoundingVolume = true;
            }
        }

        bool hasChildren = false;
        for (int i = 0; i < tree->nodes.size(); i++)
        {
            BaseTile child;
            bool result = generateTileWithChild(tree->nodes[i], &child, output);
            if (result)
            {
                RootTile rootTile;
                QuadTree *childTree = tree->nodes[i];
                rootTile.content.emplace();
                rootTile.content->uri = QString("./Tile_%1_%2_%3.json").arg(childTree->level).arg(childTree->row).arg(childTree->col);
                rootTile.boundingVolume = child.root.boundingVolume.region.value();
                rootTile.geometricError = child.geometricError;
                tile->root.children.append(rootTile);
                if (!initBoundingVolume)
                {
                    region = child.root.boundingVolume.region.value();
                    initBoundingVolume = true;
                }
                else
                {
                    region.merge(child.root.boundingVolume.region.value());
                }
                hasChildren = true;
            }
        }
        if (resultGeoms || hasChildren)
        {
            tile->root.boundingVolume = region;
            tile->geometricError = region.geometricError();
            tile->root.geometricError = tile->geometricError;
            QFile childTilesetJSONFile(
                QString("%1/Tile_%2_%3_%4.json")
                .arg(output)
                .arg(tree->getLevel())
                .arg(tree->getRow())
                .arg(tree->getCol()));
            if (!childTilesetJSONFile.open(QIODevice::WriteOnly))
            {
                qWarning() << "Can't write file: " << childTilesetJSONFile.fileName();
            }
            QByteArray tileBuffer = QJsonDocument(tile->write()).toJson();
            int childTileWriteBytes = childTilesetJSONFile.write(tileBuffer);
            if (childTileWriteBytes <= 0)
            {
                qWarning() << "Can't write file: " << childTilesetJSONFile.fileName();
            }
            childTilesetJSONFile.flush();
            childTilesetJSONFile.close();

            return true;
        }

        return false;


    }
}
