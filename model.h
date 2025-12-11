#pragma once
#include"geometry.h"
#include<vector>
#include<algorithm>
using namespace std;

class Model {
private:
	vector<vec3>verts = {};//存三维点向量
	vector<int>facet_vert = {};//存面的向量索引（扁平化处理）
	vector<vec3> norms = {};//顶点法向量
	vector<int> facet_norm = {}; // 存储每个面的法向量索引
public:
	int nvert() const;//查询点个数
	int nface() const;//查询面个数
	vec3 vert(const int i) const;//查询某点坐标
	vec3 vert(const int iface, const int ithvert)const;//第i个面的第i个点坐标
	vec3 normal(const int iface, const int ithvert) const;//第i个面的第i个点的法向量
	Model(const string filename);
};


