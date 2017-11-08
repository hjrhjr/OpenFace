///////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2017, Tadas Baltrusaitis all rights reserved.
//
// ACADEMIC OR NON-PROFIT ORGANIZATION NONCOMMERCIAL RESEARCH USE ONLY
//
// BY USING OR DOWNLOADING THE SOFTWARE, YOU ARE AGREEING TO THE TERMS OF THIS LICENSE AGREEMENT.  
// IF YOU DO NOT AGREE WITH THESE TERMS, YOU MAY NOT USE OR DOWNLOAD THE SOFTWARE.
//
// License can be found in OpenFace-license.txt
//
//     * Any publications arising from the use of this software, including but
//       not limited to academic journal and conference publications, technical
//       reports and manuals, must cite at least one of the following works:
//
//       OpenFace: an open source facial behavior analysis toolkit
//       Tadas Baltru�aitis, Peter Robinson, and Louis-Philippe Morency
//       in IEEE Winter Conference on Applications of Computer Vision, 2016  
//
//       Rendering of Eyes for Eye-Shape Registration and Gaze Estimation
//       Erroll Wood, Tadas Baltru�aitis, Xucong Zhang, Yusuke Sugano, Peter Robinson, and Andreas Bulling 
//       in IEEE International. Conference on Computer Vision (ICCV),  2015 
//
//       Cross-dataset learning and person-speci?c normalisation for automatic Action Unit detection
//       Tadas Baltru�aitis, Marwa Mahmoud, and Peter Robinson 
//       in Facial Expression Recognition and Analysis Challenge, 
//       IEEE International Conference on Automatic Face and Gesture Recognition, 2015 
//
//       Constrained Local Neural Fields for robust facial landmark detection in the wild.
//       Tadas Baltru�aitis, Peter Robinson, and Louis-Philippe Morency. 
//       in IEEE Int. Conference on Computer Vision Workshops, 300 Faces in-the-Wild Challenge, 2013.    
//
///////////////////////////////////////////////////////////////////////////////

#ifndef __RECORDER_CSV_h_
#define __RECORDER_CSV_h_

// System includes
#include <fstream>
#include <sstream>
#include <vector>

// OpenCV includes
#include <opencv2/core/core.hpp>

namespace Utilities
{

	//===========================================================================
	/**
	A class for recording CSV file from OpenFace
	*/
	class RecorderCSV {

	public:

		// The constructor for the recorder, need to specify if we are recording a sequence or not
		RecorderCSV();

		// Opening the file and preparing the header for it
		bool Open(std::string output_file_name, bool is_sequence, bool output_2D_landmarks, bool output_3D_landmarks, bool output_model_params, bool output_pose, bool output_AUs, bool output_gaze,
			int num_face_landmarks, int num_model_modes, int num_eye_landmarks, const std::vector<std::string>& au_names_class, const std::vector<std::string>& au_names_reg);

		// Closing the file and cleaning up
		void Close();

		void WriteLine(int observation_count, double time_stamp, bool landmark_detection_success, double landmark_confidence,
			const cv::Mat_<double>& landmarks_2D, const cv::Mat_<double>& landmarks_3D, const cv::Mat_<double>& pdm_model_params, const cv::Vec6d& rigid_shape_params, cv::Vec6d& pose_estimate,
			const cv::Point3f& gazeDirection0, const cv::Point3f& gazeDirection1, const cv::Vec2d& gaze_angle, const std::vector<cv::Point2d>& eye_landmarks,
			const std::vector<std::pair<std::string, double> >& au_intensities, const std::vector<std::pair<std::string, double> >& au_occurences);

		// TODO have set functions?

	private:

		// The actual output file stream that will be written
		std::ofstream output_file;

		// If we are recording results from a sequence each row refers to a frame, if we are recording an image each row is a face
		bool is_sequence;

		// Keep track of what we are recording
		bool output_2D_landmarks;
		bool output_3D_landmarks;
		bool output_model_params;
		bool output_pose;
		bool output_AUs;
		bool output_gaze;

		std::vector<std::string> au_names_class;
		std::vector<std::string> au_names_reg;

	};
}
#endif