#pragma once

#include <QString>

namespace scially {

    class ShpConvertJob{
    public:
        ShpConvertJob(const QString& input, const QString& layerName, const QString &output, const QString& height, const int minLOD = 10, const int maxLOD = 18)
            :input(input), output(output), height(height), layerName(layerName), minLOD(minLOD), maxLOD(maxLOD) {}

        void run();

    private:
        QString input;
        QString output;
        QString height;
        QString layerName;
        int minLOD = 10;
        int maxLOD = 20;
    };

   
}
