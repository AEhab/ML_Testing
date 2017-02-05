#include "optimizationbehaviors.h"
#include <fstream>

/*
 *
 *
 * Fixed Kick optimization agent
 *
 *
 */
OptimizationBehaviorFixedKick::OptimizationBehaviorFixedKick(
		const std::string teamName, int uNum,
		const map<string, string>& namedParams_, const string& rsg_,
		const string& outputFile_) :
		NaoBehavior(teamName, uNum, namedParams_, rsg_), outputFile(
				outputFile_), kick(0), INIT_WAIT_TIME(3.0) {
	initKick();
}

void OptimizationBehaviorFixedKick::beam(double& beamX, double& beamY,
		double& beamAngle) {
	beamX = atof(namedParams.find("kick_xoffset")->second.c_str());
	beamY = atof(namedParams.find("kick_yoffset")->second.c_str());
	beamAngle = atof(namedParams.find("kick_angle")->second.c_str());
}

SkillType OptimizationBehaviorFixedKick::selectSkill() {
	double time = worldModel->getTime();
	if (timeStart < 0) {
		initKick();
		return SKILL_STAND;
	}

	// Wait a bit before attempting kick
	if (time - timeStart <= INIT_WAIT_TIME) {
		return SKILL_STAND;
	}

	if (!hasKicked) {
		hasKicked = true;
		return SKILL_KICK_LEFT_LEG; // The kick skill that we're optimizing
	}

	return SKILL_STAND;
}

void OptimizationBehaviorFixedKick::updateFitness() {
	static double totalFitness = 0.0;
	if (kick == 10) {
		writeFitnessToOutputFile(totalFitness / (double(kick)));
		return;
	}

	double time = worldModel->getTime();
	VecPosition meTruth = worldModel->getMyPositionGroundTruth();
	meTruth.setZ(0);

	if (time - timeStart <= INIT_WAIT_TIME) {
		return;
	}

	static bool failedLastBeamCheck = false;
	if (!beamChecked) {
		cout << "Checking whether beam was successful\n";
		beamChecked = true;
		LOG_STR("Checking whether beam was successful");

		meTruth.setZ(0);
		double beamX, beamY, beamAngle;
		beam(beamX, beamY, beamAngle);
		VecPosition meDesired = VecPosition(beamX, beamY, 0);
		double distance = meTruth.getDistanceTo(meDesired);
		double angle = worldModel->getMyAngDegGroundTruth();
		VecPosition ballPos = worldModel->getBallGroundTruth();
		double ballDistance = ballPos.getMagnitude();

		// Check that we're close to our expected position and angle
		// and also that the ball is close to it's exepected position
		if (distance > .1 || ballDistance > .1 || abs(angle - beamAngle) > 3) {
			cout << distance << "\t" << ballDistance << "\n";
			LOG_STR("Problem with the beam!");
			LOG(distance);
			LOG(meTruth);
			LOG(meDesired);
			if (failedLastBeamCheck) {
				kick++;
				totalFitness -= 100;
				failedLastBeamCheck = false;
			} else {
				failedLastBeamCheck = true;
			}
			initKick();
			return;
		}
		failedLastBeamCheck = false;
		string msg = "(playMode KickOff_Left)";
		setMonMessage(msg);
	}

	if (!hasKicked && worldModel->getPlayMode() == PM_PLAY_ON) {
		ranIntoBall = true;
	}

	if (!hasKicked) {
		return;
	}

	VecPosition ballTruth = worldModel->getBallGroundTruth();
	if (ballTruth.getX() < -.25) {
		backwards = true;
	}

	if (worldModel->isFallen()) {
		fallen = true;
	}

	if (worldModel->isFallen()) {
		totalFitness += -1;
		kick++;
		initKick();
		return;
	}

	if (time - (timeStart + INIT_WAIT_TIME) > 15
			&& !isBallMoving(this->worldModel)) {
		double angleOffset = abs(
				VecPosition(0, 0, 0).getAngleBetweenPoints(
						VecPosition(20, 0, 0), ballTruth));
		double distance = ballTruth.getX();
		double fitness = distance;

		if (backwards || distance <= 0.1 || ranIntoBall) {
			fitness = -100;
			if (backwards) {
				cout << "Detected backward kick" << endl;
			} else if (ranIntoBall) {
				cout << "Detected ranIntoBall" << endl;
			} else {
				cout << "Detected insufficient distance" << endl;
			}
		}
		cout << "Traveled distance = " << distance << endl;
		cout << "Fitness = " << fitness << endl;
		cout << "Final position = " << ballTruth.getX() << ", "
				<< ballTruth.getY() << endl;

		totalFitness += fitness;
		kick++;
		initKick();
		return;
	}
}

void OptimizationBehaviorFixedKick::initKick() {
	hasKicked = false;
	beamChecked = false;
	backwards = false;
	ranIntoBall = false;
	timeStart = worldModel->getTime();
	initialized = false;
	initBeamed = false;
	fallen = false;
	resetSkills();

	// Beam agent and ball
	double beamX, beamY, beamAngle;
	beam(beamX, beamY, beamAngle);
	VecPosition beamPos = VecPosition(beamX, beamY, 0);
	string msg = "(playMode BeforeKickOff)";
	setMonMessage(msg);
}

void OptimizationBehaviorFixedKick::writeFitnessToOutputFile(double fitness) {
	static bool written = false;
	if (!written) {
		LOG(fitness);
		LOG(kick);
		fstream file;
		file.open(outputFile.c_str(), ios::out);
		file << fitness << endl;
		file.close();
		written = true;
		//string msg = "(killsim)";
		//setMonMessage(msg);
	}
}

/* Checks if the ball is currently moving */
bool isBallMoving(const WorldModel *worldModel) {
	static VecPosition lastBall = worldModel->getBallGroundTruth();
	static double lastTime = worldModel->getTime();

	double thisTime = worldModel->getTime();
	VecPosition thisBall = worldModel->getBallGroundTruth();

	thisBall.setZ(0);
	lastBall.setZ(0);

	if (thisBall.getDistanceTo(lastBall) > 0.01) {
		// the ball moved!
		lastBall = thisBall;
		lastTime = thisTime;
		return true;
	}

	if (thisTime - lastTime < 0.5) {
		// not sure yet if the ball has settled
		return true;
	} else {
		return false;
	}
}

void writeToOutputFile(const string &filename, const string &output) {
//  static bool written = false;
//  assert(!written);
	//  LOG(output);
	fstream file;
	file.open(filename.c_str(), ios::out);
	file << output;
	file.close();
//  written = true;
}

/*
 *
 *
 * WALK FORWARD OPTIMIZATION AGENT
 *
 *
 *
 */
OptimizationBehaviorWalkForward::OptimizationBehaviorWalkForward(
		const std::string teamName, int uNum,
		const map<string, string>& namedParams_, const string& rsg_,
		const string& outputFile_) :
		NaoBehavior(teamName, uNum, namedParams_, rsg_), outputFile(outputFile_) {

	INIT_WAIT = 1;
	run = 0;
	totalWalkDist = 0;
	expectedPerTarget = 3;
	DIST_TO_WALK = 3.5;
	START_POSITION_X_1 = -HALF_FIELD_X + 7;
	START_POSITION_Y_1 = -HALF_FIELD_Y + 3;
	START_POSITION_X_2 = -6;
	START_POSITION_Y_2 = 0;
	START_POSITION_X_3 = -1;
	START_POSITION_Y_3 = 0;
	START_POSITION_X_4 = -HALF_FIELD_X + 7;
	START_POSITION_Y_4 = -HALF_FIELD_Y + 3;
	START_POSITION_X_5 = -HALF_FIELD_X + 3;
	START_POSITION_Y_5 = 0;
	direction = -1;
	COST_OF_FALL = 5;
	// Use ground truth localization for behavior
	worldModel->setUseGroundTruthDataForLocalization(true);

	init();
}
VecPosition OptimizationBehaviorWalkForward::getTarget() {
	VecPosition target;
	srand(time(0));
	VecPosition me = worldModel->getMyPositionGroundTruth();
	double currentTime = worldModel->getTime();
	if (run != 10) {
		if (run % 5 == 0) {
			target = VecPosition(
					start.getX() + (direction * 0.8 * DIST_TO_WALK),
					start.getY() + DIST_TO_WALK, 0);
		} else if (run % 5 == 1) {
			if (curve)
				target = VecPosition(me.getX() + 1.5,
						0.2 * (me.getX() + 1.5) * (me.getX() + 1.5) - 2, 0);
			else
				target = VecPosition(12, start.getY(), 0);

		} else if (run % 5 == 2) {
			if (curve)
				target = VecPosition(me.getX() - 1.5,
						-(0.2 * (me.getX() - 1.5 + 6) * (me.getX() - 1.5 + 6)
								- 2), 0);
			else
				target = VecPosition(-15, 0, 0);

		} else if (run % 5 == 3) {
			target = VecPosition(
					start.getX() + (direction * DIST_TO_WALK * 1.5),
					start.getY() + DIST_TO_WALK, 0);
		} else {
			if (direction == -1)
				target = VecPosition(start.getX() + DIST_TO_WALK, start.getY(),
						0);
			else
				target = VecPosition(start.getX() - DIST_TO_WALK, start.getY(),
						0);
		}
	} else {
		if (!flip)
			target = oldTarget;
		else {
			target = VecPosition((rand() % (int) (1.5*FIELD_X)) - FIELD_X, (rand() % (int) (1.5*FIELD_Y)) - FIELD_Y,
					0);
			oldTarget = target;
			flip = false;
		}
	}

//	target = VecPosition(
//			me.getX() - 0.1 * (me.getY() + 0.5) * (me.getY() + 0.5)
//				+1.2,  me.getY() + 0.5, 0);

	/*
	 if (curve)
	 target = VecPosition(me.getX() + 1.5,
	 0.2 * (me.getX() + 1.5) * (me.getX() + 1.5) - 2, 0);
	 else
	 target = VecPosition(12, start.getY(), 0);
	 */

//MIRRORED
//	if (curve)
//		target = VecPosition(me.getX() - 1.5,
//				-(0.2 * (me.getX() - 1.5 + 6) * (me.getX() - 1.5 + 6) - 2), 0);
//	else
//		target = VecPosition(-15, 0, 0);
//	target = VecPosition(me.getX() + 0.5 ,me.getY() + 0.1 * (me.getX() + 0.5) * (me.getX() + 0.5)
//						-1.2, 0);
//	cout<<"me : "<<me<<endl;
//	cout<<target<<endl;
//	if (run < 3 )
//		target = VecPosition(start.getX() + (direction * 0.8 * DIST_TO_WALK),
//				start.getY() + DIST_TO_WALK, 0);
//	else if(run == 3)
//		target = VecPosition(0, 0 , 0);
//	else if (run >= 7)
//		target = VecPosition(start.getX() + DIST_TO_WALK,
//				start.getY() + (direction * 0.8 * DIST_TO_WALK), 0);
	return target;
}
void OptimizationBehaviorWalkForward::init() {
	startTime = worldModel->getTime();
	initialized = false;
	initBeamed = false;
	curve = false;
	hasFallen = false;
	falls = 0;
	flip = true;
	direction = -1;
	beamChecked = false;
	standing = false;
	targetStartTime = startTime + INIT_WAIT;
	double currentTime = worldModel->getTime();
	double x, y, z;
	beam(x, y, z);
	start = VecPosition(x, y, z);
	string msg = "(playMode BeforeKickOff)";
	setMonMessage(msg);
}

void OptimizationBehaviorWalkForward::beam(double& beamX, double& beamY,
		double& beamAngle) {
	if (run != 10) {
		if (run % 5 == 0) {
			beamX = START_POSITION_X_1;
			beamY = START_POSITION_Y_1;
			beamAngle = 0;
		} else if (run % 5 == 1) {
			beamX = START_POSITION_X_2;
			beamY = START_POSITION_Y_2;
			beamAngle = 0;
		} else if (run % 5 == 2) {
			beamX = START_POSITION_X_3;
			beamY = START_POSITION_Y_3;
			beamAngle = 0;
		} else if (run % 5 == 3) {
			beamX = START_POSITION_X_4;
			beamY = START_POSITION_Y_4;
			beamAngle = 0;
		} else {
			beamX = START_POSITION_X_5;
			beamY = START_POSITION_Y_5;
			beamAngle = 0;
		}
	} else {
		beamX = -10;
		beamY = 0;
		beamAngle = 0;
	}
}

bool OptimizationBehaviorWalkForward::checkBeam() {
	LOG_STR("Checking whether beam was successful");
	VecPosition meTruth = worldModel->getMyPositionGroundTruth();
	meTruth.setZ(0);
	double beamX, beamY, beamAngle;
	beam(beamX, beamY, beamAngle);
	VecPosition meDesired = VecPosition(beamX, beamY, 0);
	double distance = meTruth.getDistanceTo(meDesired);
	double angleOffset = abs(worldModel->getMyAngDegGroundTruth() - beamAngle);
	if (distance > 0.05 || angleOffset > 5) {
		LOG_STR("Problem with the beam!");
		LOG(distance);
		LOG(meTruth);
		return false;
	}
	beamChecked = true;
	return true;
}

SkillType OptimizationBehaviorWalkForward::selectSkill() {
	double currentTime = worldModel->getTime();
	if (currentTime - startTime < INIT_WAIT || startTime < 0 || standing) {
		return SKILL_STAND;
	}
	return goToTarget(getTarget());
}

void OptimizationBehaviorWalkForward::updateFitness() {
	static bool written = false;
//	cout<<flip<<endl;
//	cout<<getTarget()<<endl;
	if (run == 11) {
		if (!written) {
			double fitness = totalWalkDist / (double) run;
			fstream file;
			file.open(outputFile.c_str(), ios::out);
			file << fitness << endl;
			file.close();
			written = true;
		}
		return;
	}

	if (startTime < 0) {
		init();
		return;
	}

	double currentTime = worldModel->getTime();
	if (currentTime - startTime < INIT_WAIT) {
		return;
	}

	if (!beamChecked) {
		static bool failedLastBeamCheck = false;
		if (!checkBeam()) {
			// Beam failed so reinitialize everything
			if (failedLastBeamCheck) {
				// Probably something bad happened
				//if we failed the beam twice in a row (perhaps the agent can't stand) so give a bad score and
				// move on
				totalWalkDist -= 100;
				run++;
			}
			failedLastBeamCheck = true;
			init();
			return;
		} else {
			failedLastBeamCheck = false;
			// Set playmode to PlayOn to start run and move ball out of the way
			string msg = "(playMode PlayOn) (ball (pos 0 -9 0) (vel 0 0 0))";
			setMonMessage(msg);
		}
	}

	VecPosition me = worldModel->getMyPositionGroundTruth();
	me.setZ(0);
	double beamX, beamY, beamAngle;
	beam(beamX, beamY, beamAngle);
	if (worldModel->isFallen()) {
		hasFallen = true;
		falls++;
	}

	if (run != 10) {
		if (run % 5 == 0 || run % 5 == 3) {
			VecPosition target = getTarget();
			start.setZ(0);
			if ((me.getDistanceTo(target) <= 0.2)
					&& !standing) {
				double timeTaken = currentTime - targetStartTime;
				double walkdist = me.getDistanceTo(start);
				cout << timeTaken << endl;
				cout << walkdist << endl;
				totalWalkDist += walkdist;
				hasFallen = false;
				direction *= -1;
				targetStartTime = currentTime;
				start = worldModel->getMyPositionGroundTruth();
				start.setZ(0);
			}
			if (currentTime - startTime >= INIT_WAIT + 12.5) {
				hasFallen = false;
				standing = true;
			}

			if (currentTime - startTime >= INIT_WAIT + 15) {
				totalWalkDist -= hasFallen * COST_OF_FALL;
				cout << "Run " << run << " distance walked: "
						<< totalWalkDist / (run + 1) << endl;
				run++;
				init();
			}
		} else if (run % 5 == 1) {
			if (me.getX() >= -3.464 && me.getX() < 3.4) {
				totalWalkDist += me.getDistanceTo(start);
				start = worldModel->getMyPositionGroundTruth();
				curve = true;
			} else if (me.getX() >= 3.4 && curve) {
				totalWalkDist += me.getDistanceTo(start);
				start = worldModel->getMyPositionGroundTruth();
				curve = false;
			}
			if (currentTime - startTime >= INIT_WAIT + 12.5
					|| me.getDistanceTo(getTarget()) < 0.2) {
				standing = true;
			}
			if (currentTime - startTime >= INIT_WAIT + 15.0) {
				totalWalkDist -= hasFallen * COST_OF_FALL;
				run++;
				init();
			}
		} else if (run % 5 == 2) {
			if (me.getX() <= -2.838 && me.getX() > -8.9) {
				totalWalkDist += me.getDistanceTo(start);
				start = worldModel->getMyPositionGroundTruth();
				curve = true;
			} else if (me.getX() <= -8.9 && curve) {
				totalWalkDist += me.getDistanceTo(start);
				start = worldModel->getMyPositionGroundTruth();
				curve = false;
			}
			if (currentTime - startTime >= INIT_WAIT + 12.5
					|| me.getDistanceTo(getTarget()) < 0.2) {
				standing = true;
			}
			if (currentTime - startTime >= INIT_WAIT + 15.0) {
				totalWalkDist -= hasFallen * COST_OF_FALL;
				run++;
				init();
			}

		} else {
			VecPosition target = getTarget();
			start.setZ(0);
			if ((me.getDistanceTo(target) <= 0.2)) {
				double timeTaken = currentTime - targetStartTime;
				double walkdist = me.getDistanceTo(start);
				cout << timeTaken << endl;
				cout << walkdist << endl;
				totalWalkDist += walkdist;
				hasFallen = false;
				direction *= -1;
				targetStartTime = currentTime;
				start = worldModel->getMyPositionGroundTruth();
				start.setZ(0);
			}

			if (currentTime - startTime >= INIT_WAIT + 15) {
				totalWalkDist -= hasFallen * COST_OF_FALL;
				cout << "Run " << run << " distance walked: "
						<< totalWalkDist / (run + 1) << endl;
				run++;
				init();
			}
		}
	} else {
		if ((int) (currentTime - startTime) % 2 == 0)
			flip = true;
		else
			flip = false;
		if (currentTime - startTime >= INIT_WAIT + 15.0) {
			totalWalkDist += hasFallen *- COST_OF_FALL;
			run++;
			init();
		}
	}

//	if (me.getX() <= -2.838 && me.getX() > -8.9) {
//		cout << "CURVEYASA7BI" << endl;
//		curve = true;
//	}
//
//	else if (me.getX() <= -8.9) {
//		start = worldModel->getMyPositionGroundTruth();
//		cout << "END OF CURVE YA SA7BY" << endl;
//		curve = false;
//	}

//	if (run == 4 || run == 9) {
//		if (currentTime - startTime >= INIT_WAIT + 15.0) {
//			double walkedDist = me.getDistanceTo(start) - falls * COST_OF_FALL;
//			totalWalkDist += walkedDist;
//			targetStartTime = currentTime;
//			cout << "Run " << run << " was a circle and dist walked "
//					<< walkedDist << endl;
//			run++;
//			init();
//
//		}
//	} else {
//		VecPosition target = getTarget();
//		start.setZ(0);
//		if ((me.getDistanceTo(target) <= 0.2
//				|| currentTime - targetStartTime >= expectedPerTarget)
//				&& !standing) {
//			double timeTaken = currentTime - targetStartTime;
//			double walkdist = me.getDistanceTo(start)
//					* (expectedPerTarget / timeTaken)
//					- (hasFallen ? COST_OF_FALL : 0);
//			cout << timeTaken << endl;
//			cout << walkdist << endl;
//			totalWalkDist += walkdist;
//			hasFallen = false;
//			direction *= -1;
//			targetStartTime = currentTime;
//			start = worldModel->getMyPositionGroundTruth();
//			start.setZ(0);
//		}
//		if (currentTime - startTime >= INIT_WAIT + 13.5) {
//			hasFallen = false;
//			standing = true;
//		}
//
//	if (currentTime - startTime >= INIT_WAIT + 100) {
//		totalWalkDist += (falls) * COST_OF_FALL;
//		cout << "Run " << run << " distance walked: "
//				<< totalWalkDist / (run + 1) << endl;
//		run++;
//		init();
//	}
}

/*
 *
 *
 * STAND UP OPTIMIZATION AGENT
 *
 *
 *
 */

OptimizationBehaviorStand::OptimizationBehaviorStand(const std::string teamName,
		int uNum, const map<string, string>& namedParams_, const string& rsg_,
		const string& outputFile_) :
		NaoBehavior(teamName, uNum, namedParams_, rsg_), outputFile(outputFile_) {
	totalcost = 0;
	cost = 0;
	done = false;
	INIT_WAIT = 2;
	run = 0;
// Use ground truth localization for behavior
	worldModel->setUseGroundTruthDataForLocalization(true);
	initStand();
}

void OptimizationBehaviorStand::initStand() {
	beamChecked = false;
	timeStart = worldModel->getTime();
	initialized = false;
	initBeamed = false;
	resetSkills();
// Beam agent and ball
	double beamX, beamY, beamAngle;
	beam(beamX, beamY, beamAngle);
	VecPosition beamPos = VecPosition(beamX, beamY, 0);
	string msg = "(playMode BeforeKickOff)";
	setMonMessage(msg);
}

void OptimizationBehaviorStand::beam(double& beamX, double& beamY,
		double& beamAngle) {
	beamX = -5;
	beamY = 0.0;
	beamAngle = 0.0;
}

SkillType OptimizationBehaviorStand::selectSkill() {
	currentTime = worldModel->getTime();
	if (timeStart < 0) {
		initStand();
		return SKILL_STAND;
	}
// Wait a bit before attempting kick
	if (currentTime - timeStart <= INIT_WAIT) {
		return SKILL_STAND;
	}
	if (!worldModel->isFallen()) {
		done = true;
		return SKILL_DOWN; // The kick skill that we're optimizing
	}
	return SKILL_STAND;
}

void OptimizationBehaviorStand::updateFitness() {
	if (run == 10) {
		writeFitnessToOutputFile(totalcost / run);
		return;
	}
	double time = worldModel->getTime();
	if (time - timeStart <= INIT_WAIT) {
		return;
	}
	if (!beamChecked) {
		cout << "Checking whether beam was successful\n";
		beamChecked = true;
		initialized = true;
		initBeamed = true;
		double beamX, beamY, beamAngle;
		beam(beamX, beamY, beamAngle);
		VecPosition meDesired = VecPosition(beamX, beamY, 0);
		LOG(meDesired);
		initStand();
		return;
	}
	string msg = "(playMode PlayOn)";
	setMonMessage(msg);
	if (worldModel->isFallen() && done) {
		cost = -10;
		run++;
		totalcost += cost;
		initStand();
		return;
	} else if (done) {
		cost = worldModel->getTime() - timeStart;
		run++;
		return;
		totalcost += cost;
		initStand();
	}
}

void OptimizationBehaviorStand::writeFitnessToOutputFile(double fitness) {
	static bool written = false;
	if (!written) {
		LOG(fitness);
		fstream file;
		file.open(outputFile.c_str(), ios::out);
		file << fitness << endl;
		file.close();
		written = true;
	}
}
