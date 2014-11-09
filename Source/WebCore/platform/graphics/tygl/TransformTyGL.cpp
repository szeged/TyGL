/*
 * Copyright (C) 2013, 2014 University of Szeged. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY UNIVERSITY OF SZEGED ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL UNIVERSITY OF SZEGED OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "TransformTyGL.h"

namespace WebCore {
namespace TyGL {

const AffineTransform::Transform AffineTransform::kIdentity = { { 1, 0, 0, 1, 0, 0 } };

inline void AffineTransform::inlineAutoDetectType()
{
    if (m_transform.m_matrix[0] == 1 && !m_transform.m_matrix[1]
            && !m_transform.m_matrix[2] && m_transform.m_matrix[3] == 1) {
        m_type = Move;
        return;
    }

    m_type = Affine;
}

void AffineTransform::multiply(const Transform& transform)
{
    float a = m_transform.m_matrix[0];
    float b = m_transform.m_matrix[1];
    float c = m_transform.m_matrix[2];
    float d = m_transform.m_matrix[3];

    m_transform.m_matrix[0] = a * transform.m_matrix[0] + c * transform.m_matrix[1];
    m_transform.m_matrix[1] = b * transform.m_matrix[0] + d * transform.m_matrix[1];
    m_transform.m_matrix[2] = a * transform.m_matrix[2] + c * transform.m_matrix[3];
    m_transform.m_matrix[3] = b * transform.m_matrix[2] + d * transform.m_matrix[3];
    if (transform.m_matrix[4] || transform.m_matrix[5]) {
        m_transform.m_matrix[4] += a * transform.m_matrix[4] + c * transform.m_matrix[5];
        m_transform.m_matrix[5] += b * transform.m_matrix[4] + d * transform.m_matrix[5];
    }

    inlineAutoDetectType();
}

void AffineTransform::translate(float x, float y)
{
    if (m_type == Move) {
        m_transform.m_matrix[4] += x;
        m_transform.m_matrix[5] += y;
        return;
    }

    Transform matrix = { { 1, 0, 0, 1, x, y } };
    multiply(matrix);
}

void AffineTransform::scale(float sx, float sy)
{
    Transform matrix = { { sx, 0, 0, sy, 0, 0 } };
    multiply(matrix);
}

void AffineTransform::rotate(float angle)
{
    float cosAngle = cos(angle);
    float sinAngle = sin(angle);
    Transform matrix = { { cosAngle, sinAngle, -sinAngle, cosAngle, 0, 0 } };
    multiply(matrix);
}

void AffineTransform::autoDetectType()
{
    inlineAutoDetectType();
}

AffineTransform AffineTransform::inverse() const
{
    AffineTransform result;
    if (m_type == Move) {
        result.m_transform.m_matrix[4] = -m_transform.m_matrix[4];
        result.m_transform.m_matrix[5] = -m_transform.m_matrix[5];
        return result;
    }

    float determinant = m_transform.m_matrix[0] * m_transform.m_matrix[3] - m_transform.m_matrix[1] * m_transform.m_matrix[2];
    if (!determinant)
        return result;

    result.m_transform.m_matrix[0] = m_transform.m_matrix[3] / determinant;
    result.m_transform.m_matrix[1] = -m_transform.m_matrix[1] / determinant;
    result.m_transform.m_matrix[2] = -m_transform.m_matrix[2] / determinant;
    result.m_transform.m_matrix[3] = m_transform.m_matrix[0] / determinant;
    result.m_transform.m_matrix[4] = (m_transform.m_matrix[2] * m_transform.m_matrix[5]
        - m_transform.m_matrix[3] * m_transform.m_matrix[4]) / determinant;
    result.m_transform.m_matrix[5] = (m_transform.m_matrix[1] * m_transform.m_matrix[4]
        - m_transform.m_matrix[0] * m_transform.m_matrix[5]) / determinant;

    return result;
}

} // namespace TyGL
} // namespace WebCore
