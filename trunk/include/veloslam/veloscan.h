/**
 * @file
 * @brief Representation of a 3D scan and implementation of scan matching
 * @author Andreas Nuechter. Jacobs University Bremen, Germany
 * @author Li Wei, Wuhan University, China
 * @author Li Ming, Wuhan University, China
 */

#ifndef __VELOSCAN_H__
#define __VELOSCAN_H__

#ifdef _MSC_VER
#define snprintf _snprintf
#undef _STDIO_DEFINED
#define  _USE_MATH_DEFINES
#endif

#include <vector>
#include <map>

#include "slam6d/scan.h"
#include "veloslam/gridcell.h"
#include "veloslam/gridcluster.h"

bool FilterNOMovingObjcets(clusterFeature &glu, cluster &gluData);

class Trajectory
{
  public:
     Trajectory();
  public:
     vector <Point> path;
};


/**
 * @brief 3D scan representation and implementation of dynamic velodyne scan matching
 */
class VeloScan : public BasicScan {

public:
  VeloScan();
  VeloScan(const VeloScan& s);
  ~VeloScan();

  void setPoints(vector <Point>* _points) ;
  int dumpFrames();

  static void readScans(IOType type,
				    int start, int end, string &dir, int maxDist, int minDist,
				    bool openFileForWriting = false);

  static void readScansRedSearch(IOType type,
						   int start, int end, string &dir, int maxDist, int minDist,
						   double voxelSize, int nrpts, // reduction parameters
						   int nns_method, bool cuda_enabled,
						   bool openFileForWriting = false);

public:

  /**
   * Vector storing single scans of a metascan
   */
//  vector <VeloScan *> meta_parts;

  // FIXME
  void FindingAllofObject(int maxDist, int minDist);
  void TrackingAllofObject(int trackingAlgo);
  void ExchangePointCloud();
  void ClassifiAllofObject();

  int DumpScan(string filename);
  int DumpScanRedPoints(string filename);
  int DeletePoints();

  int CalcRadAndTheta();
  int TransferToCellArray(int maxDist, int minDist);

  void MarkStaticorMovingPointCloud();
  void FreeAllCellAndCluterMemory();
  void ClassifiAllObject();
  void ClassifibyTrackingAllObject(int currentNO ,int windowsize);
  void calcReducedPoints_byClassifi(double voxelSize, int nrpts, PointType pointtype);

  int CalcScanCellFeature();
  int CalcCellFeature(cell& cellobj,cellFeature& f);
  int FindAndCalcScanClusterFeature();
  int SearchNeigh(cluster& clu,charvv& flagvv,int i,int j);
  int CalcClusterFeature(cluster& clu,clusterFeature& f);
  void SaveObjectsInPCD(int index, cluster &gClusterData );
  void SaveFrameInPCD( );

  /** scanCellFeatureArray */
  cellArray scanCellArray;
  cellFeatureArray scanCellFeatureArray;

  clusterArray scanClusterArray;
  clusterFeatureArray scanClusterFeatureArray;

  bool isTrackerHandled;
  long scanid;

    /**
   * The actual file number for the *.frames file
   */
  int fileNr;
   /**
   * number elements of the array
   */
  int points_red_size;

  int clusterNum;//the number of clusters to be tracked, added by yuanjun
};

#endif
