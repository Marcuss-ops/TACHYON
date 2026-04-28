#pragma once

#include "tachyon/core/math/algebra/vector2.h"
#include <cmath>

namespace tachyon {
namespace math {

struct Matrix3x3 {
    float m[3][3] = {{1,0,0},{0,1,0},{0,0,1}};
    
    Matrix3x3 operator*(const Matrix3x3& other) const {
        Matrix3x3 result;
        for(int i=0;i<3;i++) for(int j=0;j<3;j++) { result.m[i][j]=0; for(int k=0;k<3;k++) result.m[i][j]+=m[i][k]*other.m[k][j]; }
        return result;
    }
    
    static Matrix3x3 make_translation(const Vector2& v) {
        Matrix3x3 m;
        m.m[0][2]=v.x; m.m[1][2]=v.y;
        return m;
    }
    
    static Matrix3x3 make_rotation(float angle_rad) {
        Matrix3x3 m;
        float c=cosf(angle_rad), s=sinf(angle_rad);
        m.m[0][0]=c; m.m[0][1]=-s;
        m.m[1][0]=s; m.m[1][1]=c;
        return m;
    }
    
    static Matrix3x3 make_scale(float sx, float sy) {
        Matrix3x3 m;
        m.m[0][0]=sx; m.m[1][1]=sy;
        return m;
    }
    
    Matrix3x3 inverse() const {
        Matrix3x3 inv;
        float det=m[0][0]*(m[1][1]*m[2][2]-m[1][2]*m[2][1])
                -m[0][1]*(m[1][0]*m[2][2]-m[1][2]*m[2][0])
                +m[0][2]*(m[1][0]*m[2][1]-m[1][1]*m[2][0]);
        if(fabs(det)<1e-8) return *this;
        float invdet=1.0f/det;
        inv.m[0][0]=(m[1][1]*m[2][2]-m[1][2]*m[2][1])*invdet;
        inv.m[0][1]=(m[0][2]*m[2][1]-m[0][1]*m[2][2])*invdet;
        inv.m[0][2]=(m[0][1]*m[1][2]-m[0][2]*m[1][1])*invdet;
        inv.m[1][0]=(m[1][2]*m[2][0]-m[1][0]*m[2][2])*invdet;
        inv.m[1][1]=(m[0][0]*m[2][2]-m[0][2]*m[2][0])*invdet;
        inv.m[1][2]=(m[0][2]*m[1][0]-m[0][0]*m[1][2])*invdet;
        inv.m[2][0]=(m[1][0]*m[2][1]-m[1][1]*m[2][0])*invdet;
        inv.m[2][1]=(m[0][1]*m[2][0]-m[0][0]*m[2][1])*invdet;
        inv.m[2][2]=(m[0][0]*m[1][1]-m[0][1]*m[1][0])*invdet;
        return inv;
    }
};

} // namespace math
} // namespace tachyon

