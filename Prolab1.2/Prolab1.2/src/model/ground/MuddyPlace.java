package model.ground;

import config.GameConstants;
import engine.Grid;
import model.capability.CanFly;
import model.robot.Robot;
import model.capability.CanShield;

public class MuddyPlace extends Ground {

	public MuddyPlace() {
		super('C');
	}

	@Override
	public boolean stepOn(Robot robot, Grid grid) {
		if(robot instanceof CanFly) {
			return robot.spendEnergy(GameConstants.CAMUR_ARAZISI_FLY_COST);
		}

		if(robot instanceof CanShield){
			return robot.spendEnergy(GameConstants.CAMUR_ARAZISI_SHIELD_COST);
		}

		return robot.spendEnergy(GameConstants.CAMUR_ARAZISI_DEFAULT_COST);
	}

}

