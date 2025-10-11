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
			int x,y,z;
			iss >> trash;
			while (iss >> x >> trash >> y >> trash >> z) {
				facet_vert.push_back(--x);
			}
		}
	}
	in.close();
	std::cerr << "# v# " << nvert() << " f# " << nface() << std::endl;
	std::vector<int> idx(nface());    // permutation, a map from new to old facet indices
	for (int i = 0; i < nface(); i++) // we start with the identity
		idx[i] = i;

	sort(idx.begin(), idx.end(),
		[&](const int& a, const int& b) { // given two triangles, compare their min z coordinate
			float aminz = std::min(vert(a, 0).z,std::min(vert(a, 1).z,vert(a, 2).z));
			float bminz = std::min(vert(b, 0).z,std::min(vert(b, 1).z,vert(b, 2).z));
			return aminz < bminz;
		});

	std::vector<int> facet_vrt2(nface() * 3); // allocate an array to store permutated facets
	for (int i = 0; i < nface(); i++)           // for each (new) facet
		for (int j = 0; j < 3; j++)              // copy its three vertices from the old array
			facet_vrt2[i * 3 + j] = facet_vert[idx[i] * 3 + j];

	facet_vert = facet_vrt2;                  // store the sorted triangles
}