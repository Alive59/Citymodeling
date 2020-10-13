// PCL.cpp : �������̨Ӧ�ó������ڵ㡣
//
#include "stdafx.h"
#include <pcl/io/pcd_io.h>
#include<pcl/io/ply_io.h>
#include<pcl/io/obj_io.h>
#include <pcl/point_types.h>
#include "vector"
#include <iostream>
#include <strstream>
#include <windows.h>
#include <stdlib.h>
#include <algorithm>
#include <pcl/ModelCoefficients.h>
#include <pcl/filters/extract_indices.h>
#include <pcl/filters/voxel_grid.h>
#include <pcl/features/normal_3d.h>
#include <pcl/kdtree/kdtree.h>
#include <pcl/sample_consensus/method_types.h>
#include <pcl/sample_consensus/model_types.h>
#include <pcl/segmentation/sac_segmentation.h>
#include <pcl/segmentation/extract_clusters.h>
#include <pcl/segmentation/region_growing.h>
#include <pcl/segmentation/region_growing_rgb.h>
#include <pcl/filters/passthrough.h>
#include <pcl/console/time.h>
using namespace std;
using namespace pcl;
int
main(int argc, char** argv)
{
	pcl::PointCloud<pcl::PointXYZRGB>::Ptr restcloud(new pcl::PointCloud<PointXYZRGB>());//�洢ÿ�ηָ�֮��ʣ��ĵ�������
	if (pcl::io::loadPLYFile<pcl::PointXYZRGB>("1.ply", *restcloud) == -1)
	{
		std::cout << "Cloud reading failed." << std::endl;
		return (-1);
	}

	string SavePath;
	vector<pcl::PointCloud<pcl::PointXYZRGB>::Ptr> SegedPlanes;
	pcl::PointCloud<pcl::PointXYZRGB>::Ptr RestModel(new pcl::PointCloud<pcl::PointXYZRGB>);
	//����ƽ��ָ����
	int KSearch=50;
	double NormalDistanceWeight=0.6;
	int MaxIter=100;
	double DisThreshold=0.2;
	vector<vector<float>> planeparas;
	// All the objects needed
	pcl::console::TicToc tt;
	pcl::PLYReader plyReader;
	pcl::PLYWriter plyWriter;
	pcl::SACSegmentationFromNormals<PointXYZRGB, pcl::Normal> seg;//ƽ��ָ���
	pcl::NormalEstimation<PointXYZRGB, pcl::Normal> ne;//������������
	pcl::ExtractIndices<pcl::Normal> extract_normals;
	pcl::search::KdTree<PointXYZRGB>::Ptr tree(new pcl::search::KdTree<PointXYZRGB>());//����KDtree��������
	pcl::PointCloud<PointXYZRGB>::Ptr cloud_plane(new pcl::PointCloud<PointXYZRGB>());//�洢ÿ�ηָ���ƽ��
	pcl::PointCloud<pcl::Normal>::Ptr cloud_normals(new pcl::PointCloud<pcl::Normal>);//���Ʒ�����
	pcl::ModelCoefficients::Ptr coefficients_plane(new pcl::ModelCoefficients);//�洢�ָ���ƽ�����
	pcl::PointIndices::Ptr inliers(new pcl::PointIndices);//��ȡ����ƽ������
	ne.setSearchMethod(tree);
	ne.setInputCloud(restcloud);
	ne.setKSearch(KSearch);
	ne.compute(*cloud_normals);
	// Create the segmentation object for the planar model and set all the parameters
	seg.setOptimizeCoefficients(true);
	seg.setModelType(pcl::SACMODEL_NORMAL_PLANE);
	seg.setNormalDistanceWeight(NormalDistanceWeight);
	seg.setMethodType(pcl::SAC_RANSAC);
	seg.setMaxIterations(MaxIter);
	seg.setDistanceThreshold(DisThreshold);
	int i = 0, nr_points = (int)restcloud->points.size();
	while (restcloud->points.size() > 0.1 * nr_points)
	{
		// Segment the largest planar component from the remaining cloud
		std::cerr << "segmenting...\n", tt.tic();
		seg.setInputCloud(restcloud);
		seg.setInputNormals(cloud_normals);
		seg.segment(*inliers, *coefficients_plane);
		if (inliers->indices.size() == 0)
		{
			PCL_ERROR("Could not estimate a planar model for the given dataset.");
			return false;
		}
		std::cerr << "Plane coefficients: " << *coefficients_plane << std::endl;//���ƽ�����

		// ��ȡƽ��Ҫ��
		pcl::ExtractIndices<PointXYZRGB> extract;
		extract.setInputCloud(restcloud);
		extract.setIndices(inliers);
		extract.setNegative(false);
		extract.filter(*cloud_plane);
		//ɸѡ����3000�����ƽ��
		if (cloud_plane->points.size() < 3000)
			break;;
		std::cerr << ">> Done: " << tt.toc() << " ms,\n ";
		std::cerr << "Saving the segmented plane" << i << "...", tt.tic();
		std::stringstream SaveFilePath, SavePlyFilePath;
		SavePlyFilePath << "Plane" << i << ".ply";
		//����ƽ�����
		vector<float> tempplaneparas; tempplaneparas.push_back(coefficients_plane->values[0]);
		tempplaneparas.push_back(coefficients_plane->values[1]);
		tempplaneparas.push_back(coefficients_plane->values[2]);
		tempplaneparas.push_back(coefficients_plane->values[3]);
		planeparas.push_back(tempplaneparas);
		srand((unsigned)time(NULL));
		//����趨ƽ����ɫ
		int colorr = rand() % 255, colorg = rand() % 255, colorb = rand() % 255;
		for (int pn = 0; pn < cloud_plane->points.size(); pn++)
		{
			cloud_plane->points[pn].r = colorr; cloud_plane->points[pn].g = colorg; cloud_plane->points[pn].b = colorb;
		}
		int r = rand() / 255, g = rand() / 255, b = rand() / 255;
		for (int i = 0; i < cloud_plane->size(); i++){
			cloud_plane->points[i].r = r; cloud_plane->points[i].g = g; cloud_plane->points[i].b = b;
		}
		//д������ɫ��ƽ��
		plyWriter.write(SavePlyFilePath.str(), *cloud_plane, false);

		std::cerr << ">> Done: " << tt.toc() << " ms,\n ";
		//��ȡʣ��ĵ�
		extract.setNegative(true);
		extract.filter(*restcloud);
		extract_normals.setNegative(true);
		extract_normals.setInputCloud(cloud_normals);
		extract_normals.setIndices(inliers);
		extract_normals.filter(*cloud_normals);
		i++;
	}
	for (int i = 0; i < restcloud->size(); i++){
		RestModel->push_back(restcloud->points[i]);
	}
	plyWriter.write<PointXYZRGB>("RestPoints.ply", *restcloud, false);
	return true;
}