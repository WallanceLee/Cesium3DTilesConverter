//
// Created by Wallance on 2025/3/21.
//

#ifndef SHPFEATURE_H
#define SHPFEATURE_H

#include <ogr_geometry.h>
#include <ogr_core.h>

namespace scially
{
    class ShpFeature
    {
    public:
        ShpFeature(GIntBig id, OGRGeometry* geometry, OGREnvelope& envelope, double height) : _id(id), _geometry(geometry->clone()),
            _envelope(envelope), _height(height)
        {
        }

        // 移动构造函数
        ShpFeature(ShpFeature&& other) noexcept
            : _id(other.id()),  _geometry(other.geometry()), _envelope(other.envelope()), _height(other.height())
        {
            other._geometry = nullptr;
        }

        // 移动赋值运算符
        ShpFeature& operator=(ShpFeature&& other) noexcept
        {
            if (this != &other)
            {
                if (_geometry)
                    OGRGeometryFactory::destroyGeometry(_geometry);

                _id = other._id;
                _geometry = other._geometry;
                _envelope = other._envelope;
                _height = other._height;

                other._geometry = nullptr;
            }
            return *this;
        }
        // 禁用拷贝构造函数和赋值运算符
        ShpFeature(const ShpFeature& other) {
            _id = other._id;
            _geometry = other._geometry;
            _envelope = other._envelope;
            _height = other.height();
        };
        ShpFeature& operator=(const ShpFeature& other)
        {
            _id = other._id;
            _geometry = other._geometry;
            _envelope = other._envelope;
            _height = other.height();
        };

        ~ShpFeature()
        {
            // if (_geometry)
            // {
            //     OGRGeometryFactory::destroyGeometry(_geometry);
            //     _geometry = nullptr;
            // }
        }
        GIntBig id() const { return _id; }
        OGRGeometry* geometry() const { return _geometry; }
        const OGREnvelope& envelope()  { return _envelope; }
        double height() const { return _height; }

    private:
        GIntBig _id;
        OGRGeometry* _geometry;
        OGREnvelope _envelope;
        double _height;
    };
}
#endif //SHPFEATURE_H
