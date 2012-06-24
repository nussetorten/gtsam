/* ----------------------------------------------------------------------------

 * GTSAM Copyright 2010, Georgia Tech Research Corporation, 
 * Atlanta, Georgia 30332-0415
 * All Rights Reserved
 * Authors: Frank Dellaert, et al. (see THANKS for the full author list)

 * See LICENSE for the license information

 * -------------------------------------------------------------------------- */

/**
 * @file LocalizationExample.cpp
 * @brief Simple robot localization example, with three "GPS-like" measurements
 * @author Frank Dellaert
 */

// pull in the 2D PoseSLAM domain with all typedefs and helper functions defined
#include <gtsam/slam/pose2SLAM.h>

// include this for marginals
#include <gtsam/nonlinear/LevenbergMarquardtOptimizer.h>
#include <gtsam/nonlinear/Marginals.h>

#include <iomanip>
#include <iostream>

using namespace std;
using namespace gtsam;
using namespace gtsam::noiseModel;

/**
 * UnaryFactor
 * Example on how to create a GPS-like factor on position alone.
 */
class UnaryFactor: public NoiseModelFactor1<Pose2> {
  double mx_, my_; ///< X and Y measurements

public:
  UnaryFactor(Key j, double x, double y, const SharedNoiseModel& model):
    NoiseModelFactor1<Pose2>(model, j), mx_(x), my_(y) {}

  virtual ~UnaryFactor() {}

  Vector evaluateError(const Pose2& q,
                       boost::optional<Matrix&> H = boost::none) const
  {
    if (H) (*H) = Matrix_(2,3, 1.0,0.0,0.0, 0.0,1.0,0.0);
    return Vector_(2, q.x() - mx_, q.y() - my_);
  }
};

/**
 * Example of a more complex 2D localization example
 *  - Robot poses are facing along the X axis (horizontal, to the right in 2D)
 *  - The robot moves 2 meters each step
 *  - We have full odometry between poses
 *  - We have unary measurement factors at eacht time step
 */
int main(int argc, char** argv) {

  // create the graph (defined in pose2SLAM.h, derived from NonlinearFactorGraph)
	pose2SLAM::Graph graph;

	// add two odometry factors
	Pose2 odometry(2.0, 0.0, 0.0); // create a measurement for both factors (the same in this case)
	SharedDiagonal odometryNoise = Diagonal::Sigmas(Vector_(3, 0.2, 0.2, 0.1)); // 20cm std on x,y, 0.1 rad on theta
	graph.addRelativePose(1, 2, odometry, odometryNoise);
	graph.addRelativePose(2, 3, odometry, odometryNoise);

	// add unary measurement factors, like GPS, on all three poses
	SharedDiagonal noiseModel = Diagonal::Sigmas(Vector_(2, 0.1, 0.1)); // 10cm std on x,y
	graph.push_back(boost::make_shared<UnaryFactor>(1, 0, 0, noiseModel));
	graph.push_back(boost::make_shared<UnaryFactor>(2, 2, 0, noiseModel));
	graph.push_back(boost::make_shared<UnaryFactor>(3, 4, 0, noiseModel));

	// print
	graph.print("\nFactor graph:\n");

	// create (deliberatly inaccurate) initial estimate
	pose2SLAM::Values initialEstimate;
	initialEstimate.insertPose(1, Pose2(0.5, 0.0, 0.2));
	initialEstimate.insertPose(2, Pose2(2.3, 0.1,-0.2));
	initialEstimate.insertPose(3, Pose2(4.1, 0.1, 0.1));
	initialEstimate.print("\nInitial estimate:\n  ");

	// use an explicit Optimizer object
	LevenbergMarquardtOptimizer optimizer(graph, initialEstimate);
	pose2SLAM::Values result = optimizer.optimize();
	result.print("\nFinal result:\n  ");

	// Query the marginals
	Marginals marginals(graph, result);
	cout.precision(2);
  cout << "\nP1:\n" << marginals.marginalCovariance(1) << endl;
  cout << "\nP2:\n" << marginals.marginalCovariance(2) << endl;
  cout << "\nP3:\n" << marginals.marginalCovariance(3) << endl;

	return 0;
}

