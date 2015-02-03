/*
 * vigo23dtk implementation
 *
 * Copyright (C) Andreas Nuechter
 *
 * Released under the GPL version 3.
 *
 */

#include <fstream>
#include <iostream>
using std::cout;
using std::cerr;
using std::endl;
using std::ifstream;
using std::ofstream;

#include <eigen3/Eigen/Dense>
using namespace Eigen;

#include "slam6d/globals.icc"
#include <string.h>

#ifndef _MSC_VER
#include <unistd.h>
#else
#include "XGetopt.h"
#endif

#if WIN32
#define snprintf sprintf_s
#endif 

int parseArgs(int argc,char **argv, char dir[255], int& start, int& end){
  start   = 0;
  end     = -1; // -1 indicates no limitation

  int  c;
  // from unistd.h
  extern char *optarg;
  extern int optind;

  cout << endl;
  while ((c = getopt (argc, argv, "s:e:")) != -1)
    switch (c)
   {
   case 's':
     start = atoi(optarg);
     if (start < 0) { cerr << "Error: Cannot start at a negative scan number.\n"; exit(1); }
     break;
   case 'e':
     end = atoi(optarg);
     if (end < 0)     { cerr << "Error: Cannot end at a negative scan number.\n"; exit(1); }
     if (end < start) { cerr << "Error: <end> cannot be smaller than <start>.\n"; exit(1); }
     break;
   }

  if (optind != argc-1) {
    cerr << "\n*** Directory missing ***\n" << endl; 
    cout << endl
	  << "Usage: " << argv[0] << "  [-s NR] [-e NR] directory" << endl << endl;

    cout << "  -s NR   start at scan NR (i.e., neglects the first NR scans)" << endl
       << "          [ATTENTION: counting starts with 0]" << endl
	  << "  -e NR   end after scan NR" << "" << endl
	  << endl;
    cout << "Reads txt files (generated by UVigo) from directory/(pose|scan)????.txt and converts them to directory/scan???.txt and directory/scan???.pose in the slam6D standard file format." << endl;
    abort();
  }
  strncpy(dir,argv[optind],255);

#ifndef _MSC_VER
  if (dir[strlen(dir)-1] != '/') strcat(dir,"/");
#else
  if (dir[strlen(dir)-1] != '\\') strcat(dir,"\\");
#endif
  return 0;
}


int main(int argc, char **argv)
{
  int start = 0, end = -1;
  char dir[255];
  parseArgs(argc, argv, dir, start, end);
  int fileCounter = start; 
 
  char vigoFileName[255];
  char vigoPoseFileName[255];
  char poseFileName[255];
  char dataFileName[255];
  char completeFileName[255];

  ifstream vigo_in;
  ifstream vigopose_in;
  ofstream pose_out;
  ofstream data_out;
  ofstream complete_out;

  snprintf(completeFileName,255,"%sscan000.xyz",dir);
  complete_out.open(completeFileName);
  
  for(;;) { 
    if (end > -1 && fileCounter > end) break; // 'nuf read
  
    snprintf(vigoFileName,255,"%sscan%.4d.txt",dir,fileCounter);
    snprintf(vigoPoseFileName,255,"%spose%.4d.txt",dir,fileCounter);
    
    snprintf(dataFileName,255,"%sscan%.3d.3d",dir,fileCounter);
    snprintf(poseFileName,255,"%sscan%.3d.pose",dir,fileCounter++);
    
    vigopose_in.open(vigoPoseFileName);
    // read pose
    if (!vigopose_in.good()) return -1; // no more files in the directory
  
    cout << "Processing Scan " << vigoFileName << endl;
    cout.flush();
  
    double rPosin[3], rPos[3], rPosRPY[3], rPosTheta[3];
    for (unsigned int i = 0; i < 3; vigopose_in >> rPosin[i++]);
    for (unsigned int i = 0; i < 3; vigopose_in >> rPosRPY[i++]);

    double yaw   = rad(rPosRPY[2]);
    double pitch = rad(rPosRPY[1]);
    double roll  = rad(rPosRPY[0]);
    
    vigopose_in.close();
    vigopose_in.clear();

    Eigen::Matrix3d IMU_rot, R_imutoworld, R_lasertoimu, R_res;
    Eigen::Matrix3d Mx, My, Mz;
    
    Mx << 1, 0, 0, 0, cos(roll), -sin(roll), 0,sin(roll),cos(roll) ;
    My <<  cos(pitch), 0 , sin(pitch), 0, 1, 0, -sin(pitch), 0, cos(pitch);
    Mz << cos(yaw), -sin(yaw), 0, sin(yaw), cos(yaw), 0, 0, 0, 1;
    IMU_rot = Mz*My*Mx;    
    
    //    IMU_rot = Eigen::AngleAxisd(yaw,Eigen::Vector3d::UnitZ()) * Eigen::AngleAxisd(pitch,Eigen::Vector3d::UnitY()) * Eigen::AngleAxisd(roll,Eigen::Vector3d::UnitX());
    
    R_imutoworld << 0, 1, 0, 1, 0, 0, 0, 0, -1;
    R_lasertoimu << 0, 0, 1, 0, -1, 0, 1, 0, 0;

    R_res = R_imutoworld * IMU_rot * R_lasertoimu;

    double ltmat[16],mat[16];
    M4identity(ltmat);
    ltmat[0] = R_res(0,0);
    ltmat[1] = R_res(0,1);
    ltmat[2] = R_res(0,2);
    
    ltmat[4] = R_res(1,0);
    ltmat[5] = R_res(1,1);
    ltmat[6] = R_res(1,2);

    ltmat[8] = R_res(2,0);
    ltmat[9] = R_res(2,1);
    ltmat[10] = R_res(2,2);

    ltmat[3] = rPosin[0];
    ltmat[7] = rPosin[1];
    ltmat[11] = rPosin[2];
    
    to3DTKMat(ltmat, mat);
    
    Matrix4ToEuler(mat, rPosTheta, rPos);
    
    double euler[6];
     
    euler[0] = rPos[0];
    euler[1] = rPos[1];
    euler[2] = rPos[2];
    euler[3] = rPosTheta[0];
    euler[4] = rPosTheta[1];
    euler[5] = rPosTheta[2];

    pose_out.open(poseFileName);

    for(int i = 0; i < 3; i++) {
      pose_out << euler[i] << " ";
    }
    pose_out << endl; 

    for(int i = 3; i < 6; i++) {
      pose_out << deg(euler[i]) << " ";
    }
    pose_out << endl; 

    pose_out.close();
    pose_out.clear();

    vigo_in.open(vigoFileName);
    data_out.open(dataFileName);

 
    while (vigo_in.good()) {
	 double mypoint[3], point[3];
	 vigo_in >> mypoint[0];
	 vigo_in >> mypoint[1];
	 vigo_in >> mypoint[2];

	 Eigen::Vector3d P_global, P, T;
	 P << mypoint[0] , mypoint[1] , mypoint[2];
	 T << rPosin[0] , rPosin[1] , rPosin[2];

	 P_global = T + R_imutoworld * IMU_rot * R_lasertoimu * P;

	 complete_out << P_global(0) << " " << P_global(1) << " " << P_global(2) << endl;
	   
	 point[0] = mypoint[1]*-100.0;
	 point[1] = mypoint[2]*100.0;
	 point[2] = mypoint[0]*100.0;

	 if (Len(mypoint) > 1) 
	   data_out << point[0] << " "
			  << point[1] << " "
			  << point[2] << " "
			  << fileCounter << endl;
    }

    vigo_in.close();
    vigo_in.clear();
    data_out.close();
    data_out.clear();
  }

}