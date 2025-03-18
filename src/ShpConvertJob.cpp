#include <ShpConvertJob.h>
#include <ShpConvert.h>

#include <QtDebug>

namespace scially {
	void ShpConvertJob::run() {
        try{
            ShpConvert convert(input, layerName, height);
            convert.convertTiles(output, minLOD, maxLOD);
        }catch(...){
            qCritical() << "Unkown error";
        }

	}
}
