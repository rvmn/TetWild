// This file is part of TetWild, a software for generating tetrahedral meshes.
//
// Copyright (C) 2018 Jeremie Dumas <jeremie.dumas@ens-lyon.org>
//
// This Source Code Form is subject to the terms of the Mozilla Public License
// v. 2.0. If a copy of the MPL was not distributed with this file, You can
// obtain one at http://mozilla.org/MPL/2.0/.
//
// Created by Jeremie Dumas on 09/04/18.
//
////////////////////////////////////////////////////////////////////////////////
#include "wrapper.h"
#include "internal/utils.h"
#include <tetwild/Logger.h>
#include <tetwild/DistanceQuery.h>
#include <tetwild/geogram/utils.h>
#include <igl/avg_edge_length.h>
#include <igl/bounding_box_diagonal.h>
#include <igl/boundary_facets.h>
#include <igl/signed_distance.h>
#include <igl/parallel_for.h>
#include <igl/barycenter.h>
#include <igl/winding_number.h>
#include <igl/dihedral_angles.h>
#include <igl/face_areas.h>
#include <igl/remove_unreferenced.h>
#include <tetwild/geogram/utils.h>
#include <geogram/mesh/mesh_io.h>
#include <geogram/mesh/mesh_AABB.h>
////////////////////////////////////////////////////////////////////////////////
//
// Wrapper for 3D remeshing comes from:
// https://github.com/mxncr/mmgig
//

namespace tetwild {

namespace {

bool mmgs_tri_remesh(const Eigen::MatrixXd &VI, const Eigen::MatrixXi &FI,
        Eigen::MatrixXd &VO, Eigen::MatrixXi &FO, const tetwild::MmgOptions& opt)
{
        MMG5_pMesh mesh = NULL;
        MMG5_pSol met = NULL;
        bool ok = eigen_to_mmg(VI, FI, mesh, met);
        if (!ok) {
                logger().error("mmgs: failed to convert mesh to MMG5_pMesh");
                mmgs_free(mesh, met);
                return false;
        }

        // Set remeshing options
        MMGS_Set_dparameter(mesh, met, MMGS_DPARAM_angleDetection, opt.angle_value);
        if (opt.enable_anisotropy) {
                MMGS_Set_solSize(mesh,met,MMG5_Vertex,0,MMG5_Tensor);
        }
        if (opt.hsiz == 0.0) {
                MMGS_Set_dparameter(mesh, met, MMGS_DPARAM_hmin, opt.hmin);
                MMGS_Set_dparameter(mesh, met, MMGS_DPARAM_hmax, opt.hmax);
        } else {
                met->np = 0;
                MMGS_Set_dparameter(mesh, met, MMGS_DPARAM_hsiz, opt.hsiz);
        }
        MMGS_Set_dparameter(mesh, met, MMGS_DPARAM_hausd, opt.hausd);
        MMGS_Set_dparameter(mesh, met, MMGS_DPARAM_hgrad, opt.hgrad);
        MMGS_Set_iparameter(mesh, met, MMGS_IPARAM_angle, int(opt.angle_detection));
        MMGS_Set_iparameter(mesh, met, MMGS_IPARAM_noswap, int(opt.noswap));
        MMGS_Set_iparameter(mesh, met, MMGS_IPARAM_noinsert, int(opt.noinsert));
        MMGS_Set_iparameter(mesh, met, MMGS_IPARAM_nomove, int(opt.nomove));

        int ier = MMGS_mmgslib(mesh,met);
        if (ier != MMG5_SUCCESS) {
                logger().error("mmgs_remesh: failed to remesh");
                mmgs_free(mesh, met);
                return false;
        }

        ok = mmg_to_eigen(mesh, VO, FO);

        mmgs_free(mesh, met);
        return ok;
}

bool mmg3d_tet_remesh(const Eigen::MatrixXd &VI, const Eigen::MatrixXi &FI, const Eigen::MatrixXi &TI,
        Eigen::MatrixXd &VO, Eigen::MatrixXi &FO, Eigen::MatrixXi &TO, const tetwild::MmgOptions& opt)
{
        MMG5_pMesh mesh = NULL;
        MMG5_pSol met = NULL;
        bool ok = eigen_to_mmg(VI, FI, TI, mesh, met);
        if (!ok) {
                logger().error("mmg3d: failed to convert mesh to MMG5_pMesh");
                mmg3d_free(mesh, met);
                return false;
        }

        // Set remeshing options
        MMG3D_Set_dparameter(mesh, met, MMG3D_DPARAM_angleDetection, opt.angle_value);
        if (opt.enable_anisotropy) {
                MMG3D_Set_solSize(mesh,met,MMG5_Vertex,0,MMG5_Tensor);
        }
        if (opt.hsiz == 0.0) {
                MMG3D_Set_dparameter(mesh, met, MMG3D_DPARAM_hmin, opt.hmin);
                MMG3D_Set_dparameter(mesh, met, MMG3D_DPARAM_hmax, opt.hmax);
        } else {
                met->np = 0;
                MMG3D_Set_dparameter(mesh, met, MMG3D_DPARAM_hsiz, opt.hsiz);
        }
        MMG3D_Set_dparameter(mesh, met, MMG3D_DPARAM_hausd, opt.hausd);
        MMG3D_Set_dparameter(mesh, met, MMG3D_DPARAM_hgrad, opt.hgrad);
        MMG3D_Set_iparameter(mesh, met, MMG3D_IPARAM_angle, int(opt.angle_detection));
        MMG3D_Set_iparameter(mesh, met, MMG3D_IPARAM_noswap, int(opt.noswap));
        MMG3D_Set_iparameter(mesh, met, MMG3D_IPARAM_noinsert, int(opt.noinsert));
        MMG3D_Set_iparameter(mesh, met, MMG3D_IPARAM_nomove, int(opt.nomove));
        MMG3D_Set_iparameter(mesh, met, MMG3D_IPARAM_nosurf, int(opt.nosurf));
        MMG3D_Set_iparameter(mesh, met, MMG3D_IPARAM_opnbdy, int(opt.opnbdy));
        MMG3D_Set_iparameter(mesh, met, MMG3D_IPARAM_optim, int(opt.optim));
        MMG3D_Set_iparameter(mesh, met, MMG3D_IPARAM_optimLES, int(opt.optimLES));

        int ier = MMG3D_mmg3dlib(mesh,met);
        if (ier != MMG5_SUCCESS) {
                logger().error("mmg3d_remesh: failed to remesh");
                mmg3d_free(mesh, met);
                return false;
        }

        ok = mmg_to_eigen(mesh, VO, FO, TO);

        mmg3d_free(mesh, met);
        return ok;
}

bool mmg3d_extract_iso(const Eigen::MatrixXd &VI, const Eigen::MatrixXi &FI, const Eigen::MatrixXi &TI, const Eigen::VectorXd &SI,
        Eigen::MatrixXd &VO, Eigen::MatrixXi &FO, Eigen::MatrixXi &TO, Eigen::VectorXi &RO, const tetwild::MmgOptions& opt)
{
        if (!opt.level_set) {
                logger().error("mmg3d_iso: opt.level_set is set to false, cancel");
                return false;
        }
        if (opt.angle_detection) {
                logger().warn("mmg3d_iso: angle_detection should probably be disabled because level set functions are smooth");
        }

        MMG5_pMesh mesh = NULL;
        MMG5_pSol met = NULL;
        bool ok = eigen_to_mmg(VI, FI, TI, mesh, met);
        if (!ok) {
                logger().error("mmg3d: failed to convert mesh to MMG5_pMesh");
                mmg3d_free(mesh, met);
                return false;
        }
        assert(VI.rows() == SI.size());
        for (int v = 0; v < SI.size(); ++v) {
                met->m[v+1] = SI[v];
        }

        // Set remeshing options
        MMG3D_Set_iparameter(mesh, met, MMG3D_IPARAM_iso, 1);
        MMG3D_Set_dparameter(mesh, met, MMG3D_DPARAM_ls, opt.ls_value);
        MMG3D_Set_dparameter(mesh, met, MMG3D_DPARAM_angleDetection, opt.angle_value);
        if (opt.hsiz == 0.0) {
                MMG3D_Set_dparameter(mesh, met, MMG3D_DPARAM_hmin, opt.hmin);
                MMG3D_Set_dparameter(mesh, met, MMG3D_DPARAM_hmax, opt.hmax);
        } else {
                logger().error("mmg3d_iso: should not use hsiz parameter for level set mode");
                mmg3d_free(mesh, met);
                return false;
        }
        MMG3D_Set_dparameter(mesh, met, MMG3D_DPARAM_hausd, opt.hausd);
        MMG3D_Set_dparameter(mesh, met, MMG3D_DPARAM_hgrad, opt.hgrad);
        MMG3D_Set_iparameter(mesh, met, MMG3D_IPARAM_angle, int(opt.angle_detection));
        MMG3D_Set_iparameter(mesh, met, MMG3D_IPARAM_noswap, int(opt.noswap));
        MMG3D_Set_iparameter(mesh, met, MMG3D_IPARAM_noinsert, 1);
        MMG3D_Set_iparameter(mesh, met, MMG3D_IPARAM_nomove, 1);
        MMG3D_Set_iparameter(mesh, met, MMG3D_IPARAM_nosurf, 1);

        int ier = MMG3D_mmg3dls(mesh,met);
        if (ier != MMG5_SUCCESS) {
                logger().error("mmg3d_iso: failed to remesh isovalue");
                mmg3d_free(mesh, met);
                return false;
        }

        // Convert back
        ok = mmg_to_eigen(mesh, VO, FO, TO, &RO);

        mmg3d_free(mesh, met);
        return ok;
}

} // anonymous namespace

////////////////////////////////////////////////////////////////////////////////

void remesh_uniform_sf(const Eigen::MatrixXd &V, const Eigen::MatrixXi &F,
        Eigen::MatrixXd &OV, Eigen::MatrixXi &OF, const MmgOptions &opt)
{
        assert(V.cols() == 3);
        mmgs_tri_remesh(V, F, OV, OF, opt);
}

void remesh_uniform_3d(const Eigen::MatrixXd &V, const Eigen::MatrixXi &T,
        Eigen::MatrixXd &OV, Eigen::MatrixXi &OF, Eigen::MatrixXi &OT, const MmgOptions &opt)
{
        assert(V.cols() == 3);
        Eigen::MatrixXi F;
        igl::boundary_facets(T, F);
        mmg3d_tet_remesh(V, F, T, OV, OF, OT, opt);
}

void isosurface_remeshing(const Eigen::MatrixXd &V, const Eigen::MatrixXi &T, const Eigen::VectorXd &S,
        Eigen::MatrixXd &OV, Eigen::MatrixXi &OF, Eigen::MatrixXi &OT, const MmgOptions &opt)
{
        assert(V.cols() == 3);
        MmgOptions lv_opt = opt;
        lv_opt.level_set = true;
        Eigen::MatrixXi F;
        igl::boundary_facets(T, F);
        Eigen::VectorXi R;
        mmg3d_extract_iso(V, F, T, S, OV, OF, OT, R, lv_opt);
}

void sample_box_regular(const Eigen::MatrixXd &V, double length, Eigen::MatrixXd &VO) {
        Eigen::RowVector3d pmin = V.colwise().minCoeff();
        Eigen::RowVector3d pmax = V.colwise().maxCoeff();
        Eigen::RowVector3d N = ((pmax - pmin).array() / length).unaryExpr([](double x) { return std::round(x); });
        Eigen::RowVector3d L = (pmax - pmin).array() / N.array();
        VO.resize((N.array() + 2).prod(), 3);
        for (int i = -1, cnt = 0; i < N(0) + 1; ++i) {
                for (int j = -1; j < N(1) + 1; ++j) {
                        for (int k = -1; k < N(2) + 1; ++k) {
                                VO.row(cnt++) = pmin.array() + L.array() * Eigen::RowVector3d(i, j, k).array();
                        }
                }
        }
}

////////////////////////////////////////////////////////////////////////////////

template <typename DerivedT, typename DerivedTT, typename DerivedTTi>
void tet_tet_adjacency(const Eigen::MatrixBase<DerivedT>& T,
                       Eigen::PlainObjectBase<DerivedTT>& TT,
                       Eigen::PlainObjectBase<DerivedTTi>& TTi)
{
    assert(T.cols()==4 && "Tets have four vertices.");

    //Preprocess
    using Array = std::array<int, 5>;
    std::vector<Array> TTT(4*T.rows());
    const auto loop_f = [&](const int t) {
        TTT[4*t] = {{T(t,0),T(t,1),T(t,2),t,0}};
        TTT[4*t+1] = {{T(t,0),T(t,1),T(t,3),t,1}};
        TTT[4*t+2] = {{T(t,1),T(t,2),T(t,3),t,2}};
        TTT[4*t+3] = {{T(t,2),T(t,0),T(t,3),t,3}};
        for(int i=0; i<4; ++i)
            std::sort(TTT[4*t+i].begin(), TTT[4*t+i].begin()+3);
    };

    //for(int t=0; t<T.rows(); ++t)
    //    loop_f(t);
    igl::parallel_for(T.rows(), loop_f);

    std::sort(TTT.begin(),TTT.end());

    //Compute TT and TTi
    TT.setConstant(T.rows(), T.cols(), -1);
    TTi.setConstant(T.rows(), T.cols(), -1);
    for(int i=1; i<TTT.size(); ++i) {
        const Array& r1 = TTT[i-1];
        const Array& r2 = TTT[i];
        if((r1[0]==r2[0]) && (r1[1]==r2[1]) && (r1[2]==r2[2])) {
            TT(r1[3],r1[4]) = r2[3];
            TT(r2[3],r2[4]) = r1[3];
            TTi(r1[3],r1[4]) = r2[4];
            TTi(r2[3],r2[4]) = r1[4];
        }
    }
}

void shape_parameter(const Eigen::MatrixXd &V, const Eigen::MatrixXi &T, Eigen::VectorXd &sigma) {
    sigma.resize(T.rows());
    for (int c = 0; c < T.rows(); ++c) {
        Eigen::RowVector3d v0 = V.row(T(c, 0));
        Eigen::RowVector3d v1 = V.row(T(c, 1));
        Eigen::RowVector3d v2 = V.row(T(c, 2));
        Eigen::RowVector3d v3 = V.row(T(c, 3));

        Eigen::Matrix<double, 6, 3> e;
        e.row(0) = v0 - v1;
        e.row(1) = v1 - v2;
        e.row(2) = v2 - v0;

        e.row(3) = v0 - v3;
        e.row(4) = v1 - v3;
        e.row(5) = v2 - v3;

        Eigen::Matrix<double, 6, 1> en = e.rowwise().norm();

        const double S = (e.row(0).cross(e.row(1)).norm() + e.row(0).cross(e.row(4)).norm() + e.row(4).cross(e.row(1)).norm() + e.row(2).cross(e.row(5)).norm()) / 2;
        const double V = std::abs(e.row(3).dot(e.row(2).cross(-e.row(0))))/6;
        const double rho = 3 * V / S;
        const double hp = en.maxCoeff();

        sigma(c) = rho/hp;
    }
}

void peel_slivers(const Eigen::MatrixXd &V, const Eigen::MatrixXi &T, Eigen::MatrixXd &VO, Eigen::MatrixXi &TO) {
    // Compute dihedral angles
    Eigen::VectorXd sigma;
    shape_parameter(V, T, sigma);
    double h_ref = igl::avg_edge_length(V, T);
    double rho_ref = sqrt(3.0)/6.0*h_ref;
    double sigma_ref = rho_ref / h_ref;

    // Compute tet-tet adjacency
    Eigen::MatrixXi TT, TTi;
    tet_tet_adjacency(T, TT, TTi);

    // BFS from the boundary to peel off worst elements
    typedef std::pair<double, int> TetElement;
    std::priority_queue<TetElement> q;
    std::vector<bool> marked(T.rows(), false);
    for (int e = 0; e < T.rows(); ++e) {
        for (int lf = 0; lf < 4; ++lf) {
            if (TT(e, lf) < 0) {
                logger().debug("init with tet {}, sigma {}", e, sigma(e));
                q.emplace(-sigma(e), e);
                marked[e] = true;
            }
        }
    }
    logger().debug("sigma_ref {}", sigma_ref);
    TO = T;
    while (!q.empty()) {
        auto tet = q.top(); q.pop();
        int x = tet.second;
        // logger().debug("removing tet {}, sigma {}", x, -tet.first);
        if (-tet.first > 0.1 * sigma_ref) {
            break;
        }
        TO.row(x).setConstant(-1);
        for (int lf = 0; lf < 4; ++lf) {
            int y = TT(x, lf);
            if (y >= 0 && !marked[y]) {
                q.emplace(-sigma(y), y);
                marked[y] = true;
            }
        }
    }

    // Delete vertices
    Eigen::MatrixXi NT;
    Eigen::VectorXi IM;
    igl::remove_unreferenced(V, TO, VO, NT, IM);
    TO = NT;
}

////////////////////////////////////////////////////////////////////////////////

void isosurface_remeshing(const Eigen::MatrixXd &V, const Eigen::MatrixXi &F, int num_samples,
        Eigen::MatrixXd &OV, Eigen::MatrixXi &OF, Eigen::MatrixXi &OT, const MmgOptions &opt)
{
    // Compute ambient points by sampling CVT points inside a given volume
    Eigen::MatrixXd ambient_vertices;
    Eigen::MatrixXi ambient_tets;
    resample_surface(V, F, V.rows(), ambient_vertices, 10, 0);
    sample_bbox(ambient_vertices, num_samples, 0.5 * igl::avg_edge_length(V, F), ambient_vertices, 10, 0);
    // sample_box_regular(V, opt.hmax, ambient_vertices);
    delaunay_tetrahedralization(ambient_vertices, ambient_tets);
    // peel_slivers(ambient_vertices, ambient_tets, ambient_vertices, ambient_tets);

    // Compute unsigned distance field
    Eigen::VectorXd S(ambient_vertices.rows());
    {
        logger().debug("computing unsigned distance field");
        GEO::Mesh M;
        to_geogram_mesh(V, F, M);
        GEO::MeshFacetsAABB aabb_tree(M);
        GEO::index_t nearest_facet = GEO::NO_FACET;
        GEO::vec3 nearest_point;
        double sq_dist = std::numeric_limits<double>::max();
        S.setZero();
        for (int v = 0; v < ambient_vertices.rows(); ++v) {
            auto p = ambient_vertices.row(v);
            GEO::vec3 geo_p(p[0], p[1], p[2]);
            if (nearest_facet != GEO::NO_FACET) {
                get_point_facet_nearest_point(M, geo_p, nearest_facet, nearest_point, sq_dist);
            }
            aabb_tree.nearest_facet_with_hint(geo_p, nearest_facet, nearest_point, sq_dist);
            S(v) = std::sqrt(sq_dist);
        }
        logger().debug("done");
    }

    // Compute sign using winding number
    {
        logger().debug("computing inside/outside using winding number");
        Eigen::VectorXd W;
        igl::winding_number(V, F, ambient_vertices, W);
        for (int v = 0; v < ambient_vertices.rows(); ++v) {
                if (!(W(v) > 0.5)) {
                        S(v) *= -1.0;
                }
        }
        logger().debug("done");
    }

    // Extract iso-surface from level set
    MmgOptions lv_opt = opt;
    lv_opt.level_set = true;
    Eigen::MatrixXi ambient_facets;
    igl::boundary_facets(ambient_tets, ambient_facets);

    GEO::Mesh M;
    to_geogram_mesh(ambient_vertices, ambient_facets, ambient_tets, M);
    GEO::Attribute<double> attr(M.vertices.attributes(), "sdf");
    GEO::vector<GEO::index_t> to_delete;
    for (int v = 0; v < ambient_vertices.rows(); ++v) {
        attr[v] = S[v];
    }
    mesh_save(M, "tmp.geogram");

    Eigen::VectorXi OR;
    mmg3d_extract_iso(ambient_vertices, ambient_facets, ambient_tets, S, OV, OF, OT, OR, lv_opt);

    // Keep tets inside using winding number
    {
        logger().debug("computing inside/outside using winding number");
        // Eigen::MatrixXd P;
        // Eigen::VectorXd W;
        // igl::barycenter(OV, OT, P);
        // igl::winding_number(V, F, P, W);
        Eigen::MatrixXi TT(OT.rows(), OT.cols());
        int cnt = 0;
        int rmin = OR.minCoeff();
        for (int e = 0; e < OT.rows(); ++e) {
            if (OR(e) == rmin)  {
                TT.row(cnt++) = OT.row(e);
            }
        }
        OT = TT.topRows(cnt);
        igl::boundary_facets(OT, OF);
        logger().debug("done");
    }
}

} // namespace tetwild
