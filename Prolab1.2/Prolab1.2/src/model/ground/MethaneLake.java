package model.ground;

import config.GameConstants;
import engine.Grid;
import model.capability.CanFly;
import model.robot.Robot;

public class MethaneLake extends Ground {

	public MethaneLake() {
		super('~');
	}

	@Override
	public boolean stepOn(Robot robot, Grid grid) {

		if(robot instanceof CanFly) {
			return robot.spendEnergy(GameConstants.METAN_GOLU_FLY_COST);
		}
		
		return false;
	}

}

