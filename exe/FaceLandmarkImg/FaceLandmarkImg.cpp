///////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2017, Carnegie Mellon University and University of Cambridge,
// all rights reserved.
//
// ACADEMIC OR NON-PROFIT ORGANIZATION NONCOMMERCIAL RESEARCH USE ONLY
//
// BY USING OR DOWNLOADING THE SOFTWARE, YOU ARE AGREEING TO THE TERMS OF THIS LICENSE AGREEMENT.  
// IF YOU DO NOT AGREE WITH THESE TERMS, YOU MAY NOT USE OR DOWNLOAD THE SOFTWARE.
//
// License can be found in OpenFace-license.txt

//     * Any publications arising from the use of this software, including but
//       not limited to academic journal and conference publications, technical
//       reports and manuals, must cite at least one of the following works:
//
//       OpenFace: an open source facial behavior analysis toolkit
//       Tadas Baltrušaitis, Peter Robinson, and Louis-Philippe Morency
//       in IEEE Winter Conference on Applications of Computer Vision, 2016  
//
//       Rendering of Eyes for Eye-Shape Registration and Gaze Estimation
//       Erroll Wood, Tadas Baltrušaitis, Xucong Zhang, Yusuke Sugano, Peter Robinson, and Andreas Bulling 
//       in IEEE International. Conference on Computer Vision (ICCV),  2015 
//
//       Cross-dataset learning and person-speci?c normalisation for automatic Action Unit detection
//       Tadas Baltrušaitis, Marwa Mahmoud, and Peter Robinson 
//       in Facial Expression Recognition and Analysis Challenge, 
//       IEEE International Conference on Automatic Face and Gesture Recognition, 2015 
//
//       Constrained Local Neural Fields for robust facial landmark detection in the wild.
//       Tadas Baltrušaitis, Peter Robinson, and Louis-Philippe Morency. 
//       in IEEE Int. Conference on Computer Vision Workshops, 300 Faces in-the-Wild Challenge, 2013.    
//
///////////////////////////////////////////////////////////////////////////////
// FaceLandmarkImg.cpp : Defines the entry point for the console application for detecting landmarks in images.

#include "LandmarkCoreIncludes.h"

// System includes
#include <fstream>

// OpenCV includes
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc.hpp>

// Boost includes
#include <filesystem.hpp>
#include <filesystem/fstream.hpp>

#include <dlib/image_processing/frontal_face_detector.h>

#include <tbb/tbb.h>

#include <FaceAnalyser.h>
#include <GazeEstimation.h>

#include <ImageCapture.h>
#include <Visualizer.h>
#include <VisualizationUtils.h>
#include <RecorderOpenFace.h>
#include <RecorderOpenFaceParameters.h>


#ifndef CONFIG_DIR
#define CONFIG_DIR "~"
#endif

using namespace std;

vector<string> get_arguments(int argc, char **argv)
{

	vector<string> arguments;

	for(int i = 0; i < argc; ++i)
	{
		arguments.push_back(string(argv[i]));
	}
	return arguments;
}

// TODO rem
void create_display_image(const cv::Mat& orig, cv::Mat& display_image, LandmarkDetector::CLNF& clnf_model)
{
	
	// Draw head pose if present and draw eye gaze as well

	// preparing the visualisation image
	display_image = orig.clone();		

	// Creating a display image			
	cv::Mat xs = clnf_model.detected_landmarks(cv::Rect(0, 0, 1, clnf_model.detected_landmarks.rows/2));
	cv::Mat ys = clnf_model.detected_landmarks(cv::Rect(0, clnf_model.detected_landmarks.rows/2, 1, clnf_model.detected_landmarks.rows/2));
	double min_x, max_x, min_y, max_y;

	cv::minMaxLoc(xs, &min_x, &max_x);
	cv::minMaxLoc(ys, &min_y, &max_y);

	double width = max_x - min_x;
	double height = max_y - min_y;

	int minCropX = max((int)(min_x-width/3.0),0);
	int minCropY = max((int)(min_y-height/3.0),0);

	int widthCrop = min((int)(width*5.0/3.0), display_image.cols - minCropX - 1);
	int heightCrop = min((int)(height*5.0/3.0), display_image.rows - minCropY - 1);

	double scaling = 350.0/widthCrop;
	
	// first crop the image
	display_image = display_image(cv::Rect((int)(minCropX), (int)(minCropY), (int)(widthCrop), (int)(heightCrop)));
		
	// now scale it
	cv::resize(display_image.clone(), display_image, cv::Size(), scaling, scaling);

	// Make the adjustments to points
	xs = (xs - minCropX)*scaling;
	ys = (ys - minCropY)*scaling;

	cv::Mat shape = clnf_model.detected_landmarks.clone();

	xs.copyTo(shape(cv::Rect(0, 0, 1, clnf_model.detected_landmarks.rows/2)));
	ys.copyTo(shape(cv::Rect(0, clnf_model.detected_landmarks.rows/2, 1, clnf_model.detected_landmarks.rows/2)));

	// Do the shifting for the hierarchical models as well
	for (size_t part = 0; part < clnf_model.hierarchical_models.size(); ++part)
	{
		cv::Mat xs = clnf_model.hierarchical_models[part].detected_landmarks(cv::Rect(0, 0, 1, clnf_model.hierarchical_models[part].detected_landmarks.rows / 2));
		cv::Mat ys = clnf_model.hierarchical_models[part].detected_landmarks(cv::Rect(0, clnf_model.hierarchical_models[part].detected_landmarks.rows / 2, 1, clnf_model.hierarchical_models[part].detected_landmarks.rows / 2));

		xs = (xs - minCropX)*scaling;
		ys = (ys - minCropY)*scaling;

		cv::Mat shape = clnf_model.hierarchical_models[part].detected_landmarks.clone();

		xs.copyTo(shape(cv::Rect(0, 0, 1, clnf_model.hierarchical_models[part].detected_landmarks.rows / 2)));
		ys.copyTo(shape(cv::Rect(0, clnf_model.hierarchical_models[part].detected_landmarks.rows / 2, 1, clnf_model.hierarchical_models[part].detected_landmarks.rows / 2)));

	}

	LandmarkDetector::Draw(display_image, clnf_model);
						
}

int main (int argc, char **argv)
{
		
	//Convert arguments to more convenient vector form
	vector<string> arguments = get_arguments(argc, argv);

	// Prepare for image reading
	Utilities::ImageCapture image_reader;

	// A utility for visualizing the results
	Utilities::Visualizer visualizer(arguments);

	// The sequence reader chooses what to open based on command line arguments provided
	if (!image_reader.Open(arguments))
	{
		cout << "Could not open any images" << endl;
		return 1;
	}

	// Load the models if images found
	LandmarkDetector::FaceModelParameters det_parameters(arguments);
	// No need to validate detections, as we're not doing tracking
	det_parameters.validate_detections = false;

	// The modules that are being used for tracking
	cout << "Loading the model" << endl;
	LandmarkDetector::CLNF face_model(det_parameters.model_location);
	cout << "Model loaded" << endl;

	// Load facial feature extractor and AU analyser (make sure it is static)
	FaceAnalysis::FaceAnalyserParameters face_analysis_params(arguments);
	face_analysis_params.OptimizeForImages();
	FaceAnalysis::FaceAnalyser face_analyser(face_analysis_params);


	// If bounding boxes not provided, use a face detector
	cv::CascadeClassifier classifier(det_parameters.face_detector_location);
	dlib::frontal_face_detector face_detector_hog = dlib::get_frontal_face_detector();

	cv::Mat captured_image;

	captured_image = image_reader.GetNextImage();

	cout << "Starting tracking" << endl;
	while (!captured_image.empty())
	{

		Utilities::RecorderOpenFaceParameters recording_params(arguments, false);
		Utilities::RecorderOpenFace open_face_rec(image_reader.name, recording_params, arguments);

		// Making sure the image is in uchar grayscale
		cv::Mat_<uchar> grayscale_image = image_reader.GetGrayFrame();

		// if no pose defined we just use a face detector
		if(bounding_boxes.empty())
		{
			
			// Detect faces in an image
			vector<cv::Rect_<double> > face_detections;

			if(det_parameters.curr_face_detector == LandmarkDetector::FaceModelParameters::HOG_SVM_DETECTOR)
			{
				vector<double> confidences;
				LandmarkDetector::DetectFacesHOG(face_detections, grayscale_image, face_detector_hog, confidences);
			}
			else
			{
				LandmarkDetector::DetectFaces(face_detections, grayscale_image, classifier);
			}

			// Detect landmarks around detected faces
			int face_det = 0;
			// perform landmark detection for every face detected
			for(size_t face=0; face < face_detections.size(); ++face)
			{
				// if there are multiple detections go through them
				bool success = LandmarkDetector::DetectLandmarksInImage(grayscale_image, face_detections[face], clnf_model, det_parameters);

				// Estimate head pose and eye gaze				
				cv::Vec6d headPose = LandmarkDetector::GetPose(clnf_model, fx, fy, cx, cy);

				// Gaze tracking, absolute gaze direction
				cv::Point3f gazeDirection0(0, 0, -1);
				cv::Point3f gazeDirection1(0, 0, -1);
				cv::Vec2d gazeAngle(0, 0);

				if (success && det_parameters.track_gaze)
				{
					GazeAnalysis::EstimateGaze(clnf_model, gazeDirection0, fx, fy, cx, cy, true);
					GazeAnalysis::EstimateGaze(clnf_model, gazeDirection1, fx, fy, cx, cy, false);
					gazeAngle = GazeAnalysis::GetGazeAngle(gazeDirection0, gazeDirection1);
				}

				auto ActionUnits = face_analyser.PredictStaticAUs(read_image, clnf_model.detected_landmarks, false);

				// Writing out the detected landmarks (in an OS independent manner)
				if(!output_landmark_locations.empty())
				{
					char name[100];
					// append detection number (in case multiple faces are detected)
					sprintf(name, "_det_%d", face_det);

					// Construct the output filename
					boost::filesystem::path slash("/");
					std::string preferredSlash = slash.make_preferred().string();

					boost::filesystem::path out_feat_path(output_landmark_locations.at(i));
					boost::filesystem::path dir = out_feat_path.parent_path();
					boost::filesystem::path fname = out_feat_path.filename().replace_extension("");
					boost::filesystem::path ext = out_feat_path.extension();
					string outfeatures = dir.string() + preferredSlash + fname.string() + string(name) + ext.string();
					write_out_landmarks(outfeatures, clnf_model, headPose, gazeDirection0, gazeDirection1, gazeAngle,  ActionUnits.first, ActionUnits.second, det_parameters.track_gaze);
				}

				if (!output_pose_locations.empty())
				{
					char name[100];
					// append detection number (in case multiple faces are detected)
					sprintf(name, "_det_%d", face_det);

					// Construct the output filename
					boost::filesystem::path slash("/");
					std::string preferredSlash = slash.make_preferred().string();

					boost::filesystem::path out_pose_path(output_pose_locations.at(i));
					boost::filesystem::path dir = out_pose_path.parent_path();
					boost::filesystem::path fname = out_pose_path.filename().replace_extension("");
					boost::filesystem::path ext = out_pose_path.extension();
					string outfeatures = dir.string() + preferredSlash + fname.string() + string(name) + ext.string();
					write_out_pose_landmarks(outfeatures, clnf_model.GetShape(fx, fy, cx, cy), headPose, gazeDirection0, gazeDirection1);

				}

				if (det_parameters.track_gaze)
				{
					cv::Vec6d pose_estimate_to_draw = LandmarkDetector::GetPose(clnf_model, fx, fy, cx, cy);

					// Draw it in reddish if uncertain, blueish if certain
					LandmarkDetector::DrawBox(read_image, pose_estimate_to_draw, cv::Scalar(255.0, 0, 0), 3, fx, fy, cx, cy);
					GazeAnalysis::DrawGaze(read_image, clnf_model, gazeDirection0, gazeDirection1, fx, fy, cx, cy);
				}

				// displaying detected landmarks
				cv::Mat display_image;
				create_display_image(read_image, display_image, clnf_model);

				if(visualise && success)
				{
					imshow("colour", display_image);
					cv::waitKey(1);
				}

				// Saving the display images (in an OS independent manner)
				if(!output_images.empty() && success)
				{
					string outimage = output_images.at(i);
					if(!outimage.empty())
					{
						char name[100];
						sprintf(name, "_det_%d", face_det);

						boost::filesystem::path slash("/");
						std::string preferredSlash = slash.make_preferred().string();

						// append detection number
						boost::filesystem::path out_feat_path(outimage);
						boost::filesystem::path dir = out_feat_path.parent_path();
						boost::filesystem::path fname = out_feat_path.filename().replace_extension("");
						boost::filesystem::path ext = out_feat_path.extension();
						outimage = dir.string() + preferredSlash + fname.string() + string(name) + ext.string();
						create_directory_from_file(outimage);
						bool write_success = cv::imwrite(outimage, display_image);	
						
						if (!write_success)
						{
							cout << "Could not output a processed image" << endl;
							return 1;
						}

					}

				}

				if(success)
				{
					face_det++;
				}

			}
		}
		else
		{
			// Have provided bounding boxes
			LandmarkDetector::DetectLandmarksInImage(grayscale_image, bounding_boxes[i], clnf_model, det_parameters);

			// Estimate head pose and eye gaze				
			cv::Vec6d headPose = LandmarkDetector::GetPose(clnf_model, fx, fy, cx, cy);

			// Gaze tracking, absolute gaze direction
			cv::Point3f gazeDirection0(0, 0, -1);
			cv::Point3f gazeDirection1(0, 0, -1);
			cv::Vec2d gazeAngle(0, 0);

			if (det_parameters.track_gaze)
			{
				GazeAnalysis::EstimateGaze(clnf_model, gazeDirection0, fx, fy, cx, cy, true);
				GazeAnalysis::EstimateGaze(clnf_model, gazeDirection1, fx, fy, cx, cy, false);
				gazeAngle = GazeAnalysis::GetGazeAngle(gazeDirection0, gazeDirection1);
			}

			auto ActionUnits = face_analyser.PredictStaticAUs(read_image, clnf_model.detected_landmarks, false);

			// Writing out the detected landmarks
			if(!output_landmark_locations.empty())
			{
				string outfeatures = output_landmark_locations.at(i);
				write_out_landmarks(outfeatures, clnf_model, headPose, gazeDirection0, gazeDirection1, gazeAngle, ActionUnits.first, ActionUnits.second, det_parameters.track_gaze);
			}

			// Writing out the detected landmarks
			if (!output_pose_locations.empty())
			{
				string outfeatures = output_pose_locations.at(i);
				write_out_pose_landmarks(outfeatures, clnf_model.GetShape(fx, fy, cx, cy), headPose, gazeDirection0, gazeDirection1);
			}

			// displaying detected stuff
			cv::Mat display_image;

			if (det_parameters.track_gaze)
			{
				cv::Vec6d pose_estimate_to_draw = LandmarkDetector::GetPose(clnf_model, fx, fy, cx, cy);

				// Draw it in reddish if uncertain, blueish if certain
				LandmarkDetector::DrawBox(read_image, pose_estimate_to_draw, cv::Scalar(255.0, 0, 0), 3, fx, fy, cx, cy);
				GazeAnalysis::DrawGaze(read_image, clnf_model, gazeDirection0, gazeDirection1, fx, fy, cx, cy);
			}

			create_display_image(read_image, display_image, clnf_model);

			if(visualise)
			{
				imshow("colour", display_image);
				cv::waitKey(1);
			}

			if(!output_images.empty())
			{
				string outimage = output_images.at(i);
				if(!outimage.empty())
				{
					create_directory_from_file(outimage);
					bool write_success = imwrite(outimage, display_image);	

					if (!write_success)
					{
						cout << "Could not output a processed image" << endl;
						return 1;
					}
				}
			}
		}				

	}
	
	return 0;
}

