#include"model.h"
#include <fstream>
#include <sstream>
using namespace std;

int Model::nvert() const {
	return verts.size();
}

int Model::nface() const {
	int n = facet_vert.size();
	return n / 3;
}

vec3 Model::vert(const int i) const {
	return verts[i];
}

vec3 Model::vert(const int iface, const int ithvert)const {
	int num_vert = iface * 3 + ithvert;
	return verts[facet_vert[num_vert]];
}

vec3 Model::normal(const int iface, const int ithvert) const {
	int idx = iface * 3 + ithvert;
	return norms[facet_norm[idx]];
}

Model::Model(const string filename) {
	ifstream in;
	in.open(filename);
	if (in.fail()) {
		cout << "fail to open the file" << endl;
		return;
	}
	string line;
	while (!in.eof()) {
		getline(in, line);
		istringstream iss(line.c_str());
		char trash;//用于省略/等不需要的字符
		if (!line.compare(0, 2, "v ")) {
			double x, y, z;
			iss >> trash;//省略v
			iss >> x >> y >> z;
			vec3 point = { x,y,z };
			verts.push_back(point);
		}
		else if (!line.compare(0, 2, "f ")) {
			iss >> trash;
			int v_idx, t_idx, n_idx;
			char slash;
			for (int i = 0; i < 3; i++) {
				iss >> v_idx >> slash >> t_idx >> slash >> n_idx;
				facet_vert.push_back(--v_idx);
				facet_norm.push_back(--n_idx);
			}
		}
		else if (!line.compare(0, 3, "vn ")) {
			double nx, ny, nz;
			iss >> trash >> trash; // 跳过 'v' 和 'n'
			iss >> nx >> ny >> nz;
			vec3 normal = { nx, ny, nz };
			norms.push_back(normal);
		}
	}
	in.close();
	std::cerr << "# v# " << nvert() << " f# " << nface() << std::endl;
}