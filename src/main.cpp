// This file is part of TetWild, a software for generating tetrahedral meshes.
//
// Copyright (C) 2018 Yixin Hu <yixin.hu@nyu.edu>
//
// This Source Code Form is subject to the terms of the Mozilla Public License
// v. 2.0. If a copy of the MPL was not distributed with this file, You can
// obtain one at http://mozilla.org/MPL/2.0/.

#include <tetwild/tetwild.h>
#include <tetwild/Common.h>
#include <tetwild/Logger.h>
#include <tetwild/MeshRefinement.h>
#include <tetwild/geogram/Utils.h>
#include <igl/opengl/glfw/Viewer.h>
#include <igl/boundary_facets.h>
#include <igl/remove_duplicate_vertices.h>
#include <igl/read_triangle_mesh.h>
#include <igl/write_triangle_mesh.h>
#include <igl/writeOBJ.h>
#include <pymesh/MshSaver.h>
#include <geogram/mesh/mesh_io.h>
#include <geogram/basic/command_line.h>
#include <geogram/basic/command_line_args.h>
#include <tetwild/DisableWarnings.h>
#include <CLI/CLI.hpp>
#include <tetwild/EnableWarnings.h>

#include <iostream>

using namespace tetwild;

namespace tetwild {
    void extractFinalTetmesh(MeshRefinement& MR, Eigen::MatrixXd &V_out, Eigen::MatrixXi &T_out, Eigen::VectorXd &A_out, const Args &args, const State &state);
} // namespace tetwild

#ifdef WIN32
int setenv(const char *name, const char *value, int overwrite) {
    int errcode = 0;
    if (!overwrite) {
        size_t envsize = 0;
        errcode = getenv_s(&envsize, NULL, 0, name);
        if (errcode || envsize) return errcode;
    }
    return _putenv_s(name, value);
}
#endif

// split string on a delimiter
std::vector<std::string> split(std::string s, std::string delimiter) {
    size_t pos_start = 0, pos_end, delim_len = delimiter.length();
    std::string token;
    std::vector<std::string> res;

    while ((pos_end = s.find(delimiter, pos_start)) != std::string::npos) {
        token = s.substr (pos_start, pos_end - pos_start);
        pos_start = pos_end + delim_len;
        res.push_back (token);
    }
    
    res.push_back (s.substr (pos_start));
    return res;
}

//load input surface
Eigen::MatrixXd VI, VO;
Eigen::MatrixXi FI, TO;
Eigen::VectorXd AO;
Eigen::MatrixXi F;
Eigen::MatrixXd B;

// This function is called every time a keyboard button is pressed - used in viewer
bool key_down(igl::opengl::glfw::Viewer& viewer, unsigned char key, int modifier)
{
	 using namespace std;
	using namespace Eigen;
	 if (key >= '1' && key <= '9')
	{
		double t = double((key - '1')+1) / 9.0;
		VectorXd v = B.col(2).array() - B.col(2).minCoeff();
		v /= v.col(0).maxCoeff();
		 vector<int> s;
		 for (unsigned i=0; i<v.size();++i)
			if (v(i) < t)
				s.push_back(i);
		 MatrixXd V_temp(s.size()*4,3);
		 MatrixXi F_temp(s.size()*4,3);
		 for (unsigned i=0; i<s.size();++i)
		{
			V_temp.row(i*4+0) = VO.row(T(s[i],0));
			V_temp.row(i*4+1) = VO.row(T(s[i],1));
			V_temp.row(i*4+2) = VO.row(T(s[i],2));
			V_temp.row(i*4+3) = VO.row(T(s[i],3));
			F_temp.row(i*4+0) << (i*4)+0, (i*4)+1, (i*4)+3;
			F_temp.row(i*4+1) << (i*4)+0, (i*4)+2, (i*4)+1;
			F_temp.row(i*4+2) << (i*4)+3, (i*4)+2, (i*4)+0;
			F_temp.row(i*4+3) << (i*4)+1, (i*4)+2, (i*4)+3;
		}
        viewer.data().clear();
		viewer.data().set_mesh(V_temp,F_temp);
		viewer.data().set_face_based(true);
	}
	 return false;
}

// Tests whether a string ends with a given suffix
bool endswith(const std::string &str, const std::string &suffix) {
    if (str.length() >= suffix.length()) {
        return (0 == str.compare(str.length() - suffix.length(), suffix.length(), suffix));
    } else {
        return false;
    }
}

bool endswith_nocase(const std::string &str, const std::string &suffix) {
    std::string data = str;
    std::transform(data.begin(), data.end(), data.begin(), ::tolower);
    return endswith(data, suffix);
}

void saveFinalTetmesh(const std::string &output_volume, const std::string &output_surface,
    const Eigen::MatrixXd &V, const Eigen::MatrixXi &T, const Eigen::VectorXd &A, bool save_to_commandline)
{
    logger().debug("Writing mesh to {}...", output_volume);
    if (endswith_nocase(output_volume, ".mesh")) {
        if(save_to_commandline)
            std::ofstream f;
        else
            std::ofstream f(output_volume);
        f.precision(std::numeric_limits<double>::digits10 + 1);
        f << "MeshVersionFormatted 1" << std::endl;
        f << "Dimension 3" << std::endl;

        f << "Vertices" << std::endl << V.rows() << std::endl;
        for (int i = 0; i < V.rows(); i++)
            f << V(i, 0) << " " << V(i, 1) << " " << V(i, 2) << " " << 0 << std::endl;
        f << "Triangles" << std::endl << 0 <<std::endl;
        f << "Tetrahedra" << std::endl;
        f << T.rows() << std::endl;
        for (int i = 0; i < T.rows(); i++) {
            for (int j = 0; j < 4; j++) {
                f << T(i, j) + 1 << " ";
            }
            f << 0 << std::endl;
        }

        f << "End";
        f.close();
    } else if (endswith_nocase(output_volume, ".msh")) {
        PyMesh::MshSaver mSaver(output_volume, true);
        PyMesh::VectorF V_flat(V.size());
        PyMesh::VectorI T_flat(T.size());
        Eigen::MatrixXd VV = V.transpose();
        Eigen::MatrixXi TT = T.transpose();
        std::copy_n(VV.data(), V.size(), V_flat.data());
        std::copy_n(TT.data(), T.size(), T_flat.data());
        mSaver.save_mesh(V_flat, T_flat, 3, mSaver.TET);
        mSaver.save_elem_scalar_field("min_dihedral_angle", A);
    } else {
        GEO::Mesh M;
        Eigen::MatrixXi F(0, 3);
        to_geogram_mesh(V, F, T, M);
        GEO::Attribute<double> attr(M.cells.attributes(), "min_dihedral_angle");
        for (int e = 0; e < M.cells.nb(); ++e) {
            attr[e] = A(e);
        }
        mesh_save(M, output_volume);
    }

    Eigen::MatrixXd V_sf;
    Eigen::MatrixXi F_sf;
    extractSurfaceMesh(V, T, V_sf, F_sf);
    igl::writeOBJ(output_surface, V_sf, F_sf);
}

void gtet_new_slz(const Eigen::MatrixXd &VI, const Eigen::MatrixXi &FI, const std::string& slz_file,
                  const std::array<bool, 4>& ops,
                  Eigen::MatrixXd &VO, Eigen::MatrixXi &TO, Eigen::VectorXd &AO,
                  const Args &args = Args())
{
    State state(args, VI);
    GEO::Mesh sf, b;
    MeshRefinement MR(sf, b, args, state);
    MR.deserialization(VI, FI, slz_file);

//    MR.is_dealing_unrounded = true;
    MR.refine(state.ENERGY_AMIPS, ops, false, true);

    extractFinalTetmesh(MR, VO, TO, AO, args, state); //do winding number and output the tetmesh
}

int main(int argc, char *argv[]) {
    int log_level = 1; // debug
    std::string log_filename;
    std::string input_surface;
    std::string output_volume;
    std::string output_surface;
    std::string slz_file;
    Args args;

    CLI::App app{"RobustTetMeshing"};
    app.add_option("input,--input", input_surface, "Input surface mesh INPUT in .off/.obj/.stl/.ply format. (string)")->check(CLI::ExistingFile);
    app.add_option("output,--output", output_volume, "Output tetmesh OUTPUT in .msh format. (string, optional, default: input_file+postfix+'.msh')");
    app.add_option("input_verts,--input-verts", input_vertices, "Input vertices splitted by point-comma e.g. 1,2,3;2,3,4;");
    app.add_option("input_indices,--input-indices", input_indices, "Input face indices, joined by comma");
    app.add_option("--postfix", args.postfix, "Postfix P for output files. (string, optional, default: '_')");
    app.add_option("-l,--ideal-edge-length", args.initial_edge_len_rel, "ideal_edge_length = diag_of_bbox * L / 100. (double, optional, default: 5%)");
    app.add_option("-e,--epsilon", args.eps_rel, "epsilon = diag_of_bbox * EPS / 100. (double, optional, default: 0.1%)");
    app.add_option("--stage", args.stage, "Run pipeline in stage STAGE. (integer, optional, default: 1)");
    app.add_option("--filter-energy", args.filter_energy_thres, "Stop mesh improvement when the maximum energy is smaller than ENERGY. (double, optional, default: 10)");
    app.add_option("--max-pass", args.max_num_passes, "Do PASS mesh improvement passes in maximum. (integer, optional, default: 80)");

    app.add_flag("--is-laplacian", args.smooth_open_boundary, "Do Laplacian smoothing for the surface of output on the holes of input (optional)");
    app.add_option("--targeted-num-v", args.target_num_vertices, "Output tetmesh that contains TV vertices. (integer, optional, tolerance: 5%)");
    app.add_option("--bg-mesh", args.background_mesh, "Background tetmesh BGMESH in .msh format for applying sizing field. (string, optional)");
    app.add_flag("-q,--is-quiet", args.is_quiet, "Mute console output. (optional)");
    app.add_flag("-cli,--output-to-commandline", args.use_commandline, "Output gmsh to commandline. (default: false).");
    app.add_flag("-view,--view-result", args.view_result, "View the resulting tetmesh. (default: false).");
    app.add_option("--log", log_filename, "Log info to given file.");
    app.add_option("--level", log_level, "Log level (0 = most verbose, 6 = off).");
    app.add_flag("--mmgs", args.use_mmgs, "Use mmgs in the *experimental* hybrid pipeline (default: false).");
    app.add_flag("--mmg3d", args.use_mmg3d, "Use mmg3d in the *experimental* hybrid pipeline (default: false).");

    try {
        app.parse(argc, argv);
    } catch (const CLI::ParseError &e) {
        return app.exit(e);
    }
    if(args.use_commandline && !log_filename) args.is_quiet = true;
        
    Logger::init(!args.is_quiet, log_filename);
    log_level = std::max(0, std::min(6, log_level));
    spdlog::set_level(static_cast<spdlog::level::level_enum>(log_level));
    spdlog::flush_every(std::chrono::seconds(3));

    //initialization
    setenv("GEO_NO_SIGNAL_HANDLER", "1", 1);
    GEO::initialize();
    GEO::CmdLine::import_arg_group("standard");
    GEO::CmdLine::import_arg_group("pre");
    GEO::CmdLine::import_arg_group("algo");

    if(slz_file != "") {
        args.working_dir = input_surface.substr(0, slz_file.size() - 4);
    } else {
        args.working_dir = input_surface.substr(0, input_surfacesize() - 4);
    }

    if(args.csv_file.empty()) {
        args.csv_file = args.working_dir + args.postfix + ".csv";
    }

    if(output_volume.empty()) {
        output_volume = args.working_dir + args.postfix + ".msh";
    }
    output_surface = args.working_dir + args.postfix+"_sf.obj";

    if(args.is_quiet) {
        args.write_csv_file = false;
    }
    // args.user_callback = [] (Step step, double x) {
    //     std::string name;
    //     switch (step) {
    //         case Step::Preprocess:
    //             name = "Preprocess"; break;
    //         case Step::Delaunay:
    //             name = "Delaunay"; break;
    //         case Step::FaceMatching:
    //             name = "FaceMatching"; break;
    //         case Step::BSP:
    //             name = "BSP"; break;
    //         case Step::Tetra:
    //             name = "Tetra"; break;
    //         case Step::Optimize:
    //             name = "Optimize"; break;
    //         default: {}
    //     }
    //     std::cout << "[" << name << "] " << x << std::endl;
    // };
    

    if(input_verts){
        // split the verts and indices
        std::vector<std::vector<float>> verts;
        std::vector<std::string> vert_strings = split(input_verts, ";");
        for(std::string& vert_string: vert_strings){
            verts.push_back(split(vert_string, ","));
        }
        std::vector<int> indices = split(input_indices, ",");
        
        // Populate the input vertices.
        VI.resize(verts.size(), 3);
        for (int i = 0; i < verts.size(); ++i)
        {
            VI.row(i) << vertices[i][0], vertices[i][1], vertices[i][2];
        }
    
        // Populate the input faces (triangles lists).
        FI.resize(indices.size() / 3, 3);
        for (int i = 0; i < FI.rows(); ++i)
        {
            FI.row(i) << indices[i * 3 + 0], indices[i * 3 + 1], indices[i * 3 + 2];
        }
    }else 
        igl::read_triangle_mesh(input_surface, VI, FI);
    
    if (VI.rows() == 0) {
        logger().warn("libigl failed to read input mesh: {}, trying with geogram", input_surface);
        GEO::Mesh M;
        Eigen::MatrixXi TI;
        mesh_load(input_surface, M);
        from_geogram_mesh(M, VI, FI, TI);
    }
    if (endswith_nocase(input_surface, ".stl")) {
        Eigen::MatrixXd VV;
        Eigen::MatrixXi FF;
        Eigen::VectorXi SVI, SVJ;
        igl::remove_duplicate_vertices(VI, FI, 1e-10, VV, SVI, SVJ, FF);
        VI = VV;
        FI = FF;
    }

    //do tetrahedralization
    if(slz_file != "") {
        gtet_new_slz(VI, FI, slz_file,
            {{true, false, true, true}}, VO, TO, AO, args);
    } else {
        tetwild::tetrahedralization(VI, FI, VO, TO, AO, args);
    }

    if(args.view_result){
        igl::boundary_facets(VO, F);
        
        // Compute barycenters
    	igl::barycenter(VO, F, B);
    
    	// Plot the generated mesh
    	igl::opengl::glfw::Viewer viewer;
    
    	viewer.data().clear();
    	viewer.data().set_mesh(VO, F);
    	viewer.data().set_face_based(true);
    
    	viewer.callback_key_down = &key_down;
    	key_down(viewer,'5',0);
    	viewer.launch();
    }
        
    //save output volume
    saveFinalTetmesh(output_volume, output_surface, VO, TO, AO, args.use_commandline);

    spdlog::shutdown();

    return 0;
}
