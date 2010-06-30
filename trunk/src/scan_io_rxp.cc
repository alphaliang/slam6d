/**
 * @file
 * @brief Implementation of reading 3D scans
 * @author Andreas Nuechter. Jacobs University Bremen gGmbH
 * @author Dorit Borrmann. Smart Systems Group, Jacobs University Bremen gGmbH, Germany. 
 */

#include "scan_io_rxp.h"
#include "riegl/scanlib.hpp"
#include "globals.icc"
#include <fstream>
using std::ifstream;
#include <iostream>
using std::cerr;
using std::endl;

#include <algorithm>
using std::swap;

#ifdef _MSC_VER
#include <windows.h>
#endif

using namespace scanlib;
using namespace std;
using namespace std::tr1;
/**
 * The importer class is the interface to riegl's pointcloud class, and will convert their point struct to slam6d's point class.
 *
 * Code adapted from rivlib/example/pointcloudcpp.cpp available from http://www.riegl.com .
 */
class importer
    : public pointcloud
{
    vector<Point> *o;

public:
    importer(vector<Point> *o_)
        : pointcloud(false) // set this to true if you need gps aligned timing
        , o(o_)
    {}

protected:

    // overridden from pointcloud class
    void on_echo_transformed(echo_type echo)
    {
	 static ofstream out ("scan.txt");

        // here we select which target types we are interested in
        for (unsigned int i = 0; i < target_count; i++) {
            // targets is a member std::vector that contains all
            // echoes seen so far, i.e. the current echo is always
            // indexed by target_count-1.
            target& t(targets[i]);

            // target_count > 1      =>  multiple echos
            //  target_count == i+1  =>  i is last echo
            //  target_count != i+1  =>  i is first echo
            // target_count == 1     =>  single echo
            // target.reflectance
            // target.amplitude
            // target.deviation
            // target.time
            // target.vertex  point coordinates
            //

		  out << t.vertex[0] << " " << t.vertex[1] << " " << t.vertex[2] << " ";
		  
		  double range = std::sqrt(t.vertex[0]*t.vertex[0]
							  + t.vertex[1]*t.vertex[1]
							  + t.vertex[2]*t.vertex[2]);
            if (range > numeric_limits<double>::epsilon()) {
                double phi = atan2(t.vertex[1],t.vertex[0]);
                phi = ((phi<0.0)?(phi+2.0*pi):phi);
                double theta = std::acos(t.vertex[2]/range);
                t.vertex[0] = static_cast<float>(range);
                t.vertex[1] = static_cast<float>((360.0/(2.0*pi))*theta);
                t.vertex[2] = static_cast<float>((360.0/(2.0*pi))*phi);
            }
		  out << t.vertex[0] << " " << t.vertex[1] << " " << t.vertex[2] << " " << t.reflectance << endl; 

		    
            Point p;

            p.x = t.vertex[0]*100.0;
            p.y = t.vertex[2]*100.0;
            p.z = t.vertex[1]*100.0;

            p.reflectance = t.reflectance;
            p.amplitude   = t.amplitude;
            p.deviation   = t.deviation;

            o->push_back(p);
        }
    }

    // overridden from basic_packets
    // this function gets called when a the scanner emits a notification
    // about an exceptional state.
    void on_unsolicited_message(const unsolicited_message<iterator_type>& arg) {
        basic_packets::on_unsolicited_message(arg);
        // in this example we just print a warning to stderr
        cerr << "WARNING: " << arg.message << endl;
        // the following line would put out the entire content of the packet
        // converted to ASCII format:
        // cerr << arg << endl;
    }
};

/**
 * Reads specified scans from given directory in
 * the file format Riegl Laser Measurement GmbH 
 * uses. It will be compiled as shared lib.
 *
 * Scan poses will NOT be initialized after a call
 * to this function. Initial pose estimation works 
 * only with the -p switch, i.e., trusting the initial
 * estimations by Riegl.
 *
 * @param start Starts to read with this scan
 * @param end Stops with this scan
 * @param dir The directory from which to read
 * @param maxDist Reads only Points up to this Distance
 * @param minDist Reads only Points from this Distance
 * @param euler Initital pose estimates (will not be applied to the points
 * @param ptss Vector containing the read points
 */
int ScanIO_rxp::readScans(int start, int end, string &dir, int maxDist, int mindist,
						  double *euler, vector<Point> &ptss)
{
  static int fileCounter = start;
  string scanFileName;
  string poseFileName;

  ifstream pose_in;

  if (end > -1 && fileCounter > end) return -1; // 'nuf read

  
  poseFileName = dir + "scan" + to_string(fileCounter,3) + ".pose";
  scanFileName = "file://" + dir + "scan" + to_string(fileCounter,3) + ".rxp";
    
  pose_in.open(poseFileName.c_str());
  // read 3D scan

  if (!pose_in.good()) return -1; // no more files in the directory
  if (!pose_in.good()) { cerr << "ERROR: Missing file " << poseFileName << endl; exit(1); }
  cout << "Processing Scan " << scanFileName;
  cout.flush();
  
  for (unsigned int i = 0; i < 6; pose_in >> euler[i++]);

  cout << " @ pose (" << euler[0] << "," << euler[1] << "," << euler[2]
	  << "," << euler[3] << "," << euler[4] << ","  << euler[5] << ")" << endl;
  
  // convert angles from deg to rad
  for (unsigned int i = 3; i <= 5; i++) euler[i] = rad(euler[i]);
  pose_in.close();
  pose_in.clear();


  // open scanfile
  shared_ptr<basic_rconnection> rc;
  rc = basic_rconnection::create(scanFileName);
  rc->open();

  // decoder splits the binary file into readable chunks
  decoder_rxpmarker dec(rc);
  // importer interprets the chunks
  importer imp(&ptss);

  // iterate over chunks
  buffer  buf;
  for ( dec.get(buf); !dec.eoi(); dec.get(buf) ) {
    imp.dispatch(buf.begin(), buf.end());
  }

  //done
  rc->close();



  fileCounter++;
  
  return fileCounter-1;
}


/**
 * class factory for object construction
 *
 * @return Pointer to new object
 */
#ifdef _MSC_VER
extern "C" __declspec(dllexport) ScanIO* create()
#else
extern "C" ScanIO* create()
#endif
{
  return new ScanIO_rxp;
}


/**
 * class factory for object construction
 *
 * @return Pointer to new object
 */
#ifdef _MSC_VER
extern "C" __declspec(dllexport) void destroy(ScanIO *sio)
#else
extern "C" void destroy(ScanIO *sio)
#endif
{
  delete sio;
}

#ifdef _MSC_VER
BOOL APIENTRY DllMain(HANDLE hModule, DWORD dwReason, LPVOID lpReserved)
{
	return TRUE;
}
#endif
