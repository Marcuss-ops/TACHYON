#include <array>
#include <algorithm>
#include <cmath>
#include <limits>

namespace tachyon::tracker {

template <std::size_t N>
using Mat = std::array<std::array<float, N>, N>;

template <std::size_t N>
using Vec = std::array<float, N>;

template <std::size_t N>
Mat<N> identity_matrix() {
    Mat<N> out{};
    for (std::size_t i = 0; i < N; ++i) {
        out[i][i] = 1.0f;
    }
    return out;
}

template <std::size_t N>
Mat<N> transpose(const Mat<N>& m) {
    Mat<N> out{};
    for (std::size_t r = 0; r < N; ++r) {
        for (std::size_t c = 0; c < N; ++c) {
            out[c][r] = m[r][c];
        }
    }
    return out;
}

template <std::size_t N>
Mat<N> multiply(const Mat<N>& a, const Mat<N>& b) {
    Mat<N> out{};
    for (std::size_t r = 0; r < N; ++r) {
        for (std::size_t c = 0; c < N; ++c) {
            float sum = 0.0f;
            for (std::size_t k = 0; k < N; ++k) {
                sum += a[r][k] * b[k][c];
            }
            out[r][c] = sum;
        }
    }
    return out;
}

template <std::size_t N>
Vec<N> multiply(const Mat<N>& a, const Vec<N>& v) {
    Vec<N> out{};
    for (std::size_t r = 0; r < N; ++r) {
        float sum = 0.0f;
        for (std::size_t k = 0; k < N; ++k) {
            sum += a[r][k] * v[k];
        }
        out[r] = sum;
    }
    return out;
}

float determinant3x3(const Mat<3>& m) {
    return m[0][0] * (m[1][1] * m[2][2] - m[1][2] * m[2][1])
         - m[0][1] * (m[1][0] * m[2][2] - m[1][2] * m[2][0])
         + m[0][2] * (m[1][0] * m[2][1] - m[1][1] * m[2][0]);
}

template <std::size_t N>
void sort_eigensystem(Mat<N>& eigenvectors, Vec<N>& eigenvalues) {
    std::array<std::size_t, N> order{};
    for (std::size_t i = 0; i < N; ++i) {
        order[i] = i;
    }

    std::sort(order.begin(), order.end(), [&](std::size_t a, std::size_t b) {
        return eigenvalues[a] < eigenvalues[b];
    });

    Mat<N> vecs = eigenvectors;
    Vec<N> vals = eigenvalues;
    for (std::size_t i = 0; i < N; ++i) {
        eigenvalues[i] = vals[order[i]];
        for (std::size_t r = 0; r < N; ++r) {
            eigenvectors[r][i] = vecs[r][order[i]];
        }
    }
}

template <std::size_t N>
void jacobi_eigen_symmetric(Mat<N>& a, Mat<N>& eigenvectors, Vec<N>& eigenvalues) {
    eigenvectors = identity_matrix<N>();

    const std::size_t max_iterations = 64 * N;
    for (std::size_t iter = 0; iter < max_iterations; ++iter) {
        std::size_t p = 0;
        std::size_t q = 1;
        float max_offdiag = std::abs(a[p][q]);

        for (std::size_t i = 0; i < N; ++i) {
            for (std::size_t j = i + 1; j < N; ++j) {
                const float v = std::abs(a[i][j]);
                if (v > max_offdiag) {
                    max_offdiag = v;
                    p = i;
                    q = j;
                }
            }
        }

        if (max_offdiag < 1.0e-8f) {
            break;
        }

        const float app = a[p][p];
        const float aqq = a[q][q];
        const float apq = a[p][q];
        const float tau = (aqq - app) / (2.0f * apq);
        const float t = (tau >= 0.0f)
            ? 1.0f / (tau + std::sqrt(1.0f + tau * tau))
            : -1.0f / (-tau + std::sqrt(1.0f + tau * tau));
        const float c = 1.0f / std::sqrt(1.0f + t * t);
        const float s = t * c;

        for (std::size_t k = 0; k < N; ++k) {
            if (k == p || k == q) continue;
            const float aik = a[k][p];
            const float ajk = a[k][q];
            a[k][p] = a[p][k] = c * aik - s * ajk;
            a[k][q] = a[q][k] = s * aik + c * ajk;
        }

        const float app_new = c * c * app - 2.0f * s * c * apq + s * s * aqq;
        const float aqq_new = s * s * app + 2.0f * s * c * apq + c * c * aqq;
        a[p][p] = app_new;
        a[q][q] = aqq_new;
        a[p][q] = a[q][p] = 0.0f;

        for (std::size_t k = 0; k < N; ++k) {
            const float vip = eigenvectors[k][p];
            const float viq = eigenvectors[k][q];
            eigenvectors[k][p] = c * vip - s * viq;
            eigenvectors[k][q] = s * vip + c * viq;
        }
    }

    for (std::size_t i = 0; i < N; ++i) {
        eigenvalues[i] = a[i][i];
    }
    sort_eigensystem(eigenvectors, eigenvalues);
}

struct SVD3 {
    Mat<3> U{};
    Vec<3> S{};
    Mat<3> V{};
};

SVD3 svd3(const Mat<3>& input) {
    SVD3 svd;

    Mat<3> ata{};
    const Mat<3> at = transpose(input);
    ata = multiply(at, input);

    Mat<3> v = identity_matrix<3>();
    Vec<3> evals{};
    jacobi_eigen_symmetric(ata, v, evals);

    // Convert eigenvalues to singular values and sort descending.
    std::array<std::size_t, 3> order{0, 1, 2};
    std::sort(order.begin(), order.end(), [&](std::size_t a, std::size_t b) {
        return evals[a] > evals[b];
    });

    Mat<3> sorted_v{};
    for (std::size_t c = 0; c < 3; ++c) {
        const std::size_t idx = order[c];
        svd.S[c] = std::sqrt(std::max(evals[idx], 0.0f));
        for (std::size_t r = 0; r < 3; ++r) {
            sorted_v[r][c] = v[r][idx];
        }
    }
    svd.V = sorted_v;

    auto orthogonalize_against = [](Vec3& v, const Vec3& basis) {
        const float dot = Vec3::dot(v, basis);
        v -= basis * dot;
    };

    for (std::size_t c = 0; c < 3; ++c) {
        Vec3 col{0.0f, 0.0f, 0.0f};
        if (svd.S[c] > 1.0e-8f) {
            col.x = (input[0][0] * svd.V[0][c] + input[0][1] * svd.V[1][c] + input[0][2] * svd.V[2][c]) / svd.S[c];
            col.y = (input[1][0] * svd.V[0][c] + input[1][1] * svd.V[1][c] + input[1][2] * svd.V[2][c]) / svd.S[c];
            col.z = (input[2][0] * svd.V[0][c] + input[2][1] * svd.V[1][c] + input[2][2] * svd.V[2][c]) / svd.S[c];
        } else {
            col = (c == 0) ? Vec3{1.0f, 0.0f, 0.0f}
                     : (c == 1) ? Vec3{0.0f, 1.0f, 0.0f}
                                : Vec3{0.0f, 0.0f, 1.0f};
        }

        if (c > 0) {
            for (std::size_t prev = 0; prev < c; ++prev) {
                Vec3 prev_col{svd.U[0][prev], svd.U[1][prev], svd.U[2][prev]};
                orthogonalize_against(col, prev_col);
            }
        }
        const float norm = col.length();
        if (norm > 1.0e-8f) {
            col /= norm;
        }
        svd.U[0][c] = col.x;
        svd.U[1][c] = col.y;
        svd.U[2][c] = col.z;
    }

    // Re-orthonormalize U so the decomposition is stable enough for pose use.
    Vec3 u0{svd.U[0][0], svd.U[1][0], svd.U[2][0]};
    Vec3 u1{svd.U[0][1], svd.U[1][1], svd.U[2][1]};
    Vec3 u2 = Vec3::cross(u0, u1);
    if (u0.length_squared() < 1.0e-8f) u0 = {1.0f, 0.0f, 0.0f};
    if (u1.length_squared() < 1.0e-8f) u1 = {0.0f, 1.0f, 0.0f};
    u0 = u0.normalized();
    u1 = (u1 - u0 * Vec3::dot(u1, u0)).normalized();
    if (u1.length_squared() < 1.0e-8f) {
        u1 = Vec3::cross({0.0f, 0.0f, 1.0f}, u0).normalized();
        if (u1.length_squared() < 1.0e-8f) {
            u1 = Vec3::cross({0.0f, 1.0f, 0.0f}, u0).normalized();
        }
    }
    u2 = Vec3::cross(u0, u1).normalized();

    svd.U[0][0] = u0.x; svd.U[1][0] = u0.y; svd.U[2][0] = u0.z;
    svd.U[0][1] = u1.x; svd.U[1][1] = u1.y; svd.U[2][1] = u1.z;
    svd.U[0][2] = u2.x; svd.U[1][2] = u2.y; svd.U[2][2] = u2.z;

    if (determinant3x3(svd.U) < 0.0f) {
        for (std::size_t r = 0; r < 3; ++r) {
            svd.U[r][2] = -svd.U[r][2];
        }
    }
    if (determinant3x3(svd.V) < 0.0f) {
        for (std::size_t r = 0; r < 3; ++r) {
            svd.V[r][2] = -svd.V[r][2];
        }
    }

    return svd;
}

} // namespace tachyon::tracker
