#ifndef _OPTIMIZATION_BEHAVIORS_H
#define _OPTIMIZATION_BEHAVIORS_H

#include "../behaviors/naobehavior.h"

bool isBallMoving(const WorldModel *worldModel);

class OptimizationBehaviorFixedKick: public NaoBehavior {
	const string outputFile;

	double timeStart;
	bool hasKicked;
	bool beamChecked;
	bool backwards;
	bool ranIntoBall;
	bool fallen;

	int kick;

	double INIT_WAIT_TIME;

	VecPosition ballInitPos;
	void initKick();
	void writeFitnessToOutputFile(double fitness);

public:

	OptimizationBehaviorFixedKick(const std::string teamName, int uNum,
			const map<string, string>& namedParams_, const string& rsg_,
			const string& outputFile_);

	virtual void beam(double& beamX, double& beamY, double& beamAngle);
	virtual SkillType selectSkill();
	virtual void updateFitness();

};

class OptimizationBehaviorWalkForward: public NaoBehavior {
	const string outputFile;

	int run;
	double startTime;
	bool beamChecked;
	bool hasFallen;
	int falls;
	double INIT_WAIT;
	double DIST_TO_WALK;
	double COST_OF_FALL;
	double START_POSITION_X_1;
	double START_POSITION_Y_1;
	double START_POSITION_X_2;
	double START_POSITION_Y_2;
	double START_POSITION_X_3;
	double START_POSITION_Y_3;
	double START_POSITION_X_4;
	double START_POSITION_Y_4;
	double START_POSITION_X_5;
	double START_POSITION_Y_5;
	int direction;
	bool curve;
	bool flip;
	bool standing;
	double expectedPerTarget;
	double totalWalkDist;
	double targetStartTime;
	int currentTarget;
	VecPosition start;
	VecPosition oldTarget;
	void init();
	VecPosition getTarget();
	bool checkBeam();

public:

	OptimizationBehaviorWalkForward(const std::string teamName, int uNum,
			const map<string, string>& namedParams_, const string& rsg_,
			const string& outputFile_);

	virtual void beam(double& beamX, double& beamY, double& beamAngle);
	virtual SkillType selectSkill();
	virtual void updateFitness();

};

class OptimizationBehaviorStand: public NaoBehavior {

	const string outputFile;
	bool beamChecked;
	double totalcost;
	double cost;
	bool done;
	int run;
	double INIT_WAIT;
	double currentTime;
	double timeStart;
	void initStand();
	void writeFitnessToOutputFile(double fitness);

public:
	OptimizationBehaviorStand(const std::string teamName, int uNum,
			const map<string, string>& namedParams_, const string& rsg_,
			const string& outputFile_);
	virtual void beam(double& beamX, double& beamY, double& beamAngle);
	virtual SkillType selectSkill();
	virtual void updateFitness();
};

#endif
