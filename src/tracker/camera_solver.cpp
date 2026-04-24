#include "tachyon/tracker/camera_solver.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>

namespace tachyon::tracker {

// ---------------------------------------------------------------------------
// CameraTrack::apply_to
// ---------------------------------------------------------------------------

void CameraTrack::apply_to(camera::CameraState& state, float time) const {
    if (keyframes.empty()) return;

    if (time <= keyframes.front().time) {
        const auto& kf = keyframes.front();
        state.transform.position = kf.position;
        state.transform.rotation = kf.rotation;
        state.focal_length_mm = kf.focal_length_mm;
        return;
    }

    if (time >= keyframes.back().time) {
        const auto& kf = keyframes.back();
        state.transform.position = kf.position;
        state.transform.rotation = kf.rotation;
        state.focal_length_mm = kf.focal_length_mm;
        return;
    }

    auto it = std::lower_bound(
        keyframes.begin(), keyframes.end(), time,
        [](const CameraTrackKeyframe& kf, float t) { return kf.time < t; });

    if (it == keyframes.begin()) {
        const auto& kf = keyframes.front();
        state.transform.position = kf.position;
        state.transform.rotation = kf.rotation;
        state.focal_length_mm = kf.focal_length_mm;
        return;
    }

    const CameraTrackKeyframe& b = *it;
    const CameraTrackKeyframe& a = *(it - 1);
    if (a.time == b.time) {
        state.transform.position = a.position;
        state.transform.rotation = a.rotation;
        state.focal_length_mm = a.focal_length_mm;
        return;
    }

    const float t = (time - a.time) / (b.time - a.time);
    state.transform.position = {
        a.position.x + t * (b.position.x - a.position.x),
        a.position.y + t * (b.position.y - a.position.y),
        a.position.z + t * (b.position.z - a.position.z)
    };
    state.transform.rotation = math::Quaternion::slerp(a.rotation, b.rotation, t);
    state.focal_length_mm = a.focal_length_mm + t * (b.focal_length_mm - a.focal_length_mm);
}

namespace {

constexpr float kSampleTimeEpsilon = 1.0e-4f;
constexpr float kConfidenceThreshold = 0.1f;

template <std::size_t N>
using Mat = std::array<std::array<float, N>, N>;

template <std::size_t N>
using Vec = std::array<float, N>;

using Vec2 = math::Vector2;
using Vec3 = math::Vector3;

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

std::optional<Vec2> lookup_sample_position(const Track& track, float time) {
    for (const auto& sample : track.samples) {
        if (std::abs(sample.time - time) <= kSampleTimeEpsilon && sample.confidence >= kConfidenceThreshold) {
            return Vec2{sample.position.x, sample.position.y};
        }
    }
    return std::nullopt;
}

struct Correspondence {
    Vec2 p_prev{};
    Vec2 p_curr{};
};

std::vector<Correspondence> build_correspondences(
    const std::vector<Track>& tracks,
    float t_prev,
    float t_curr) {
    std::vector<Correspondence> out;
    out.reserve(tracks.size());

    for (const auto& track : tracks) {
        const auto prev = lookup_sample_position(track, t_prev);
        const auto curr = lookup_sample_position(track, t_curr);
        if (prev && curr) {
            out.push_back({*prev, *curr});
        }
    }

    return out;
}

Vec2 normalize_point(Vec2 p, int w, int h) {
    const float half_w = std::max(1.0f, static_cast<float>(w) * 0.5f);
    const float half_h = std::max(1.0f, static_cast<float>(h) * 0.5f);
    return {
        (p.x - half_w) / half_w,
        (p.y - half_h) / half_h
    };
}

bool fundamental_matrix_8pt(const std::vector<Correspondence>& corr, Mat<3>& F, int w, int h) {
    if (corr.size() < 8 || w <= 0 || h <= 0) {
        return false;
    }

    const std::size_t n = corr.size();
    std::vector<float> A(n * 9, 0.0f);
    for (std::size_t i = 0; i < n; ++i) {
        const Vec2 p0 = normalize_point(corr[i].p_prev, w, h);
        const Vec2 p1 = normalize_point(corr[i].p_curr, w, h);
        float* row = &A[i * 9];
        row[0] = p1.x * p0.x;
        row[1] = p1.x * p0.y;
        row[2] = p1.x;
        row[3] = p1.y * p0.x;
        row[4] = p1.y * p0.y;
        row[5] = p1.y;
        row[6] = p0.x;
        row[7] = p0.y;
        row[8] = 1.0f;
    }

    Mat<9> ata{};
    for (std::size_t r = 0; r < 9; ++r) {
        for (std::size_t c = 0; c < 9; ++c) {
            float sum = 0.0f;
            for (std::size_t i = 0; i < n; ++i) {
                sum += A[i * 9 + r] * A[i * 9 + c];
            }
            ata[r][c] = sum;
        }
    }

    Mat<9> eigenvectors = identity_matrix<9>();
    Vec<9> eigenvalues{};
    jacobi_eigen_symmetric(ata, eigenvectors, eigenvalues);

    // Smallest eigenvector gives the null-space of A.
    const std::size_t idx = 0;
    Vec<9> f{};
    for (std::size_t i = 0; i < 9; ++i) {
        f[i] = eigenvectors[i][idx];
    }

    F = {{
        {f[0], f[1], f[2]},
        {f[3], f[4], f[5]},
        {f[6], f[7], f[8]}
    }};

    // Enforce rank-2 constraint on F.
    const SVD3 svd = svd3(F);
    const float s0 = svd.S[0];
    const float s1 = svd.S[1];
    const float s_avg = 0.5f * (s0 + s1);

    Mat<3> S = identity_matrix<3>();
    S[0][0] = s_avg;
    S[1][1] = s_avg;
    S[2][2] = 0.0f;

    const Mat<3> U_S = multiply(svd.U, S);
    F = multiply(U_S, transpose(svd.V));
    return true;
}

void compute_K(Mat<3>& K, const CameraSolver::Config& cfg) {
    K = identity_matrix<3>();

    const float sensor_width = (cfg.sensor_width_mm > 1.0e-6f) ? cfg.sensor_width_mm : 36.0f;
    const float focal = cfg.initial_focal_length_mm.value_or(35.0f);
    const float fx = static_cast<float>(cfg.frame_width) * (focal / sensor_width);
    const float fy = fx;
    const float cx = static_cast<float>(cfg.frame_width) * 0.5f;
    const float cy = static_cast<float>(cfg.frame_height) * 0.5f;

    K[0][0] = fx;
    K[1][1] = fy;
    K[0][2] = cx;
    K[1][2] = cy;
}

Mat<3> essential_from_fundamental(const Mat<3>& F, const Mat<3>& K) {
    const Mat<3> Kt = transpose(K);
    return multiply(multiply(Kt, F), K);
}

struct PoseCandidate {
    Mat<3> R{};
    Vec3 t{};
};

void decompose_essential(const Mat<3>& E, PoseCandidate out[4]) {
    const SVD3 svd = svd3(E);
    Mat<3> U = svd.U;
    Mat<3> V = svd.V;

    if (determinant3x3(U) < 0.0f) {
        for (std::size_t r = 0; r < 3; ++r) {
            U[r][2] = -U[r][2];
        }
    }
    if (determinant3x3(V) < 0.0f) {
        for (std::size_t r = 0; r < 3; ++r) {
            V[r][2] = -V[r][2];
        }
    }

    const Mat<3> W{{
        {0.0f, -1.0f, 0.0f},
        {1.0f,  0.0f, 0.0f},
        {0.0f,  0.0f, 1.0f}
    }};
    const Mat<3> Wt = transpose(W);
    const Mat<3> Vt = transpose(V);

    Mat<3> R1 = multiply(multiply(U, W), Vt);
    Mat<3> R2 = multiply(multiply(U, Wt), Vt);

    Vec3 t{U[0][2], U[1][2], U[2][2]};

    out[0].R = R1; out[0].t = t;
    out[1].R = R1; out[1].t = -t;
    out[2].R = R2; out[2].t = t;
    out[3].R = R2; out[3].t = -t;
}

std::optional<Vec3> triangulate_point(
    const Mat<3>& R,
    const Vec3& t,
    const Vec2& p_prev,
    const Vec2& p_curr,
    int w,
    int h) {
    const Vec2 a = normalize_point(p_prev, w, h);
    const Vec2 b = normalize_point(p_curr, w, h);

    // Camera 1: P1 = [I | 0]
    // Camera 2: P2 = [R | t]
    Mat<4> A{};
    A[0] = {{-1.0f, 0.0f, a.x, 0.0f}};
    A[1] = {{0.0f, -1.0f, a.y, 0.0f}};
    A[2] = {{
        b.x * R[2][0] - R[0][0],
        b.x * R[2][1] - R[0][1],
        b.x * R[2][2] - R[0][2],
        b.x * t.z - t.x
    }};
    A[3] = {{
        b.y * R[2][0] - R[1][0],
        b.y * R[2][1] - R[1][1],
        b.y * R[2][2] - R[1][2],
        b.y * t.z - t.y
    }};

    Mat<4> ata{};
    const Mat<4> at = transpose(A);
    ata = multiply(at, A);

    Mat<4> eigenvectors = identity_matrix<4>();
    Vec<4> eigenvalues{};
    jacobi_eigen_symmetric(ata, eigenvectors, eigenvalues);

    Vec<4> x{};
    for (std::size_t i = 0; i < 4; ++i) {
        x[i] = eigenvectors[i][0];
    }
    if (std::abs(x[3]) < 1.0e-8f) {
        return std::nullopt;
    }

    return Vec3{x[0] / x[3], x[1] / x[3], x[2] / x[3]};
}

bool cheirality_check(
    const Mat<3>& R,
    const Vec3& t,
    const std::vector<Correspondence>& corr,
    int w,
    int h) {
    if (corr.empty()) return false;

    const std::size_t sample_count = std::min<std::size_t>(corr.size(), 8);
    std::size_t positive = 0;
    std::size_t total = 0;

    for (std::size_t i = 0; i < sample_count; ++i) {
        const auto X = triangulate_point(R, t, corr[i].p_prev, corr[i].p_curr, w, h);
        if (!X) continue;

        const float z1 = X->z;
        const float z2 = R[2][0] * X->x + R[2][1] * X->y + R[2][2] * X->z + t.z;
        ++total;
        if (z1 > 1.0e-4f && z2 > 1.0e-4f) {
            ++positive;
        }
    }

    return total > 0 && positive * 2 >= total;
}

math::Quaternion rotmat_to_quat(const Mat<3>& R) {
    const float trace = R[0][0] + R[1][1] + R[2][2];
    math::Quaternion q;

    if (trace > 0.0f) {
        const float s = std::sqrt(trace + 1.0f) * 2.0f;
        q.w = 0.25f * s;
        q.x = (R[2][1] - R[1][2]) / s;
        q.y = (R[0][2] - R[2][0]) / s;
        q.z = (R[1][0] - R[0][1]) / s;
    } else if (R[0][0] > R[1][1] && R[0][0] > R[2][2]) {
        const float s = std::sqrt(std::max(1.0f + R[0][0] - R[1][1] - R[2][2], 0.0f)) * 2.0f;
        q.w = (R[2][1] - R[1][2]) / s;
        q.x = 0.25f * s;
        q.y = (R[0][1] + R[1][0]) / s;
        q.z = (R[0][2] + R[2][0]) / s;
    } else if (R[1][1] > R[2][2]) {
        const float s = std::sqrt(std::max(1.0f + R[1][1] - R[0][0] - R[2][2], 0.0f)) * 2.0f;
        q.w = (R[0][2] - R[2][0]) / s;
        q.x = (R[0][1] + R[1][0]) / s;
        q.y = 0.25f * s;
        q.z = (R[1][2] + R[2][1]) / s;
    } else {
        const float s = std::sqrt(std::max(1.0f + R[2][2] - R[0][0] - R[1][1], 0.0f)) * 2.0f;
        q.w = (R[1][0] - R[0][1]) / s;
        q.x = (R[0][2] + R[2][0]) / s;
        q.y = (R[1][2] + R[2][1]) / s;
        q.z = 0.25f * s;
    }

    return q.normalized();
}

Mat<3> transpose3(const Mat<3>& m) {
    return transpose(m);
}

Vec3 camera_center_from_extrinsic(const Mat<3>& R, const Vec3& t) {
    const Mat<3> Rt = transpose3(R);
    return {
        -(Rt[0][0] * t.x + Rt[0][1] * t.y + Rt[0][2] * t.z),
        -(Rt[1][0] * t.x + Rt[1][1] * t.y + Rt[1][2] * t.z),
        -(Rt[2][0] * t.x + Rt[2][1] * t.y + Rt[2][2] * t.z)
    };
}

} // namespace

CameraTrack CameraSolver::solve(
    const std::vector<Track>& tracks,
    const Config& config) const {

    CameraTrack result;
    if (tracks.empty() || config.frame_width <= 0 || config.frame_height <= 0) {
        return result;
    }

    (void)config.bundle_adjustment_iterations;

    std::vector<float> all_times;
    for (const auto& track : tracks) {
        for (const auto& sample : track.samples) {
            all_times.push_back(sample.time);
        }
    }

    std::sort(all_times.begin(), all_times.end());
    all_times.erase(std::unique(all_times.begin(), all_times.end(),
        [](float a, float b) { return std::abs(a - b) <= kSampleTimeEpsilon; }),
        all_times.end());

    if (all_times.empty()) {
        return result;
    }

    Mat<3> K{};
    compute_K(K, config);

    Mat<3> R_curr = identity_matrix<3>();
    Vec3 t_curr{0.0f, 0.0f, 0.0f};

    auto push_keyframe = [&](float time) {
        const Vec3 camera_center = camera_center_from_extrinsic(R_curr, t_curr);
        const Mat<3> world_rotation = transpose(R_curr);
        result.keyframes.push_back({
            time,
            camera_center,
            rotmat_to_quat(world_rotation),
            config.initial_focal_length_mm.value_or(35.0f),
            0.0f
        });
    };

    push_keyframe(all_times.front());

    for (std::size_t i = 1; i < all_times.size(); ++i) {
        const std::vector<Correspondence> corr = build_correspondences(tracks, all_times[i - 1], all_times[i]);

        Mat<3> F{};
        PoseCandidate candidates[4]{};
        bool solved = false;

        if (corr.size() >= 8 && fundamental_matrix_8pt(corr, F, config.frame_width, config.frame_height)) {
            const Mat<3> E = essential_from_fundamental(F, K);
            decompose_essential(E, candidates);

            int best = -1;
            for (int c = 0; c < 4; ++c) {
                if (cheirality_check(candidates[c].R, candidates[c].t, corr, config.frame_width, config.frame_height)) {
                    best = c;
                    break;
                }
            }

            if (best >= 0) {
                Mat<3> R_rel = candidates[best].R;
                Vec3 t_rel = candidates[best].t;
                const float t_len = t_rel.length();
                if (t_len > 1.0e-8f) {
                    t_rel /= t_len;
                } else {
                    t_rel = {0.0f, 0.0f, 0.0f};
                }

                const Mat<3> R_next = multiply(R_rel, R_curr);
                const Vec3 t_next{
                    R_rel[0][0] * t_curr.x + R_rel[0][1] * t_curr.y + R_rel[0][2] * t_curr.z + t_rel.x,
                    R_rel[1][0] * t_curr.x + R_rel[1][1] * t_curr.y + R_rel[1][2] * t_curr.z + t_rel.y,
                    R_rel[2][0] * t_curr.x + R_rel[2][1] * t_curr.y + R_rel[2][2] * t_curr.z + t_rel.z
                };

                R_curr = R_next;
                t_curr = t_next;
                solved = true;
            }
        }

        (void)solved;
        push_keyframe(all_times[i]);
    }

    return result;
}

} // namespace tachyon::tracker
