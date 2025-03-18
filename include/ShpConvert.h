#pragma once

#include <QString>

namespace scially
{
    class ShpConvert
    {
    public:
        ShpConvert(const QString& fileName, const QString& layerName, const QString& heightField)
            : fileName(fileName), layerName(layerName), heightField(heightField)
        {
        }

        void convertTiles(const QString& output, int minLOD, int maxLOD);

    private:
        QString fileName;
        QString layerName;
        QString heightField;
    };

}
